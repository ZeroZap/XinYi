#include "ch32v30x.h"
#include "xy_os.h"

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the exact source commit"
#endif

#define THREAD_FLAG_READY 0x01U
#define THREAD_FLAGS_ERROR 0x80000000U

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
    xy_os_thread_id_t self;
    const char* name;
    (void)argument;
    mark("OPENCH_OSAL_THREAD_LIFECYCLE_BOOT_" XINYI_FIRMWARE_COMMIT "\r\n");

    self = xy_os_thread_get_id();
    name = xy_os_thread_get_name(self);
    if (self == NULL || name == NULL || name[0] != 'm' ||
        xy_os_thread_get_state(self) != XY_OS_THREAD_RUNNING ||
        xy_os_thread_get_stack_size(self) == 0U ||
        xy_os_thread_get_priority(self) != XY_OS_PRIORITY_NORMAL)
        fail("OPENCH_OSAL_THREAD_LIFECYCLE_METADATA_ERROR_A\r\n");
    mark("OPENCH_OSAL_THREAD_LIFECYCLE_METADATA_OK_A\r\n");

    if (xy_os_thread_flags_set(self, THREAD_FLAG_READY) != THREAD_FLAG_READY ||
        xy_os_thread_flags_get() != THREAD_FLAG_READY ||
        xy_os_thread_flags_wait(THREAD_FLAG_READY, XY_OS_FLAGS_WAIT_ALL, 0U) != THREAD_FLAG_READY ||
        xy_os_thread_flags_clear(THREAD_FLAG_READY) != THREAD_FLAG_READY ||
        xy_os_thread_flags_get() != 0U)
        fail("OPENCH_OSAL_THREAD_LIFECYCLE_FLAGS_ERROR_A\r\n");
    mark("OPENCH_OSAL_THREAD_LIFECYCLE_FLAGS_OK_A\r\n");

    if (xy_os_thread_suspend(self) != XY_OS_OK || xy_os_thread_resume(self) != XY_OS_OK)
        fail("OPENCH_OSAL_THREAD_LIFECYCLE_SUSPEND_ERROR_A\r\n");
    mark("OPENCH_OSAL_THREAD_LIFECYCLE_SUSPEND_OK_A\r\n");

    if (xy_os_thread_get_count() != 0U || xy_os_thread_enumerate(NULL, 0U) != 0U ||
        xy_os_thread_flags_set(self, 0U) != THREAD_FLAGS_ERROR ||
        xy_os_thread_terminate(NULL) != XY_OS_ERROR_PARAMETER ||
        xy_os_thread_join(NULL) != XY_OS_ERROR_PARAMETER ||
        xy_os_thread_detach(NULL) != XY_OS_ERROR_PARAMETER)
        fail("OPENCH_OSAL_THREAD_LIFECYCLE_BOUNDARY_ERROR_A\r\n");
    mark("OPENCH_OSAL_THREAD_LIFECYCLE_BOUNDARY_OK_A\r\n");
    mark("OPENCH_OSAL_THREAD_LIFECYCLE_PASS_20261018_A\r\n");
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
    mark("OPENCH_OSAL_THREAD_LIFECYCLE_PRE_SCHED_" XINYI_FIRMWARE_COMMIT "\r\n");
    (void)xy_os_kernel_init();
    if (xy_os_thread_new(monitor, NULL, &attr) == NULL)
        fail("OPENCH_OSAL_THREAD_LIFECYCLE_THREAD_ERROR_A\r\n");
    (void)xy_os_kernel_start();
    fail("OPENCH_OSAL_THREAD_LIFECYCLE_SCHED_RETURN_ERROR_A\r\n");
}
