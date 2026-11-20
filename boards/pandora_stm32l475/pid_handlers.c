/*
 * FreeRTOS exception vectors for the PID board image.
 *
 * The Pandora rtos image has its own handlers file, but that one is wired to
 * the DMA and TIM6 interrupts that image uses. This image needs only the
 * kernel's three vectors, so it gets its own file rather than a second copy of
 * the other one with unused interrupts left in it.
 */

#include "stm32l4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "xy_hal_sys.h"

void vPortSVCHandler(void);
void xPortPendSVHandler(void);
void xPortSysTickHandler(void);

/* The same post-mortem words the image's fail() path publishes, so a stack
   overflow is diagnosable from the debugger instead of looking like a board
   whose UART went quiet. */
extern volatile uint32_t xy_image_failed;
extern volatile const char *xy_image_failed_task;

void SVC_Handler(void) __attribute__((naked));
void SVC_Handler(void)
{
    __asm volatile("b vPortSVCHandler");
}

void PendSV_Handler(void) __attribute__((naked));
void PendSV_Handler(void)
{
    __asm volatile("b xPortPendSVHandler");
}

void SysTick_Handler(void)
{
    /* The HAL tick handler keeps the HAL timebase in step with the kernel. */
    xy_hal_sys_tick_irq_handler();
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        xPortSysTickHandler();
    }
}

/* A stack overflow must stop the image, not continue with a corrupted frame. */
void vApplicationStackOverflowHook(TaskHandle_t task, char *task_name)
{
    (void)task;
    xy_image_failed_task = task_name;
    xy_image_failed = 1U;
    __disable_irq();
    for (;;) {
    }
}
