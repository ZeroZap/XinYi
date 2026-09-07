#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_tim.h"
#include "FreeRTOS.h"
#include "task.h"
#include "xy_hal_dma.h"
#include "xy_hal_sys.h"
#include "xy_hal_timer.h"
#include "xy_os.h"

void vPortSVCHandler(void);
void xPortPendSVHandler(void);
void xPortSysTickHandler(void);
extern xy_os_semaphore_id_t pandora_isr_sem;
extern TIM_HandleTypeDef pandora_tim6;
extern DMA_HandleTypeDef dma1_channel1;
extern DMA_HandleTypeDef spi1_tx_dma;


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

    xy_hal_sys_tick_irq_handler();
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
    xy_hal_timer_irq_handler(&pandora_tim6);
}

void DMA1_Channel1_IRQHandler(void)
{
    xy_hal_dma_irq_handler(&dma1_channel1);
}

void DMA1_Channel3_IRQHandler(void)
{
    xy_hal_dma_irq_handler(&spi1_tx_dma);
}
