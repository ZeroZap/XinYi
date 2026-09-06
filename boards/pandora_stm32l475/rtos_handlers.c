#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_tim.h"
#include "FreeRTOS.h"
#include "task.h"
#include "xy_broker_isr_ingress.h"
#include "xy_os.h"

void vPortSVCHandler(void);
void xPortPendSVHandler(void);
void xPortSysTickHandler(void);
extern xy_os_semaphore_id_t pandora_isr_sem;
extern xy_os_semaphore_id_t pandora_tim6_sem;
extern TIM_HandleTypeDef pandora_tim6;
extern DMA_HandleTypeDef dma1_channel1;
extern DMA_HandleTypeDef spi1_tx_dma;
extern xy_broker_isr_ingress_t pandora_ipc_isr_ingress;
extern volatile uint32_t pandora_ipc_isr_attempts;
extern volatile uint32_t pandora_ipc_isr_accepted;
extern volatile uint32_t pandora_ipc_isr_full;
extern volatile int pandora_ipc_isr_result;

#define IPC_ISR_SEQUENCE_FLAG 0x80000000U

static void pandora_ipc_isr_publish(void)
{
    uint32_t sequence = IPC_ISR_SEQUENCE_FLAG | ++pandora_ipc_isr_attempts;

    pandora_ipc_isr_result = xy_broker_isr_publish(
        &pandora_ipc_isr_ingress, XY_BROKER_SERVER_TIMER, XY_BROKER_SERVER_TIMER,
        XY_BROKER_MSG_SENSOR_DATA, &sequence, sizeof(sequence), XY_BROKER_PRIORITY_HIGH);
    if (pandora_ipc_isr_result == XY_BROKER_OK) {
        ++pandora_ipc_isr_accepted;
    } else if (pandora_ipc_isr_result == XY_BROKER_QUEUE_FULL) {
        ++pandora_ipc_isr_full;
    }
}

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

void DMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&dma1_channel1);
}

void DMA1_Channel3_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&spi1_tx_dma);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *timer)
{
    if (timer == &pandora_tim6) {
        (void)xy_os_semaphore_release_from_isr(pandora_tim6_sem);
        if (pandora_ipc_isr_accepted < 16U) {
            pandora_ipc_isr_publish();
        }
    }
}
