#include "stm32l4xx_hal.h"
#include "xy_os.h"

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the source commit used for this image"
#endif

static UART_HandleTypeDef uart1;

void _init(void) {}
void _fini(void) {}

void vApplicationStackOverflowHook(void *task, char *task_name)
{
    (void)task;
    (void)task_name;
    __disable_irq();
    for (;;) {
    }
}

static void fail(void)
{
    __disable_irq();
    for (;;) {
    }
}

static void uart_text(const char *text)
{
    uint16_t length = 0;
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
    __HAL_RCC_USART1_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_7;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &gpio);

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

static void fast_task(void *argument)
{
    (void)argument;
    for (;;) {
        HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_7);
        uart_text("OSAL_TASK_FAST\r\n");
        (void)xy_os_delay(500U);
    }
}

static void slow_task(void *argument)
{
    (void)argument;
    for (;;) {
        uart_text("OSAL_TASK_SLOW\r\n");
        (void)xy_os_delay(1000U);
    }
}

int main(void)
{
    static const xy_os_thread_attr_t fast_attr = {
        .name = "osal-fast",
        .stack_size = 512U,
        .priority = XY_OS_PRIORITY_NORMAL,
    };
    static const xy_os_thread_attr_t slow_attr = {
        .name = "osal-slow",
        .stack_size = 512U,
        .priority = XY_OS_PRIORITY_LOW,
    };

    HAL_Init();
    clock_init();
    gpio_uart_init();
    uart_text("PANDORA STM32L475VE XINYI OSAL FREERTOS READY\r\n");
    uart_text("FIRMWARE_COMMIT " XINYI_FIRMWARE_COMMIT "\r\n");

    if (xy_os_kernel_init() != XY_OS_OK ||
        xy_os_thread_new(fast_task, NULL, &fast_attr) == NULL ||
        xy_os_thread_new(slow_task, NULL, &slow_attr) == NULL) {
        fail();
    }
    (void)xy_os_kernel_start();
    fail();
}
