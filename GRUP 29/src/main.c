#include "scheduler.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

// Global Scheduler nesnesi. Tüm kuyruklar ve zamanlayıcı durumu burada tutulur.
Scheduler_t g_scheduler;

/**
 * @brief Dosyadan görevleri okur ve sistemin başlangıç (pending) listesine ekler.
 * * @param filename Okunacak dosya adı (örn: giris.txt)
 * @param scheduler Yönetici yapı (struct)
 * @return int Okunan görev sayısı veya hata durumunda -1
 */
int load_tasks_from_file(const char* filename, Scheduler_t* scheduler) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) { 
        printf("Hata: %s dosyası açılamadı!\n", filename); 
        return -1; 
    }

    char line[256];
    int task_count = 0;

    // Dosyayı satır satır oku
    while (fgets(line, sizeof(line), file) != NULL) {
        // Boş satırları, '#' ile başlayan yorumları ve satır sonu karakterlerini atla
        if (line[0] == '\n' || line[0] == '#' || line[0] == '\r') continue;
        
        // Satır sonundaki yeni satır karakterini temizle
        line[strcspn(line, "\r\n")] = 0;

        uint32_t arrival_time, priority, duration;
        
        // CSV formatını parse et: Varış Zamanı, Öncelik, Çalışma Süresi
        if (sscanf(line, "%u, %u, %u", &arrival_time, &priority, &duration) != 3) continue;

        // Yeni bir görev yapısı oluştur (task_create yardımcı fonksiyonu ile)
        // task_counter++ ile her göreve benzersiz bir ID verilir.
        Task_t* task = task_create(scheduler->task_counter++, arrival_time, priority, duration);
        
        if (task) {
            // Görevi "Bekleyenler" (Pending) listesine ekle. 
            // Henüz varış zamanı gelmediği için kuyruğa girmez.
            scheduler_add_pending_task(scheduler, task);
            task_count++;
        }
    }
    fclose(file);
    return task_count;
}

/**
 * @brief Simülasyon görevi için gerçek bir FreeRTOS görevi (thread) oluşturur.
 * * @param task Simülasyon görev yapısı
 * @return BaseType_t FreeRTOS görev oluşturma sonucu (pdPASS veya pdFAIL)
 */
BaseType_t create_freertos_task_for_scheduler(Task_t* task) {
    if (task == NULL) return pdFAIL;

    // FreeRTOS önceliğini belirle:
    // Eğer görev Real-Time (PRIORITY_RT) ise en yüksek önceliği ver.
    // Değilse bir alt önceliği ver. Bu, simülasyonun akışını bozmamak içindir.
    UBaseType_t prio = (task->priority == PRIORITY_RT) ? configMAX_PRIORITIES - 1 : configMAX_PRIORITIES - 2;

    // FreeRTOS task'ını oluştur. 'task_function' bu görevin çalıştıracağı asıl C kodudur.
    return xTaskCreate(task_function, task->task_name, configMINIMAL_STACK_SIZE * 4, (void*)task, prio, &task->task_handle);
}

/**
 * @brief DISPATCHER (Dağıtıcı) GÖREVİ
 * * Bu fonksiyon simülasyonun kalbidir. Merkezi İşlem Birimi (CPU) gibi davranır.
 * Zamanı ilerletir, görevleri seçer, bağlam (context) değişimini yönetir.
 */
