#include "ch32v30x.h"
#include "xy_hal_gpio.h"
#include "xy_hal_i2c.h"
#include "xy_hal_uart.h"
#include "xy_os.h"

#include <stddef.h>

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the exact source commit"
#endif

#define READY_FLAG 0x1U
#define UART_TIMEOUT_MS 1000U
/* xy_os_event_flags_wait reports an unsatisfied wait with this sentinel, not 0. */
#define EVENT_FLAGS_ERROR 0x80000000U

/*
 * Progress counters that the debugger reads straight out of RAM.
 *
 * The WCH-Link VCP only holds a couple of hundred bytes, so a one-shot image
 * cannot show over serial where it stopped. Every step below bumps
 * xy_probe_stage before the call and moves it on afterwards, so halting the
 * target and reading this word names the exact call the task never returned
 * from. The *_ret words record what each blocking call actually returned.
 */
volatile uint32_t xy_probe_stage;
volatile uint32_t xy_probe_loops;
volatile uint32_t xy_probe_delay_ret;
volatile uint32_t xy_probe_wait_ret;
volatile uint32_t xy_probe_get_ret;
volatile uint32_t xy_probe_timeout_sentinel_ok;
/* Counts how often xy_hal_uart_send reports anything but XY_HAL_OK. Under an
   RTOS the HAL derives its polling timeout from SysTick->CNT, which the kernel
   turned into a 0..CMP saw-tooth, so a non-zero count here measures how far
   that broken timebase is from the running kernel. */
volatile uint32_t xy_probe_uart_fails;
volatile uint32_t xy_probe_uart_sends;
/* The same measurement for a peripheral that really has to wait: an I2C transfer
   at 100 kHz takes real microseconds, so a timebase that reports garbage makes
   these time out. MPU6050 WHO_AM_I (0x75) answers 0x68 on this board. */
volatile uint32_t xy_probe_i2c_ops;
volatile uint32_t xy_probe_i2c_fails;
volatile uint32_t xy_probe_whoami;

static xy_os_event_flags_id_t event_id;
static xy_os_msgqueue_id_t queue_id;

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

static void mark(const char* text) {
    size_t length = 0U;
    while (text[length] != '\0')
        ++length;
    ++xy_probe_uart_sends;
    if (xy_hal_uart_send(USART1, (const uint8_t*)text, length, UART_TIMEOUT_MS) != XY_HAL_OK)
        ++xy_probe_uart_fails;
}

static void probe_task(void* argument) {
    uint32_t index;
    uint32_t value = 0U;
    (void)argument;

    xy_probe_stage = 2U;
    mark("OPENCH_OSAL_PROBE_BOOT_" XINYI_FIRMWARE_COMMIT "\r\n");

    /* Stage 3 means "inside xy_os_delay"; stage 4 means it came back. A target
       halted at 3 with xy_probe_loops unchanged proves the delay never expires. */
    for (index = 0U; index < 3U; ++index) {
        xy_probe_stage = 3U;
        xy_probe_delay_ret = (uint32_t)xy_os_delay(100);
        xy_probe_loops = index + 1U;
        xy_probe_stage = 4U;
    }
    mark("OPENCH_OSAL_PROBE_DELAYS_DONE_A\r\n");

    xy_probe_stage = 5U;
    xy_probe_wait_ret = xy_os_event_flags_wait(event_id, READY_FLAG, XY_OS_FLAGS_WAIT_ALL, 200);
    xy_probe_stage = 6U;
    if (xy_probe_wait_ret == EVENT_FLAGS_ERROR)
        xy_probe_timeout_sentinel_ok = 1U;

    xy_probe_stage = 7U;
    xy_probe_get_ret = (uint32_t)xy_os_msgqueue_get(queue_id, &value, NULL, 200);
    xy_probe_stage = 8U;

    /* Stage 9 covers a real I2C transfer running under the kernel. Unlike the
       bare-metal images this must not touch SysTick, because the scheduler owns
       it now. */
    xy_probe_stage = 9U;
    {
        const xy_hal_gpio_config_t od = {XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_NONE,
                                         XY_HAL_GPIO_OTYPE_OD, XY_HAL_GPIO_SPEED_HIGH, 0U};
        const xy_hal_i2c_config_t bus = {100000U, XY_HAL_I2C_ADDR_7BIT, XY_HAL_I2C_DUTY_2, 0U, 0U};
        uint32_t round;
        uint8_t identity = 0U;
        (void)xy_hal_gpio_init(GPIOB, 10U, &od);
        (void)xy_hal_gpio_init(GPIOB, 11U, &od);
        (void)xy_hal_i2c_init(I2C2, &bus);
        for (round = 0U; round < 4U; ++round) {
            ++xy_probe_i2c_ops;
            if (xy_hal_i2c_mem_read(I2C2, 0x68U, 0x75U, &identity, 1U, 100U) != XY_HAL_OK)
                ++xy_probe_i2c_fails;
            else
                xy_probe_whoami = identity;
        }
    }
    xy_probe_stage = 10U;

    mark("OPENCH_OSAL_PROBE_SEQUENCE_DONE_A\r\n");
    for (;;) {
        (void)xy_os_delay(1000);
    }
}

int main(void) {
    const xy_os_thread_attr_t probe_attr = {
        .name = "probe", .stack_size = 1024, .priority = XY_OS_PRIORITY_NORMAL};
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    if (!uart_init())
        for (;;) {
        }
    mark("OPENCH_OSAL_PROBE_PRE_SCHED_" XINYI_FIRMWARE_COMMIT "\r\n");
    (void)xy_os_kernel_init();
    event_id = xy_os_event_flags_new(NULL);
    queue_id = xy_os_msgqueue_new(4, sizeof(uint32_t), NULL);
    if (event_id == NULL || queue_id == NULL ||
        xy_os_thread_new(probe_task, NULL, &probe_attr) == NULL) {
        mark("OPENCH_OSAL_PROBE_CREATE_ERROR_A\r\n");
        for (;;) {
        }
    }
    xy_probe_stage = 1U;
    (void)xy_os_kernel_start();
    mark("OPENCH_OSAL_PROBE_SCHED_RETURN_ERROR_A\r\n");
    for (;;) {
    }
}
