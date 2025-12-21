#include "scheduler.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Scheduler yapısını başlatır.
 * Kuyrukları hazırlar, mutex oluşturur ve sayaçları sıfırlar.
 */
void scheduler_init(Scheduler_t* scheduler) {
    if (scheduler == NULL) return;
    
    // Tüm öncelik kuyruklarını (0, 1, 2, 3) başlat
    for (int i = 0; i < MAX_PRIORITY_LEVELS; i++) {
        queue_init(&scheduler->queues[i]);
    }
    
    scheduler->current_time = 0.0;
    scheduler->current_task = NULL;
    scheduler->task_counter = 0;
    scheduler->pending_tasks = NULL; // Henüz zamanı gelmeyenler listesi
    scheduler->scheduler_mutex = xSemaphoreCreateMutex(); // Veri bütünlüğü için Mutex
    scheduler->skip_next_log = false; // Çift log basmayı engelleme bayrağı
}

/**
 * @brief Kuyruk yapısını (Linked List) başlatır.
 */
void queue_init(PriorityQueue_t* queue) {
    if (queue == NULL) return;
    queue->head = NULL; 
    queue->tail = NULL; 
    queue->count = 0;
}

/**
 * @brief Kuyruğun sonuna eleman ekler (FIFO Mantığı).
 */
void queue_enqueue(PriorityQueue_t* queue, Task_t* task) {
    if (queue == NULL || task == NULL) return;
    task->next = NULL;
    
    // Eğer kuyruk boşsa, baş ve kuyruk aynı elemandır
    if (queue->head == NULL) { 
        queue->head = task; 
        queue->tail = task; 
    } else { 
        // Değilse, sona ekle ve kuyruk pointer'ını güncelle
        queue->tail->next = task; 
        queue->tail = task; 
    }
    queue->count++;
}

/**
 * @brief Kuyruğun başından eleman çeker ve döner.
 */
Task_t* queue_dequeue(PriorityQueue_t* queue) {
    if (queue == NULL || queue->head == NULL) return NULL;
    
    Task_t* task = queue->head;
    queue->head = queue->head->next; // Baş pointer'ı bir yana kaydır
    
    if (queue->head == NULL) queue->tail = NULL; // Kuyruk tamamen boşaldıysa
    
    queue->count--;
    task->next = NULL; // Bağlantıyı kopar
    return task;
}

/**
 * @brief Kuyruğun boş olup olmadığını kontrol eder.
 */
bool queue_is_empty(PriorityQueue_t* queue) {
    return (queue == NULL || queue->head == NULL);
}

/**
 * @brief Thread-safe (güvenli) bir şekilde kuyruğa görev ekler.
 * Genellikle dispatcher dışından manuel eklemeler için kullanılır.
 */
void scheduler_add_task(Scheduler_t* scheduler, Task_t* task) {
    if (scheduler == NULL || task == NULL) return;
    
    // Mutex alarak başka threadlerin araya girmesini engelle
    if (xSemaphoreTake(scheduler->scheduler_mutex, portMAX_DELAY) == pdTRUE) {
        if (task->priority < MAX_PRIORITY_LEVELS) {
            queue_enqueue(&scheduler->queues[task->priority], task);
        }
        xSemaphoreGive(scheduler->scheduler_mutex);
    }
}

/**
 * @brief Henüz varış zamanı gelmemiş görevleri "Pending" (Bekleyen) listesine ekler.
 * Bu liste basit bir bağlı listedir (Linked List).
 */
void scheduler_add_pending_task(Scheduler_t* scheduler, Task_t* task) {
    if (scheduler == NULL || task == NULL) return;
    task->next = NULL; 
    
    if (scheduler->pending_tasks == NULL) scheduler->pending_tasks = task;
    else {
        // Listenin sonuna kadar git ve ekle
        Task_t* current = scheduler->pending_tasks;
        while (current->next != NULL) current = current->next;
        current->next = task;
    }
}

/**
 * @brief Bekleyenler listesini tarar ve zamanı gelenleri "Ready" (Hazır) kuyruğuna taşır.
 */
