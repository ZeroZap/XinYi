#include "pandora_hw_i2c.h"
#include "sensor_ap3216c.h"
#include "stm32l4xx_hal.h"
#include "xy_device.h"

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the source commit used for this image"
#endif

static UART_HandleTypeDef uart1;
static xy_i2c_device_t ap_bus;

void _init(void) {}
void _fini(void) {}
void SysTick_Handler(void) { HAL_IncTick(); }
void delay_ms(uint32_t ms) { HAL_Delay(ms); }
uint32_t get_tick_ms(void) { return HAL_GetTick(); }
uint32_t xy_os_tick_get(void) { return HAL_GetTick(); }

static void fail(void)
{
    __disable_irq();
    for (;;) {}
}

static void uart_text(const char *text)
{
    uint16_t length = 0U;
    while (text[length] != '\0') { ++length; }
    (void)HAL_UART_Transmit(&uart1, (uint8_t *)text, length, 100U);
}

static void uart_u32(uint32_t value)
{
    uint8_t digits[10];
    uint32_t count = 0U;
    do { digits[count++] = (uint8_t)('0' + value % 10U); value /= 10U; } while (value != 0U);
    while (count != 0U) { --count; (void)HAL_UART_Transmit(&uart1, &digits[count], 1U, 100U); }
}

static void uart_hex8(uint8_t value)
{
    static const char hex[] = "0123456789ABCDEF";
    char encoded[2] = {hex[value >> 4], hex[value & 0x0FU]};
    (void)HAL_UART_Transmit(&uart1, (uint8_t *)encoded, sizeof(encoded), 100U);
}

static void clock_init(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    __HAL_RCC_PWR_CLK_ENABLE();
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) { fail(); }
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM = 1; osc.PLL.PLLN = 20; osc.PLL.PLLP = RCC_PLLP_DIV7;
    osc.PLL.PLLQ = RCC_PLLQ_DIV2; osc.PLL.PLLR = RCC_PLLR_DIV2;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) { fail(); }
    clk.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 |
                    RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1; clk.APB1CLKDivider = RCC_HCLK_DIV1;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_4) != HAL_OK) { fail(); }
}

static void uart_init(void)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE(); __HAL_RCC_USART1_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_9 | GPIO_PIN_10; gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_PULLUP; gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF7_USART1; HAL_GPIO_Init(GPIOA, &gpio);
    uart1.Instance = USART1; uart1.Init.BaudRate = 115200;
    uart1.Init.WordLength = UART_WORDLENGTH_8B; uart1.Init.StopBits = UART_STOPBITS_1;
    uart1.Init.Parity = UART_PARITY_NONE; uart1.Init.Mode = UART_MODE_TX_RX;
    uart1.Init.HwFlowCtl = UART_HWCONTROL_NONE; uart1.Init.OverSampling = UART_OVERSAMPLING_16;
    uart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&uart1) != HAL_OK) { fail(); }
}

int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    xy_i2c_device_t *dev = bus;
    return dev != NULL && dev->dev_addr == addr &&
                   xy_i2c_device_read_reg(dev, reg, data, len) == XY_DEVICE_OK ? 0 : -1;
}

int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    xy_i2c_device_t *dev = bus;
    return dev != NULL && dev->dev_addr == addr &&
                   xy_i2c_device_write_reg(dev, reg, data, len) == XY_DEVICE_OK ? 0 : -1;
}

