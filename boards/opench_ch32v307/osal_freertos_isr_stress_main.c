#include "ch32v30x.h"
#include "FreeRTOS.h"
#include "portmacro.h"
#include "xy_os.h"

#ifndef XINYI_FIRMWARE_COMMIT
#define XINYI_FIRMWARE_COMMIT "unknown"
#endif

#define STRESS_TARGET 2000U

static xy_os_semaphore_id_t irq_sem;
static volatile uint32_t irq_count;
static volatile uint32_t irq_release_failures;
static volatile uint32_t worker_count;

static void uart_init(void) {
    GPIO_InitTypeDef gpio = {0};
    USART_InitTypeDef uart = {0};
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);
    gpio.GPIO_Pin = GPIO_Pin_9;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &gpio);
    gpio.GPIO_Pin = GPIO_Pin_10;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);
    uart.USART_BaudRate = 115200;
    uart.USART_WordLength = USART_WordLength_8b;
    uart.USART_StopBits = USART_StopBits_1;
    uart.USART_Parity = USART_Parity_No;
    uart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    uart.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART1, &uart);
    USART_Cmd(USART1, ENABLE);
}

static void mark(const char* text) {
    while (*text != '\0') {
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET) {
        }
        USART_SendData(USART1, (uint8_t)*text++);
    }
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {
    }
}

static void timer_init(void) {
    TIM_TimeBaseInitTypeDef timer = {0};
    NVIC_InitTypeDef irq = {0};
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    timer.TIM_Period = 99;
    timer.TIM_Prescaler = 1439;
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
            irq_release_failures++;
        if (irq_count == STRESS_TARGET) {
            TIM_Cmd(TIM2, DISABLE);
            TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);
        }
    }
    FREE_INT_SP();
}

static void worker_task(void* argument) {
    (void)argument;
    for (;;) {
        worker_count++;
        (void)xy_os_delay(10);
    }
}

static void irq_task(void* argument) {
    uint32_t received = 0;
    (void)argument;
    mark("OPENCH_OSAL_ISR_STRESS_BOOT_" XINYI_FIRMWARE_COMMIT "\r\n");
    timer_init();
    while (received < STRESS_TARGET) {
        if (xy_os_semaphore_acquire(irq_sem, 500) != XY_OS_OK) {
            mark("OPENCH_OSAL_ISR_STRESS_TIMEOUT_ERROR_A\r\n");
            continue;
        }
        received++;
    }
    if (irq_count == STRESS_TARGET && irq_release_failures == 0U)
        mark("OPENCH_OSAL_ISR_STRESS_ACCOUNTING_OK_A\r\n");
    else
        mark("OPENCH_OSAL_ISR_STRESS_ACCOUNTING_ERROR_A\r\n");
    if (worker_count >= 100U)
        mark("OPENCH_OSAL_ISR_STRESS_WORKER_OK_A\r\n");
    else
        mark("OPENCH_OSAL_ISR_STRESS_WORKER_ERROR_A\r\n");
    for (;;) {
        (void)xy_os_delay(1000);
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
    mark("OPENCH_OSAL_ISR_STRESS_PRE_SCHED_" XINYI_FIRMWARE_COMMIT "\r\n");
    (void)xy_os_kernel_init();
    irq_sem = xy_os_semaphore_new(64, 0, NULL);
    if (irq_sem == NULL || xy_os_thread_new(worker_task, NULL, &worker_attr) == NULL ||
        xy_os_thread_new(irq_task, NULL, &irq_attr) == NULL) {
        mark("OPENCH_OSAL_ISR_STRESS_CREATE_ERROR_A\r\n");
        for (;;) {
        }
    }
    (void)xy_os_kernel_start();
    mark("OPENCH_OSAL_ISR_STRESS_SCHED_RETURN_ERROR_A\r\n");
    for (;;) {
    }
}
