/*
 * FreeRTOS Kernel Configuration for POSIX Port
 * 4 Seviyeli Öncelikli Görevlendirici Projesi
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stdio.h>
#include <stdlib.h>

/******************************************************************************/
/* Hardware description related definitions. **********************************/
/******************************************************************************/

/* POSIX port için CPU clock tanımı (gerçek donanım değil, simülasyon için) */
#define configCPU_CLOCK_HZ    ( ( unsigned long ) 1000000000 )

/******************************************************************************/
/* Scheduling behaviour related definitions. **********************************/
/******************************************************************************/

/* Tick rate: 1000 Hz = 1ms per tick (POSIX port için uygun) */
#define configTICK_RATE_HZ                         1000

/* Preemptive scheduling kullan */
#define configUSE_PREEMPTION                       1

/* Time slicing kullanma (bizim scheduler kendi mantığımızı kullanıyor) */
#define configUSE_TIME_SLICING                     0

/* Port optimizasyonu kullanma */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION    0

/* Tickless idle kullanma */
#define configUSE_TICKLESS_IDLE                    0

/* Maksimum öncelik sayısı (en az 5 olmalı - scheduler için) */
#define configMAX_PRIORITIES                       10

/* Minimal stack size (POSIX için yeterli) */
#define configMINIMAL_STACK_SIZE                   128

/* Task name uzunluğu */
#define configMAX_TASK_NAME_LEN                    32

/* Tick type: 64 bit (POSIX için uygun) */
#define configTICK_TYPE_WIDTH_IN_BITS              TICK_TYPE_WIDTH_64_BITS

/* Idle task yield */
#define configIDLE_SHOULD_YIELD                    1

/* Task notification array entries */
#define configTASK_NOTIFICATION_ARRAY_ENTRIES      1

/* Queue registry size */
#define configQUEUE_REGISTRY_SIZE                  0

/* Backward compatibility */
#define configENABLE_BACKWARD_COMPATIBILITY        0

/* Thread local storage */
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS    0

/* Mini list item */
#define configUSE_MINI_LIST_ITEM                   1

/* Stack depth type */
#define configSTACK_DEPTH_TYPE                     size_t

/* Message buffer length type */
#define configMESSAGE_BUFFER_LENGTH_TYPE           size_t

/* Heap clear memory on free */
#define configHEAP_CLEAR_MEMORY_ON_FREE            0

/* Stats buffer max length */
#define configSTATS_BUFFER_MAX_LENGTH              0xFFFF

/* Newlib reentrant */
#define configUSE_NEWLIB_REENTRANT                 0

/******************************************************************************/
/* Software timer related definitions. ****************************************/
/******************************************************************************/

/* Software timer kullan */
#define configUSE_TIMERS                1

/* Timer task priority */
#define configTIMER_TASK_PRIORITY       ( configMAX_PRIORITIES - 1 )

/* Timer task stack depth */
#define configTIMER_TASK_STACK_DEPTH    configMINIMAL_STACK_SIZE

/* Timer queue length */
#define configTIMER_QUEUE_LENGTH        10

/******************************************************************************/
/* Event Group related definitions. *******************************************/
/******************************************************************************/

/* Event groups kullan */
#define configUSE_EVENT_GROUPS    1

/******************************************************************************/
/* Stream Buffer related definitions. *****************************************/
/******************************************************************************/

/* Stream buffers kullan */
#define configUSE_STREAM_BUFFERS    1

/******************************************************************************/
/* Memory allocation related definitions. *************************************/
/******************************************************************************/

/* Static allocation desteği */
#define configSUPPORT_STATIC_ALLOCATION              1

/* Dynamic allocation desteği (bizim proje için gerekli) */
#define configSUPPORT_DYNAMIC_ALLOCATION             1

/* Total heap size (POSIX için yeterli) */
#define configTOTAL_HEAP_SIZE                        ( 64 * 1024 )

/* Application allocated heap */
#define configAPPLICATION_ALLOCATED_HEAP             0

/* Stack allocation from separate heap */
#define configSTACK_ALLOCATION_FROM_SEPARATE_HEAP    0

/* Heap protector */
#define configENABLE_HEAP_PROTECTOR                  0

/******************************************************************************/
/* Interrupt nesting behaviour configuration. *********************************/
/******************************************************************************/

/* POSIX port için interrupt priority tanımları gerekmez */
/* Bu tanımlar ARM Cortex-M için, POSIX port için kullanılmaz */

/******************************************************************************/
/* Hook and callback function related definitions. ****************************/
/******************************************************************************/

/* Hook fonksiyonları kullanma */
#define configUSE_IDLE_HOOK                   0
#define configUSE_TICK_HOOK                   0
#define configUSE_MALLOC_FAILED_HOOK          0
#define configUSE_DAEMON_TASK_STARTUP_HOOK    0

/* Stream buffer completed callback */
#define configUSE_SB_COMPLETED_CALLBACK       0

/* Stack overflow checking */
#define configCHECK_FOR_STACK_OVERFLOW        2

/******************************************************************************/
/* Run time and task stats gathering related definitions. *********************/
/******************************************************************************/

/* Run time stats */
#define configGENERATE_RUN_TIME_STATS           0

/* Trace facility */
#define configUSE_TRACE_FACILITY                0

/* Stats formatting functions */
#define configUSE_STATS_FORMATTING_FUNCTIONS    0

/******************************************************************************/
/* Static Memory Configuration **********************************************/
/******************************************************************************/

/* Kernel provided static memory - FreeRTOS kendi static buffer'larını kullanır */
#define configKERNEL_PROVIDED_STATIC_MEMORY    1

/******************************************************************************/
/* Co-routine related definitions. ********************************************/
/******************************************************************************/

/* Co-routines kullanma */
#define configUSE_CO_ROUTINES              0
#define configMAX_CO_ROUTINE_PRIORITIES    1

/******************************************************************************/
/* Debugging assistance. ******************************************************/
/******************************************************************************/

/* Assert tanımı - POSIX port için uygun */
#define configASSERT( x ) \
    if( ( x ) == 0 ) { \
        printf("FreeRTOS Assert Failed: %s:%d\n", __FILE__, __LINE__); \
        abort(); \
    }

/******************************************************************************/
/* Definitions that include or exclude functionality. *************************/
/******************************************************************************/

/* Feature flags */
#define configUSE_TASK_NOTIFICATIONS           1
#define configUSE_MUTEXES                      1
#define configUSE_RECURSIVE_MUTEXES            1
#define configUSE_COUNTING_SEMAPHORES          1
#define configUSE_QUEUE_SETS                   0
#define configUSE_APPLICATION_TASK_TAG         0
#define configUSE_POSIX_ERRNO                  0

/* Include API functions */
#define INCLUDE_vTaskPrioritySet               1
#define INCLUDE_uxTaskPriorityGet              1
#define INCLUDE_vTaskDelete                    1
#define INCLUDE_vTaskSuspend                   1
#define INCLUDE_xTaskDelayUntil                1
#define INCLUDE_vTaskDelay                     1
#define INCLUDE_xTaskGetSchedulerState         1
#define INCLUDE_xTaskGetCurrentTaskHandle      1
#define INCLUDE_uxTaskGetStackHighWaterMark    0
#define INCLUDE_xTaskGetIdleTaskHandle         0
#define INCLUDE_eTaskGetState                  0
#define INCLUDE_xTimerPendFunctionCall         0
#define INCLUDE_xTaskAbortDelay                0
#define INCLUDE_xTaskGetHandle                 0
#define INCLUDE_xTaskResumeFromISR             1

#endif /* FREERTOS_CONFIG_H */