int main(void)
{
    sensor_device_t *light;
    sensor_device_t *proximity;
    sensor_device_t *ir;
    sensor_data_t als_data, ps_data, ir_data;
    uint8_t config = 0U;
    uint8_t status = 0U;
    uint8_t als_config = 0U;
    uint8_t ps_config = 0U;
    uint8_t ps_led = 0U;
    uint8_t raw[6];
    void *i2c3;

    HAL_Init(); clock_init(); uart_init();
    uart_text("PANDORA AP3216C SENSOR READY\r\nFIRMWARE_COMMIT " XINYI_FIRMWARE_COMMIT
              "\r\nAP3216C_BUS=HW_I2C3 SCL=PC0 SDA=PC1 ADDR=0x1E\r\n");
    i2c3 = pandora_hw_i2c3_init();
    if (i2c3 == NULL) {
        uart_text("PANDORA_AP3216C_HW_I2C_INIT_ERROR\r\n"); fail();
    }
    uart_text("AP3216C_HW_I2C_READY\r\n");
    if (xy_i2c_device_init(&ap_bus, i2c3, AP3216C_ADDR_DEFAULT, 100U) != XY_DEVICE_OK) {
        uart_text("PANDORA_AP3216C_DEVICE_ERROR\r\n"); fail();
    }
    light = ap3216c_create_light("pandora-ap-als", &ap_bus);
    proximity = ap3216c_create_proximity("pandora-ap-ps", &ap_bus);
    ir = ap3216c_create_ir("pandora-ap-ir", &ap_bus);
    if (light == NULL || proximity == NULL || ir == NULL || light->ops->init(light) != SENSOR_EOK) {
        uart_text("PANDORA_AP3216C_INIT_ERROR\r\n"); fail();
    }
    if (xy_i2c_device_read_reg(&ap_bus, AP3216C_REG_SYS_CONFIG, &config, 1U) != XY_DEVICE_OK ||
        config != AP3216C_MODE_ALS_PS) {
        uart_text("PANDORA_AP3216C_CONFIG_ERROR\r\n"); fail();
    }
    uart_text("AP3216C_CONFIG=0x"); uart_hex8(config); uart_text(" MODE=ALS_PS\r\n");
    if (xy_i2c_device_read_reg(&ap_bus, AP3216C_REG_INT_STATUS, &status, 1U) != XY_DEVICE_OK ||
        xy_i2c_device_read_reg(&ap_bus, 0x10U, &als_config, 1U) != XY_DEVICE_OK ||
        xy_i2c_device_read_reg(&ap_bus, 0x20U, &ps_config, 1U) != XY_DEVICE_OK ||
        xy_i2c_device_read_reg(&ap_bus, 0x21U, &ps_led, 1U) != XY_DEVICE_OK) {
        uart_text("PANDORA_AP3216C_DIAG_IO_ERROR\r\n"); fail();
    }
    uart_text("AP3216C_DIAG INT=0x"); uart_hex8(status);
    uart_text(" ALS_CONF=0x"); uart_hex8(als_config);
    uart_text(" PS_CONF=0x"); uart_hex8(ps_config);
    uart_text(" PS_LED=0x"); uart_hex8(ps_led); uart_text("\r\n");

    for (;;) {
        if (xy_i2c_device_read_reg(&ap_bus, AP3216C_REG_INT_STATUS, &status, 1U) !=
            XY_DEVICE_OK) {
            uart_text("PANDORA_AP3216C_STATUS_IO_ERROR\r\n"); fail();
        }
        for (uint32_t i = 0U; i < sizeof(raw); ++i) {
            if (xy_i2c_device_read_reg(&ap_bus, (uint8_t)(AP3216C_REG_IR_DATA_L + i),
                                       &raw[i], 1U) != XY_DEVICE_OK) {
                uart_text("PANDORA_AP3216C_RAW_IO_ERROR\r\n"); fail();
            }
        }
        if (light->ops->read(light, &als_data) != SENSOR_EOK ||
            proximity->ops->read(proximity, &ps_data) != SENSOR_EOK ||
            ir->ops->read(ir, &ir_data) != SENSOR_EOK) {
            uart_text("PANDORA_AP3216C_SAMPLE_ERROR\r\n"); fail();
        }
        if (als_data.value.val_uint32 > 22937U || ps_data.value.val_int32 < 0 ||
            ps_data.value.val_int32 > 1023 || ir_data.value.val_uint32 > 1023U) {
            uart_text("PANDORA_AP3216C_RANGE_ERROR\r\n"); fail();
        }
        uart_text("AP3216C_INT=0x"); uart_hex8(status); uart_text(" RAW_HEX=");
        for (uint32_t i = 0U; i < sizeof(raw); ++i) { uart_hex8(raw[i]); }
        uart_text(" ALS_lux="); uart_u32(als_data.value.val_uint32);
        uart_text(" PS_raw="); uart_u32((uint32_t)ps_data.value.val_int32);
        uart_text(" IR_raw="); uart_u32(ir_data.value.val_uint32); uart_text("\r\n");
        HAL_Delay(100U);
    }
}
