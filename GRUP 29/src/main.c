#include "scheduler.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h> 

// Global Scheduler yapısı (Tüm kuyruklar ve durumlar burada tutulur)
Scheduler_t g_scheduler;

// FreeRTOS tick sayısını saniyeye çeviren makro
// (xTaskGetTickCount() tick döner, configTICK_RATE_HZ ile saniyeye çevrilir)
#define GET_REAL_TIME() ((double)xTaskGetTickCount() / (double)configTICK_RATE_HZ)

/**
 * @brief Dosyadan görevleri okur ve "bekleyenler" listesine ekler.
 * Henüz kuyruklara (Ready Queue) eklemez, çünkü varış zamanları gelmemiştir.
 */
int load_tasks_from_file(const char* filename, Scheduler_t* scheduler) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) { 
        printf("Hata: '%s' dosyası açılamadı!\n", filename); 
        return -1; 
    }
    char line[256];
    int task_count = 0;
    
    // Satır satır okuma döngüsü
    while (fgets(line, sizeof(line), file) != NULL) {
        // Boş satırları veya yorum satırlarını (#) atla
        if (line[0] == '\n' || line[0] == '#' || line[0] == '\r') continue;
        
        // Satır sonundaki yeni satır karakterini temizle
        line[strcspn(line, "\r\n")] = 0;
        
        uint32_t arrival_time, priority, duration;
        // Formatı ayrıştır: "Varış, Öncelik, Süre"
        if (sscanf(line, "%u, %u, %u", &arrival_time, &priority, &duration) != 3) continue;
        
        // Görev yapısını oluştur (bellek tahsisi yapılır)
        Task_t* task = task_create(scheduler->task_counter++, arrival_time, priority, duration);
        if (task) {
            // Görevi şimdilik "pending" (bekleyen) listesine at
            scheduler_add_pending_task(scheduler, task);
            task_count++;
        }
    }
    fclose(file);
    return task_count;
}

/**
 * @brief Simülasyon görevi için gerçek bir FreeRTOS görevi (thread) oluşturur.
 * Simülasyondaki öncelik ile FreeRTOS önceliğini eşleştirir.
 */
BaseType_t create_freertos_task_for_scheduler(Task_t* task) {
    if (task == NULL) return pdFAIL;
    
    // Simülasyon önceliğini FreeRTOS önceliğine çevir.
    // Eğer PRIORITY_RT (0) ise en yüksek FreeRTOS önceliğini ver.
    // Değilse bir alt önceliği ver.
    UBaseType_t prio = (task->priority == PRIORITY_RT) ? configMAX_PRIORITIES - 1 : configMAX_PRIORITIES - 2;
    
    // xTaskCreate: FreeRTOS'un görev oluşturma fonksiyonu
    return xTaskCreate(task_function,        // Çalışacak fonksiyon
                       task->task_name,      // Görev adı (debug için)
                       configMINIMAL_STACK_SIZE * 4, // Stack boyutu
                       (void*)task,          // Parametre (görev yapısı)
                       prio,                 // Belirlenen öncelik
                       &task->task_handle);  // Görev handle'ı (kontrol için gerekli)
}

/**
 * @brief Ana Dağıtıcı (Dispatcher) Görevi
 * Tüm zamanlama mantığı, kuyruk yönetimi ve bağlam değişimi (context switch) burada döner.
 */
