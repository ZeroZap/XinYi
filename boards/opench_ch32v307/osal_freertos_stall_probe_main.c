#include "ch32v30x.h"
#include "xy_hal_gpio.h"
#include "xy_hal_i2c.h"
#include "xy_hal_time.h"
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
/* Milliseconds measured across a 256-byte UART burst (~22 ms at 115200). The
   HAL times its own waits with the same helper this uses, so a saw-tooth reading
   cannot land in the plausible window - that is the discriminator. */
volatile uint32_t xy_probe_uart_burst_ms;

/* A receive that has to give up: the code it returned and the wall time it took. */
volatile uint32_t xy_probe_uart_recv_ms;
volatile uint32_t xy_probe_uart_recv_code;

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
    /* These two counters used to be declared and never touched, so the compiler
       dropped them and the "uart N/N" reading had nothing behind it. Count the
       send that just happened instead of assuming it worked. */
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
    /* Stage 10: a wait that has to cost real time. 256 bytes at 115200 take
       about 22 ms of wall clock; the HAL's TXE polls and this measurement share
       the same millisecond helper, so the reading is only plausible if that
       helper tracks the kernel tick. */
    xy_probe_stage = 10U;
    {
        static const char burst[257] =
            "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"
            "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"
            "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"
            "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF";
        uint32_t burst_before = xy_hal_time_ms();
        (void)xy_hal_uart_send(USART1, (const uint8_t*)burst, sizeof(burst) - 1U, 1000U);
        xy_probe_uart_burst_ms = xy_hal_time_ms() - burst_before;
    }
    xy_probe_stage = 11U;

    if (xy_probe_uart_burst_ms >= 10U && xy_probe_uart_burst_ms <= 200U)
        mark("OPENCH_OSAL_PROBE_TIMEBASE_OK_A\r\n");
    else
        mark("OPENCH_OSAL_PROBE_TIMEBASE_ERROR_A\r\n");

    /* The receive path is the only one that has to give up on its own: nothing
       arrives on a silent line, so recv() must return TIMEOUT and the wall time it
       spent has to be the timeout it was given. The old saw-tooth helper made that
       comparison meaningless - it fired at once or never - so this judgement only
       became possible once the kernel owned the clock. */
    xy_probe_stage = 12U;
    {
        uint8_t byte = 0U;
        uint32_t recv_before = xy_hal_time_ms();
        xy_hal_error_t recv_result = xy_hal_uart_recv(USART1, &byte, 1U, 10U);
        xy_probe_uart_recv_ms = xy_hal_time_ms() - recv_before;
        xy_probe_uart_recv_code = (uint32_t)recv_result;
    }
    xy_probe_stage = 13U;

    if (xy_probe_uart_recv_code == (uint32_t)(int32_t)XY_HAL_ERROR_TIMEOUT &&
        xy_probe_uart_recv_ms >= 5U && xy_probe_uart_recv_ms <= 200U)
        mark("OPENCH_OSAL_PROBE_RECV_TIMEOUT_OK_A\r\n");
    else
        mark("OPENCH_OSAL_PROBE_RECV_TIMEOUT_ERROR_A\r\n");

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
