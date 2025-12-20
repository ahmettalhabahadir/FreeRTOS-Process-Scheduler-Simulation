#include "scheduler.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* * DİKKAT: task_create ve task_destroy fonksiyonları tasks.c dosyasına taşınmıştır.
 * Bu ayrım, kodun modülerliğini artırmak ve "Multiple Definition" hatasını önlemek içindir.
 */

/**
 * @brief Zamanlayıcıyı (Scheduler) başlatır ve varsayılan değerleri atar.
 * @param scheduler Başlatılacak scheduler yapısı.
 */
void scheduler_init(Scheduler_t* scheduler) {
    if (scheduler == NULL) return;
    
    // Tüm öncelik kuyruklarını (0, 1, 2, 3) başlat
    for (int i = 0; i < MAX_PRIORITY_LEVELS; i++) {
        queue_init(&scheduler->queues[i]);
    }
    
    scheduler->current_time = 0;       // Simülasyon saatini sıfırla
    scheduler->current_task = NULL;    // Şu an çalışan görev yok
    scheduler->task_counter = 0;       // ID sayacını sıfırla
    scheduler->pending_tasks = NULL;   // Bekleyenler listesi boş
    
    // İş parçacığı güvenliği (Thread-safety) için Mutex oluştur
    scheduler->scheduler_mutex = xSemaphoreCreateMutex();
}

/**
 * @brief Bir kuyruk yapısını (Linked List) başlatır.
 */
void queue_init(PriorityQueue_t* queue) {
    if (queue == NULL) return;
    queue->head = NULL; // Liste başı boş
    queue->tail = NULL; // Liste sonu boş
    queue->count = 0;   // Eleman sayısı 0
}

/**
 * @brief Kuyruğun sonuna eleman ekler (FIFO mantığı).
 * @param queue Hedef kuyruk
 * @param task Eklenecek görev
 */
void queue_enqueue(PriorityQueue_t* queue, Task_t* task) {
    if (queue == NULL || task == NULL) return;
    
    task->next = NULL; // Yeni gelen en sonda olacağı için next'i NULL'dır.
    
    // Eğer kuyruk boşsa, hem baş hem son bu yeni görevdir.
    if (queue->head == NULL) { 
        queue->head = task; 
        queue->tail = task; 
    } else { 
        // Kuyruk doluysa, şu anki son elemanın next'ini yeni göreve bağla
        queue->tail->next = task; 
        // Kuyruğun yeni sonu (tail) bu görev olsun
        queue->tail = task; 
    }
    queue->count++;
}

/**
 * @brief Kuyruğun başındaki elemanı çıkarır ve döndürür (FIFO mantığı).
 * @return Task_t* Çıkarılan görev veya NULL
 */
Task_t* queue_dequeue(PriorityQueue_t* queue) {
    if (queue == NULL || queue->head == NULL) return NULL;
    
    Task_t* task = queue->head;       // Baştaki elemanı al
    queue->head = queue->head->next;  // Baş işaretçisini bir sonrakine kaydır
    
    // Eğer listede eleman kalmadıysa tail'i de boşa çıkar
    if (queue->head == NULL) queue->tail = NULL;
    
    queue->count--;
    task->next = NULL; // Çıkarılan elemanın liste ile bağını kopar
    return task;
}

/**
 * @brief Kuyruğun boş olup olmadığını kontrol eder.
 */
bool queue_is_empty(PriorityQueue_t* queue) {
    return (queue == NULL || queue->head == NULL);
}

/**
 * @brief Thread-safe (güvenli) bir şekilde sisteme görev ekler.
 * Genellikle dışarıdan manuel görev eklemek için kullanılır.
 */
void scheduler_add_task(Scheduler_t* scheduler, Task_t* task) {
    if (scheduler == NULL || task == NULL) return;
    
    // Mutex alarak veri bütünlüğünü koru
    if (xSemaphoreTake(scheduler->scheduler_mutex, portMAX_DELAY) == pdTRUE) {
        if (task->priority < MAX_PRIORITY_LEVELS) {
            queue_enqueue(&scheduler->queues[task->priority], task);
        }
        xSemaphoreGive(scheduler->scheduler_mutex);
    }
}

/**
 * @brief Henüz varış zamanı gelmemiş görevleri "Bekleyenler" listesine ekler.
 * Bu liste bir kuyruk değil, varış zamanına göre taranacak düz bir bağlı listedir.
 */
