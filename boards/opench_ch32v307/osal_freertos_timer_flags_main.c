#include "ch32v30x.h"
#include "xy_os.h"

#ifndef XINYI_FIRMWARE_COMMIT
#define XINYI_FIRMWARE_COMMIT "unknown"
#endif

#define TIMER_FLAG 0x1U
#define TIMER_TARGET 20U
#define FLAGS_ERROR 0x80000000U

static xy_os_thread_id_t monitor_id;
static xy_os_timer_id_t timer_id;
static volatile uint32_t callback_count;
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

static void timer_callback(void* argument) {
    (void)argument;
    callback_count++;
    (void)xy_os_thread_flags_set(monitor_id, TIMER_FLAG);
}

static void worker(void* argument) {
    (void)argument;
    for (;;) {
        worker_count++;
        (void)xy_os_delay(10);
    }
}

static void monitor(void* argument) {
    uint32_t observed = 0;
    uint32_t stopped_count;
    (void)argument;
    mark("OPENCH_OSAL_TIMER_FLAGS_BOOT_" XINYI_FIRMWARE_COMMIT "\r\n");
    if (xy_os_timer_start(timer_id, 10) != XY_OS_OK) {
        mark("OPENCH_OSAL_TIMER_FLAGS_START_ERROR_A\r\n");
        for (;;) {
        }
    }
    while (observed < TIMER_TARGET) {
        uint32_t flags = xy_os_thread_flags_wait(TIMER_FLAG, XY_OS_FLAGS_WAIT_ALL, 100);
        if ((flags & TIMER_FLAG) == 0U) {
            mark("OPENCH_OSAL_TIMER_FLAGS_WAIT_ERROR_A\r\n");
            continue;
        }
        observed++;
    }
    if (callback_count == TIMER_TARGET && xy_os_timer_is_running(timer_id) == 1U)
        mark("OPENCH_OSAL_TIMER_FLAGS_PERIODIC_OK_A\r\n");
    else
        mark("OPENCH_OSAL_TIMER_FLAGS_PERIODIC_ERROR_A\r\n");
    if (xy_os_timer_stop(timer_id) != XY_OS_OK) {
        mark("OPENCH_OSAL_TIMER_FLAGS_STOP_ERROR_A\r\n");
        for (;;) {
        }
    }
    (void)xy_os_delay(30);
    stopped_count = callback_count;
    (void)xy_os_delay(50);
    if (callback_count == stopped_count && xy_os_timer_is_running(timer_id) == 0U)
        mark("OPENCH_OSAL_TIMER_FLAGS_STOP_STABLE_OK_A\r\n");
    else
        mark("OPENCH_OSAL_TIMER_FLAGS_STOP_STABLE_ERROR_A\r\n");
    if (xy_os_thread_flags_wait(TIMER_FLAG, XY_OS_FLAGS_WAIT_ALL, 20) == FLAGS_ERROR)
        mark("OPENCH_OSAL_TIMER_FLAGS_TIMEOUT_OK_A\r\n");
    else
        mark("OPENCH_OSAL_TIMER_FLAGS_TIMEOUT_ERROR_A\r\n");
    if (worker_count >= 20U)
        mark("OPENCH_OSAL_TIMER_FLAGS_WORKER_OK_A\r\n");
    else
        mark("OPENCH_OSAL_TIMER_FLAGS_WORKER_ERROR_A\r\n");
    for (;;) {
        (void)xy_os_delay(1000);
    }
}

int main(void) {
    const xy_os_thread_attr_t worker_attr = {
        .name = "worker", .stack_size = 1024, .priority = XY_OS_PRIORITY_LOW};
    const xy_os_thread_attr_t monitor_attr = {
        .name = "monitor", .stack_size = 1024, .priority = XY_OS_PRIORITY_NORMAL};
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    uart_init();
    mark("OPENCH_OSAL_TIMER_FLAGS_PRE_SCHED_" XINYI_FIRMWARE_COMMIT "\r\n");
    (void)xy_os_kernel_init();
    monitor_id = xy_os_thread_new(monitor, NULL, &monitor_attr);
    timer_id = xy_os_timer_new(timer_callback, XY_OS_TIMER_PERIODIC, NULL, NULL);
    if (monitor_id == NULL || timer_id == NULL ||
        xy_os_thread_new(worker, NULL, &worker_attr) == NULL) {
        mark("OPENCH_OSAL_TIMER_FLAGS_CREATE_ERROR_A\r\n");
        for (;;) {
        }
    }
    (void)xy_os_kernel_start();
    mark("OPENCH_OSAL_TIMER_FLAGS_SCHED_RETURN_ERROR_A\r\n");
    for (;;) {
    }
}
