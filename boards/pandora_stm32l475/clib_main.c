/*
 * Clib on the real board.
 *
 * The clib component (xy_frame codec, xy_civil calendar math, xy_bcd register
 * encoding, xy_rtc_alarm_mask) has 40 Host test cases but no board evidence, so
 * the matrix still lists its Board column as pending. Everything here is pure
 * computation, which is exactly the part that can differ on the target: the
 * calendar math is 64-bit, the frame codec indexes fixed-size buffers, and the
 * CRC-8/ATM loop is a different code path on a Cortex-M4 than on the host.
 *
 * The checks below are the same published values the Host tests pin, so a
 * divergence between host and board shows up as a failed marker instead of as
 * a silently different answer.
 */

#include "pandora_platform_startup.h"
#include "stm32l4xx_hal.h"
#include "xy_bcd.h"
#include "xy_frame.h"
#include "xy_hal_delay.h"
#include "xy_hal_sys.h"
#include "xy_hal_uart.h"
#include "xy_rtc_alarm_mask.h"
#include "xy_time_civil.h"

#include <stddef.h>
#include <stdint.h>

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the source commit used for this image"
#endif

static UART_HandleTypeDef uart1;

void _init(void) {}
void _fini(void) {}
void SysTick_Handler(void) { xy_hal_sys_tick_irq_handler(); }

static void fail(void)
{
    __disable_irq();
    for (;;) {
    }
}

static void uart_text(const char *text)
{
    uint16_t length = 0U;

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
    /* The handle's Instance must name the peripheral before the HAL touches it:
       HAL_UART_Init dereferences it to reach the registers. */
    uart1.Instance = USART1;
    if (xy_hal_uart_init(&uart1, &uart) != XY_HAL_OK) {
        fail();
    }
}

static uint32_t check_frame_codec(void)
{
    static const uint8_t payload[] = {0x11U, 0x22U, 0x33U};
    uint8_t wire[XY_FRAME_PAYLOAD_MAX + 8U];
    uint8_t too_small[2];
    xy_frame_decoder_t decoder;
    xy_frame_t frame;
    size_t encoded;
    size_t index;
    uint8_t expected_crc;

    /* CRC-8/ATM check value: this is what makes it the standard polynomial
       rather than eight bits of arbitrary feedback. */
    {
        static const uint8_t vector[] = "123456789";
        if (xy_frame_crc8(vector, sizeof(vector) - 1U) != 0xF4U) {
            return 1U;
        }
    }

    /* A transposition must change the checksum: {0x12,0x34} and {0x34,0x12} have
       identical xor sums, which is why the check is a CRC and not a sum. */
    {
        static const uint8_t one[] = {0x12U, 0x34U};
        static const uint8_t other[] = {0x34U, 0x12U};
        if (xy_frame_crc8(one, sizeof(one)) == xy_frame_crc8(other, sizeof(other))) {
            return 2U;
        }
    }

    /* Encode, then decode the same bytes back. */
    encoded = xy_frame_encode(0x43U, payload, sizeof(payload), wire, sizeof(wire));
    if (encoded == 0U) {
        return 3U;
    }
    if (wire[0] != XY_FRAME_STX) {
        return 4U;
    }
    /* The wire layout is STX, length, type, payload..., checksum. */
    if (wire[1] != sizeof(payload) + 1U || wire[2] != 0x43U) {
        return 5U;
    }
    xy_frame_decoder_reset(&decoder);
    for (index = 0U; index < encoded; ++index) {
        if (xy_frame_decode_byte(&decoder, wire[index], &frame) == XY_FRAME_COMPLETE) {
            break;
        }
    }
    if (index >= encoded) {
        return 6U;
    }
    if (frame.type != 0x43U || frame.length != sizeof(payload)) {
        return 7U;
    }
    for (index = 0U; index < sizeof(payload); ++index) {
        if (frame.payload[index] != payload[index]) {
            return 8U;
        }
    }

    /* The checksum covers the type byte and the payload, computed independently
       here rather than read back from the encoder. */
    expected_crc = xy_frame_crc8(&wire[2], (size_t)(sizeof(payload) + 1U));
    if (wire[encoded - 1U] != expected_crc) {
        return 9U;
    }

    /* A corrupted checksum must be rejected. */
    wire[encoded - 1U] = (uint8_t)(wire[encoded - 1U] ^ 0x01U);
    xy_frame_decoder_reset(&decoder);
    for (index = 0U; index < encoded; ++index) {
        if (xy_frame_decode_byte(&decoder, wire[index], &frame) == XY_FRAME_REJECTED) {
            break;
        }
    }
    if (index >= encoded) {
        return 10U;
    }

    /* A start byte inside the payload must not split the frame. */
    {
        uint8_t tricky[] = {0x55U, XY_FRAME_STX, 0x66U};
        xy_frame_decoder_reset(&decoder);
        encoded = xy_frame_encode(0x44U, tricky, sizeof(tricky), wire, sizeof(wire));
        if (encoded == 0U) {
            return 11U;
        }
        xy_frame_decoder_reset(&decoder);
        for (index = 0U; index < encoded; ++index) {
            if (xy_frame_decode_byte(&decoder, wire[index], &frame) == XY_FRAME_COMPLETE) {
                break;
            }
        }
        if (index >= encoded || frame.length != sizeof(tricky)) {
            return 12U;
        }
        if (frame.payload[0] != 0x55U || frame.payload[1] != XY_FRAME_STX ||
            frame.payload[2] != 0x66U) {
            return 13U;
        }
    }

    /* A buffer that cannot hold the frame must be refused, not truncated. */
    if (xy_frame_encode(0x43U, payload, sizeof(payload), too_small, sizeof(too_small)) != 0U) {
        return 14U;
    }

    /* A payload longer than the limit must be refused. */
    {
        uint8_t big[XY_FRAME_PAYLOAD_MAX + 1U];
        for (index = 0U; index < sizeof(big); ++index) {
            big[index] = (uint8_t)index;
        }
        if (xy_frame_encode(0x43U, big, sizeof(big), wire, sizeof(wire)) != 0U) {
            return 15U;
        }
    }

    /* Noise before a frame must be ignored, and the frame after it must still
       decode. */
    {
        uint8_t noisy[] = {0x00U, 0xFFU, 0x01U};
        xy_frame_decoder_reset(&decoder);
        for (index = 0U; index < sizeof(noisy); ++index) {
            if (xy_frame_decode_byte(&decoder, noisy[index], &frame) != XY_FRAME_NONE) {
                return 16U;
            }
        }
        encoded = xy_frame_encode(0x45U, payload, sizeof(payload), wire, sizeof(wire));
        for (index = 0U; index < encoded; ++index) {
            if (xy_frame_decode_byte(&decoder, wire[index], &frame) == XY_FRAME_COMPLETE) {
                break;
            }
        }
        if (index >= encoded || frame.type != 0x45U) {
            return 17U;
        }
    }

    /* An impossible length must be rejected. */
    {
        uint8_t bad[] = {XY_FRAME_STX, 0x00U};
        xy_frame_decoder_reset(&decoder);
        for (index = 0U; index < sizeof(bad); ++index) {
            if (xy_frame_decode_byte(&decoder, bad[index], &frame) == XY_FRAME_REJECTED) {
                break;
            }
        }
        if (index >= sizeof(bad)) {
            return 18U;
        }
    }

    return 0U;
}