void scheduler_check_arrivals(Scheduler_t* scheduler) {
    if (scheduler == NULL) return;
    Task_t* current = scheduler->pending_tasks;
    Task_t* prev = NULL;
    
    while (current != NULL) {
        // Eğer görevin varış zamanı şimdiki zamana eşit veya küçükse
        if (current->arrival_time <= scheduler->current_time) {
            
            // Listeden çıkar (Bağlantıyı kopar)
            if (prev == NULL) scheduler->pending_tasks = current->next;
            else prev->next = current->next;
            
            Task_t* task_to_add = current;
            current = current->next; // Döngü için bir sonrakine geç
            task_to_add->next = NULL;
            
            // İlk oluşturulma zamanını ve bekleme başlangıcını ayarla
            if (task_to_add->creation_time == 0) {
                task_to_add->creation_time = scheduler->current_time;
            }
            task_to_add->abs_wait_start = scheduler->current_time;

            // İlgili öncelik kuyruğuna (Ready Queue) ekle
            if (task_to_add->priority < MAX_PRIORITY_LEVELS) {
                queue_enqueue(&scheduler->queues[task_to_add->priority], task_to_add);
            }
        } else {
            prev = current;
            current = current->next;
        }
    }
}

/**
 * @brief Kuyrukta çok uzun süre (20 sn) bekleyen görevleri bulur ve siler (Timeout).
 */
void scheduler_check_timeouts(Scheduler_t* scheduler) {
    if (scheduler == NULL) return;
    
    // Öncelik 0 (RT) genelde timeout olmaz, o yüzden 1'den başlatıyoruz.
    for (int priority = 1; priority < MAX_PRIORITY_LEVELS; priority++) {
        PriorityQueue_t* q = &scheduler->queues[priority];
        if (q->head == NULL) continue;
        
        Task_t* curr = q->head;
        Task_t* prev = NULL;
        
        while (curr != NULL) {
            // (Şimdiki Zaman - Kuyruğa Giriş Zamanı) >= 20 saniye mi?
            if ((scheduler->current_time - curr->abs_wait_start) >= 20.0) {
                // Timeout logunu bas
                print_task_info(curr, "TIMEOUT", scheduler->current_time);
                Task_t* to_delete = curr;
                
                // Kuyruktan çıkar
                if (prev == NULL) { 
                    q->head = curr->next; 
                    curr = q->head; 
                } else { 
                    prev->next = curr->next; 
                    curr = curr->next; 
                }
                
                if (curr == NULL) q->tail = prev;
                q->count--;
                
                // FreeRTOS görevini ve belleği temizle
                if (to_delete->task_handle != NULL) vTaskDelete(to_delete->task_handle);
                task_destroy(to_delete);
            } else {
                prev = curr;
                curr = curr->next;
            }
        }
    }
}

/**
 * @brief Çalıştırılacak bir sonraki görevi seçer.
 * Öncelik sırasına göre bakar: Önce RT (0), sonra 1, 2, 3...
 */
Task_t* scheduler_get_next_task(Scheduler_t* scheduler) {
    if (scheduler == NULL) return NULL;
    
    // 1. Önce Gerçek Zamanlı (RT) kuyruğa bak
    if (!queue_is_empty(&scheduler->queues[PRIORITY_RT])) {
        return queue_dequeue(&scheduler->queues[PRIORITY_RT]);
    }
    
    // 2. Sonra diğer öncelikleri sırasıyla kontrol et
    for (int priority = PRIORITY_HIGH; priority < MAX_PRIORITY_LEVELS; priority++) {
        if (!queue_is_empty(&scheduler->queues[priority])) {
            return queue_dequeue(&scheduler->queues[priority]);
        }
    }
    return NULL;
}

/**
 * @brief Görev ID'sine göre renk kodu döndürür (Görselleştirme için).
 */