void scheduler_add_pending_task(Scheduler_t* scheduler, Task_t* task) {
    if (scheduler == NULL || task == NULL) return;
    task->next = NULL; 
    
    if (scheduler->pending_tasks == NULL) {
        scheduler->pending_tasks = task;
    } else {
        // Listenin sonuna kadar git ve ekle
        Task_t* current = scheduler->pending_tasks;
        while (current->next != NULL) current = current->next;
        current->next = task;
    }
}

/**
 * @brief Varış zamanı (Arrival Time) gelen görevleri kontrol eder.
 * Pending listesinden çıkarıp ilgili öncelik kuyruğuna (Ready Queue) taşır.
 */
void scheduler_check_arrivals(Scheduler_t* scheduler) {
    if (scheduler == NULL) return;
    
    Task_t* current = scheduler->pending_tasks;
    Task_t* prev = NULL;
    
    while (current != NULL) {
        // Görevin varış zamanı şimdiki zamana eşit veya küçük mü?
        if (current->arrival_time <= scheduler->current_time) {
            
            // --- 1. Bağlı listeden (Pending) çıkar ---
            if (prev == NULL) scheduler->pending_tasks = current->next; // Baştaysa
            else prev->next = current->next; // Aradaysa
            
            Task_t* task_to_add = current;
            current = current->next; // Döngü için current'ı ilerlet
            task_to_add->next = NULL; // Bağlantıyı temizle
            
            // --- 2. Gerekli zaman damgalarını ayarla ---
            // Eğer görev ilk kez sisteme giriyorsa creation time'ı ata
            if (task_to_add->creation_time == 0) {
                task_to_add->creation_time = scheduler->current_time;
            }
            
            // Kuyruğa girdiği an bekleme sayacı (Timeout kontrolü için) başlar.
            task_to_add->abs_wait_start = scheduler->current_time;

            // --- 3. İlgili öncelik kuyruğuna ekle ---
            if (task_to_add->priority < MAX_PRIORITY_LEVELS) {
                queue_enqueue(&scheduler->queues[task_to_add->priority], task_to_add);
            }
        } else {
            // Varış zamanı gelmediyse bir sonraki elemana geç
            prev = current;
            current = current->next;
        }
    }
}

/**
 * @brief Bekleme süresi 20 saniyeyi geçen görevleri kontrol eder (Starvation Control).
 * @note PRIORITY_RT (0) dışındaki kuyruklar kontrol edilir.
 */
void scheduler_check_timeouts(Scheduler_t* scheduler) {
    if (scheduler == NULL) return;
    
    // DÜZELTME: Döngü 1'den başlar. PRIORITY_RT (0) zamanaşımına uğramaz.
    for (int priority = 1; priority < MAX_PRIORITY_LEVELS; priority++) {
        PriorityQueue_t* q = &scheduler->queues[priority];
        if (q->head == NULL) continue;
        
        Task_t* curr = q->head;
        Task_t* prev = NULL;
        
        while (curr != NULL) {
            // (Şu anki zaman - Kuyruğa giriş zamanı) >= 20 sn mi?
            if ((scheduler->current_time - curr->abs_wait_start) >= 20) {
                
                // Zaman aşımı logunu yazdır
                print_task_info(curr, "TIMEOUT", scheduler->current_time);
                
                Task_t* to_delete = curr;
                
                // --- Görevi Kuyruktan Çıkar ---
                if (prev == NULL) { 
                    q->head = curr->next; 
                    curr = q->head; 
                } else { 
                    prev->next = curr->next; 
                    curr = curr->next; 
                }
                
                if (curr == NULL) q->tail = prev; // Kuyruk sonunu güncelle
                q->count--;
                
                // --- Görevi ve FreeRTOS Kaynaklarını Sil ---
                if (to_delete->task_handle != NULL) vTaskDelete(to_delete->task_handle);
                task_destroy(to_delete);
                
            } else {
                // Zaman aşımı yoksa ilerle
                prev = curr;
                curr = curr->next;
            }
        }
    }
}

/**
 * @brief Yürütülecek bir sonraki görevi seçer.
 * @return Seçilen Task_t işaretçisi veya NULL.
 */
