#include "pandora_soft_i2c.h"
#include "sensor_icm20608.h"
#include "stm32l4xx_hal.h"
#include "xy_device.h"

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
    (void)HAL_UART_Transmit(&uart1, (uint8_t *)text, length, 100U);
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
        (void)HAL_UART_Transmit(&uart1, &digits[count], 1U, 100U);
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
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &gpio);
    uart1.Instance = USART1;
    uart1.Init.BaudRate = 115200;
    uart1.Init.WordLength = UART_WORDLENGTH_8B;
    uart1.Init.StopBits = UART_STOPBITS_1;
    uart1.Init.Parity = UART_PARITY_NONE;
    uart1.Init.Mode = UART_MODE_TX_RX;
    uart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uart1.Init.OverSampling = UART_OVERSAMPLING_16;
    uart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&uart1) != HAL_OK) {
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
    uart_text("ICM20608_ADDR=0x68 WHO_AM_I=0xAF\r\n");

    for (;;) {
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
