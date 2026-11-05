/*
 * The Pandora watchdog refresh policy, run on the board.
 *
 * Why this image exists
 * ---------------------
 * `docs/validation/component-evidence-matrix.md` lists, in the SYS row's limitations
 * column, "watchdog refresh policy" as still pending. The reset-reason half of that
 * row is closed -- S2-04 and S5-107 already produced write/verify/read-back plus
 * UART B1/B2 for a software reset, an IWDG timeout and an ST-Link pin reset. What
 * has never been run is the other half: the policy itself.
 *
 * The board smoke image (`main.c`) starts the IWDG in the PANDORA_RESET_KIND_SOFTWARE
 * branch and then spins in an empty `for (;;)`. That is deliberately the *absence* of
 * a refresh: the board is handed to the watchdog so the WATCHDOG boot path gets
 * exercised. It proves the board can detect a watchdog reset. It proves nothing about
 * whether feeding the watchdog keeps the board alive, because nothing ever fed it.
 *
 * What "refresh policy works" has to mean
 * --------------------------------------
 * A claim that a refresh policy works is worth nothing unless it could have failed.
 * Three things have to hold at once, and each is separately falsifiable:
 *
 *   1. The watchdog is armed and counting.  Asserting it proves nothing; the image
 *      reports the window it armed, through the canonical HAL.
 *   2. The refresh extends the deadline.  The board has to stay alive across a span
 *      that is a large multiple of the watchdog window, which is only possible
 *      because the feeds happened.
 *   3. Removing the refresh kills the board.  This is the negative control, and it
 *      is the one that makes the evidence mean something: a capture that only shows
 *      "the board survived" is indistinguishable from a capture of an inert watchdog.
 *
 * Point 3 is why this image ends by *stopping* to feed. The board is meant to die of
 * watchdog expiry, and the next boot reports that expiry. The whole sequence
 * therefore spans a reset, so the capture window has to be open across it and the
 * capture has to tolerate the partial line a reset cuts in half.
 *
 * How the two phases are told apart
 * ---------------------------------
 * The image runs the feed phase on one boot and the report on the next. A retained
 * RTC backup slot records that the feed phase finished: the backup domain survives
 * a system reset, which is the same mechanism the board's own reset-request
 * retention already uses. The slot is read *before* it is cleared at every boot, so
 * "the marker is set" means exactly one thing -- the previous boot completed the
 * feed phase and then stopped feeding. The reset cause is checked independently, so
 * the report phase also proves the reset really was a watchdog expiry rather than
 * the marker being stale.
 *
 * A note on the remaining-time reading
 * ------------------------------------
 * `xy_hal_iwdg_get_remaining_time()` on the STM32L4 backend is *computed* from the
 * configured reload value and prescaler against a nominal 32 kHz LSI -- it is not a
 * read of a live hardware counter, so it cannot show a countdown decreasing. This
 * image therefore does not pretend to observe one. It reports the nominal window it
 * armed, and it anchors the refresh claim on elapsed wall time measured outside the
 * board instead. That also keeps the claim honest about the LSI: the real LSI is
 * only accurate to a range, so an armed window computed from a nominal 32 kHz is a
 * nominal window, and the evidence must not pretend otherwise.
 *
 * Boundaries
 * ----------
 * This is a bare-metal image, so the feed interval is a busy-wait interval and not a
 * task deadline. It proves the canonical `xy_hal_iwdg_*` refresh contract on the
 * target. It does not prove that any application has a correct refresh policy, it is
 * not an endurance or timing-precision claim, and it is not a power-loss or brownout
 * claim.
 */

#include "stm32l4xx_hal.h"
#include "pandora_platform_startup.h"
#include "pandora_sys.h"
#include "xy_hal_delay.h"
#include "xy_hal_gpio.h"
#include "xy_hal_rtc.h"
#include "xy_hal_sys.h"
#include "xy_hal_uart.h"
#include "xy_hal_wdg.h"
#include "xy_sys.h"

#include <stdint.h>

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the source commit used for this image"
#endif

/* The retained "the feed phase finished" marker. Slot 4 is unowned: slots 0..2
   belong to the W25Q128 and FOTA confirm/restage flows and slot 3 to the board's
   reset-request retention. */
#define PANDORA_WATCHDOG_BACKUP_SLOT 4U
#define PANDORA_WATCHDOG_BACKUP_MAGIC 0x57524450UL /* "WRDP" */

/* The feed interval. It has to be comfortably shorter than the armed window, so
   that a schedule which is merely "too slow" is caught rather than rounded into a
   pass by an LSI that happens to run fast. The worst case matters: the STM32L4
   LSI is specified only as a range around 32 kHz, so a window computed from a
   nominal 32 kHz can be materially shorter in reality. At the top of that range
   the real window is roughly 80 ms, so a 40 ms period would leave less than two
   feeds of margin. A 25 ms period keeps the feed comfortably inside the window
   under any LSI the part is allowed to have. */
