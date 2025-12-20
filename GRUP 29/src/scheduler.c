#include "scheduler.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* * DİKKAT: task_create ve task_destroy fonksiyonları tasks.c dosyasına taşınmıştır. */

/**
 * @brief Zamanlayıcıyı başlatır.
 */
void scheduler_init(Scheduler_t* scheduler) {
    if (scheduler == NULL) return;
    
    for (int i = 0; i < MAX_PRIORITY_LEVELS; i++) {
        queue_init(&scheduler->queues[i]);
    }
    
    scheduler->current_time = 0.0;     // DEĞİŞİKLİK: Ondalıklı başlatma (0.0)
    scheduler->current_task = NULL;
    scheduler->task_counter = 0;
    scheduler->pending_tasks = NULL;
    scheduler->scheduler_mutex = xSemaphoreCreateMutex();
}

void queue_init(PriorityQueue_t* queue) {
    if (queue == NULL) return;
    queue->head = NULL; 
    queue->tail = NULL; 
    queue->count = 0;
}

void queue_enqueue(PriorityQueue_t* queue, Task_t* task) {
    if (queue == NULL || task == NULL) return;
    task->next = NULL;
    if (queue->head == NULL) { 
        queue->head = task; 
        queue->tail = task; 
    } else { 
        queue->tail->next = task; 
        queue->tail = task; 
    }
    queue->count++;
}

Task_t* queue_dequeue(PriorityQueue_t* queue) {
    if (queue == NULL || queue->head == NULL) return NULL;
    Task_t* task = queue->head;
    queue->head = queue->head->next;
    if (queue->head == NULL) queue->tail = NULL;
    queue->count--;
    task->next = NULL;
    return task;
}

bool queue_is_empty(PriorityQueue_t* queue) {
    return (queue == NULL || queue->head == NULL);
}

void scheduler_add_task(Scheduler_t* scheduler, Task_t* task) {
    if (scheduler == NULL || task == NULL) return;
    if (xSemaphoreTake(scheduler->scheduler_mutex, portMAX_DELAY) == pdTRUE) {
        if (task->priority < MAX_PRIORITY_LEVELS) {
            queue_enqueue(&scheduler->queues[task->priority], task);
        }
        xSemaphoreGive(scheduler->scheduler_mutex);
    }
}

void scheduler_add_pending_task(Scheduler_t* scheduler, Task_t* task) {
    if (scheduler == NULL || task == NULL) return;
    task->next = NULL; 
    if (scheduler->pending_tasks == NULL) scheduler->pending_tasks = task;
    else {
        Task_t* current = scheduler->pending_tasks;
        while (current->next != NULL) current = current->next;
        current->next = task;
    }
}

/**
 * @brief Varış zamanı gelen görevleri kontrol eder.
 */
void scheduler_check_arrivals(Scheduler_t* scheduler) {
    if (scheduler == NULL) return;
    Task_t* current = scheduler->pending_tasks;
    Task_t* prev = NULL;
    
    while (current != NULL) {
        // Double (current_time) ile integer (arrival_time) kıyaslaması güvenlidir.
        if (current->arrival_time <= scheduler->current_time) {
            
            if (prev == NULL) scheduler->pending_tasks = current->next;
            else prev->next = current->next;
            
            Task_t* task_to_add = current;
            current = current->next; 
            task_to_add->next = NULL;
            
            if (task_to_add->creation_time == 0) {
                task_to_add->creation_time = scheduler->current_time;
            }
            
            // DEĞİŞİKLİK: Bekleme süresi başlangıcını tam o anki hassas süre olarak kaydet
            task_to_add->abs_wait_start = scheduler->current_time;

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
 * @brief Zaman aşımı kontrolü (Hassas zamanlı)
 */
void scheduler_check_timeouts(Scheduler_t* scheduler) {
    if (scheduler == NULL) return;
    
    for (int priority = 1; priority < MAX_PRIORITY_LEVELS; priority++) {
        PriorityQueue_t* q = &scheduler->queues[priority];
        if (q->head == NULL) continue;
        
        Task_t* curr = q->head;
        Task_t* prev = NULL;
        
        while (curr != NULL) {
            // DEĞİŞİKLİK: Farkı double olarak hesapla (Örn: 20.005 >= 20.0)
            if ((scheduler->current_time - curr->abs_wait_start) >= 20.0) {
                
                print_task_info(curr, "TIMEOUT", scheduler->current_time);
                
                Task_t* to_delete = curr;
                
                if (prev == NULL) { 
                    q->head = curr->next; 
                    curr = q->head; 
                } else { 
                    prev->next = curr->next; 
                    curr = curr->next; 
                }
                
                if (curr == NULL) q->tail = prev;
                q->count--;
                
                if (to_delete->task_handle != NULL) vTaskDelete(to_delete->task_handle);
                task_destroy(to_delete);
                
            } else {
                prev = curr;
                curr = curr->next;
            }
        }
    }
}

Task_t* scheduler_get_next_task(Scheduler_t* scheduler) {
    if (scheduler == NULL) return NULL;
    
    if (!queue_is_empty(&scheduler->queues[PRIORITY_RT])) {
        return queue_dequeue(&scheduler->queues[PRIORITY_RT]);
    }
    
    for (int priority = PRIORITY_HIGH; priority < MAX_PRIORITY_LEVELS; priority++) {
        if (!queue_is_empty(&scheduler->queues[priority])) {
            return queue_dequeue(&scheduler->queues[priority]);
        }
    }
    return NULL;
}

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

// DEĞİŞİKLİK: Parametre double olarak güncellendi
void print_task_info(Task_t* task, const char* event, double current_time) {
    print_task_info_with_old_priority(task, event, current_time, task->priority);
}

// DEĞİŞİKLİK: Parametre double ve format %.4f olarak güncellendi
void print_task_info_with_old_priority(Task_t* task, const char* event, double current_time, uint32_t old_priority) {
    (void)old_priority; 
    if (task == NULL) return;
    
    const char* color = get_color_for_task(task->task_id);
    const char* event_tr = translate_event_name(event);
    uint32_t disp_time = task->remaining_time;
    
    if(strcmp(event, "TIMEOUT") == 0 || strcmp(event, "COMPLETED") == 0) {
        disp_time = 0;
    }

    // GÖRSEL FORMAT DEĞİŞİKLİĞİ:
    // %u.0000 yerine %.4f kullanılarak gerçek ondalıklı değer yazdırılır.
    printf("%s%.4f sn proses %s(id:%04u öncelik:%u kalan süre:%u sn)%s\n",
           color, current_time, event_tr, task->task_id, task->priority, disp_time, COLOR_RESET);
    fflush(stdout);
}

void scheduler_demote_task(Scheduler_t* scheduler, Task_t* task) {
    if (scheduler == NULL || task == NULL) return;
    if (task->priority == PRIORITY_RT) return;
    
    if (task->priority < (MAX_PRIORITY_LEVELS - 1)) {
        task->priority++;
        if (task->task_handle != NULL) {
            vTaskPrioritySet(task->task_handle, configMAX_PRIORITIES - 2);
        }
    }
}

bool scheduler_is_empty(Scheduler_t* scheduler) {
    if (scheduler == NULL) return true;
    for (int i = 0; i < MAX_PRIORITY_LEVELS; i++) {
        if (!queue_is_empty(&scheduler->queues[i])) return false;
    }
    if (scheduler->pending_tasks != NULL) return false;
    return true;
}