static uint32_t check_civil_calendar(void)
{
    int year = 0;
    unsigned int month = 0U;
    unsigned int day = 0U;
    int64_t seconds;

    /* The epoch is day zero. */
    if (xy_civil_days_from_epoch(1970, 1U, 1U) != 0) {
        return 1U;
    }
    if (xy_civil_seconds_from_epoch(1970, 1U, 1U, 0U, 0U, 0U) != 0) {
        return 2U;
    }

    /* Known dates, against the Unix epoch. */
    if (xy_civil_days_from_epoch(2000, 3U, 1U) != 11017) {
        return 3U;
    }
    if (xy_civil_days_from_epoch(2026, 11U, 3U) != 20760) {
        return 4U;
    }
    seconds = xy_civil_seconds_from_epoch(2026, 11U, 3U, 12U, 30U, 45U);
    if (seconds != 20760LL * 86400LL + 12LL * 3600LL + 30LL * 60LL + 45LL) {
        return 5U;
    }

    /* Months are not thirty days: March has 31, April has 30. */
    if (xy_civil_days_from_epoch(2026, 2U, 1U) - xy_civil_days_from_epoch(2026, 1U, 1U) != 31) {
        return 6U;
    }
    if (xy_civil_days_from_epoch(2026, 4U, 1U) - xy_civil_days_from_epoch(2026, 3U, 1U) != 31) {
        return 7U;
    }
    if (xy_civil_days_from_epoch(2026, 5U, 1U) - xy_civil_days_from_epoch(2026, 4U, 1U) != 30) {
        return 8U;
    }

    /* Leap years: 2024 and 2000 are, 2026 and 1900 are not. */
    if (xy_civil_days_from_epoch(2024, 3U, 1U) - xy_civil_days_from_epoch(2024, 2U, 1U) != 29) {
        return 9U;
    }
    if (xy_civil_days_from_epoch(2026, 3U, 1U) - xy_civil_days_from_epoch(2026, 2U, 1U) != 28) {
        return 10U;
    }
    if (xy_civil_days_from_epoch(1900, 3U, 1U) - xy_civil_days_from_epoch(1900, 2U, 1U) != 28) {
        return 11U;
    }
    if (xy_civil_days_from_epoch(2000, 3U, 1U) - xy_civil_days_from_epoch(2000, 2U, 1U) != 29) {
        return 12U;
    }

    /* The inverse must round-trip. */
    xy_civil_from_days(20760, &year, &month, &day);
    if (year != 2026 || month != 11U || day != 3U) {
        return 13U;
    }
    xy_civil_from_days(0, &year, &month, &day);
    if (year != 1970 || month != 1U || day != 1U) {
        return 14U;
    }
    xy_civil_from_days(11017, &year, &month, &day);
    if (year != 2000 || month != 3U || day != 1U) {
        return 15U;
    }

    /* Weekday: 1 = Monday through 7 = Sunday. 1970-01-01 was a Thursday. */
    if (xy_civil_weekday_from_days(0) != 4U) {
        return 16U;
    }
    if (xy_civil_weekday_from_days(20760) != 2U) {
        return 17U;
    }

    return 0U;
}