const char* get_color_for_task(uint32_t task_id) {
    switch (task_id % 14) {
        case 0: return COLOR_YELLOW;
        case 1: return COLOR_BLUE;
        case 2: return COLOR_RED;
        case 3: return COLOR_GREEN;
        case 4: return COLOR_CYAN;
        case 5: return COLOR_MAGENTA;
        case 6: return COLOR_ORANGE;
        case 7: return COLOR_PURPLE;
        case 8: return COLOR_TEAL;
        case 9: return COLOR_PINK;
        case 10: return COLOR_LIME;
        case 11: return COLOR_BROWN;
        case 12: return COLOR_INDIGO;
        case 13: return COLOR_NAVY;
        default: return COLOR_RESET;
    }
}

/**
 * @brief İngilizce olay adlarını Türkçe çıktı formatına çevirir.
 */
const char* translate_event_name(const char* event) {
    if (strcmp(event, "READY") == 0) return "başladı";
    if (strcmp(event, "STARTED") == 0) return "başladı";
    
    // ÖZEL DURUM: Askıdan dönen görev için de "başladı" yazılması istendi.
    if (strcmp(event, "RESUMED") == 0) return "başladı"; 
    
    if (strcmp(event, "RUNNING") == 0) return "yürütülüyor";
    if (strcmp(event, "COMPLETED") == 0) return "sonlandı";
    if (strcmp(event, "SUSPENDED") == 0) return "askıda";
    if (strcmp(event, "TIMEOUT") == 0) return "zamanaşımı";
    return event;
}

/**
 * @brief Görev bilgilerini konsola basan ana fonksiyon.
 */
void print_task_info(Task_t* task, const char* event, double current_time) {
    print_task_info_with_old_priority(task, event, current_time, task->priority);
}

/**
 * @brief Görev bilgilerini (eski öncelik bilgisiyle) konsola basar.
 */
void print_task_info_with_old_priority(Task_t* task, const char* event, double current_time, uint32_t old_priority) {
    (void)old_priority; // Uyarıyı susturmak için
    if (task == NULL) return;
    
    const char* color = get_color_for_task(task->task_id);
    const char* event_tr = translate_event_name(event);
    uint32_t disp_time = task->remaining_time;
    
    // Bitti veya Timeout olduysa kalan süreyi 0 göster
    if(strcmp(event, "TIMEOUT") == 0 || strcmp(event, "COMPLETED") == 0) {
        disp_time = 0;
    }

    printf("%s%.4f sn proses %s(id:%04u öncelik:%u kalan süre:%u sn)%s\n",
           color, current_time, event_tr, task->task_id, task->priority, disp_time, COLOR_RESET);
    fflush(stdout); // Çıktının anında görünmesini sağla
}

/**
 * @brief Görevin önceliğini düşürür (Priority Demotion / Aging).
 * RT görevlerinin (Öncelik 0) önceliği düşürülmez.
 */
void scheduler_demote_task(Scheduler_t* scheduler, Task_t* task) {
    if (scheduler == NULL || task == NULL) return;
    if (task->priority == PRIORITY_RT) return; // RT görevlere dokunma
    
    // Maksimum öncelik seviyesine (en düşük öncelik) ulaşmadıysa artır (sayısal artış = öncelik düşüşü)
    if (task->priority < (MAX_PRIORITY_LEVELS - 1)) {
        task->priority++;
        // FreeRTOS tarafındaki önceliği de güncelle
        if (task->task_handle != NULL) {
            vTaskPrioritySet(task->task_handle, configMAX_PRIORITIES - 2);
        }
    }
}

/**
 * @brief Tüm kuyrukların ve bekleyen listelerin boş olup olmadığını kontrol eder.
 * Simülasyonun bitip bitmediğini anlamak için kullanılır.
 */
bool scheduler_is_empty(Scheduler_t* scheduler) {
    if (scheduler == NULL) return true;
    
    // Kuyrukları kontrol et
    for (int i = 0; i < MAX_PRIORITY_LEVELS; i++) {
        if (!queue_is_empty(&scheduler->queues[i])) return false;
    }
    
    // Bekleyenler listesini kontrol et
    if (scheduler->pending_tasks != NULL) return false;
    
    return true;
}