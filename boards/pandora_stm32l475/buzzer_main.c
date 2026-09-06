#include "stm32l4xx_hal.h"
#include "xy_actuator_buzzer.h"

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the source commit used for this image"
#endif

static UART_HandleTypeDef uart1;

void _init(void) {}
void _fini(void) {}
void SysTick_Handler(void) { HAL_IncTick(); }

static void fail(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    (void)xy_hal_gpio_write(GPIOB, 2U, 0U);
    __disable_irq();
    for (;;) {}
}

static void uart_text(const char *text)
{
    uint16_t length = 0U;
    while (text[length] != '\0') {
        ++length;
    }
    (void)HAL_UART_Transmit(&uart1, (uint8_t *)text, length, 100U);
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
    clk.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 |
                    RCC_CLOCKTYPE_PCLK2;
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

static void board_delay(uint32_t milliseconds)
{
    HAL_Delay(milliseconds);
}

int main(void)
{
    static const xy_actuator_buzzer_step_t short_short_long[] = {
        {true, 150U}, {false, 120U}, {true, 150U}, {false, 120U}, {true, 450U},
    };
    xy_actuator_buzzer_t buzzer = {
        .port = GPIOB,
        .pin = 2U,
        .active_high = true,
        .delay_ms = board_delay,
    };

    HAL_Init();
    clock_init();
    uart_init();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    uart_text("PANDORA BUZZER PB2 READY\r\nFIRMWARE_COMMIT " XINYI_FIRMWARE_COMMIT "\r\n");
    if (xy_actuator_buzzer_init(&buzzer) != ACTUATOR_EOK) {
        uart_text("PANDORA_BUZZER_INIT_ERROR\r\n");
        fail();
    }
    uart_text("PANDORA_BUZZER_PATTERN_START SHORT_SHORT_LONG\r\n");
    if (xy_actuator_buzzer_play(&buzzer, short_short_long,
                                sizeof(short_short_long) / sizeof(short_short_long[0])) !=
        ACTUATOR_EOK) {
        uart_text("PANDORA_BUZZER_PATTERN_ERROR\r\n");
        fail();
    }
    uart_text("PANDORA_BUZZER_PATTERN_DONE\r\n");
    if (buzzer.is_on || xy_hal_gpio_read(GPIOB, 2U) != 0) {
        uart_text("PANDORA_BUZZER_FINAL_OFF_ERROR\r\n");
        fail();
    }
    uart_text("PANDORA_BUZZER_FINAL_OFF\r\n");
    for (;;) {
        HAL_Delay(1000U);
    }
}
