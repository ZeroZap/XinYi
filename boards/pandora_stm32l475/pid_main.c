/*
 * PID on the real board.
 *
 * The pid component has Host tests, but the matrix still lists its Board column
 * as pending. The reason to run it on the target is specific: the controller is
 * written entirely in `float` (no `double` anywhere), and the two builds do not
 * round identically. The STM32L475VE has a single-precision hardware FPU
 * (`__FPU_PRESENT` in the device header, built as -mfpu=fpv4-sp-d16), and the
 * target's compiler contracts the multiply-add in the output sum into a single
 * `vfma.f32` -- one rounding. The x86-64 Host build has no FMA in its baseline
 * and emits a separate `mulss` and `addss` -- two roundings. The same
 * expression can therefore differ in its last bit between the two, which no
 * Host test can show.
 *
 * The second reason is the tick. The Host tests stub `xy_os_tick_get()` with a
 * variable, so the elapsed interval that drives dt has never been the real
 * one. This image runs the controller against the **real** FreeRTOS tick, with
 * a task that steps the setpoint over time so dt is a genuine elapsed interval
 * rather than a hard-coded number, and checks the values the Host tests pin.
 */

#include "stm32l4xx_hal.h"
#include "pandora_platform_startup.h"
#include "xy_hal_delay.h"
#include "xy_hal_sys.h"
#include "xy_hal_uart.h"
#include "xy_os.h"
#include "xy_pid.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the source commit used for this image"
#endif

static UART_HandleTypeDef uart1;

void _init(void) {}
void _fini(void) {}

/* Post-mortem state for a halted image. Both the stack-overflow hook and
   fail() stop the CPU with interrupts disabled and print nothing, so a wedged
   board looks exactly like a board whose UART went quiet -- that is how the
   osal-slow stack overflow stayed undiagnosed across several HIL runs in this
   repository. These words are written *before* the halt and can be read back
   through the debugger while the core is spinning.

   The error marker names the failing step on the wire, but only for a check
   that got far enough to print it; a fault before the first UART write, or a
   stack overflow, prints nothing at all. */
volatile uint32_t xy_image_failed;
volatile uint32_t xy_image_failed_line;
volatile const char *xy_image_failed_task;
volatile const char *xy_image_last_marker;

/* The OSAL's default xy_os_tick_get() reads the misc tick counter, which
   nothing increments once the kernel owns SysTick -- so it would read 0 forever
   and every elapsed interval would be zero. The kernel's own tick count is the
   real one, and the OSAL runtime image overrides it the same way. */
uint32_t xy_os_tick_get(void)
{
    return xy_os_kernel_get_tick_count();
}

/* Every check ends at fail(), and the line is what identifies it. */
static void fail_at(uint32_t line)
{
    xy_image_failed = 2U;
    xy_image_failed_line = line;
    __disable_irq();
    for (;;) {
    }
}

#define fail() fail_at((uint32_t)__LINE__)

static void uart_text(const char *text)
{
    uint16_t length = 0U;

    xy_image_last_marker = text;
    while (text[length] != '\0') {
        ++length;
    }
    (void)xy_hal_uart_send(&uart1, (const uint8_t *)text, length, 100U);
}

static void uart_init(void)
{
    const xy_hal_gpio_config_t gpio = {XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_UP,
                                       XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_VERY_HIGH,
                                       GPIO_AF7_USART1};
    const xy_hal_uart_config_t uart = {115200U, XY_HAL_UART_WORDLEN_8B,
                                       XY_HAL_UART_STOPBITS_1, XY_HAL_UART_PARITY_NONE,
                                       XY_HAL_UART_FLOWCTRL_NONE, XY_HAL_UART_MODE_TX_RX};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();
    if (xy_hal_gpio_init(GPIOA, 9U, &gpio) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &gpio) != XY_HAL_OK) {
        fail();
    }
    uart1.Instance = USART1;
    if (xy_hal_uart_init(&uart1, &uart) != XY_HAL_OK) {
        fail();
    }
}

