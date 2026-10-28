/**
 * @file sensor_ap3216c_main.c
 * @brief Pandora AP3216C validation image on the canonical board-owned driver.
 *
 * The board owns one `xy_ap3216c` instance through `pandora_sensor_owners`; the
 * legacy `sensor_*` compatibility wrapper is no longer linked into this image.
 */

#include "pandora_ap3216c_owner.h"
#include "pandora_hw_i2c.h"
#include "pandora_platform_startup.h"
#include "stm32l4xx_hal.h"
#include "xy_device.h"
#include "xy_hal_delay.h"
#include "xy_hal_i2c.h"
#include "xy_hal_sys.h"
#include "xy_hal_uart.h"

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the source commit used for this image"
#endif

/* AP3216C registers the validation image still addresses directly. */
#define AP_REG_INT_STATUS 0x01U
#define AP_REG_ALS_CONF 0x10U
#define AP_REG_PS_CONF 0x20U
#define AP_REG_PS_LED 0x21U

static UART_HandleTypeDef uart1;
static pandora_ap3216c_owner_t ap_owner;

void _init(void) {}
void _fini(void) {}
void SysTick_Handler(void) { xy_hal_sys_tick_irq_handler(); }
void delay_ms(uint32_t ms) { xy_hal_delay_ms(ms); }
uint32_t get_tick_ms(void) { return xy_hal_sys_get_tick_count(); }

static void fail(void)
{
    __disable_irq();
    for (;;) {}
}

static void uart_text(const char *text)
{
    uint16_t length = 0U;
    while (text[length] != '\0') { ++length; }
    (void)xy_hal_uart_send(&uart1, (const uint8_t *)text, length, 100U);
}

static void uart_u32(uint32_t value)
{
    uint8_t digits[10];
    uint32_t count = 0U;
    do { digits[count++] = (uint8_t)('0' + value % 10U); value /= 10U; } while (value != 0U);
    while (count != 0U) {
        --count;
        (void)xy_hal_uart_send(&uart1, &digits[count], 1U, 100U);
    }
}

static void uart_hex8(uint8_t value)
{
    static const char hex[] = "0123456789ABCDEF";
    char encoded[2] = {hex[value >> 4], hex[value & 0x0FU]};
    (void)xy_hal_uart_send(&uart1, (const uint8_t *)encoded, sizeof(encoded), 100U);
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

int main(void)
{
    uint8_t config = 0U;
    uint8_t status = 0U;
    uint8_t als_config = 0U;
    uint8_t ps_config = 0U;
    uint8_t ps_led = 0U;
    uint8_t raw[PANDORA_AP3216C_RAW_LEN];
    pandora_ap3216c_sample_t sample;
    void *i2c3;

    if (xy_hal_sys_init() != XY_HAL_OK) { fail(); }
    if (pandora_platform_startup() != 0) { fail(); }
    uart_init();
    uart_text("PANDORA AP3216C SENSOR READY\r\nFIRMWARE_COMMIT " XINYI_FIRMWARE_COMMIT
              "\r\nAP3216C_BUS=HW_I2C3 SCL=PC0 SDA=PC1 ADDR=0x1E\r\n");
    i2c3 = pandora_hw_i2c3_init();
    if (i2c3 == NULL) {
        uart_text("PANDORA_AP3216C_HW_I2C_INIT_ERROR\r\n"); fail();
    }
    uart_text("AP3216C_HW_I2C_READY\r\n");
    {
        uint8_t probe = 0U;
        if (xy_hal_i2c_master_receive(i2c3, 0x7FU, &probe, 1U, 20U) != XY_HAL_ERROR_IO) {
            uart_text("PANDORA_AP3216C_NACK_EXPECTED_ERROR\r\n"); fail();
        }
        uart_text("AP3216C_NACK_OBSERVED ADDR=0x7F\r\n");
    }
    if (pandora_ap3216c_owner_init(&ap_owner, i2c3) != XY_DEVICE_OK) {
        uart_text("PANDORA_AP3216C_INIT_ERROR\r\n"); fail();
    }
    if (pandora_ap3216c_owner_read_reg(&ap_owner, XY_AP3216C_REG_SYSTEM_CONFIG, &config, 1U) !=
            XY_DEVICE_OK ||
        config != XY_AP3216C_MODE_ALS_PS) {
        uart_text("PANDORA_AP3216C_CONFIG_ERROR\r\n"); fail();
    }
    uart_text("AP3216C_NACK_RECOVERED ADDR=0x1E\r\n");
    uart_text("AP3216C_CONFIG=0x"); uart_hex8(config); uart_text(" MODE=ALS_PS\r\n");
    if (pandora_ap3216c_owner_read_reg(&ap_owner, AP_REG_INT_STATUS, &status, 1U) != XY_DEVICE_OK ||
        pandora_ap3216c_owner_read_reg(&ap_owner, AP_REG_ALS_CONF, &als_config, 1U) !=
            XY_DEVICE_OK ||
        pandora_ap3216c_owner_read_reg(&ap_owner, AP_REG_PS_CONF, &ps_config, 1U) != XY_DEVICE_OK ||
        pandora_ap3216c_owner_read_reg(&ap_owner, AP_REG_PS_LED, &ps_led, 1U) != XY_DEVICE_OK) {
        uart_text("PANDORA_AP3216C_DIAG_IO_ERROR\r\n"); fail();
    }
    uart_text("AP3216C_DIAG INT=0x"); uart_hex8(status);
    uart_text(" ALS_CONF=0x"); uart_hex8(als_config);
    uart_text(" PS_CONF=0x"); uart_hex8(ps_config);
    uart_text(" PS_LED=0x"); uart_hex8(ps_led); uart_text("\r\n");

    for (;;) {
        uint32_t lux;

        if (pandora_ap3216c_owner_read_reg(&ap_owner, AP_REG_INT_STATUS, &status, 1U) !=
            XY_DEVICE_OK) {
            uart_text("PANDORA_AP3216C_STATUS_IO_ERROR\r\n"); fail();
        }
        if (pandora_ap3216c_owner_read_raw(&ap_owner, raw) != XY_DEVICE_OK) {
            uart_text("PANDORA_AP3216C_RAW_IO_ERROR\r\n"); fail();
        }
        if (pandora_ap3216c_owner_sample(&ap_owner, &sample) != XY_DEVICE_OK) {
            uart_text("PANDORA_AP3216C_SAMPLE_ERROR\r\n"); fail();
        }
        lux = sample.illuminance_millilux / 1000U;
        if (lux > 22937U || sample.proximity_raw > 1023U || sample.infrared_raw > 1023U) {
            uart_text("PANDORA_AP3216C_RANGE_ERROR\r\n"); fail();
        }
        uart_text("AP3216C_INT=0x"); uart_hex8(status); uart_text(" RAW_HEX=");
        for (uint32_t i = 0U; i < sizeof(raw); ++i) { uart_hex8(raw[i]); }
        uart_text(" ALS_lux="); uart_u32(lux);
        uart_text(" PS_raw="); uart_u32((uint32_t)sample.proximity_raw);
        uart_text(" IR_raw="); uart_u32((uint32_t)sample.infrared_raw); uart_text("\r\n");
        xy_hal_delay_ms(100U);
    }
}
