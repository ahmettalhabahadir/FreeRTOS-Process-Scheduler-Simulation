/*
 * FreeRTOS Hook Functions
 * POSIX port için gerekli hook fonksiyonları
 */

#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

/* Stack overflow hook - configCHECK_FOR_STACK_OVERFLOW=2 için gerekli */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    
    printf("ERROR: Stack overflow in task: %s\n", pcTaskName ? pcTaskName : "Unknown");
    fflush(stdout);
    
    /* Sonsuz döngü - debug için */
    for(;;)
    {
        /* Burada breakpoint koyabilirsiniz */
    }
}

/* configKERNEL_PROVIDED_STATIC_MEMORY=1 olduğu için bu fonksiyonlar 
 * FreeRTOS tarafından sağlanıyor, burada tanımlamaya gerek yok */

