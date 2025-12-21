#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdint.h>
#include <stdbool.h>

/* * --- RENK TANIMLARI (ANSI Escape Codes) ---
 * Konsol çıktısında her görevin farklı renkte görünmesini sağlar.
 * Debug işlemleri ve görsel takip için kullanılır.
 */
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_WHITE   "\033[37m"
#define COLOR_ORANGE  "\033[38;5;208m"
#define COLOR_PURPLE  "\033[38;5;129m"
#define COLOR_TEAL    "\033[38;5;30m"
#define COLOR_PINK    "\033[38;5;205m"
#define COLOR_LIME    "\033[38;5;118m"
#define COLOR_BROWN   "\033[38;5;94m"
#define COLOR_INDIGO  "\033[38;5;54m"
#define COLOR_NAVY    "\033[38;5;18m"

/* * --- SİMÜLASYON AYARLARI ---
 */
// Toplam öncelik kuyruğu sayısı (0, 1, 2, 3)
#define MAX_PRIORITY_LEVELS 4

// Gerçek Zamanlı (Real-Time) öncelik seviyesi (En yüksek öncelik)
#define PRIORITY_RT 0

// Yüksek öncelik seviyesi (RT olmayan en yüksek)
#define PRIORITY_HIGH 1

// Zaman Dilimi (Time Quantum): Her görevin kesintisiz çalışacağı süre (ms)
#define TIME_QUANTUM 1000 

/*
 * --- GÖREV YAPISI (Process Control Block - PCB) ---
 * Bir görevin tüm durumunu ve özelliklerini tutar.
 */
typedef struct Task {
    uint32_t task_id;         // Görevin benzersiz kimliği (0000, 0001...)
    uint32_t arrival_time;    // Sisteme varış zamanı
    uint32_t priority;        // Güncel öncelik değeri (0-3 arası)
    uint32_t burst_time;      // Toplam çalışması gereken süre
    uint32_t remaining_time;  // Kalan çalışma süresi (Her saniye azalır)
    
    double start_time;        // İşlemciye ilk girdiği an (Loglama için)
    double creation_time;     // Oluşturulma zamanı
    double abs_wait_start;    // Kuyruğa en son giriş zamanı (20 sn Timeout kontrolü için kritik)
    
    TaskHandle_t task_handle; // FreeRTOS tarafındaki görev tutamacı (Handle)
    bool is_running;          // Görev şu an çalışıyor mu?
    char task_name[16];       // Debug için isim (örn: "Task_0")
    
    struct Task* next;        // Bağlı liste (Linked List) için sonraki eleman pointer'ı
} Task_t;

/*
 * --- ÖNCELİK KUYRUĞU YAPISI ---
 * FIFO (First In First Out) mantığıyla çalışan basit bağlı liste.
 */
typedef struct {
    Task_t* head; // Kuyruğun başı (İlk çıkacak eleman)
    Task_t* tail; // Kuyruğun sonu (Yeni eklenen eleman)
    int count;    // Kuyruktaki toplam eleman sayısı
} PriorityQueue_t;

/*
 * --- SCHEDULER (ZAMANLAYICI) ANA YAPISI ---
 * Tüm sistemi yöneten ana kontrol bloğu.
 */
typedef struct {
    PriorityQueue_t queues[MAX_PRIORITY_LEVELS]; // Öncelik kuyrukları dizisi (0,1,2,3)
    Task_t* pending_tasks;       // Varış zamanı gelmemiş görevlerin beklediği liste
    Task_t* current_task;        // Şu an CPU'da çalışan görev (Yoksa NULL)
    double current_time;         // Simülasyonun güncel saati
    uint32_t task_counter;       // ID atamak için sayaç
    SemaphoreHandle_t scheduler_mutex; // Veri bütünlüğü için kilit (Mutex)
    bool skip_next_log;          // Çift log basmayı engellemek için kontrol bayrağı
} Scheduler_t;

/* --- FONKSİYON PROTOTİPLERİ --- */

// Scheduler Başlatma ve Yönetim
void scheduler_init(Scheduler_t* scheduler);
void scheduler_add_task(Scheduler_t* scheduler, Task_t* task);        // Doğrudan kuyruğa ekle
void scheduler_add_pending_task(Scheduler_t* scheduler, Task_t* task);// Bekleyen listesine ekle
void scheduler_check_arrivals(Scheduler_t* scheduler);                // Varış zamanı gelenleri kuyruğa al
void scheduler_check_timeouts(Scheduler_t* scheduler);                // 20 sn bekleyenleri sil
Task_t* scheduler_get_next_task(Scheduler_t* scheduler);              // Sıradaki görevi seç
void scheduler_demote_task(Scheduler_t* scheduler, Task_t* task);     // Öncelik düşür (Aging)
bool scheduler_is_empty(Scheduler_t* scheduler);                      // Sistem boş mu?

// Kuyruk İşlemleri (Linked List Operasyonları)
void queue_init(PriorityQueue_t* queue);
void queue_enqueue(PriorityQueue_t* queue, Task_t* task); // Sona ekle
Task_t* queue_dequeue(PriorityQueue_t* queue);            // Baştan çıkar
bool queue_is_empty(PriorityQueue_t* queue);              // Boş mu kontrol et

// Görev (Task) İşlemleri
Task_t* task_create(uint32_t id, uint32_t arrival, uint32_t priority, uint32_t duration); // Bellek ayır
void task_destroy(Task_t* task);           // Belleği temizle
void task_function(void* pvParameters);    // FreeRTOS görev fonksiyonu (Dummy)

// Loglama ve Ekran Çıktıları
void print_task_info(Task_t* task, const char* event, double current_time);
void print_task_info_with_old_priority(Task_t* task, const char* event, double current_time, uint32_t old_priority);

#endif // SCHEDULER_H