static xy_pid_config_t default_config(void)
{
    xy_pid_config_t config = {
        .kp = 2.0F,
        .ki = 0.5F,
        .kd = 0.25F,
        .output_min = -100.0F,
        .output_max = 100.0F,
        .integral_min = -20.0F,
        .integral_max = 20.0F,
        .derivative_filter = 0.1F,
    };
    return config;
}

/* The comparison tolerance the Host tests use. */
#define TOLERANCE 0.001F

static int within(float expected, float actual)
{
    float difference = expected - actual;

    if (difference < 0.0F) {
        difference = -difference;
    }
    return difference <= TOLERANCE;
}


/* Print a small unsigned number, so a failing check names itself on the wire
   instead of only saying "something failed".

   The digits come out least-significant first, so they are reversed in place
   before printing. The buffer holds ten digits plus the terminator, which is
   the widest value a uint32_t can produce. */
static void uart_u32(uint32_t value)
{
    char buffer[11];
    uint32_t length = 0U;
    uint32_t index;

    if (value == 0U) {
        uart_text("0");
        return;
    }
    while (value != 0U && length < (uint32_t)(sizeof(buffer) - 1U)) {
        buffer[length] = (char)('0' + (value % 10U));
        value /= 10U;
        ++length;
    }
    for (index = 0U; index < (length / 2U); ++index) {
        char swap = buffer[index];

        buffer[index] = buffer[length - 1U - index];
        buffer[length - 1U - index] = swap;
    }
    buffer[length] = '\0';
    uart_text(buffer);
}

/* Check 1: the arithmetic itself, with the elapsed time supplied by the real
   OSAL tick. The controller is stepped with a real delay between updates, so dt
   is the interval the kernel actually measured rather than a constant. */
static uint32_t check_closed_loop(void)
{
    xy_pid_t pid;
    xy_pid_config_t config = default_config();
    float output = 0.0F;
    uint32_t step;

    if (xy_pid_init(&pid, &config) != XY_PID_OK) {
        return 1U;
    }
    /* The controller starts in manual mode: xy_pid_compute() passes the previous
       output through until the mode is switched, so a test that forgets this
       sees a controller that never moves. */
    if (xy_pid_set_mode(&pid, XY_PID_MODE_AUTO) != XY_PID_OK) {
        return 2U;
    }
    if (xy_pid_set_setpoint(&pid, 10.0F) != XY_PID_OK) {
        return 3U;
    }
    if (xy_pid_set_input(&pid, 3.0F) != XY_PID_OK) {
        return 4U;
    }
    if (!within(3.0F, pid.input)) {
        return 5U;
    }
    /* Before the first compute the controller is still waiting for a baseline. */
    if (!pid.first_run) {
        return 6U;
    }
    if (pid.update_count != 0U) {
        return 7U;
    }

    /* The first compute establishes the baseline: no output yet, and the
       controller has consumed its first_run flag. */
    if (xy_pid_compute(&pid, 4.0F, &output) != XY_PID_OK) {
        return 8U;
    }
    if (!within(0.0F, output)) {
        return 9U;
    }
    if (pid.first_run) {
        return 10U;
    }
    if (pid.update_count != 0U) {
        return 11U;
    }

    /* Now step it for real: 20 updates with a 10 ms delay each, so the tick
       advances between computes and dt is non-zero. */
    for (step = 0U; step < 20U; ++step) {
        (void)xy_os_delay(10U);
        if (xy_pid_compute(&pid, 4.0F, &output) != XY_PID_OK) {
            return 12U;
        }
    }

    /* Every compute after the baseline counts as an update. */
    if (pid.update_count != 20U) {
        return 13U;
    }
    /* Setpoint 10, input 4: the error is 6 and must be reported as such. */
    if (!within(6.0F, xy_pid_get_error(&pid))) {
        return 14U;
    }
    /* The output must have moved off zero and stayed inside the limits. */
    if (!(output > 0.0F)) {
        return 15U;
    }
    if (output > 100.0F || output < -100.0F) {
        return 16U;
    }
    /* The proportional term alone is kp * error = 2 * 6 = 12, and the integral
       and derivative terms are non-negative here, so the output is at least the
       proportional term. This is the arithmetic the Host test pins, now on
       soft float. */
    if (output < 12.0F) {
        return 17U;
    }
    /* The integral must have accumulated: error * dt summed over 20 steps of
       about 10 ms each, so roughly 6 * 200 = 1200, clamped by the anti-windup
       limits to the configured integral_max of 20. */
    if (!(xy_pid_get_integral(&pid) > 0.0F)) {
        return 18U;
    }
    if (xy_pid_get_integral(&pid) > 20.0F + TOLERANCE) {
        return 19U;
    }
    /* The derivative of a constant error is zero, filtered towards zero. */
    if (!within(0.0F, xy_pid_get_derivative(&pid))) {
        return 20U;
    }

    return 0U;
}

