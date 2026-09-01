#include "stm32l4xx_hal.h"

static UART_HandleTypeDef uart1;

#define SOFT_I2C_SDA_PORT GPIOC
#define SOFT_I2C_SDA_PIN GPIO_PIN_1
#define SOFT_I2C_SCL_PORT GPIOD
#define SOFT_I2C_SCL_PIN GPIO_PIN_6

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

static int aht10_probe(void)
{
    soft_i2c_start();
    int ack = soft_i2c_write_byte(0x38U << 1);
    soft_i2c_stop();
    return ack;
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
    static const uint8_t key0[] = "KEY0\r\n";
    static const uint8_t aht_ack[] = "AHT10 0x38 ACK\r\n";
    static const uint8_t aht_nack[] = "AHT10 0x38 NACK\r\n";

    HAL_Init();
    clock_init();
    gpio_uart_init();
    for (;;) {
        HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_7);
        HAL_UART_Transmit(&uart1, (uint8_t *)banner, sizeof(banner) - 1U, 100U);
        if (aht10_probe()) {
            HAL_UART_Transmit(&uart1, (uint8_t *)aht_ack, sizeof(aht_ack) - 1U, 100U);
        } else {
            HAL_UART_Transmit(&uart1, (uint8_t *)aht_nack, sizeof(aht_nack) - 1U, 100U);
        }
        if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_10) == GPIO_PIN_RESET) {
            HAL_UART_Transmit(&uart1, (uint8_t *)key0, sizeof(key0) - 1U, 100U);
        }
        HAL_Delay(500U);
    }
}
