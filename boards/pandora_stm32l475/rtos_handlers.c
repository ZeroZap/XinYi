#include "stm32l4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

void vPortSVCHandler(void);
void xPortPendSVHandler(void);
void xPortSysTickHandler(void);

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
    HAL_IncTick();
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        xPortSysTickHandler();
    }
}
