#include "ch32v30x.h"
#include "xy_os.h"

#define TIMER_FLAG 0x1U
#define FLAGS_ERROR 0x80000000U

static xy_os_thread_id_t monitor_id;
static volatile uint32_t callback_count;
static volatile uint32_t callback_generation;
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
    callback_generation = *(uint32_t*)argument;
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
    uint32_t generation = 1;
    uint32_t deleted_count;
    xy_os_timer_id_t timer;
    (void)argument;
    mark("OPENCH_OSAL_ONESHOT_TIMER_BOOT_20261016_A\r\n");
    timer = xy_os_timer_new(timer_callback, XY_OS_TIMER_ONCE, &generation, NULL);
    if (timer == NULL || xy_os_timer_start(timer, 20) != XY_OS_OK ||
        (xy_os_thread_flags_wait(TIMER_FLAG, XY_OS_FLAGS_WAIT_ALL, 100) & TIMER_FLAG) == 0U) {
        mark("OPENCH_OSAL_ONESHOT_TIMER_FIRST_ERROR_A\r\n");
        for (;;) {
        }
    }
    (void)xy_os_delay(1);
    if (callback_count != 1U || callback_generation != 1U || xy_os_timer_is_running(timer) != 0U) {
        mark("OPENCH_OSAL_ONESHOT_TIMER_FIRST_ERROR_A\r\n");
        for (;;) {
        }
    }
    mark("OPENCH_OSAL_ONESHOT_TIMER_FIRST_OK_A\r\n");

    generation = 2;
    if (xy_os_timer_start(timer, 20) != XY_OS_OK ||
        (xy_os_thread_flags_wait(TIMER_FLAG, XY_OS_FLAGS_WAIT_ALL, 100) & TIMER_FLAG) == 0U) {
        mark("OPENCH_OSAL_ONESHOT_TIMER_RESTART_ERROR_A\r\n");
        for (;;) {
        }
    }
    (void)xy_os_delay(1);
    if (callback_count != 2U || callback_generation != 2U || xy_os_timer_is_running(timer) != 0U) {
        mark("OPENCH_OSAL_ONESHOT_TIMER_RESTART_ERROR_A\r\n");
        for (;;) {
        }
    }
    mark("OPENCH_OSAL_ONESHOT_TIMER_RESTART_OK_A\r\n");

    generation = 3;
    if (xy_os_timer_start(timer, 100) != XY_OS_OK || xy_os_timer_delete(timer) != XY_OS_OK) {
        mark("OPENCH_OSAL_ONESHOT_TIMER_DELETE_ERROR_A\r\n");
        for (;;) {
        }
    }
    deleted_count = callback_count;
    (void)xy_os_delay(130);
    if (callback_count == deleted_count &&
        xy_os_thread_flags_wait(TIMER_FLAG, XY_OS_FLAGS_WAIT_ALL, 20) == FLAGS_ERROR)
        mark("OPENCH_OSAL_ONESHOT_TIMER_DELETE_STABLE_OK_A\r\n");
    else
        mark("OPENCH_OSAL_ONESHOT_TIMER_DELETE_STABLE_ERROR_A\r\n");

    timer = xy_os_timer_new(timer_callback, XY_OS_TIMER_ONCE, &generation, NULL);
    if (timer != NULL && xy_os_timer_delete(timer) == XY_OS_OK)
        mark("OPENCH_OSAL_ONESHOT_TIMER_RESOURCE_OK_A\r\n");
    else
        mark("OPENCH_OSAL_ONESHOT_TIMER_RESOURCE_ERROR_A\r\n");
    if (worker_count >= 15U)
        mark("OPENCH_OSAL_ONESHOT_TIMER_WORKER_OK_A\r\n");
    else
        mark("OPENCH_OSAL_ONESHOT_TIMER_WORKER_ERROR_A\r\n");
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
    mark("OPENCH_OSAL_ONESHOT_TIMER_PRE_SCHED_20261016_A\r\n");
    (void)xy_os_kernel_init();
    monitor_id = xy_os_thread_new(monitor, NULL, &monitor_attr);
    if (monitor_id == NULL || xy_os_thread_new(worker, NULL, &worker_attr) == NULL) {
        mark("OPENCH_OSAL_ONESHOT_TIMER_CREATE_ERROR_A\r\n");
        for (;;) {
        }
    }
    (void)xy_os_kernel_start();
    mark("OPENCH_OSAL_ONESHOT_TIMER_SCHED_RETURN_ERROR_A\r\n");
    for (;;) {
    }
}
