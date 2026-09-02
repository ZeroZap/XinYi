#include "stm32l4xx_hal.h"

static UART_HandleTypeDef uart1;

#define SOFT_I2C_SDA_PORT GPIOC
#define SOFT_I2C_SDA_PIN GPIO_PIN_1
#define SOFT_I2C_SCL_PORT GPIOD
#define SOFT_I2C_SCL_PIN GPIO_PIN_6

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the source commit used for this image"
#endif

void _init(void) {}
void _fini(void) {}

void SysTick_Handler(void)
{
    HAL_IncTick();
}

static void fail(void)
{
    __disable_irq();
    for (;;) {
    }
}

static void soft_i2c_delay(void)
{
    for (volatile uint32_t i = 0; i < 80U; ++i) {
        __NOP();
    }
}

static void soft_i2c_sda(GPIO_PinState state)
{
    HAL_GPIO_WritePin(SOFT_I2C_SDA_PORT, SOFT_I2C_SDA_PIN, state);
    soft_i2c_delay();
}

static void soft_i2c_scl(GPIO_PinState state)
{
    HAL_GPIO_WritePin(SOFT_I2C_SCL_PORT, SOFT_I2C_SCL_PIN, state);
    soft_i2c_delay();
}

static void soft_i2c_start(void)
{
    soft_i2c_sda(GPIO_PIN_SET);
    soft_i2c_scl(GPIO_PIN_SET);
    soft_i2c_sda(GPIO_PIN_RESET);
    soft_i2c_scl(GPIO_PIN_RESET);
}

static void soft_i2c_stop(void)
{
    soft_i2c_sda(GPIO_PIN_RESET);
    soft_i2c_scl(GPIO_PIN_SET);
    soft_i2c_sda(GPIO_PIN_SET);
}

static int soft_i2c_write_byte(uint8_t value)
{
    GPIO_InitTypeDef gpio = {0};
    for (uint32_t bit = 0; bit < 8U; ++bit) {
        soft_i2c_sda((value & 0x80U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        soft_i2c_scl(GPIO_PIN_SET);
        soft_i2c_scl(GPIO_PIN_RESET);
        value <<= 1;
    }

    gpio.Pin = SOFT_I2C_SDA_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(SOFT_I2C_SDA_PORT, &gpio);
    soft_i2c_scl(GPIO_PIN_SET);
    int ack = HAL_GPIO_ReadPin(SOFT_I2C_SDA_PORT, SOFT_I2C_SDA_PIN) == GPIO_PIN_RESET;
    soft_i2c_scl(GPIO_PIN_RESET);
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    HAL_GPIO_Init(SOFT_I2C_SDA_PORT, &gpio);
    return ack;
}

static uint8_t soft_i2c_read_byte(int send_ack)
{
    GPIO_InitTypeDef gpio = {0};
    uint8_t value = 0;
    gpio.Pin = SOFT_I2C_SDA_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(SOFT_I2C_SDA_PORT, &gpio);
    for (uint32_t bit = 0; bit < 8U; ++bit) {
        value <<= 1;
        soft_i2c_scl(GPIO_PIN_SET);
        if (HAL_GPIO_ReadPin(SOFT_I2C_SDA_PORT, SOFT_I2C_SDA_PIN) == GPIO_PIN_SET) {
            value |= 1U;
        }
        soft_i2c_scl(GPIO_PIN_RESET);
    }
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    HAL_GPIO_Init(SOFT_I2C_SDA_PORT, &gpio);
    soft_i2c_sda(send_ack ? GPIO_PIN_RESET : GPIO_PIN_SET);
    soft_i2c_scl(GPIO_PIN_SET);
    soft_i2c_scl(GPIO_PIN_RESET);
    soft_i2c_sda(GPIO_PIN_SET);
    return value;
}

static int aht10_write(const uint8_t *data, uint32_t length)
{
    soft_i2c_start();
    if (!soft_i2c_write_byte(0x38U << 1)) {
        soft_i2c_stop();
        return 0;
    }
    for (uint32_t i = 0; i < length; ++i) {
        if (!soft_i2c_write_byte(data[i])) {
            soft_i2c_stop();
            return 0;
        }
    }
    soft_i2c_stop();
    return 1;
}

static int aht10_read(uint8_t *data, uint32_t length)
{
    soft_i2c_start();
    if (!soft_i2c_write_byte((0x38U << 1) | 1U)) {
        soft_i2c_stop();
        return 0;
    }
    for (uint32_t i = 0; i < length; ++i) {
        data[i] = soft_i2c_read_byte(i + 1U < length);
    }
    soft_i2c_stop();
    return 1;
}

static int aht10_probe(void)
{
    soft_i2c_start();
    int ack = soft_i2c_write_byte(0x38U << 1);
    soft_i2c_stop();
    return ack;
}

static int aht10_init(void)
{
    static const uint8_t command[] = {0xE1U, 0x08U, 0x00U};
    HAL_Delay(40U);
    if (!aht10_write(command, sizeof(command))) {
        return 0;
    }
    HAL_Delay(10U);
    return 1;
}

static int aht10_measure(uint32_t *humidity_milli_percent, int32_t *temperature_milli_c)
{
    static const uint8_t command[] = {0xACU, 0x33U, 0x00U};
    uint8_t data[6];
    if (!aht10_write(command, sizeof(command))) {
        return 0;
    }
    HAL_Delay(80U);
    if (!aht10_read(data, sizeof(data)) || (data[0] & 0x80U) != 0U) {
        return 0;
    }
    uint32_t raw_humidity = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) |
                            ((uint32_t)data[3] >> 4);
    uint32_t raw_temperature = ((uint32_t)(data[3] & 0x0FU) << 16) |
                               ((uint32_t)data[4] << 8) | data[5];
    *humidity_milli_percent = (uint32_t)(((uint64_t)raw_humidity * 100000U) >> 20);
    *temperature_milli_c = (int32_t)(((uint64_t)raw_temperature * 200000U) >> 20) - 50000;
    return 1;
}

static void uart_text(const char *text)
{
    uint16_t length = 0;
    while (text[length] != '\0') ++length;
    HAL_UART_Transmit(&uart1, (uint8_t *)text, length, 100U);
}

static void uart_u32(uint32_t value)
{
    uint8_t digits[10];
    uint32_t count = 0;
    do {
        digits[count++] = (uint8_t)('0' + value % 10U);
        value /= 10U;
    } while (value != 0U);
    while (count != 0U) {
        --count;
        HAL_UART_Transmit(&uart1, &digits[count], 1U, 100U);
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

static void gpio_uart_init(void)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_7;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &gpio);

    gpio.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10; /* KEY2/KEY1/KEY0 */
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOD, &gpio);

    gpio.Pin = SOFT_I2C_SDA_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SOFT_I2C_SDA_PORT, &gpio);
    gpio.Pin = SOFT_I2C_SCL_PIN;
    HAL_GPIO_Init(SOFT_I2C_SCL_PORT, &gpio);
    soft_i2c_sda(GPIO_PIN_SET);
    soft_i2c_scl(GPIO_PIN_SET);

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

