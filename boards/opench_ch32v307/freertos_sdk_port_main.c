#include "ch32v30x.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

static SemaphoreHandle_t irq_sem;
static volatile uint32_t irq_count;
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
    BaseType_t wake = pdFALSE;
    GET_INT_SP();
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
        irq_count++;
        (void)xSemaphoreGiveFromISR(irq_sem, &wake);
        portYIELD_FROM_ISR(wake);
    }
    FREE_INT_SP();
}

static void worker_task(void* argument) {
    (void)argument;
    for (;;) {
        worker_count++;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

static void irq_task(void* argument) {
    uint32_t received = 0;
    (void)argument;
    mark("OPENCH_FREERTOS_SDK_BOOT_20261016_A\r\n");
    timer_init();
    for (;;) {
        if (xSemaphoreTake(irq_sem, pdMS_TO_TICKS(500)) != pdTRUE) {
            mark("OPENCH_FREERTOS_ISR_TIMEOUT_ERROR_A\r\n");
            continue;
        }
        received++;
        if (received == 5U && irq_count >= 5U && worker_count >= 5U)
            mark("OPENCH_FREERTOS_TASK_SCHED_OK_A\r\n");
        if (received == 10U && irq_count >= 10U && worker_count >= 10U)
            mark("OPENCH_FREERTOS_FAST_ISR_TO_TASK_OK_A\r\n");
    }
}

int main(void) {
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    uart_init();
    mark("OPENCH_FREERTOS_SDK_PRE_SCHED_20261016_A\r\n");
    irq_sem = xSemaphoreCreateBinary();
    if (irq_sem == NULL || xTaskCreate(worker_task, "worker", 256, NULL, 2, NULL) != pdPASS ||
        xTaskCreate(irq_task, "irq", 256, NULL, 3, NULL) != pdPASS) {
        mark("OPENCH_FREERTOS_CREATE_ERROR_A\r\n");
        for (;;) {
        }
    }
    vTaskStartScheduler();
    mark("OPENCH_FREERTOS_SCHED_RETURN_ERROR_A\r\n");
    for (;;) {
    }
}