void dispatcher_task(void* pvParameters) {
    Scheduler_t* scheduler = (Scheduler_t*)pvParameters;
    scheduler->current_time = 0.0; 
    
    while (1) {
        // Kritik bölgeye giriş: Scheduler verilerini korumak için Mutex alıyoruz.
        if (xSemaphoreTake(scheduler->scheduler_mutex, portMAX_DELAY) == pdTRUE) {
            
            // --- 1. ZAMANI GÜNCELLE ---
            // Simülasyonun o anki zamanını alıyoruz.
            scheduler->current_time = GET_REAL_TIME();
            bool just_started = false; // Yeni başlatılan/devam ettirilen görev kontrolü

            // --- 2. YENİ GELENLERİ KONTROL ET ---
            // Pending listesindeki görevlerin varış zamanı geldiyse ilgili kuyruğa taşı.
            scheduler_check_arrivals(scheduler);
            
            // --- 3. PREEMPTION (KESME) KONTROLÜ ---
            // RT (Gerçek Zamanlı) görevler, normal görevleri kesmelidir.
            
            // Eğer şu an çalışan bir görev varsa VE bu görev RT (Öncelik 0) değilse...
            if (scheduler->current_task != NULL && scheduler->current_task->priority > PRIORITY_RT) {
                // ...ve RT kuyruğunda bekleyen acil bir görev varsa:
                if (!queue_is_empty(&scheduler->queues[PRIORITY_RT])) {
                    Task_t* preempted_task = scheduler->current_task;
                    
                    // 1. O anki (düşük öncelikli) görevi fiziksel olarak askıya al
                    if (preempted_task->task_handle != NULL) {
                        vTaskSuspend(preempted_task->task_handle);
                    }
                    
                    // 2. Log bas (Askıya alındı bilgisini göster)
                    print_task_info(preempted_task, "SUSPENDED", scheduler->current_time);
                    
                    // 3. Görevi kendi kuyruğunun sonuna geri ekle ki sırası gelince devam etsin.
                    // Kuyruğa giriş zamanını güncelle (Timeout hatalı tetiklenmesin diye)
                    preempted_task->abs_wait_start = scheduler->current_time;
                    queue_enqueue(&scheduler->queues[preempted_task->priority], preempted_task);
                    
                    // 4. İşlemciyi (pointer'ı) boşa çıkar.
                    // Böylece aşağıdaki "GÖREV SEÇİMİ" bloğu RT görevini seçebilecek.
                    scheduler->current_task = NULL;
                }
            }
            
            // --- 4. TIMEOUT KONTROLÜ ---
            // Kuyrukta 20 saniyeden fazla bekleyen görevleri iptal et.
            scheduler_check_timeouts(scheduler); 
            
            // --- 5. GÖREV SEÇİMİ (Scheduling) ---
            // Eğer şu an işlemcide kimse yoksa (veya az önce preemption ile boşalttıysak)
            if (scheduler->current_task == NULL) {
                // En yüksek öncelikli kuyruktan sıradaki görevi al
                Task_t* next_task = scheduler_get_next_task(scheduler);
                
                if (next_task != NULL) {
                    scheduler->current_task = next_task;
                    
                    // RT olmayan görevlerin başlangıç zamanını kaydet (istatistik için)
                    if (next_task->priority != PRIORITY_RT) {
                        next_task->start_time = scheduler->current_time; 
                    }

                    // Eğer görev ilk kez çalışacaksa (henüz FreeRTOS task'ı yoksa)
                    if (next_task->task_handle == NULL) {
                        create_freertos_task_for_scheduler(next_task);
                        next_task->creation_time = scheduler->current_time;
                        next_task->abs_wait_start = scheduler->current_time; 
                        
                        print_task_info(next_task, "STARTED", scheduler->current_time);
                        just_started = true;
                        scheduler->skip_next_log = true; // "başladı" yazdık, hemen altına "yürütülüyor" yazma
                    } 
                    // Görev daha önce oluşturulmuş ve askıdaysa
                    else {
                        vTaskResume(next_task->task_handle); // Kaldığı yerden devam ettir
                        // Not: scheduler.c içinde RESUMED -> "başladı" olarak çevrilir.
                        print_task_info(next_task, "RESUMED", scheduler->current_time);
                        just_started = true; 
                        scheduler->skip_next_log = true; // "başladı" yazdık, hemen altına "yürütülüyor" yazma
                    }
                }
            }

            // Seçili olan (aktif) görev üzerinden işlemler
            Task_t* current = scheduler->current_task;
            
            if (current != NULL) {
                // --- YÜRÜTME LOGU ---
                // Eğer az önce "başladı" yazmadıysak "yürütülüyor" yaz.
                if (!just_started && !scheduler->skip_next_log) {
                     print_task_info(current, "RUNNING", scheduler->current_time);
                }
                
                // Flag'i sıfırla ki bir sonraki saniyede log basabilsin
                scheduler->skip_next_log = false;
                
                // Tekrar timeout kontrolü (güvenlik için)
                scheduler_check_timeouts(scheduler);

                // Görevin kalan süresini 1 saniye azalt
                if (current->remaining_time > 0) current->remaining_time--;
                
                // --- FİZİKSEL BEKLEME (TIME QUANTUM) ---
                // Mutex'i bırakıyoruz ki diğer tasklar çalışabilsin veya sistem nefes alsın.
                xSemaphoreGive(scheduler->scheduler_mutex);
                vTaskDelay(pdMS_TO_TICKS(TIME_QUANTUM)); // 1 saniye (1000ms) bekle
                // Tekrar mutex'i al, çünkü veri yapısını değiştireceğiz.
                xSemaphoreTake(scheduler->scheduler_mutex, portMAX_DELAY);
                
                // --- SONUÇ KONTROLÜ ---
                scheduler->current_time = GET_REAL_TIME();
                
                // Görev bitti mi?
                if (current->remaining_time == 0) {
                    print_task_info(current, "COMPLETED", scheduler->current_time);
                    current->is_running = false;
                    
                    // FreeRTOS kaynaklarını temizle
                    if (current->task_handle != NULL) vTaskDelete(current->task_handle);
                    task_destroy(current); // Belleği temizle
                    scheduler->current_task = NULL; // İşlemciyi boşa çıkar
                }
                // Görev bitmedi ama RT değil (Round Robin / Priority Decay)
                else if (current->priority != PRIORITY_RT) {
                    // --- GÖREV DEĞİŞİM MANTIĞI ---
                    uint32_t old_priority = current->priority;
                    
                    // Önceliği düşür (Demotion/Aging mantığı)
                    scheduler_demote_task(scheduler, current); 
                    
                    // Görevi yeni önceliğine göre kuyruğa geri ekle
                    if (current->priority < MAX_PRIORITY_LEVELS) {
                        current->abs_wait_start = scheduler->current_time; 
                        queue_enqueue(&scheduler->queues[current->priority], current);
                    }
                    
                    // Sıradaki göreve bak
                    Task_t* next_task = scheduler_get_next_task(scheduler);
                    
                    // Eğer sıradaki görev yine aynıysa (başka kimse yoksa)
                    if (next_task == current) {
                        scheduler->current_task = next_task; // Devam et
                    } 
                    // Eğer farklı bir görev seçildiyse (Context Switch)
                    else {
                        // Mevcut görevi askıya al
                        if (current->task_handle != NULL) vTaskSuspend(current->task_handle);
                        print_task_info_with_old_priority(current, "SUSPENDED", scheduler->current_time, old_priority);
                        
                        scheduler->current_task = next_task;
                        
                        // Yeni görevi başlat veya devam ettir
                        if (next_task != NULL) {
                             if (next_task->priority != PRIORITY_RT) {
                                    next_task->start_time = scheduler->current_time;
                             }
                             
                             if (next_task->task_handle == NULL) {
                                 create_freertos_task_for_scheduler(next_task);
                                 next_task->creation_time = scheduler->current_time;
                                 next_task->abs_wait_start = scheduler->current_time;
                                 print_task_info(next_task, "STARTED", scheduler->current_time);
                                 scheduler->skip_next_log = true;
                             } else {
                                 vTaskResume(next_task->task_handle);
                                 print_task_info(next_task, "RESUMED", scheduler->current_time);
                                 scheduler->skip_next_log = true;
                             }
                        }
                    }
                }
                
            } 
            // Eğer çalışacak hiçbir görev yoksa (IDLE)
            else {
                xSemaphoreGive(scheduler->scheduler_mutex);
                vTaskDelay(pdMS_TO_TICKS(TIME_QUANTUM)); // Boşta bekle
                xSemaphoreTake(scheduler->scheduler_mutex, portMAX_DELAY);
            }
            
            // Tüm görevler bitti mi?
            if (scheduler_is_empty(scheduler) && scheduler->current_task == NULL) {
                xSemaphoreGive(scheduler->scheduler_mutex);
                printf("\nSimülasyon tamamlandı. Çıkış yapılıyor...\n");
                vTaskDelay(pdMS_TO_TICKS(1000));
                exit(0);
            }
            xSemaphoreGive(scheduler->scheduler_mutex);
        }
    }
}

int main(int argc, char* argv[]) {
    // Argüman kontrolü (dosya adı)
    const char* filename = "giris.txt";
    if (argc >= 2) filename = argv[1];
    else printf("Bilgi: Varsayılan '%s' kullanılıyor.\n", filename);

    // Scheduler'ı başlat
    scheduler_init(&g_scheduler);

    // Dosyadan görevleri yükle
    if (load_tasks_from_file(filename, &g_scheduler) <= 0) {
        printf("Hata: Görev yüklenemedi.\n");
        return -1;
    }

    printf("Simülasyon başlatılıyor...\n");
    
    // Dispatcher görevini oluştur (Sistemdeki en yüksek 2. öncelik)
    xTaskCreate(dispatcher_task, "Dispatcher", configMINIMAL_STACK_SIZE * 4, (void*)&g_scheduler, configMAX_PRIORITIES - 1, NULL);
    
    // FreeRTOS Kernel'i başlat (Artık kontrol FreeRTOS'ta)
    vTaskStartScheduler();
    return 0;
}