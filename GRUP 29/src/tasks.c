#include "scheduler.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* * Görev Fonksiyonu (FreeRTOS Tarafından Çalıştırılan) 
 * Bu fonksiyon FreeRTOS'un task yapısına uygundur.
 */
void task_function(void* pvParameters) {
    Task_t* task = (Task_t*)pvParameters;
    
    if (task == NULL) { 
        vTaskDelete(NULL); 
        return; 
    }
    
    task->is_running = true;
    
    // Sonsuz döngü: Görev kendini asla bitirmez, Dispatcher onu yönetir.
    // Ancak CPU'yu serbest bırakmak için delay koyuyoruz.
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    }
}

/* * Görev Oluşturma ve Başlatma Fonksiyonu 
 * scheduler.h'deki prototipe uyumlu hale getirildi.
 */
Task_t* task_create(uint32_t task_id, uint32_t arrival_time, uint32_t priority, uint32_t duration) {
    // 1. Bellek Tahsisi
    Task_t* new_task = (Task_t*)malloc(sizeof(Task_t));
    if (new_task == NULL) {
        return NULL; // Bellek hatası
    }

    // 2. İsim Ataması
    // scheduler.h'de task_name[16] tanımlı olduğu için buffer overflow olmamasına dikkat ediyoruz.
    snprintf(new_task->task_name, sizeof(new_task->task_name), "T%u", task_id);

    // 3. Değişkenlerin Atanması
    new_task->task_id = task_id;
    new_task->arrival_time = arrival_time;
    new_task->priority = priority;
    
    // DÜZELTME: scheduler.h yapısında 'total_duration' değil 'burst_time' var.
    new_task->burst_time = duration;  
    
    new_task->remaining_time = duration;
    
    // Zamanlayıcı double kullandığı için 0.0 atıyoruz
    new_task->creation_time = 0.0;   
    new_task->start_time = 0.0;      
    
    // --- Bekleme Süresi ---
    // Double tipine cast ederek atıyoruz.
    new_task->abs_wait_start = (double)arrival_time; 

    new_task->task_handle = NULL;  // FreeRTOS handle henüz yok
    new_task->is_running = false;
    new_task->next = NULL;

    return new_task;
}

/* * Görev Silme ve Bellek Temizleme Fonksiyonu 
 */
void task_destroy(Task_t* task) {
    if (task == NULL) return;
    
    // task_name sabit bir dizi olduğu için free edilmez.
    // Sadece struct'ın kendisi free edilir.
    free(task);
}