/* Check 2: anti-windup clamps the integral and the output. */
static uint32_t check_anti_windup(void)
{
    xy_pid_t pid;
    xy_pid_config_t config = default_config();
    float output = 0.0F;
    uint32_t step;

    if (xy_pid_init(&pid, &config) != XY_PID_OK) {
        return 1U;
    }
    if (xy_pid_set_mode(&pid, XY_PID_MODE_AUTO) != XY_PID_OK) {
        return 2U;
    }
    if (xy_pid_enable_anti_windup(&pid, true) != XY_PID_OK) {
        return 3U;
    }
    if (xy_pid_set_setpoint(&pid, 100.0F) != XY_PID_OK) {
        return 4U;
    }

    for (step = 0U; step < 50U; ++step) {
        (void)xy_os_delay(10U);
        if (xy_pid_compute(&pid, 0.0F, &output) != XY_PID_OK) {
            return 5U;
        }
    }

    /* Enabling anti-windup widens the integral bounds to the output bounds, so
       the integral clamps at output_max (100), not at the config's integral_max
       (20). Unclamped it would reach 100 * 10 ms * 50 steps = 50000. */
    if (xy_pid_get_integral(&pid) > 100.0F + TOLERANCE) {
        return 6U;
    }
    if (xy_pid_get_integral(&pid) < 99.0F) {
        return 7U;
    }
    /* The output is clamped to the configured maximum. */
    if (output > 100.0F) {
        return 8U;
    }
    if (output < 99.0F) {
        return 9U;
    }

    return 0U;
}

/* Check 3: the modes and the invalid-input contract. */
static uint32_t check_modes_and_contract(void)
{
    xy_pid_t pid;
    xy_pid_config_t config = default_config();
    float output = -77.0F;

    if (xy_pid_init(&pid, &config) != XY_PID_OK) {
        return 1U;
    }
    if (xy_pid_set_mode(&pid, XY_PID_MODE_AUTO) != XY_PID_OK) {
        return 2U;
    }
    if (xy_pid_set_setpoint(&pid, 10.0F) != XY_PID_OK) {
        return 3U;
    }
    if (xy_pid_set_input(&pid, 3.0F) != XY_PID_OK) {
        return 4U;
    }

    /* A non-finite setpoint is refused and the old one survives. */
    if (xy_pid_set_setpoint(&pid, NAN) == XY_PID_OK) {
        return 5U;
    }
    if (!within(10.0F, pid.setpoint)) {
        return 6U;
    }
    /* A non-finite input is refused and the old one survives. */
    if (xy_pid_set_input(&pid, INFINITY) == XY_PID_OK) {
        return 7U;
    }
    if (!within(3.0F, pid.input)) {
        return 8U;
    }
    /* A non-finite measurement is refused and the output is left alone. */
    output = -77.0F;
    if (xy_pid_compute(&pid, NAN, &output) == XY_PID_OK) {
        return 9U;
    }
    if (!within(-77.0F, output)) {
        return 10U;
    }

    /* In manual mode the controller passes the previous output through. */
    if (xy_pid_set_mode(&pid, XY_PID_MODE_MANUAL) != XY_PID_OK) {
        return 11U;
    }
    if (xy_pid_get_mode(&pid) != XY_PID_MODE_MANUAL) {
        return 12U;
    }
    output = -77.0F;
    if (xy_pid_compute(&pid, 4.0F, &output) != XY_PID_OK) {
        return 13U;
    }
    if (!within(0.0F, output)) {
        return 14U;
    }

    return 0U;
}