void dispatcher_task(void* pvParameters) {
    Scheduler_t* scheduler = (Scheduler_t*)pvParameters;
    
    scheduler->current_time = 0; // Simülasyon zamanını sıfırla
    
    while (1) {
        // Mutex al: Scheduler veri yapısına aynı anda sadece bir kişi erişsin.
        if (xSemaphoreTake(scheduler->scheduler_mutex, portMAX_DELAY) == pdTRUE) {
            
            bool just_started = false; // Görev bu çevrimde mi başladı? (Loglama için flag)

            // --- 1. YENİ GELEN GÖREVLERİ KONTROL ET ---
            // Pending listesindeki görevlerin varış zamanı (arrival_time) geldi mi?
            // Geldiyse ilgili öncelik kuyruğuna (Queue) taşınır.
            scheduler_check_arrivals(scheduler);
            
            // --- 2. ZAMAN AŞIMI (STARVATION) KONTROLÜ ---
            // Düşük öncelikli kuyrukta çok bekleyen görevleri (20 sn kuralı)
            // bir üst kuyruğa veya RT kuyruğuna terfi ettir.
            scheduler_check_timeouts(scheduler); 
            
            // --- 3. GÖREV SEÇİMİ (SCHEDULING) ---
            // Eğer şu an çalışan bir görev yoksa (CPU boşsa), kuyruktan yenisini seç.
            if (scheduler->current_task == NULL) {
                Task_t* next_task = scheduler_get_next_task(scheduler);
                
                if (next_task != NULL) {
                    scheduler->current_task = next_task; // CPU'yu bu göreve ata
                    
                    // RT olmayan görevlerde başlangıç zamanını kaydet
                    if (next_task->priority != PRIORITY_RT) {
                        next_task->start_time = scheduler->current_time; 
                    }

                    // -> DURUM A: Görev sisteme ilk defa mı giriyor?
                    if (next_task->task_handle == NULL) {
                        create_freertos_task_for_scheduler(next_task); // FreeRTOS thread'i yarat
                        
                        next_task->creation_time = scheduler->current_time;
                        next_task->abs_wait_start = scheduler->current_time; // Bekleme sayacını başlat
                        
                        print_task_info(next_task, "STARTED", scheduler->current_time);
                        just_started = true; // Bu tur sadece "STARTED" yazsın, "RUNNING" yazmasın.
                    } 
                    // -> DURUM B: Görev daha önce çalışmış ve askıya (suspend) mı alınmıştı?
                    else {
                        vTaskResume(next_task->task_handle); // Kaldığı yerden devam ettir

                        // Askıdan dönünce tekrar "STARTED" (veya RESUMED) bilgisi veriyoruz.
                        print_task_info(next_task, "STARTED", scheduler->current_time);
                        just_started = true;
                    }
                }
            }

            // Şu an CPU'da bir görev var mı?
            Task_t* current = scheduler->current_task;
            
            if (current != NULL) {
                // --- YÜRÜTME AŞAMASI (EXECUTION) ---
                
                // 1. Loglama: "RUNNING"
                // Eğer RT göreviyse ve bu tur yeni başlamadıysa "Yürütülüyor" yaz.
                // (just_started bayrağı burada çift loglamayı engeller)
                if (current->priority == PRIORITY_RT && !just_started) {
                     print_task_info(current, "RUNNING", scheduler->current_time);
                }
                
                // 2. Arka plan timeout kontrolünü tekrar yap (Güvenlik için)
                scheduler_check_timeouts(scheduler);

                // 3. İşlemi Yap (Sanal CPU zamanını tüket)
                if (current->remaining_time > 0) current->remaining_time--;
                
                // --- 4. FİZİKSEL ZAMAN GEÇİŞİ SİMÜLASYONU ---
                // Burası çok önemli: FreeRTOS'un vTaskDelay fonksiyonu gerçek zamanı harcar.
                // Mutex'i bırakıyoruz ki diğer threadler (varsa) araya girebilsin.
                xSemaphoreGive(scheduler->scheduler_mutex);
                vTaskDelay(pdMS_TO_TICKS(TIME_QUANTUM)); // Örneğin 1 saniye (1000ms) bekle
                xSemaphoreTake(scheduler->scheduler_mutex, portMAX_DELAY); // Tekrar kitle
                
                // 5. Simülasyon zamanını 1 birim (1 sn) artır
                scheduler->current_time++;
                
                // --- 6. GÖREV SONUÇ KONTROLÜ ---
                
                // A) Görev bitti mi?
                if (current->remaining_time == 0) {
                    print_task_info(current, "COMPLETED", scheduler->current_time);
                    current->is_running = false;
                    
                    // FreeRTOS kaynağını temizle
                    if (current->task_handle != NULL) vTaskDelete(current->task_handle);
                    task_destroy(current); // Belleği temizle
                    
                    scheduler->current_task = NULL; // CPU'yu boşa çıkar
                }
                // B) Görev bitmedi ama süresi doldu (RT olmayanlar için Round Robin / Multi-level Feedback)
                else if (current->priority != PRIORITY_RT) {
                    uint32_t old_priority = current->priority;
                    
                    // Görevin önceliğini düşür (Cezalandırma / Aging mantığı)
                    scheduler_demote_task(scheduler, current); 
                    
                    // FreeRTOS görevini askıya al (CPU'dan çek)
                    if (current->task_handle != NULL) vTaskSuspend(current->task_handle);
                    
                    // Eğer öncelik en alt seviyenin altına düşmediyse kuyruğa geri koy
                    if (current->priority < MAX_PRIORITY_LEVELS) {
                        // Kuyruğa girdiği anı kaydet (Timeout hesaplamak için)
                        current->abs_wait_start = scheduler->current_time;
                        queue_enqueue(&scheduler->queues[current->priority], current);
                    }
                    
                    print_task_info_with_old_priority(current, "SUSPENDED", scheduler->current_time, old_priority);
                    scheduler->current_task = NULL; // CPU'yu boşa çıkar, sıradaki görev gelsin
                }
                
            } else {
                // --- IDLE (BOŞTA) DURUMU ---
                // Çalışacak hiçbir görev yoksa sadece zamanı geçir.
                xSemaphoreGive(scheduler->scheduler_mutex);
                vTaskDelay(pdMS_TO_TICKS(TIME_QUANTUM));
                xSemaphoreTake(scheduler->scheduler_mutex, portMAX_DELAY);
                
                scheduler->current_time++;
            }
            
            // --- 7. SİMÜLASYON BİTİŞ KONTROLÜ ---
            // Tüm kuyruklar boşsa, pending listesi boşsa ve CPU boşsa program biter.
            if (scheduler_is_empty(scheduler) && scheduler->current_task == NULL) {
                xSemaphoreGive(scheduler->scheduler_mutex);
                vTaskDelay(pdMS_TO_TICKS(1000)); // Çıkmadan önce az bekle
                exit(0); // Programı sonlandır
            }
            
            // Döngü sonu mutex'i bırak
            xSemaphoreGive(scheduler->scheduler_mutex);
        }
    }
}

int main(void) {
    // 1. Scheduler yapısını ilklendir (Kuyrukları, mutex'i oluştur)
    scheduler_init(&g_scheduler);

    // 2. Dosyadan görevleri oku
    if (load_tasks_from_file("giris.txt", &g_scheduler) <= 0) return -1;

    // 3. Dispatcher (Yönetici) görevini başlat
    // Bu görev diğer tüm görevleri yönetecek olan ana mekanizmadır.
    xTaskCreate(dispatcher_task, "Dispatcher", configMINIMAL_STACK_SIZE * 4, (void*)&g_scheduler, configMAX_PRIORITIES - 1, NULL);

    // 4. FreeRTOS Çekirdeğini (Kernel) Başlat
    // Bu noktadan sonra kontrol FreeRTOS'a geçer ve dispatcher_task çalışmaya başlar.
    vTaskStartScheduler();

    return 0;
}