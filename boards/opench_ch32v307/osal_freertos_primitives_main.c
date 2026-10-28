#include "ch32v30x.h"
#include "xy_hal_gpio.h"
#include "xy_hal_delay.h"
#include "xy_hal_time.h"
#include "xy_hal_uart.h"
#include "xy_os.h"

#include <stddef.h>

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the exact source commit"
#endif

#define READY_FLAG 0x1U
#define UART_TIMEOUT_MS 1000U
/* xy_os_event_flags_wait reports an unsatisfied wait with this sentinel, not with
   0, so a timeout assertion must compare against it. */
#define EVENT_FLAGS_ERROR 0x80000000U

static xy_os_mutex_id_t mutex_id;
static xy_os_event_flags_id_t event_id;
static xy_os_msgqueue_id_t queue_id;
static volatile uint32_t shared_value;

/* USART1 stays on PA9/PA10 because that pair is what the board wires to the
   WCH-Link VCP; only the driver underneath changes here. */
static int uart_init(void) {
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
    if (xy_hal_gpio_init(GPIOA, 9U, &tx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &rx) != XY_HAL_OK ||
        xy_hal_uart_init(USART1, &uart) != XY_HAL_OK)
        return 0;
    return 1;
}

/* The WCH-Link virtual COM port drops and reorders bytes, so a marker is not
   evidence once it leaves this function. Publish the same facts in RAM, where
   the debugger can read them: how many markers were emitted, whether any of them
   was a failure, and which marker came last. "ERROR" in a marker name is this
   board's convention for a check that failed; no marker carries both ERROR and
   OK, so the substring test is unambiguous. */
volatile uint32_t xy_image_steps;
volatile uint32_t xy_image_failed;
volatile const char* xy_image_last;

static void mark(const char* text) {
    size_t length = 0U;
    size_t index;
    while (text[length] != '\0')
        ++length;
    xy_image_last = text;
    ++xy_image_steps;
    for (index = 0U; index + 5U <= length; ++index) {
        if (text[index] == 'E' && text[index + 1U] == 'R' && text[index + 2U] == 'R' &&
            text[index + 3U] == 'O' && text[index + 4U] == 'R')
            xy_image_failed = 1U;
    }
    (void)xy_hal_uart_send(USART1, (const uint8_t*)text, length, UART_TIMEOUT_MS);
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
    mark("OPENCH_OSAL_PRIMITIVES_BOOT_" XINYI_FIRMWARE_COMMIT "\r\n");
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

    /* The HAL polling loops time their waits with a helper that derived
       milliseconds from SysTick, which this kernel owns: CNT becomes a
       saw-tooth, a difference of two readings underflows and every wait fires
       at once. The kernel installs its own tick as that source, so sample the
       helper across a known kernel delay rather than assuming the hook is
       wired. A 20 tick delay at 1 kHz has to read back as tens of ms. */
    {
        uint32_t tick_before = xy_hal_time_ms();
        uint32_t elapsed;

        (void)xy_os_delay(20U);
        elapsed = xy_hal_time_ms() - tick_before;
        if (elapsed >= 10U && elapsed <= 200U)
            mark("OPENCH_OSAL_PRIMITIVES_TIMEBASE_OK_A\r\n");
        else
            mark("OPENCH_OSAL_PRIMITIVES_TIMEBASE_ERROR_A\r\n");
    }

    /* The WCH default xy_hal_delay_* reprograms SysTick->CMP/CTLR and clears the
       counter's enable bit, which stops this kernel's tick outright. The kernel
       installs a task-aware source instead, so a HAL delay has to cost the time
       it claims without the tick ceasing to advance. */
    {
        uint32_t delay_before = xy_hal_time_ms();
        uint32_t delay_elapsed;

        xy_hal_delay_ms(20U);
        delay_elapsed = xy_hal_time_ms() - delay_before;
        if (delay_elapsed >= 15U && delay_elapsed <= 200U)
            mark("OPENCH_OSAL_PRIMITIVES_HAL_DELAY_OK_A\r\n");
        else
            mark("OPENCH_OSAL_PRIMITIVES_HAL_DELAY_ERROR_A\r\n");
    }
    value = 0;
    if (xy_os_msgqueue_get(queue_id, &value, NULL, 25) == XY_OS_ERROR_TIMEOUT &&
        xy_os_event_flags_wait(event_id, READY_FLAG, XY_OS_FLAGS_WAIT_ALL, 25) == EVENT_FLAGS_ERROR)
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
    if (!uart_init())
        for (;;) {
        }
    mark("OPENCH_OSAL_PRIMITIVES_PRE_SCHED_" XINYI_FIRMWARE_COMMIT "\r\n");
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
