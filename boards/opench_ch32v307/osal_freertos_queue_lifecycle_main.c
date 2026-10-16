#include "ch32v30x.h"
#include "xy_os.h"

#ifndef XINYI_FIRMWARE_COMMIT
#define XINYI_FIRMWARE_COMMIT "unknown"
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

static void monitor(void* argument) {
    xy_os_msgqueue_id_t queue;
    uint32_t values[3] = {11U, 22U, 33U};
    uint32_t output = 0;
    uint32_t index;
    (void)argument;
    mark("OPENCH_OSAL_QUEUE_LIFECYCLE_BOOT_" XINYI_FIRMWARE_COMMIT "\r\n");
    queue = xy_os_msgqueue_new(3, sizeof(uint32_t), NULL);
    if (queue == NULL || xy_os_msgqueue_get_capacity(queue) != 3U ||
        xy_os_msgqueue_get_msg_size(queue) != sizeof(uint32_t) ||
        xy_os_msgqueue_get_count(queue) != 0U || xy_os_msgqueue_get_space(queue) != 3U) {
        mark("OPENCH_OSAL_QUEUE_LIFECYCLE_METADATA_ERROR_A\r\n");
        for (;;) {
        }
    }
    mark("OPENCH_OSAL_QUEUE_LIFECYCLE_METADATA_OK_A\r\n");
    for (index = 0; index < 3U; ++index) {
        if (xy_os_msgqueue_put(queue, &values[index], 0, 10) != XY_OS_OK) {
            mark("OPENCH_OSAL_QUEUE_LIFECYCLE_PUT_ERROR_A\r\n");
            for (;;) {
            }
        }
    }
    if (xy_os_msgqueue_get_count(queue) != 3U || xy_os_msgqueue_get_space(queue) != 0U ||
        xy_os_msgqueue_put(queue, &values[0], 0, 20) != XY_OS_ERROR_TIMEOUT) {
        mark("OPENCH_OSAL_QUEUE_LIFECYCLE_FULL_ERROR_A\r\n");
        for (;;) {
        }
    }
    for (index = 0; index < 3U; ++index) {
        if (xy_os_msgqueue_get(queue, &output, NULL, 10) != XY_OS_OK || output != values[index]) {
            mark("OPENCH_OSAL_QUEUE_LIFECYCLE_FIFO_ERROR_A\r\n");
            for (;;) {
            }
        }
    }
    mark("OPENCH_OSAL_QUEUE_LIFECYCLE_FIFO_OK_A\r\n");
    if (xy_os_msgqueue_get(queue, &output, NULL, 20) == XY_OS_ERROR_TIMEOUT)
        mark("OPENCH_OSAL_QUEUE_LIFECYCLE_TIMEOUT_OK_A\r\n");
    else
        mark("OPENCH_OSAL_QUEUE_LIFECYCLE_TIMEOUT_ERROR_A\r\n");
    (void)xy_os_msgqueue_put(queue, &values[0], 0, 10);
    if (xy_os_msgqueue_reset(queue) == XY_OS_OK && xy_os_msgqueue_get_count(queue) == 0U &&
        xy_os_msgqueue_get_space(queue) == 3U)
        mark("OPENCH_OSAL_QUEUE_LIFECYCLE_RESET_OK_A\r\n");
    else
        mark("OPENCH_OSAL_QUEUE_LIFECYCLE_RESET_ERROR_A\r\n");
    if (xy_os_msgqueue_delete(queue) != XY_OS_OK) {
        mark("OPENCH_OSAL_QUEUE_LIFECYCLE_DELETE_ERROR_A\r\n");
        for (;;) {
        }
    }
    queue = xy_os_msgqueue_new(2, sizeof(uint32_t), NULL);
    if (queue != NULL && xy_os_msgqueue_get_capacity(queue) == 2U &&
        xy_os_msgqueue_get_msg_size(queue) == sizeof(uint32_t) &&
        xy_os_msgqueue_delete(queue) == XY_OS_OK)
        mark("OPENCH_OSAL_QUEUE_LIFECYCLE_RESOURCE_OK_A\r\n");
    else
        mark("OPENCH_OSAL_QUEUE_LIFECYCLE_RESOURCE_ERROR_A\r\n");
    for (;;) {
        (void)xy_os_delay(1000);
    }
}

int main(void) {
    const xy_os_thread_attr_t attr = {
        .name = "monitor", .stack_size = 1024, .priority = XY_OS_PRIORITY_NORMAL};
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    uart_init();
    mark("OPENCH_OSAL_QUEUE_LIFECYCLE_PRE_SCHED_" XINYI_FIRMWARE_COMMIT "\r\n");
    (void)xy_os_kernel_init();
    if (xy_os_thread_new(monitor, NULL, &attr) == NULL) {
        mark("OPENCH_OSAL_QUEUE_LIFECYCLE_CREATE_ERROR_A\r\n");
        for (;;) {
        }
    }
    (void)xy_os_kernel_start();
    mark("OPENCH_OSAL_QUEUE_LIFECYCLE_SCHED_RETURN_ERROR_A\r\n");
    for (;;) {
    }
}
