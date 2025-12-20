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
 * scheduler.h'deki prototipe (Task_t* task_create(uint32_t, uint32_t, uint32_t, uint32_t)) uyumlu hale getirildi.
 */
Task_t* task_create(uint32_t task_id, uint32_t arrival_time, uint32_t priority, uint32_t duration) {
    // 1. Bellek Tahsisi
    Task_t* new_task = (Task_t*)malloc(sizeof(Task_t));
    if (new_task == NULL) {
        return NULL; // Bellek hatası
    }

    // 2. İsim Ataması Kısmı
    // scheduler.h'de task_name[32] bir dizi olduğu için malloc yapılmaz, 
    // doğrudan sprintf ile içine yazılır.
    sprintf(new_task->task_name, "T%u", task_id);

    // 3. Değişkenlerin Atanması Kısmı (scheduler.h'deki isimlere göre)
    new_task->task_id = task_id;          // id -> task_id
    new_task->arrival_time = arrival_time;
    new_task->priority = priority;
    new_task->total_duration = duration;  // burst_time -> total_duration
    new_task->remaining_time = duration;
    
    new_task->creation_time = 0;   // Dispatch edilince ayarlanabilir
    new_task->start_time = 0;      // Henüz başlamadı
    
    // --- Bekleme Süresi ---
    // Görev oluşturulduğu (geldiği) an, bekleme süresi başlar.
    new_task->abs_wait_start = arrival_time; 

    new_task->task_handle = NULL;  // FreeRTOS handle henüz yok
    new_task->is_running = false;
    new_task->next = NULL;

    return new_task;
}

/* * Görev Silme ve Bellek Temizleme Fonksiyonuları
 */
void task_destroy(Task_t* task) {
    if (task == NULL) return;
    
    // task_name sabit bir dizi olduğu için (char[32]), free(task->task_name) YAPILMAZ.
    // Sadece struct'ın kendisi free edilir.
    
    free(task);
}