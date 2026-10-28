/**
 * @file sensor_icm20608_main.c
 * @brief Pandora ICM20608 validation image on the canonical board-owned driver.
 *
 * The board owns one `xy_icm20608` instance through `pandora_sensor_owners`; the
 * legacy `sensor_*` compatibility wrapper is no longer linked into this image.
 */

#include "pandora_icm20608_owner.h"
#include "pandora_platform_startup.h"
#include "pandora_soft_i2c.h"
#include "stm32l4xx_hal.h"
#include "xy_device.h"
#include "xy_hal_delay.h"
#include "xy_hal_sys.h"
#include "xy_hal_uart.h"

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the source commit used for this image"
#endif

static UART_HandleTypeDef uart1;
static pandora_icm20608_owner_t icm_owner;

void _init(void) {}
void _fini(void) {}
void SysTick_Handler(void) { xy_hal_sys_tick_irq_handler(); }
void delay_ms(uint32_t ms) { xy_hal_delay_ms(ms); }
uint32_t get_tick_ms(void) { return xy_hal_sys_get_tick_count(); }

static void fail(void)
{
    __disable_irq();
    for (;;)
    {
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
        (void)xy_hal_uart_send(&uart1, &digits[count], 1U, 100U);
    }
}

static void uart_i32(int32_t value)
{
    if (value < 0) {
        uart_text("-");
        uart_u32((uint32_t)(-(int64_t)value));
    } else {
        uart_u32((uint32_t)value);
    }
}

static void uart_hex8(uint8_t value)
{
    static const char hex[] = "0123456789ABCDEF";
    char encoded[2] = {hex[value >> 4], hex[value & 0x0FU]};
    (void)xy_hal_uart_send(&uart1, (const uint8_t *)encoded, sizeof(encoded), 100U);
}

static int read_reg(uint8_t reg, uint8_t *data, uint16_t len)
{
    return pandora_icm20608_owner_read_reg(&icm_owner, reg, data, len) == XY_DEVICE_OK;
}

static void log_register_snapshot(void)
{
    static const uint8_t registers[] = {
        XY_ICM20608_REG_PWR_MGMT_1, XY_ICM20608_REG_PWR_MGMT_2,
        XY_ICM20608_REG_ACCEL_CONFIG, XY_ICM20608_REG_GYRO_CONFIG,
        XY_ICM20608_REG_CONFIG, XY_ICM20608_REG_ACCEL_CONFIG2,
        XY_ICM20608_REG_INT_STATUS,
    };
    uint8_t value;

    uart_text("ICM20608_REGS_HEX=");
    for (size_t i = 0; i < sizeof(registers); ++i) {
        if (!read_reg(registers[i], &value, 1U)) {
            uart_text("IO_ERROR\r\n");
            fail();
        }
        if (i != 0U) {
            uart_text(",");
        }
        uart_hex8(value);
    }
    uart_text("\r\n");
}