#define WATCHDOG_FEED_INTERVAL_MS 25U

/* The armed window, expressed as the reload value and prescaler the HAL takes.
   The STM32L4 IWDG counts LSI/prescaler ticks down from reload+1, so the nominal
   window is (reload+1) * prescaler / LSI. With prescaler 32 and reload 124 that is
   125 * 32 / 32000 = 125 ms nominal. */
#define WATCHDOG_RELOAD 124U
#define WATCHDOG_PRESCALER IWDG_PRESCALER_32

/* How many feed cycles to run. The span has to be a large multiple of the window,
   otherwise "the board survived" could just mean "the board had not expired yet". */
#define WATCHDOG_FEED_CYCLES 40U

static UART_HandleTypeDef uart1;

/* Post-mortem state for a halted image. Both the fail() path and a stack overflow
   stop the CPU with interrupts disabled and print nothing, so a wedged board looks
   exactly like a board whose UART went quiet. These words are written *before* the
   halt and can be read back through the debugger while the core is spinning. */
volatile uint32_t xy_image_failed;
volatile uint32_t xy_image_failed_line;
volatile const char *xy_image_failed_task;
volatile const char *xy_image_last_marker;

void _init(void) {}
void _fini(void) {}

void SysTick_Handler(void)
{
    xy_hal_sys_tick_irq_handler();
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
    (void)xy_hal_uart_send(&uart1, (const uint8_t *)text, length, 200U);
}

static void uart_u32(uint32_t value)
{
    uint8_t digits[10];
    uint32_t count = 0U;

    do {
        digits[count++] = (uint8_t)('0' + value % 10U);
        value /= 10U;
    } while (value != 0U);
    while (count != 0U) {
        --count;
        (void)xy_hal_uart_send(&uart1, &digits[count], 1U, 200U);
    }
}

static void gpio_led_init(void)
{
    const xy_hal_gpio_config_t led_gpio = {
        XY_HAL_GPIO_MODE_OUTPUT, XY_HAL_GPIO_PULL_NONE, XY_HAL_GPIO_OTYPE_PP,
        XY_HAL_GPIO_SPEED_LOW, 0U,
    };

    __HAL_RCC_GPIOE_CLK_ENABLE();
    if (xy_hal_gpio_init(GPIOE, 7U, &led_gpio) != XY_HAL_OK) {
        fail();
    }
}

static void uart_init(void)
{
    const xy_hal_gpio_config_t uart_gpio = {
        XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_UP, XY_HAL_GPIO_OTYPE_PP,
        XY_HAL_GPIO_SPEED_VERY_HIGH, GPIO_AF7_USART1,
    };
    const xy_hal_uart_config_t uart_config = {
        115200U, XY_HAL_UART_WORDLEN_8B, XY_HAL_UART_STOPBITS_1, XY_HAL_UART_PARITY_NONE,
        XY_HAL_UART_FLOWCTRL_NONE, XY_HAL_UART_MODE_TX_RX,
    };

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();
    uart1.Instance = USART1;
    if (xy_hal_gpio_init(GPIOA, 9U, &uart_gpio) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &uart_gpio) != XY_HAL_OK ||
        xy_hal_uart_init(&uart1, &uart_config) != XY_HAL_OK) {
        fail();
    }
}

/* The retained marker, read before it is cleared so that "the marker is set" means
   exactly one thing: the previous boot completed the feed phase. */
static uint32_t watchdog_marker;

static void marker_read(void)
{
    watchdog_marker = 0U;
    if (xy_hal_rtc_backup_read(PANDORA_WATCHDOG_BACKUP_SLOT, &watchdog_marker) != XY_HAL_OK) {
        watchdog_marker = 0U;
    }
    (void)xy_hal_rtc_backup_write(PANDORA_WATCHDOG_BACKUP_SLOT, 0U);
}

