/*
 * SCHEDULER. H - FreeRTOS Process Scheduler Header Dosyasi
 * 
 * Cok seviyeli geri beslemeli kuyruk (MLFQ) zamanlayici sisteminin
 * veri yapilari ve fonksiyon prototiplerini icerir.
 */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdint.h>
#include <stdbool.h>

/*
 * Oncelik Seviyeleri
 * Dusuk sayi = Yuksek oncelik (FreeRTOS standardi)
 */
#define PRIORITY_RT 0      // Real Time
#define PRIORITY_HIGH 1    // Yüksek
#define PRIORITY_MEDIUM 2  // Orta
#define PRIORITY_LOW 3     // Düşük

// Hoca çıktısında öncelik 5'e kadar düşüyor, sınırı genişlettik. 
#define MAX_PRIORITY_LEVELS 20 
#define TIME_QUANTUM 1  // 1 saniye simülasyon

/*
 * Task_t - Gorev Yapisi
 * 
 * Her bir process'i temsil eder.  MLFQ algoritmasinda oncelik degisimi
 * ve timeout kontrolu icin gereken tum zamanlama bilgilerini icerir.
 */
typedef struct Task {
    uint32_t task_id;           
    uint32_t arrival_time;      
    uint32_t priority;          
    uint32_t total_duration;    
    uint32_t remaining_time;    
    uint32_t start_time;        
    uint32_t abs_wait_start;    // Timeout hesabi icin:  son CPU'ya girme zamani
    uint32_t creation_time;     
    bool is_running;            
    char task_name[32];         
    TaskHandle_t task_handle;   
    struct Task* next;          // Linked-list pointer
} Task_t;

/*
 * PriorityQueue_t - FIFO Kuyruk Yapisi
 * 
 * Her oncelik seviyesi icin ayri bir kuyruk.  Linked-list implementasyonu. 
 */
typedef struct PriorityQueue {
    Task_t* head;               
    Task_t* tail;               
    uint32_t count;             
} PriorityQueue_t;

/*
 * Scheduler_t - Ana Zamanlayici Yapisi
 * 
 * MLFQ algoritmasinin tum durumunu tutar. Her oncelik seviyesi icin
 * ayri kuyruk, bekleyen gorevler listesi ve mutex ile thread-safe erisim.
 */
typedef struct Scheduler {
    PriorityQueue_t queues[MAX_PRIORITY_LEVELS];
    uint32_t current_time;      
    Task_t* current_task;       
    uint32_t task_counter;      
    Task_t* pending_tasks;      // Henuz varis zamani gelmemis gorevler
    SemaphoreHandle_t scheduler_mutex;
} Scheduler_t;

/*
 * ANSI Renk Kodlari
 * Terminal ciktisinda gorevleri farkli renklerde gostermek icin
 */
#define COLOR_RESET       "\033[0m"

// Standart Parlak (Bold) Renkler
#define COLOR_RED         "\033[1;31m"
#define COLOR_GREEN       "\033[1;32m"
#define COLOR_YELLOW      "\033[1;33m"
#define COLOR_BLUE        "\033[1;34m"
#define COLOR_MAGENTA     "\033[1;35m"
#define COLOR_CYAN        "\033[1;36m"
#define COLOR_WHITE       "\033[1;37m"

// Ekstra Özel Renkler (256-Color Mode)
#define COLOR_ORANGE      "\033[38;5;208m" // Turuncu
#define COLOR_PURPLE      "\033[38;5;129m" // Mor
#define COLOR_PINK        "\033[38;5;205m" // Pembe
#define COLOR_LIME        "\033[38;5;118m" // Açık Yeşil (Lime)
#define COLOR_TEAL        "\033[38;5;37m"  // Turkuaz/Teal
#define COLOR_NAVY        "\033[38;5;19m"  // Lacivert
#define COLOR_BROWN       "\033[38;5;94m"  // Kahverengi
#define COLOR_GRAY        "\033[38;5;240m" // Koyu Gri
#define COLOR_INDIGO      "\033[38;5;54m"  // İndigo

/*
 * Fonksiyon Prototipleri
 */

void scheduler_init(Scheduler_t* scheduler);

/*
 * task_create - Yeni gorev olusturur
 * 
 * Bellekte Task_t yapisi ayirir ve parametrelerle doldurur.
 * FreeRTOS task'i henuz yaratilmaz, sadece veri yapisi hazirlanir.
 */
Task_t* task_create(uint32_t task_id, uint32_t arrival_time, uint32_t priority, uint32_t duration);

void task_destroy(Task_t* task);
void queue_init(PriorityQueue_t* queue);
void queue_enqueue(PriorityQueue_t* queue, Task_t* task);
Task_t* queue_dequeue(PriorityQueue_t* queue);
bool queue_is_empty(PriorityQueue_t* queue);

/*
 * scheduler_add_task - Gorevi uygun oncelik kuyruguna ekler
 * 
 * Gorev, mevcut priority degerine gore queues[priority] kuyruguna eklenir.
 */
void scheduler_add_task(Scheduler_t* scheduler, Task_t* task);

/*
 * scheduler_add_pending_task - Henuz varis zamani gelmemis gorev ekler
 * 
 * Gorev, varis zamani gelene kadar pending_tasks linked-listinde bekler.
 */
void scheduler_add_pending_task(Scheduler_t* scheduler, Task_t* task);

/*
 * scheduler_check_arrivals - Bekleyen gorevleri kontrol eder
 * 
 * pending_tasks listesindeki gorevlerden arrival_time <= current_time
 * olanlari uygun kuyruga aktarir.
 */
void scheduler_check_arrivals(Scheduler_t* scheduler);

/*
 * scheduler_get_next_task - MLFQ algoritmasina gore gorev secer
 * 
 * Once RT (priority=0) kuyruguna bakar.  Bossa, oncelik sirasina gore
 * diger kuyruklari tarar ve ilk bos olmayan kuyruktan gorev alir.
 */
Task_t* scheduler_get_next_task(Scheduler_t* scheduler);

/*
 * scheduler_demote_task - Gorevin onceligini dusurur (aging)
 * 
 * Zaman dilimini tamamlayan gorevler bir alt oncelik seviyesine iner.
 * MAX_PRIORITY_LEVELS'e ulasan gorevler daha fazla dusmez.
 */
void scheduler_demote_task(Scheduler_t* scheduler, Task_t* task);

/*
 * scheduler_check_timeouts - 20 saniye CPU alamayan gorevleri sonlandirir
 * 
 * abs_wait_start degerine gore timeout hesabi yapar.
 * Starvation'i onlemek icin kritik fonksiyon.
 */
void scheduler_check_timeouts(Scheduler_t* scheduler);

void print_task_info(Task_t* task, const char* event, uint32_t current_time);
void print_task_info_with_old_priority(Task_t* task, const char* event, uint32_t current_time, uint32_t old_priority);
const char* get_color_for_priority(uint32_t priority);
const char* get_priority_name(uint32_t priority);
bool scheduler_is_empty(Scheduler_t* scheduler);

// tasks.c içindeki fonksiyon
void task_function(void* pvParameters);

#endif // SCHEDULER_H
