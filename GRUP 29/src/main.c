#include "scheduler.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h> // Zaman fonksiyonları için

// Global Scheduler nesnesi
Scheduler_t g_scheduler;

// Gerçek zamanı (saniye cinsinden ondalıklı) hesaplayan yardımcı makro
// Varsayım: configTICK_RATE_HZ 1000 olarak ayarlı (1 tick = 1 ms)
#define GET_REAL_TIME() ((double)xTaskGetTickCount() / (double)configTICK_RATE_HZ)

/**
 * @brief Dosyadan görevleri okur.
 */
int load_tasks_from_file(const char* filename, Scheduler_t* scheduler) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) { printf("Hata: %s dosyası açılamadı!\n", filename); return -1; }
    char line[256];
    int task_count = 0;
    while (fgets(line, sizeof(line), file) != NULL) {
        if (line[0] == '\n' || line[0] == '#' || line[0] == '\r') continue;
        line[strcspn(line, "\r\n")] = 0;
        uint32_t arrival_time, priority, duration;
        if (sscanf(line, "%u, %u, %u", &arrival_time, &priority, &duration) != 3) continue;
        
        // Task oluştur
        Task_t* task = task_create(scheduler->task_counter++, arrival_time, priority, duration);
        if (task) {
            scheduler_add_pending_task(scheduler, task);
            task_count++;
        }
    }
    fclose(file);
    return task_count;
}

/**
 * @brief FreeRTOS görevi oluşturur.
 */
BaseType_t create_freertos_task_for_scheduler(Task_t* task) {
    if (task == NULL) return pdFAIL;
    UBaseType_t prio = (task->priority == PRIORITY_RT) ? configMAX_PRIORITIES - 1 : configMAX_PRIORITIES - 2;
    return xTaskCreate(task_function, task->task_name, configMINIMAL_STACK_SIZE * 4, (void*)task, prio, &task->task_handle);
}

/**
 * @brief DISPATCHER GÖREVİ (GÜNCELLENDİ)
 * Artık gerçek zamanlı çalışır ve zamanı FreeRTOS tick'lerinden okur.
 */
void dispatcher_task(void* pvParameters) {
    Scheduler_t* scheduler = (Scheduler_t*)pvParameters;
    
    // Başlangıçta zamanı sıfırla (ondalıklı olarak)
    scheduler->current_time = 0.0; 
    
    while (1) {
        if (xSemaphoreTake(scheduler->scheduler_mutex, portMAX_DELAY) == pdTRUE) {
            
            // --- 1. ZAMANI GÜNCELLE ---
            // FreeRTOS'un çalıştığı gerçek süreyi alıyoruz.
            scheduler->current_time = GET_REAL_TIME();
            
            bool just_started = false;

            // --- 2. YENİ GELENLERİ KONTROL ET ---
            scheduler_check_arrivals(scheduler);
            
            // --- 3. TIMEOUT KONTROLÜ ---
            scheduler_check_timeouts(scheduler); 
            
            // --- 4. GÖREV SEÇİMİ ---
            if (scheduler->current_task == NULL) {
                Task_t* next_task = scheduler_get_next_task(scheduler);
                if (next_task != NULL) {
                    scheduler->current_task = next_task;
                    
                    if (next_task->priority != PRIORITY_RT) {
                        next_task->start_time = scheduler->current_time; 
                    }

                    if (next_task->task_handle == NULL) {
                        create_freertos_task_for_scheduler(next_task);
                        
                        next_task->creation_time = scheduler->current_time;
                        next_task->abs_wait_start = scheduler->current_time; 
                        
                        // NOT: print_task_info artık double kabul etmeli
                        print_task_info(next_task, "STARTED", scheduler->current_time);
                        just_started = true; 
                    } 
                    else {
                        vTaskResume(next_task->task_handle);
                        print_task_info(next_task, "STARTED", scheduler->current_time);
                        just_started = true; 
                    }
                }
            }

            Task_t* current = scheduler->current_task;
            
            if (current != NULL) {
                // --- YÜRÜTME ---
                
                if (current->priority == PRIORITY_RT && !just_started) {
                     print_task_info(current, "RUNNING", scheduler->current_time);
                }
                
                scheduler_check_timeouts(scheduler);

                // İşlem süresinden düş (Saniye bazlı düşüş)
                // Burası mantıksal kalan süredir, fiziksel süreyle senkronize azalır.
                if (current->remaining_time > 0) current->remaining_time--;
                
                // --- FİZİKSEL BEKLEME (REAL TIME DELAY) ---
                xSemaphoreGive(scheduler->scheduler_mutex);
                
                // Program burada GERÇEKTEN belirtilen süre kadar bekler.
                // TIME_QUANTUM genellikle 1000 (ms) yani 1 saniyedir.
                vTaskDelay(pdMS_TO_TICKS(TIME_QUANTUM)); 
                
                xSemaphoreTake(scheduler->scheduler_mutex, portMAX_DELAY);
                
                // --- SONUÇ KONTROLÜ ---
                // Zamanı tekrar güncelle (Bekleme sonrası zaman ilerledi)
                scheduler->current_time = GET_REAL_TIME();
                
                if (current->remaining_time == 0) {
                    print_task_info(current, "COMPLETED", scheduler->current_time);
                    current->is_running = false;
                    if (current->task_handle != NULL) vTaskDelete(current->task_handle);
                    task_destroy(current);
                    scheduler->current_task = NULL;
                }
                else if (current->priority != PRIORITY_RT) {
                    uint32_t old_priority = current->priority;
                    scheduler_demote_task(scheduler, current); 
                    
                    if (current->task_handle != NULL) vTaskSuspend(current->task_handle);
                    
                    if (current->priority < MAX_PRIORITY_LEVELS) {
                        current->abs_wait_start = scheduler->current_time;
                        queue_enqueue(&scheduler->queues[current->priority], current);
                    }
                    
                    print_task_info_with_old_priority(current, "SUSPENDED", scheduler->current_time, old_priority);
                    scheduler->current_task = NULL;
                }
                
            } else {
                // IDLE DURUM (CPU Boş)
                xSemaphoreGive(scheduler->scheduler_mutex);
                // Boşta da olsa gerçek zaman geçmeli
                vTaskDelay(pdMS_TO_TICKS(TIME_QUANTUM)); 
                xSemaphoreTake(scheduler->scheduler_mutex, portMAX_DELAY);
            }
            
            // Simülasyon Bitiş Kontrolü
            if (scheduler_is_empty(scheduler) && scheduler->current_task == NULL) {
                xSemaphoreGive(scheduler->scheduler_mutex);
                vTaskDelay(pdMS_TO_TICKS(1000));
                exit(0);
            }
            
            xSemaphoreGive(scheduler->scheduler_mutex);
        }
    }
}

int main(void) {
    scheduler_init(&g_scheduler);
    if (load_tasks_from_file("giris.txt", &g_scheduler) <= 0) return -1;
    xTaskCreate(dispatcher_task, "Dispatcher", configMINIMAL_STACK_SIZE * 4, (void*)&g_scheduler, configMAX_PRIORITIES - 1, NULL);
    vTaskStartScheduler();
    return 0;
}