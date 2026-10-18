#include "ch32v30x.h"
#include "xy_os.h"

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the exact source commit"
#endif

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

static void fail(const char* marker) {
    mark(marker);
    for (;;) {
    }
}

static void monitor(void* argument) {
    xy_os_mutex_id_t mutex;
    xy_os_mutex_id_t recursive;
    const xy_os_mutex_attr_t recursive_attr = {.attr_bits = XY_OS_MUTEX_RECURSIVE};
    (void)argument;
    mark("OPENCH_OSAL_MUTEX_LIFECYCLE_BOOT_" XINYI_FIRMWARE_COMMIT "\r\n");

    mutex = xy_os_mutex_new(NULL);
    if (mutex == NULL || xy_os_mutex_acquire(mutex, 0U) != XY_OS_OK)
        fail("OPENCH_OSAL_MUTEX_LIFECYCLE_CREATE_ERROR_A\r\n");
    mark("OPENCH_OSAL_MUTEX_LIFECYCLE_CREATE_OK_A\r\n");

    if (xy_os_mutex_acquire(mutex, 0U) != XY_OS_ERROR_TIMEOUT)
        fail("OPENCH_OSAL_MUTEX_LIFECYCLE_REACQUIRE_ERROR_A\r\n");
    mark("OPENCH_OSAL_MUTEX_LIFECYCLE_REACQUIRE_OK_A\r\n");

    if (xy_os_mutex_release(mutex) != XY_OS_OK ||
        xy_os_mutex_release(mutex) != XY_OS_ERROR_RESOURCE)
        fail("OPENCH_OSAL_MUTEX_LIFECYCLE_RELEASE_ERROR_A\r\n");
    mark("OPENCH_OSAL_MUTEX_LIFECYCLE_RELEASE_OK_A\r\n");

    recursive = xy_os_mutex_new(&recursive_attr);
    if (recursive == NULL || xy_os_mutex_acquire(recursive, 0U) != XY_OS_OK ||
        xy_os_mutex_acquire(recursive, 0U) != XY_OS_OK ||
        xy_os_mutex_release(recursive) != XY_OS_OK || xy_os_mutex_release(recursive) != XY_OS_OK)
        fail("OPENCH_OSAL_MUTEX_LIFECYCLE_RECURSIVE_ERROR_A\r\n");
    mark("OPENCH_OSAL_MUTEX_LIFECYCLE_RECURSIVE_OK_A\r\n");

    if (xy_os_mutex_delete(mutex) != XY_OS_OK ||
        xy_os_mutex_acquire(mutex, 0U) != XY_OS_ERROR_PARAMETER ||
        xy_os_mutex_release(mutex) != XY_OS_ERROR_PARAMETER ||
        xy_os_mutex_delete(mutex) != XY_OS_ERROR_PARAMETER)
        fail("OPENCH_OSAL_MUTEX_LIFECYCLE_DELETE_ERROR_A\r\n");
    if (xy_os_mutex_delete(recursive) != XY_OS_OK)
        fail("OPENCH_OSAL_MUTEX_LIFECYCLE_RECURSIVE_DELETE_ERROR_A\r\n");
    mark("OPENCH_OSAL_MUTEX_LIFECYCLE_DELETE_OK_A\r\n");
    mark("OPENCH_OSAL_MUTEX_LIFECYCLE_PASS_20261018_A\r\n");
    for (;;) {
        (void)xy_os_delay(1000U);
    }
}

int main(void) {
    const xy_os_thread_attr_t attr = {
        .name = "monitor", .stack_size = 1024U, .priority = XY_OS_PRIORITY_NORMAL};
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    uart_init();
    mark("OPENCH_OSAL_MUTEX_LIFECYCLE_PRE_SCHED_" XINYI_FIRMWARE_COMMIT "\r\n");
    (void)xy_os_kernel_init();
    if (xy_os_thread_new(monitor, NULL, &attr) == NULL)
        fail("OPENCH_OSAL_MUTEX_LIFECYCLE_THREAD_ERROR_A\r\n");
    (void)xy_os_kernel_start();
    fail("OPENCH_OSAL_MUTEX_LIFECYCLE_SCHED_RETURN_ERROR_A\r\n");
}
