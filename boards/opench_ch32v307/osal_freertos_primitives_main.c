#include "ch32v30x.h"
#include "xy_os.h"

#define READY_FLAG 0x1U

static xy_os_mutex_id_t mutex_id;
static xy_os_event_flags_id_t event_id;
static xy_os_msgqueue_id_t queue_id;
static volatile uint32_t shared_value;

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

static void producer(void* argument) {
    uint32_t value;
    (void)argument;
    for (value = 1; value <= 12U; ++value) {
        if (xy_os_mutex_acquire(mutex_id, 100) != XY_OS_OK) {
            mark("OPENCH_OSAL_PRIMITIVES_MUTEX_ERROR_A\r\n");
            continue;
        }
        shared_value = value;
        (void)xy_os_mutex_release(mutex_id);
        if (xy_os_msgqueue_put(queue_id, &value, 0, 100) != XY_OS_OK)
            mark("OPENCH_OSAL_PRIMITIVES_QUEUE_PUT_ERROR_A\r\n");
        (void)xy_os_event_flags_set(event_id, READY_FLAG);
        (void)xy_os_delay(20);
    }
    for (;;) {
        (void)xy_os_delay(1000);
    }
}

static void consumer(void* argument) {
    uint32_t value;
    uint32_t expected = 1;
    (void)argument;
    mark("OPENCH_OSAL_PRIMITIVES_BOOT_20261016_A\r\n");
    while (expected <= 12U) {
        uint32_t flags = xy_os_event_flags_wait(event_id, READY_FLAG, XY_OS_FLAGS_WAIT_ALL, 200);
        if ((flags & READY_FLAG) == 0U) {
            mark("OPENCH_OSAL_PRIMITIVES_EVENT_WAIT_ERROR_A\r\n");
            continue;
        }
        if (xy_os_msgqueue_get(queue_id, &value, NULL, 200) != XY_OS_OK || value != expected) {
            mark("OPENCH_OSAL_PRIMITIVES_QUEUE_GET_ERROR_A\r\n");
            continue;
        }
        if (xy_os_mutex_acquire(mutex_id, 100) != XY_OS_OK) {
            mark("OPENCH_OSAL_PRIMITIVES_MUTEX_ERROR_A\r\n");
            continue;
        }
        if (shared_value < value)
            mark("OPENCH_OSAL_PRIMITIVES_SHARED_ERROR_A\r\n");
        (void)xy_os_mutex_release(mutex_id);
        expected++;
    }
    mark("OPENCH_OSAL_PRIMITIVES_PIPELINE_OK_A\r\n");
    value = 0;
    if (xy_os_msgqueue_get(queue_id, &value, NULL, 25) == XY_OS_ERROR_TIMEOUT &&
        xy_os_event_flags_wait(event_id, READY_FLAG, XY_OS_FLAGS_WAIT_ALL, 25) == 0U)
        mark("OPENCH_OSAL_PRIMITIVES_TIMEOUT_OK_A\r\n");
    else
        mark("OPENCH_OSAL_PRIMITIVES_TIMEOUT_ERROR_A\r\n");
    for (;;) {
        (void)xy_os_delay(1000);
    }
}

int main(void) {
    const xy_os_thread_attr_t producer_attr = {
        .name = "producer", .stack_size = 1024, .priority = XY_OS_PRIORITY_LOW};
    const xy_os_thread_attr_t consumer_attr = {
        .name = "consumer", .stack_size = 1024, .priority = XY_OS_PRIORITY_NORMAL};
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    uart_init();
    mark("OPENCH_OSAL_PRIMITIVES_PRE_SCHED_20261016_A\r\n");
    (void)xy_os_kernel_init();
    mutex_id = xy_os_mutex_new(NULL);
    event_id = xy_os_event_flags_new(NULL);
    queue_id = xy_os_msgqueue_new(4, sizeof(uint32_t), NULL);
    if (mutex_id == NULL || event_id == NULL || queue_id == NULL ||
        xy_os_thread_new(producer, NULL, &producer_attr) == NULL ||
        xy_os_thread_new(consumer, NULL, &consumer_attr) == NULL) {
        mark("OPENCH_OSAL_PRIMITIVES_CREATE_ERROR_A\r\n");
        for (;;) {
        }
    }
    (void)xy_os_kernel_start();
    mark("OPENCH_OSAL_PRIMITIVES_SCHED_RETURN_ERROR_A\r\n");
    for (;;) {
    }
}
