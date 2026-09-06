#include "stm32l4xx_hal.h"
#include "xy_actuator_rgb_led.h"
#include "xy_hal_uart.h"

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the source commit used for this image"
#endif

static UART_HandleTypeDef uart1;

void _init(void) {}
void _fini(void) {}
void SysTick_Handler(void) { HAL_IncTick(); }

static void rgb_force_off(void)
{
    __HAL_RCC_GPIOE_CLK_ENABLE();
    (void)xy_hal_gpio_write(GPIOE, 7U, 1U);
    (void)xy_hal_gpio_write(GPIOE, 8U, 1U);
    (void)xy_hal_gpio_write(GPIOE, 9U, 1U);
}

static void fail(void)
{
    rgb_force_off();
    __disable_irq();
    for (;;) {}
}

static void uart_text(const char *text)
{
    uint16_t length = 0U;
    while (text[length] != '\0') {
        ++length;
    }
    (void)xy_hal_uart_send(&uart1, (const uint8_t *)text, length, 100U);
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
    const xy_hal_gpio_config_t gpio = {XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_UP,
                                       XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_VERY_HIGH,
                                       GPIO_AF7_USART1};
    const xy_hal_uart_config_t uart = {115200U, XY_HAL_UART_WORDLEN_8B,
                                       XY_HAL_UART_STOPBITS_1, XY_HAL_UART_PARITY_NONE,
                                       XY_HAL_UART_FLOWCTRL_NONE, XY_HAL_UART_MODE_TX_RX};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();
    if (xy_hal_gpio_init(GPIOA, 9U, &gpio) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &gpio) != XY_HAL_OK) fail();
    uart1.Instance = USART1;
    if (xy_hal_uart_init(&uart1, &uart) != XY_HAL_OK) {
        fail();
    }
}

static void board_delay(uint32_t milliseconds)
{
    HAL_Delay(milliseconds);
}

int main(void)
{
    static const xy_actuator_rgb_led_step_t pattern[] = {
        {XY_ACTUATOR_RGB_RED, 500U},
        {XY_ACTUATOR_RGB_OFF, 150U},
        {XY_ACTUATOR_RGB_GREEN, 500U},
        {XY_ACTUATOR_RGB_OFF, 150U},
        {XY_ACTUATOR_RGB_BLUE, 500U},
        {XY_ACTUATOR_RGB_OFF, 150U},
        {XY_ACTUATOR_RGB_WHITE, 700U},
    };
    xy_actuator_rgb_led_t led = {
        .port = GPIOE,
        .red_pin = 7U,
        .green_pin = 8U,
        .blue_pin = 9U,
        .active_low = true,
        .delay_ms = board_delay,
    };

    HAL_Init();
    rgb_force_off();
    clock_init();
    uart_init();
    uart_text("PANDORA RGB PE7/PE8/PE9 READY ACTIVE_LOW\r\n"
              "FIRMWARE_COMMIT " XINYI_FIRMWARE_COMMIT "\r\n"
              "PANDORA_MOTOR_SKIPPED TC214B_TRUTH_TABLE_UNVERIFIED\r\n");
    if (xy_actuator_rgb_led_init(&led) != ACTUATOR_EOK) {
        uart_text("PANDORA_RGB_INIT_ERROR\r\n");
        fail();
    }
    uart_text("PANDORA_RGB_PATTERN_START RED_GREEN_BLUE_WHITE\r\n");
    if (xy_actuator_rgb_led_play(&led, pattern, sizeof(pattern) / sizeof(pattern[0])) !=
        ACTUATOR_EOK) {
        uart_text("PANDORA_RGB_PATTERN_ERROR\r\n");
        fail();
    }
    uart_text("PANDORA_RGB_PATTERN_DONE\r\n");
    if (led.color != XY_ACTUATOR_RGB_OFF || xy_hal_gpio_read(GPIOE, 7U) != 1 ||
        xy_hal_gpio_read(GPIOE, 8U) != 1 || xy_hal_gpio_read(GPIOE, 9U) != 1) {
        uart_text("PANDORA_RGB_FINAL_OFF_ERROR\r\n");
        fail();
    }
    uart_text("PANDORA_RGB_FINAL_OFF\r\n");
    for (;;) {
        HAL_Delay(1000U);
    }
}
