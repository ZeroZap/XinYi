#include "ch32v30x.h"
#include "xy_hal_gpio.h"
#include "xy_hal_uart.h"
#include "FreeRTOS.h"
#include "portmacro.h"
#include "xy_os.h"

#include <stddef.h>

#define UART_TIMEOUT_MS 1000U

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the exact source commit"
#endif

static xy_os_semaphore_id_t irq_sem;
static volatile uint32_t irq_count;
static volatile uint32_t worker_count;

static void uart_init(void) {
    const xy_hal_gpio_config_t tx = {XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_NONE,
                                     XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_VERY_HIGH, 0U};
    const xy_hal_gpio_config_t rx = {XY_HAL_GPIO_MODE_INPUT, XY_HAL_GPIO_PULL_NONE,
                                     XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_LOW, 0U};
    const xy_hal_uart_config_t uart = {115200U,
                                       XY_HAL_UART_WORDLEN_8B,
                                       XY_HAL_UART_STOPBITS_1,
                                       XY_HAL_UART_PARITY_NONE,
                                       XY_HAL_UART_FLOWCTRL_NONE,
                                       XY_HAL_UART_MODE_TX_RX};
    (void)xy_hal_gpio_init(GPIOA, 9U, &tx);
    (void)xy_hal_gpio_init(GPIOA, 10U, &rx);
    (void)xy_hal_uart_init(USART1, &uart);
}

static void mark(const char* text) {
    size_t length = 0U;
    while (text[length] != '\0')
        ++length;
    (void)xy_hal_uart_send(USART1, (const uint8_t*)text, length, UART_TIMEOUT_MS);
}

static void timer_init(void) {
    TIM_TimeBaseInitTypeDef timer = {0};
    NVIC_InitTypeDef irq = {0};
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    timer.TIM_Period = 9999;
    timer.TIM_Prescaler = 959;
    timer.TIM_ClockDivision = TIM_CKD_DIV1;
    timer.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &timer);
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    irq.NVIC_IRQChannel = TIM2_IRQn;
    irq.NVIC_IRQChannelPreemptionPriority = 3;
    irq.NVIC_IRQChannelSubPriority = 0;
    irq.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&irq);
    TIM_Cmd(TIM2, ENABLE);
}

void TIM2_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM2_IRQHandler(void) {
    GET_INT_SP();
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
        irq_count++;
        if (xy_os_semaphore_release_from_isr(irq_sem) != XY_OS_OK)
            mark("OPENCH_OSAL_FREERTOS_ISR_RELEASE_ERROR_A\r\n");
    }
    FREE_INT_SP();
}

static void worker_task(void* argument) {
    (void)argument;
    for (;;) {
        worker_count++;
        (void)xy_os_delay(50);
    }
}

static void irq_task(void* argument) {
    uint32_t received = 0;
    (void)argument;
    mark("OPENCH_OSAL_FREERTOS_BOOT_" XINYI_FIRMWARE_COMMIT "\r\n");
    timer_init();
    for (;;) {
        if (xy_os_semaphore_acquire(irq_sem, 500) != XY_OS_OK) {
            mark("OPENCH_OSAL_FREERTOS_TIMEOUT_ERROR_A\r\n");
            continue;
        }
        received++;
        if (received == 5U && irq_count >= 5U && worker_count >= 5U)
            mark("OPENCH_OSAL_FREERTOS_TASK_SYNC_OK_A\r\n");
        if (received == 10U && irq_count >= 10U && worker_count >= 10U)
            mark("OPENCH_OSAL_FREERTOS_FAST_ISR_OK_A\r\n");
    }
}

int main(void) {
    const xy_os_thread_attr_t worker_attr = {
        .name = "worker", .stack_size = 1024, .priority = XY_OS_PRIORITY_LOW};
    const xy_os_thread_attr_t irq_attr = {
        .name = "irq", .stack_size = 1024, .priority = XY_OS_PRIORITY_NORMAL};
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    uart_init();
    mark("OPENCH_OSAL_FREERTOS_PRE_SCHED_" XINYI_FIRMWARE_COMMIT "\r\n");
    if (xy_os_kernel_init() != XY_OS_OK) {
        mark("OPENCH_OSAL_FREERTOS_INIT_ERROR_A\r\n");
        for (;;) {
        }
    }
    irq_sem = xy_os_semaphore_new(1, 0, NULL);
    if (irq_sem == NULL || xy_os_thread_new(worker_task, NULL, &worker_attr) == NULL ||
        xy_os_thread_new(irq_task, NULL, &irq_attr) == NULL) {
        mark("OPENCH_OSAL_FREERTOS_CREATE_ERROR_A\r\n");
        for (;;) {
        }
    }
    (void)xy_os_kernel_start();
    mark("OPENCH_OSAL_FREERTOS_SCHED_RETURN_ERROR_A\r\n");
    for (;;) {
    }
}