static uint32_t check_bcd(void)
{
    uint8_t decoded8 = 0U;
    uint16_t decoded16 = 0U;

    if (xy_bcd_encode8(0U) != 0x00U || xy_bcd_encode8(9U) != 0x09U ||
        xy_bcd_encode8(10U) != 0x10U || xy_bcd_encode8(99U) != 0x99U) {
        return 1U;
    }
    if (xy_bcd_decode8(0x42U, &decoded8) != 1 || decoded8 != 42U) {
        return 2U;
    }
    /* A nibble above nine is not a number. */
    if (xy_bcd_decode8(0x4AU, &decoded8) != 0) {
        return 3U;
    }

    if (xy_bcd_encode16(1234U) != 0x1234U) {
        return 4U;
    }
    if (xy_bcd_decode16(0x5678U, &decoded16) != 1 || decoded16 != 5678U) {
        return 5U;
    }
    if (xy_bcd_decode16(0x567AU, &decoded16) != 0) {
        return 6U;
    }

    /* The RTC year is a full year in the public header, and the format argument
       only decides how the digits are spelled. */
    if (xy_rtc_year_decode(0x26U, 1) != 2026) {
        return 7U;
    }
    if (xy_rtc_year_decode(0x1AU, 0) != 2026) {
        return 8U;
    }
    /* 0x1A read as BCD is not a number, so it must be refused, not guessed. */
    if (xy_rtc_year_decode(0x1AU, 1) != -1) {
        return 9U;
    }
    if (xy_rtc_year_encode(2026U, 1) != 0x26) {
        return 10U;
    }
    if (xy_rtc_year_encode(2026U, 0) != 26) {
        return 11U;
    }
    /* A year the register cannot hold is refused rather than truncated. */
    if (xy_rtc_year_encode(2100U, 0) != -1) {
        return 12U;
    }
    if (xy_rtc_year_encode(1999U, 0) != -1) {
        return 13U;
    }

    return 0U;
}

static uint32_t check_alarm_mask(void)
{
    const uint8_t all_fields = XY_RTC_ALARM_FIELD_ALL;

    /* Requiring everything skips nothing; requiring nothing skips everything. */
    if (xy_rtc_alarm_skip_from_match(all_fields, all_fields) != 0x00U) {
        return 1U;
    }
    if (xy_rtc_alarm_skip_from_match(0x00U, all_fields) != all_fields) {
        return 2U;
    }

    /* The inversion is per field. */
    if (xy_rtc_alarm_skip_from_match(XY_RTC_ALARM_FIELD_WEEKDAY, all_fields) !=
        (uint8_t)(all_fields & ~XY_RTC_ALARM_FIELD_WEEKDAY)) {
        return 3U;
    }

    /* Every mask round-trips. */
    {
        uint8_t match;
        for (match = 0x00U; match <= all_fields; ++match) {
            uint8_t skip = xy_rtc_alarm_skip_from_match(match, all_fields);
            if (xy_rtc_alarm_match_from_skip(skip, all_fields) != match) {
                return 4U;
            }
        }
    }

    /* A mask is valid only for the fields the backend has. */
    if (xy_rtc_alarm_mask_valid(all_fields, all_fields) != 1) {
        return 5U;
    }
    if (xy_rtc_alarm_mask_valid(0x1FU, all_fields) != 0) {
        return 6U;
    }

    return 0U;
}

int main(void)
{
    uint32_t step;

    if (xy_hal_sys_init() != XY_HAL_OK || pandora_platform_startup() != 0) {
        fail();
    }
    uart_init();
    uart_text("PANDORA CLIB BOARD READY\r\n");
    uart_text("FIRMWARE_COMMIT " XINYI_FIRMWARE_COMMIT "\r\n");

    step = check_frame_codec();
    if (step != 0U) {
        uart_text("PANDORA_CLIB_FRAME_ERROR\r\n");
        fail();
    }
    uart_text("PANDORA_CLIB_FRAME_OK_A\r\n");

    step = check_civil_calendar();
    if (step != 0U) {
        uart_text("PANDORA_CLIB_CIVIL_ERROR\r\n");
        fail();
    }
    uart_text("PANDORA_CLIB_CIVIL_OK_A\r\n");

    step = check_bcd();
    if (step != 0U) {
        uart_text("PANDORA_CLIB_BCD_ERROR\r\n");
        fail();
    }
    uart_text("PANDORA_CLIB_BCD_OK_A\r\n");

    step = check_alarm_mask();
    if (step != 0U) {
        uart_text("PANDORA_CLIB_MASK_ERROR\r\n");
        fail();
    }
    uart_text("PANDORA_CLIB_MASK_OK_A\r\n");

    uart_text("B1_CLIB_BOARD_RUNTIME_PASS\r\n");

    for (;;) {
        xy_hal_delay_ms(1000U);
    }
}