int main(void)
{
    IWDG_HandleTypeDef watchdog = {0};
    const xy_hal_iwdg_config_t watchdog_config = {
        .prescaler = WATCHDOG_PRESCALER,
        .reload = WATCHDOG_RELOAD,
        .timeout_ms = 0U,
    };
    pandora_reset_kind_t reset_kind;
    uint32_t marker_set;
    uint32_t cycle;
    uint32_t started_ms;
    uint32_t elapsed_ms;
    int window_ms;

    if (xy_hal_sys_init() != XY_HAL_OK) {
        fail();
    }
    if (pandora_platform_startup() != 0) {
        fail();
    }
    gpio_led_init();
    uart_init();

    /* The board SYS owner latches the inherited reset cause and its raw register
       before the vendor flags are cleared, and attributes it to one name. */
    xy_sys_init();
    reset_kind = pandora_sys_reset_kind();

    marker_read();
    marker_set = (watchdog_marker == PANDORA_WATCHDOG_BACKUP_MAGIC) ? 1U : 0U;

    uart_text("PANDORA WATCHDOG REFRESH READY\r\n");
    uart_text("FIRMWARE_COMMIT " XINYI_FIRMWARE_COMMIT "\r\n");
    uart_text("SYS_RESET_KIND ");
    uart_text(pandora_reset_kind_name(reset_kind));
    uart_text("\r\n");
    uart_text("WATCHDOG_PHASE ");
    uart_text(marker_set != 0U ? "REPORT" : "FEED");
    uart_text("\r\n");

    if (marker_set != 0U) {
        /* The previous boot completed the feed phase and then stopped feeding, so
           the board is now waking up from a watchdog expiry. If the cause is not
           the watchdog, the marker was stale and this report would be a lie. */
        if (reset_kind != PANDORA_RESET_KIND_WATCHDOG) {
            uart_text("WATCHDOG_RESET_CAUSE_MISMATCH\r\n");
            fail();
        }
        uart_text("WATCHDOG_EXPIRY_RESET_OK\r\n");
        uart_text("WATCHDOG_REFRESH_POLICY_PASS\r\n");
        /* Stay alive so the capture ends on a clean marker instead of a reset. */
        for (;;) {
            (void)xy_hal_gpio_toggle(GPIOE, 7U);
            xy_hal_delay_ms(100U);
        }
    }

    /* Phase one: feed on a schedule. Each feed is a canonical HAL refresh, and the
       marker names the cycle so the interval between feeds can be measured from the
       arrival times rather than from the image's own arithmetic. */
    watchdog.Instance = IWDG;
    if (xy_hal_iwdg_init(&watchdog, &watchdog_config) != XY_HAL_OK) {
        uart_text("WATCHDOG_INIT_ERROR\r\n");
        fail();
    }

    /* The nominal window this image armed, as the HAL reports it. This is computed
       from the configuration against a nominal 32 kHz LSI, not a live counter read,
       and the marker says so. */
    window_ms = xy_hal_iwdg_get_remaining_time(&watchdog);
    if (window_ms <= 0) {
        uart_text("WATCHDOG_WINDOW_ERROR\r\n");
        fail();
    }
    uart_text("WATCHDOG_ARMED nominal_window_ms=");
    uart_u32((uint32_t)window_ms);
    uart_text(" feed_interval_ms=");
    uart_u32(WATCHDOG_FEED_INTERVAL_MS);
    uart_text(" cycles=");
    uart_u32(WATCHDOG_FEED_CYCLES);
    uart_text("\r\n");

    started_ms = xy_hal_sys_get_tick_count();
    for (cycle = 1U; cycle <= WATCHDOG_FEED_CYCLES; ++cycle) {
        if (xy_hal_iwdg_feed(&watchdog) != XY_HAL_OK) {
            uart_text("WATCHDOG_FEED_ERROR\r\n");
            fail();
        }
        uart_text("WATCHDOG_FEED cycle=");
        uart_u32(cycle);
        uart_text(" elapsed_ms=");
        uart_u32(xy_hal_sys_get_tick_count() - started_ms);
        uart_text("\r\n");
        (void)xy_hal_gpio_toggle(GPIOE, 7U);
        xy_hal_delay_ms(WATCHDOG_FEED_INTERVAL_MS);
    }

    /* The board is still alive after a large multiple of the window. That is only
       possible because the feeds happened. */
    elapsed_ms = xy_hal_sys_get_tick_count() - started_ms;
    uart_text("WATCHDOG_REFRESH_SURVIVED cycles=");
    uart_u32(WATCHDOG_FEED_CYCLES);
    uart_text(" elapsed_ms=");
    uart_u32(elapsed_ms);
    uart_text("\r\n");

    /* Record that the feed phase is done, so the next boot knows to report the
       expiry instead of feeding again. */
    if (xy_hal_rtc_backup_write(PANDORA_WATCHDOG_BACKUP_SLOT,
                                PANDORA_WATCHDOG_BACKUP_MAGIC) != XY_HAL_OK) {
        uart_text("WATCHDOG_MARKER_ERROR\r\n");
        fail();
    }
    uart_text("WATCHDOG_MARKER_SET\r\n");

    /* Phase two, the negative control: stop feeding. The watchdog now has to expire
       and reset the board. If it does not, the refresh claim above was decoration. */
    uart_text("WATCHDOG_FEED_STOPPED\r\n");
    for (;;) {
        (void)xy_hal_gpio_toggle(GPIOE, 7U);
        xy_hal_delay_ms(50U);
    }
}
