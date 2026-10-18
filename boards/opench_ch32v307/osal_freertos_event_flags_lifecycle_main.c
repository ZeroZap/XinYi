#include "ch32v30x.h"
#include "xy_os.h"

#ifndef XINYI_FIRMWARE_COMMIT
#define XINYI_FIRMWARE_COMMIT "unknown"
#endif

#define EVENT_FLAG_A 0x01U
#define EVENT_FLAG_B 0x02U
#define EVENT_FLAG_ALL (EVENT_FLAG_A | EVENT_FLAG_B)
#define EVENT_FLAGS_ERROR 0x80000000U

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
    xy_os_event_flags_id_t flags;
    uint32_t observed;
    (void)argument;
    mark("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_BOOT_" XINYI_FIRMWARE_COMMIT "\r\n");

    flags = xy_os_event_flags_new(NULL);
    if (flags == NULL || xy_os_event_flags_get(flags) != 0U)
        fail("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_CREATE_ERROR_A\r\n");
    mark("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_CREATE_OK_A\r\n");

    if (xy_os_event_flags_set(flags, EVENT_FLAG_A) != EVENT_FLAG_A ||
        xy_os_event_flags_get(flags) != EVENT_FLAG_A)
        fail("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_SET_ERROR_A\r\n");
    mark("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_SET_OK_A\r\n");

    observed = xy_os_event_flags_wait(flags, EVENT_FLAG_A, XY_OS_FLAGS_WAIT_ALL, 0U);
    if (observed != EVENT_FLAG_A)
        fail("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_WAIT_ERROR_A\r\n");
    mark("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_WAIT_OK_A\r\n");

    if (xy_os_event_flags_clear(flags, EVENT_FLAG_A) != EVENT_FLAG_A ||
        xy_os_event_flags_get(flags) != 0U)
        fail("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_CLEAR_ERROR_A\r\n");
    mark("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_CLEAR_OK_A\r\n");

    if (xy_os_event_flags_set(flags, EVENT_FLAG_ALL) != EVENT_FLAG_ALL ||
        xy_os_event_flags_wait(flags, EVENT_FLAG_ALL, XY_OS_FLAGS_WAIT_ALL, 0U) != EVENT_FLAG_ALL)
        fail("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_ALL_ERROR_A\r\n");
    if (xy_os_event_flags_clear(flags, EVENT_FLAG_ALL) != EVENT_FLAG_ALL)
        fail("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_ALL_CLEAR_ERROR_A\r\n");
    if (xy_os_event_flags_wait(flags, EVENT_FLAG_A, XY_OS_FLAGS_WAIT_ALL, 0U) != EVENT_FLAGS_ERROR)
        fail("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_TIMEOUT_ERROR_A\r\n");
    mark("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_TIMEOUT_OK_A\r\n");

    if (xy_os_event_flags_delete(flags) != XY_OS_OK ||
        xy_os_event_flags_get(flags) != EVENT_FLAGS_ERROR ||
        xy_os_event_flags_set(flags, EVENT_FLAG_A) != EVENT_FLAGS_ERROR ||
        xy_os_event_flags_clear(flags, EVENT_FLAG_A) != EVENT_FLAGS_ERROR ||
        xy_os_event_flags_wait(flags, EVENT_FLAG_A, XY_OS_FLAGS_WAIT_ALL, 0U) != EVENT_FLAGS_ERROR ||
        xy_os_event_flags_delete(flags) != XY_OS_ERROR_PARAMETER)
        fail("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_STALE_ERROR_A\r\n");
    mark("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_STALE_OK_A\r\n");
    mark("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_PASS_20261018_A\r\n");
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
    mark("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_PRE_SCHED_" XINYI_FIRMWARE_COMMIT "\r\n");
    (void)xy_os_kernel_init();
    if (xy_os_thread_new(monitor, NULL, &attr) == NULL)
        fail("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_THREAD_ERROR_A\r\n");
    (void)xy_os_kernel_start();
    fail("OPENCH_OSAL_EVENT_FLAGS_LIFECYCLE_SCHED_RETURN_ERROR_A\r\n");
}