int main(void)
{
    static const uint8_t banner[] = "PANDORA STM32L475VE XINYI SMOKE OK\r\n";
    static const uint8_t firmware_commit[] = "FIRMWARE_COMMIT " XINYI_FIRMWARE_COMMIT "\r\n";
    static const uint8_t key0[] = "KEY0\r\n";
    static const uint8_t aht_ack[] = "AHT10 0x38 ACK\r\n";
    static const uint8_t aht_nack[] = "AHT10 0x38 NACK\r\n";
    uint32_t humidity_milli_percent = 0;
    int32_t temperature_milli_c = 0;

    HAL_Init();
    clock_init();
    gpio_uart_init();
    int aht_initialized = aht10_init();
    for (;;) {
        HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_7);
        HAL_UART_Transmit(&uart1, (uint8_t *)banner, sizeof(banner) - 1U, 100U);
        HAL_UART_Transmit(&uart1, (uint8_t *)firmware_commit, sizeof(firmware_commit) - 1U, 100U);
        if (aht_initialized && aht10_measure(&humidity_milli_percent, &temperature_milli_c)) {
            HAL_UART_Transmit(&uart1, (uint8_t *)aht_ack, sizeof(aht_ack) - 1U, 100U);
            uart_text("AHT10 RH_milli_percent=");
            uart_u32(humidity_milli_percent);
            uart_text(" T_milli_c=");
            uart_i32(temperature_milli_c);
            uart_text("\r\n");
        } else {
            HAL_UART_Transmit(&uart1, (uint8_t *)aht_nack, sizeof(aht_nack) - 1U, 100U);
            aht_initialized = aht10_probe() && aht10_init();
        }
        if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_10) == GPIO_PIN_RESET) {
            HAL_UART_Transmit(&uart1, (uint8_t *)key0, sizeof(key0) - 1U, 100U);
        }
        HAL_Delay(500U);
    }
}