Task_t* scheduler_get_next_task(Scheduler_t* scheduler) {
    if (scheduler == NULL) return NULL;
    
    // 1. Kural: Önce Real-Time (Öncelik 0) görevler kontrol edilir.
    if (!queue_is_empty(&scheduler->queues[PRIORITY_RT])) {
        return queue_dequeue(&scheduler->queues[PRIORITY_RT]);
    }
    
    // 2. Kural: RT yoksa, Yüksek öncelikten Düşük önceliğe doğru tara (1 -> 2 -> 3)
    for (int priority = PRIORITY_HIGH; priority < MAX_PRIORITY_LEVELS; priority++) {
        if (!queue_is_empty(&scheduler->queues[priority])) {
            return queue_dequeue(&scheduler->queues[priority]);
        }
    }
    return NULL; // Çalışacak görev yok
}

/**
 * @brief Görev ID'sine göre terminalde renkli çıktı vermek için renk kodunu döndürür.
 */
const char* get_color_for_task(uint32_t task_id) {
    // 14 farklı renk döngüsü kullanılarak görevlerin görsel olarak ayrışması sağlanır.
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
 * @brief İngilizce olay isimlerini (RUNNING, COMPLETED vb.) Türkçe'ye çevirir.
 */
const char* translate_event_name(const char* event) {
    if (strcmp(event, "READY") == 0) return "başladı";
    if (strcmp(event, "STARTED") == 0) return "başladı";
    if (strcmp(event, "RUNNING") == 0) return "yürütülüyor";
    if (strcmp(event, "COMPLETED") == 0) return "sonlandı";
    if (strcmp(event, "SUSPENDED") == 0) return "askıda";
    if (strcmp(event, "RESUMED") == 0) return "yürütülüyor";
    if (strcmp(event, "TIMEOUT") == 0) return "zamanaşımı";
    return event;
}

/**
 * @brief Görev durumu hakkında ekrana formatlı bilgi yazdırır.
 */
void print_task_info(Task_t* task, const char* event, uint32_t current_time) {
    // Varsayılan olarak görevin şu anki önceliğini kullanır
    print_task_info_with_old_priority(task, event, current_time, task->priority);
}

/**
 * @brief Görev durumunu yazdırır, ancak eski bir öncelik değerini referans alabilir.
 * (Özellikle görev askıya alındığında eski önceliğini göstermek için kullanılır)
 */
void print_task_info_with_old_priority(Task_t* task, const char* event, uint32_t current_time, uint32_t old_priority) {
    (void)old_priority; // Derleyici uyarısını (unused variable) susturmak için
    if (task == NULL) return;
    
    const char* color = get_color_for_task(task->task_id);
    const char* event_tr = translate_event_name(event);
    uint32_t disp_time = task->remaining_time;
    
    // Görev bittiyse veya zaman aşımına uğradıysa kalan süre 0 görünmelidir.
    if(strcmp(event, "TIMEOUT") == 0 || strcmp(event, "COMPLETED") == 0) {
        disp_time = 0;
    }

    // İstenilen çıktı formatı:
    // 0.0000 sn proses başladı (id:0000 öncelik:1 kalan süre:2 sn)
    printf("%s%u.0000 sn proses %s(id:%04u öncelik:%u kalan süre:%u sn)%s\n",
           color, current_time, event_tr, task->task_id, task->priority, disp_time, COLOR_RESET);
    fflush(stdout);
}

/**
 * @brief (Feedback Queue Algoritması)
 * Görevin önceliğini düşürür. RT görevlere dokunulmaz.
 */
void scheduler_demote_task(Scheduler_t* scheduler, Task_t* task) {
    if (scheduler == NULL || task == NULL) return;
    
    // RT görevlerin (0) önceliği asla değişmez.
    if (task->priority == PRIORITY_RT) return;
    
    // Eğer öncelik en alt seviyede değilse bir kademe düşür (sayısal olarak artır)
    if (task->priority < (MAX_PRIORITY_LEVELS - 1)) {
        task->priority++;
        
        // FreeRTOS tarafındaki önceliği de güncelle (Simülasyon akışını bozmamak için düşük tutulur)
        if (task->task_handle != NULL) {
            vTaskPrioritySet(task->task_handle, configMAX_PRIORITIES - 2);
        }
    }
}

/**
 * @brief Sistemin tamamen boş olup olmadığını kontrol eder.
 * @return true: Tüm kuyruklar ve pending listesi boşsa.
 */
bool scheduler_is_empty(Scheduler_t* scheduler) {
    if (scheduler == NULL) return true;
    
    // Kuyrukları kontrol et
    for (int i = 0; i < MAX_PRIORITY_LEVELS; i++) {
        if (!queue_is_empty(&scheduler->queues[i])) return false;
    }
    
    // Bekleyen görev listesini kontrol et
    if (scheduler->pending_tasks != NULL) return false;
    
    return true;
}