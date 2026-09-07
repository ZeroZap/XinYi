#include "pandora_soft_i2c.h"
#include "sensor_icm20608.h"
#include "stm32l4xx_hal.h"
#include "xy_device.h"
#include "xy_hal_uart.h"

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the source commit used for this image"
#endif

static UART_HandleTypeDef uart1;
static xy_i2c_device_t icm_bus;

void _init(void) {}
void _fini(void) {}
void SysTick_Handler(void) { HAL_IncTick(); }
void delay_ms(uint32_t ms) { HAL_Delay(ms); }
uint32_t get_tick_ms(void) { return HAL_GetTick(); }
uint32_t xy_os_tick_get(void) { return HAL_GetTick(); }

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

static int read_reg(uint8_t reg, uint8_t *data, size_t len)
{
    return xy_i2c_device_read_reg(&icm_bus, reg, data, len) == XY_DEVICE_OK;
}

static void log_register_snapshot(void)
{
    static const uint8_t registers[] = {
        ICM20608_REG_PWR_MGMT_1, ICM20608_REG_PWR_MGMT_2, ICM20608_REG_ACCEL_CONFIG,
        ICM20608_REG_GYRO_CONFIG, ICM20608_REG_CONFIG, ICM20608_REG_ACCEL_CONFIG2,
        ICM20608_REG_INT_STATUS,
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
    uint8_t raw[14];

    if (!read_reg(ICM20608_REG_ACCEL_XOUT_H, raw, sizeof(raw))) {
        uart_text("PANDORA_ICM20608_BURST_IO_ERROR\r\n");
        fail();
    }
    uart_text("ICM20608_RAW14_HEX=");
    for (size_t i = 0; i < sizeof(raw); ++i) {
        uart_hex8(raw[i]);
    }
    uart_text("\r\n");
}

static void clock_init(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
        fail();
    }
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM = 1;
    osc.PLL.PLLN = 20;
    osc.PLL.PLLP = RCC_PLLP_DIV7;
    osc.PLL.PLLQ = RCC_PLLQ_DIV2;
    osc.PLL.PLLR = RCC_PLLR_DIV2;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
        fail();
    }
    clk.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                    RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_4) != HAL_OK) {
        fail();
    }
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

int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    xy_i2c_device_t *dev = bus;
    return dev != NULL && dev->dev_addr == addr &&
                   xy_i2c_device_read_reg(dev, reg, data, len) == XY_DEVICE_OK
               ? 0
               : -1;
}

int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    xy_i2c_device_t *dev = bus;
    return dev != NULL && dev->dev_addr == addr &&
                   xy_i2c_device_write_reg(dev, reg, data, len) == XY_DEVICE_OK
               ? 0
               : -1;
}

int hal_spi_read_reg(void *bus, uint8_t reg, uint8_t *data, uint16_t len)
{
    (void)bus;
    (void)reg;
    (void)data;
    (void)len;
    return -1;
}

int hal_spi_write_reg(void *bus, uint8_t reg, uint8_t *data, uint16_t len)
{
    (void)bus;
    (void)reg;
    (void)data;
    (void)len;
    return -1;
}

int main(void)
{
    sensor_device_t *accel;
    sensor_device_t *gyro;
    sensor_data_t accel_data;
    sensor_data_t gyro_data;
    uint8_t whoami = 0U;
    void *i2c3;

    HAL_Init();
    clock_init();
    uart_init();
    uart_text("PANDORA ICM20608 SENSOR READY\r\nFIRMWARE_COMMIT " XINYI_FIRMWARE_COMMIT
              "\r\nI2C3_PINS SCL=PC0 SDA=PC1\r\n");
    i2c3 = pandora_soft_i2c3_init();
    uart_text("PANDORA_I2C3_INIT_DONE\r\n");
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
    if (xy_i2c_device_init(&icm_bus, i2c3, ICM20608_ADDR_DEFAULT, 100U) != XY_DEVICE_OK) {
        uart_text("PANDORA_ICM20608_INIT_ERROR\r\n");
        fail();
    }
    if (xy_i2c_device_read_reg(&icm_bus, ICM20608_REG_WHOAMI, &whoami, 1U) != XY_DEVICE_OK) {
        uart_text("PANDORA_ICM20608_WHOAMI_IO_ERROR\r\n");
        fail();
    }
    uart_text("ICM20608_WHO_AM_I_RAW=");
    uart_u32(whoami);
    uart_text("\r\n");
    if (whoami != ICM20608_WHOAMI_VALUE) {
        uart_text("PANDORA_ICM20608_WHOAMI_MISMATCH\r\n");
        fail();
    }
    accel = icm20608_create_accel("pandora-icm-accel", &icm_bus, false);
    gyro = icm20608_create_gyro("pandora-icm-gyro", &icm_bus, false);
    if (accel == NULL || gyro == NULL) {
        uart_text("PANDORA_ICM20608_CREATE_ERROR\r\n");
        fail();
    }
    sensor_err_t init_result = accel->ops->init(accel);
    if (init_result != SENSOR_EOK) {
        uart_text("PANDORA_ICM20608_DRIVER_INIT_ERROR result=");
        uart_i32(init_result);
        uart_text("\r\n");
        fail();
    }
    if (
        xy_i2c_device_read_reg(&icm_bus, ICM20608_REG_WHOAMI, &whoami, 1U) != XY_DEVICE_OK ||
        whoami != ICM20608_WHOAMI_VALUE) {
        uart_text("PANDORA_ICM20608_INIT_ERROR\r\n");
        fail();
    }
    uart_text("ICM20608_ADDR=0x68 WHO_AM_I=0xAE\r\n");
    uart_text("ICM20608_INIT_PATH=ACCEL_ONLY RESET_COUNT=1\r\n");
    log_register_snapshot();

    for (;;) {
        log_raw_burst();
        log_register_snapshot();
        if (accel->ops->read(accel, &accel_data) != SENSOR_EOK ||
            gyro->ops->read(gyro, &gyro_data) != SENSOR_EOK) {
            uart_text("PANDORA_ICM20608_READ_ERROR\r\n");
            fail();
        }
        if (accel_data.value.val_3axis.x < -4000 || accel_data.value.val_3axis.x > 4000 ||
            accel_data.value.val_3axis.y < -4000 || accel_data.value.val_3axis.y > 4000 ||
            accel_data.value.val_3axis.z < -4000 || accel_data.value.val_3axis.z > 4000 ||
            gyro_data.value.val_3axis.x < -500 || gyro_data.value.val_3axis.x > 500 ||
            gyro_data.value.val_3axis.y < -500 || gyro_data.value.val_3axis.y > 500 ||
            gyro_data.value.val_3axis.z < -500 || gyro_data.value.val_3axis.z > 500) {
            uart_text("PANDORA_ICM20608_RANGE_ERROR\r\n");
            fail();
        }
        uart_text("ICM20608 ACCEL_mg=");
        uart_i32(accel_data.value.val_3axis.x);
        uart_text(",");
        uart_i32(accel_data.value.val_3axis.y);
        uart_text(",");
        uart_i32(accel_data.value.val_3axis.z);
        uart_text(" GYRO_dps=");
        uart_i32(gyro_data.value.val_3axis.x);
        uart_text(",");
        uart_i32(gyro_data.value.val_3axis.y);
        uart_text(",");
        uart_i32(gyro_data.value.val_3axis.z);
        uart_text("\r\n");
        HAL_Delay(100U);
    }
}