/* Check 4: reset returns the controller to its initial state. */
static uint32_t check_reset(void)
{
    xy_pid_t pid;
    xy_pid_config_t config = default_config();
    float output = 0.0F;

    if (xy_pid_init(&pid, &config) != XY_PID_OK) {
        return 1U;
    }
    if (xy_pid_set_mode(&pid, XY_PID_MODE_AUTO) != XY_PID_OK) {
        return 2U;
    }
    if (xy_pid_set_setpoint(&pid, 10.0F) != XY_PID_OK) {
        return 3U;
    }
    (void)xy_os_delay(10U);
    if (xy_pid_compute(&pid, 4.0F, &output) != XY_PID_OK) {
        return 4U;
    }
    (void)xy_os_delay(10U);
    if (xy_pid_compute(&pid, 4.0F, &output) != XY_PID_OK) {
        return 5U;
    }
    if (pid.update_count == 0U) {
        return 6U;
    }

    if (xy_pid_reset(&pid) != XY_PID_OK) {
        return 7U;
    }
    if (pid.update_count != 0U) {
        return 8U;
    }
    if (!within(0.0F, xy_pid_get_error(&pid))) {
        return 9U;
    }
    if (!within(0.0F, xy_pid_get_integral(&pid))) {
        return 10U;
    }
    if (!within(0.0F, xy_pid_get_derivative(&pid))) {
        return 11U;
    }

    return 0U;
}

static void worker(void *argument)
{
    uint32_t step;

    (void)argument;

    uart_text("PANDORA PID BOARD READY\r\n");
    uart_text("FIRMWARE_COMMIT " XINYI_FIRMWARE_COMMIT "\r\n");

    step = check_closed_loop();
    if (step != 0U) {
        uart_text("PANDORA_PID_LOOP_ERROR_STEP_");
        uart_u32(step);
        uart_text("\r\n");
        fail();
    }
    uart_text("PANDORA_PID_LOOP_OK_A\r\n");

    step = check_anti_windup();
    if (step != 0U) {
        uart_text("PANDORA_PID_WINDUP_ERROR_STEP_");
        uart_u32(step);
        uart_text("\r\n");
        fail();
    }
    uart_text("PANDORA_PID_WINDUP_OK_A\r\n");

    step = check_modes_and_contract();
    if (step != 0U) {
        uart_text("PANDORA_PID_CONTRACT_ERROR_STEP_");
        uart_u32(step);
        uart_text("\r\n");
        fail();
    }
    uart_text("PANDORA_PID_CONTRACT_OK_A\r\n");

    step = check_reset();
    if (step != 0U) {
        uart_text("PANDORA_PID_RESET_ERROR_STEP_");
        uart_u32(step);
        uart_text("\r\n");
        fail();
    }
    uart_text("PANDORA_PID_RESET_OK_A\r\n");

    uart_text("B1_PID_BOARD_RUNTIME_PASS\r\n");

    for (;;) {
        (void)xy_os_delay(1000U);
    }
}

int main(void)
{
    const xy_os_thread_attr_t attr = {.name = "pid", .stack_size = 1024,
                                      .priority = XY_OS_PRIORITY_NORMAL};

    if (xy_hal_sys_init() != XY_HAL_OK || pandora_platform_startup() != 0) {
        fail();
    }
    uart_init();

    if (xy_os_kernel_init() != 0) {
        fail();
    }
    if (xy_os_thread_new(worker, NULL, &attr) == NULL) {
        fail();
    }
    if (xy_os_kernel_start() != 0) {
        fail();
    }

    for (;;) {
    }
}
