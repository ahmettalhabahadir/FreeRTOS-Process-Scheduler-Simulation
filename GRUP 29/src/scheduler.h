#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdint.h>
#include <stdbool.h>

// Öncelik seviyeleri
#define PRIORITY_RT 0      // Real Time
#define PRIORITY_HIGH 1    // Yüksek
#define PRIORITY_MEDIUM 2  // Orta
#define PRIORITY_LOW 3     // Düşük

// Hoca çıktısında öncelik 5'e kadar düşüyor, sınırı genişlettik.
#define MAX_PRIORITY_LEVELS 20 
#define TIME_QUANTUM 1000  // 1000ms = 1 saniye (FreeRTOS Tick'i ms bazlıdır)

// Görev yapısı
typedef struct Task {
    uint32_t task_id;           
    uint32_t arrival_time;      
    uint32_t priority;          
    uint32_t total_duration;    
    uint32_t remaining_time;    
    uint32_t start_time;        
    
    // DEĞİŞİKLİK: Hassas zaman takibi için double yapıldı
    double abs_wait_start;      // Kuyruğa giriş zamanı (timeout kontrolü için)
    double creation_time;       // Sisteme giriş zamanı
    
    bool is_running;            
    char task_name[32];         
    TaskHandle_t task_handle;   
    struct Task* next;          
} Task_t;

// Öncelik kuyruğu yapısı
typedef struct PriorityQueue {
    Task_t* head;               
    Task_t* tail;               
    uint32_t count;             
} PriorityQueue_t;

// Scheduler yapısı
typedef struct Scheduler {
    PriorityQueue_t queues[MAX_PRIORITY_LEVELS];
    
    // DEĞİŞİKLİK: Zaman artık ondalıklı (saniye.milisaniye) tutuluyor
    double current_time;      
    
    Task_t* current_task;       
    uint32_t task_counter;      
    Task_t* pending_tasks;      
    SemaphoreHandle_t scheduler_mutex;
} Scheduler_t;

// --- GENİŞLETİLMİŞ RENK KODLARI (ANSI) ---
#define COLOR_RESET       "\033[0m"
#define COLOR_RED         "\033[1;31m"
#define COLOR_GREEN       "\033[1;32m"
#define COLOR_YELLOW      "\033[1;33m"
#define COLOR_BLUE        "\033[1;34m"
#define COLOR_MAGENTA     "\033[1;35m"
#define COLOR_CYAN        "\033[1;36m"
#define COLOR_WHITE       "\033[1;37m"
#define COLOR_ORANGE      "\033[38;5;208m"
#define COLOR_PURPLE      "\033[38;5;129m" 
#define COLOR_PINK        "\033[38;5;205m" 
#define COLOR_LIME        "\033[38;5;118m" 
#define COLOR_TEAL        "\033[38;5;37m"  
#define COLOR_NAVY        "\033[38;5;19m"  
#define COLOR_BROWN       "\033[38;5;94m"  
#define COLOR_GRAY        "\033[38;5;240m" 
#define COLOR_INDIGO      "\033[38;5;54m"  

// Fonksiyon prototipleri
void scheduler_init(Scheduler_t* scheduler);
Task_t* task_create(uint32_t task_id, uint32_t arrival_time, uint32_t priority, uint32_t duration);
void task_destroy(Task_t* task);
void queue_init(PriorityQueue_t* queue);
void queue_enqueue(PriorityQueue_t* queue, Task_t* task);
Task_t* queue_dequeue(PriorityQueue_t* queue);
bool queue_is_empty(PriorityQueue_t* queue);
void scheduler_add_task(Scheduler_t* scheduler, Task_t* task);
void scheduler_add_pending_task(Scheduler_t* scheduler, Task_t* task);
void scheduler_check_arrivals(Scheduler_t* scheduler);
Task_t* scheduler_get_next_task(Scheduler_t* scheduler);
void scheduler_demote_task(Scheduler_t* scheduler, Task_t* task);
void scheduler_check_timeouts(Scheduler_t* scheduler);

// DEĞİŞİKLİK: uint32_t yerine double kullanıldı
void print_task_info(Task_t* task, const char* event, double current_time);
void print_task_info_with_old_priority(Task_t* task, const char* event, double current_time, uint32_t old_priority);

const char* get_color_for_priority(uint32_t priority);
const char* get_priority_name(uint32_t priority);
bool scheduler_is_empty(Scheduler_t* scheduler);

// tasks.c içindeki fonksiyon
void task_function(void* pvParameters);

#endif // SCHEDULER_H