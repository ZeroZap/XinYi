/**
 * @file fota_slot_seed_main.c
 * @brief Leave the RTC confirm slot in a chosen state, then stop.
 *
 * Why this exists
 * ---------------
 * The bootloader's confirm branch is driven by a request that the *application*
 * writes, and an application only runs after its own install. So the state that
 * matters - "a request is in the slot at the moment a candidate gets installed" -
 * cannot be produced by any build in this repository: the only writer of that slot
 * is firmware, and every firmware that can write it also has to have been installed
 * first.
 *
 * This image exists to leave exactly that state behind. It writes the slot, reads it
 * back, prints what it read, and then spins. It deliberately does not reset: the value
 * has to survive into the next boot, and the capture has to be able to show that the
 * write really landed before the bootloader runs.
 *
 * Two targets are built from this one file, because the same image does both halves of
 * the setup and a second copy would only be a place for the two to disagree:
 *
 *   pandora_stm32l475_fota_slot_seed   PANDORA_SLOT_CONFIRM_VALUE = REQUEST magic
 *   pandora_stm32l475_fota_slot_clear  PANDORA_SLOT_CONFIRM_VALUE = 0
 *
 * The restage slot is always cleared with it, so a review-gated bootloader cannot be
 * held at the restage gate by a value left over from an earlier run.
 */

#include "pandora_fota_boot_flow.h"
#include "pandora_platform_startup.h"
#include "stm32l4xx_hal.h"
#include "xy_hal_rtc.h"
#include "xy_hal_sys.h"
#include "xy_hal_uart.h"

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the source commit used for this image"
#endif

#ifndef PANDORA_SLOT_CONFIRM_VALUE
#error "PANDORA_SLOT_CONFIRM_VALUE must say what the confirm slot is set to"
#endif

static UART_HandleTypeDef uart1;

void _init(void) {}
void _fini(void) {}

void SysTick_Handler(void) {
    xy_hal_sys_tick_irq_handler();
}

static void stop(void) {
    __disable_irq();
    for (;;) {
    }
}

static void uart_text(const char* text) {
    uint16_t length = 0U;

    while (text[length] != '\0') {
        ++length;
    }
    (void)xy_hal_uart_send(&uart1, (const uint8_t*)text, length, 100U);
}

static void uart_hex32(uint32_t value) {
    static const char digits[] = "0123456789ABCDEF";
    char encoded[8];
    uint32_t index;

    for (index = 0U; index < 8U; ++index) {
        encoded[index] = digits[(value >> ((7U - index) * 4U)) & 0x0FU];
    }
    (void)xy_hal_uart_send(&uart1, (const uint8_t*)encoded, sizeof(encoded), 100U);
}

static void uart_slot(const char* name, uint8_t slot) {
    uint32_t value = 0U;

    if (xy_hal_rtc_backup_read(slot, &value) != XY_HAL_OK) {
        uart_text("SEED_READ_ERROR ");
        uart_text(name);
        uart_text("\r\n");
        stop();
    }
    uart_text(name);
    uart_text("=0x");
    uart_hex32(value);
    uart_text("\r\n");
}

static void uart_init(void) {
    const xy_hal_gpio_config_t gpio = {XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_UP,
                                       XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_VERY_HIGH,
                                       GPIO_AF7_USART1};
    const xy_hal_uart_config_t uart = {115200U,
                                       XY_HAL_UART_WORDLEN_8B,
                                       XY_HAL_UART_STOPBITS_1,
                                       XY_HAL_UART_PARITY_NONE,
                                       XY_HAL_UART_FLOWCTRL_NONE,
                                       XY_HAL_UART_MODE_TX_RX};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();
    if (xy_hal_gpio_init(GPIOA, 9U, &gpio) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &gpio) != XY_HAL_OK) {
        stop();
    }
    uart1.Instance = USART1;
    if (xy_hal_uart_init(&uart1, &uart) != XY_HAL_OK) {
        stop();
    }
}

int main(void) {
    if (xy_hal_sys_init() != XY_HAL_OK) {
        stop();
    }
    if (pandora_platform_startup() != 0) {
        stop();
    }
    uart_init();
    uart_text("PANDORA FOTA SLOT SEED READY\r\nSEED_COMMIT " XINYI_FIRMWARE_COMMIT "\r\n");
    uart_text("SEED_INTENT=0x");
    uart_hex32(PANDORA_SLOT_CONFIRM_VALUE);
    uart_text("\r\n");

    /* Read before writing: the point of the record is the state the next boot inherits. */
    uart_slot("SEED_SLOT1_BEFORE", PANDORA_FOTA_RTC_SLOT_CONFIRM);
    if (xy_hal_rtc_backup_write(PANDORA_FOTA_RTC_SLOT_CONFIRM, PANDORA_SLOT_CONFIRM_VALUE) !=
        XY_HAL_OK) {
        uart_text("SEED_WRITE_ERROR slot1\r\n");
        stop();
    }
    if (xy_hal_rtc_backup_write(PANDORA_FOTA_RTC_SLOT_RESTAGE, 0U) != XY_HAL_OK) {
        uart_text("SEED_WRITE_ERROR slot2\r\n");
        stop();
    }
    uart_slot("SEED_SLOT1", PANDORA_FOTA_RTC_SLOT_CONFIRM);
    uart_slot("SEED_SLOT2", PANDORA_FOTA_RTC_SLOT_RESTAGE);
    uart_text("SEED_DONE\r\n");

    /* Spin, never reset: the slot has to survive into the next boot on its own. */
    for (;;) {
    }
}