static void log_raw_burst(void)
{
    uint8_t raw[PANDORA_ICM20608_RAW_LEN];

    if (pandora_icm20608_owner_read_raw(&icm_owner, raw) != XY_DEVICE_OK) {
        uart_text("PANDORA_ICM20608_BURST_IO_ERROR\r\n");
        fail();
    }
    uart_text("ICM20608_RAW14_HEX=");
    for (size_t i = 0; i < sizeof(raw); ++i) {
        uart_hex8(raw[i]);
    }
    uart_text("\r\n");
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
    xy_icm20608_sample_t sample;
    uint8_t whoami = 0U;
    void *i2c3;

    if (xy_hal_sys_init() != XY_HAL_OK) {
        fail();
    }
    if (pandora_platform_startup() != 0) {
        fail();
    }
    uart_init();
    uart_text("PANDORA ICM20608 SENSOR READY\r\nFIRMWARE_COMMIT " XINYI_FIRMWARE_COMMIT
              "\r\nI2C3_PINS SCL=PC0 SDA=PC1\r\n");
    i2c3 = pandora_soft_i2c3_init();
    uart_text("PANDORA_I2C3_INIT_DONE\r\n");
    {
        uint32_t ack_count = 0U;
        for (uint32_t address = 0x08U; address <= 0x77U; ++address) {
            if (pandora_soft_i2c_probe(i2c3, (uint8_t)address)) {
                uart_text("I2C3_ACK=0x");
                uart_hex8((uint8_t)address);
                uart_text("\r\n");
                ++ack_count;
            }
        }
        uart_text("PANDORA_I2C3_SCAN_DONE count=");
        uart_u32(ack_count);
        uart_text("\r\n");
    }
    {
        /* Identity must be observed on the bus before the driver is trusted. */
        xy_i2c_device_t probe_bus;

        if (xy_i2c_device_init(&probe_bus, i2c3, XY_ICM20608_ADDR_DEFAULT, 100U) !=
                XY_DEVICE_OK ||
            xy_i2c_device_read_reg(&probe_bus, XY_ICM20608_REG_WHO_AM_I, &whoami, 1U) !=
                XY_DEVICE_OK) {
            uart_text("PANDORA_ICM20608_WHOAMI_IO_ERROR\r\n");
            fail();
        }
        uart_text("ICM20608_WHO_AM_I_RAW=");
        uart_u32(whoami);
        uart_text("\r\n");
        if (whoami != XY_ICM20608_WHO_AM_I) {
            uart_text("PANDORA_ICM20608_WHOAMI_MISMATCH\r\n");
            fail();
        }
    }
    {
        xy_error_t init_result = pandora_icm20608_owner_init(&icm_owner, i2c3);

        if (init_result != XY_DEVICE_OK) {
            uart_text("PANDORA_ICM20608_DRIVER_INIT_ERROR result=");
            uart_i32(init_result);
            uart_text("\r\n");
            fail();
        }
    }
    if (!read_reg(XY_ICM20608_REG_WHO_AM_I, &whoami, 1U) || whoami != XY_ICM20608_WHO_AM_I) {
        uart_text("PANDORA_ICM20608_INIT_ERROR\r\n");
        fail();
    }
    uart_text("ICM20608_ADDR=0x68 WHO_AM_I=0xAE\r\n");
    uart_text("ICM20608_INIT_PATH=ACCEL_ONLY RESET_COUNT=1\r\n");
    log_register_snapshot();

    for (;;) {
        log_raw_burst();
        log_register_snapshot();
        if (pandora_icm20608_owner_sample(&icm_owner, &sample) != XY_DEVICE_OK) {
            uart_text("PANDORA_ICM20608_READ_ERROR\r\n");
            fail();
        }
        if (sample.accel.x_mg < -4000 || sample.accel.x_mg > 4000 ||
            sample.accel.y_mg < -4000 || sample.accel.y_mg > 4000 ||
            sample.accel.z_mg < -4000 || sample.accel.z_mg > 4000 ||
            sample.gyro.x_mdps < -500000 || sample.gyro.x_mdps > 500000 ||
            sample.gyro.y_mdps < -500000 || sample.gyro.y_mdps > 500000 ||
            sample.gyro.z_mdps < -500000 || sample.gyro.z_mdps > 500000) {
            uart_text("PANDORA_ICM20608_RANGE_ERROR\r\n");
            fail();
        }
        uart_text("ICM20608 ACCEL_mg=");
        uart_i32(sample.accel.x_mg);
        uart_text(",");
        uart_i32(sample.accel.y_mg);
        uart_text(",");
        uart_i32(sample.accel.z_mg);
        uart_text(" GYRO_dps=");
        uart_i32(sample.gyro.x_mdps / 1000);
        uart_text(",");
        uart_i32(sample.gyro.y_mdps / 1000);
        uart_text(",");
        uart_i32(sample.gyro.z_mdps / 1000);
        uart_text("\r\n");
        xy_hal_delay_ms(100U);
    }
}
