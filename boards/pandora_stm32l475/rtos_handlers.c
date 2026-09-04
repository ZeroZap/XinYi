#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_tim.h"
#include "FreeRTOS.h"
#include "task.h"
#include "xy_os.h"

void vPortSVCHandler(void);
void xPortPendSVHandler(void);
void xPortSysTickHandler(void);
extern xy_os_semaphore_id_t pandora_isr_sem;
extern xy_os_semaphore_id_t pandora_tim6_sem;
extern TIM_HandleTypeDef pandora_tim6;

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
    static uint32_t isr_ticks;

    HAL_IncTick();
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        xPortSysTickHandler();
        ++isr_ticks;
        if (isr_ticks >= 1000U) {
            isr_ticks = 0U;
            (void)xy_os_semaphore_release_from_isr(pandora_isr_sem);
        }
    }
}

void TIM6_DAC_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&pandora_tim6);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *timer)
{
    if (timer == &pandora_tim6) {
        (void)xy_os_semaphore_release_from_isr(pandora_tim6_sem);
    }
}
