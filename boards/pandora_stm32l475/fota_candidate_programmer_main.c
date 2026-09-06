#include "pandora_fota_flash.h"
#include "pandora_fota_candidate_blob.h"

#include "stm32l4xx_hal.h"
#include "xy_device.h"
#include "xy_fota_w25q128.h"
#include "xy_hal_qspi.h"
#include "xy_w25q128.h"

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the programmer source commit"
#endif

#define W25Q128_FOTA_IMAGE_ADDRESS 0x00F00000U
#define W25Q128_FOTA_IMAGE_SIZE 0x00080000U

static UART_HandleTypeDef uart1;
static QSPI_HandleTypeDef qspi;
static xy_w25q128_t w25q128;
static const xy_hal_qspi_config_t qspi_config = {
    .clock_prescaler = 3U,
    .fifo_threshold = 1U,
    .flash_size_bits = 24U,
    .chip_select_high_cycles = 2U,
};

void _init(void) {}
void _fini(void) {}

void SysTick_Handler(void)
{
    HAL_IncTick();
}

static void stop(void)
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
    (void)HAL_UART_Transmit(&uart1, (uint8_t *)text, length, 200U);
}

static void clock_init(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
        stop();
    }
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM = 1U;
    osc.PLL.PLLN = 20U;
    osc.PLL.PLLP = RCC_PLLP_DIV7;
    osc.PLL.PLLQ = RCC_PLLQ_DIV2;
    osc.PLL.PLLR = RCC_PLLR_DIV2;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
        stop();
    }
    clk.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 |
                    RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_4) != HAL_OK) {
        stop();
    }
}

static void peripherals_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_QSPI_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &gpio);
    uart1.Instance = USART1;
    uart1.Init.BaudRate = 115200U;
    uart1.Init.WordLength = UART_WORDLENGTH_8B;
    uart1.Init.StopBits = UART_STOPBITS_1;
    uart1.Init.Parity = UART_PARITY_NONE;
    uart1.Init.Mode = UART_MODE_TX_RX;
    uart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uart1.Init.OverSampling = UART_OVERSAMPLING_16;
    uart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&uart1) != HAL_OK) {
        stop();
    }
    gpio.Pin = GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 |
               GPIO_PIN_15;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF10_QUADSPI;
    HAL_GPIO_Init(GPIOE, &gpio);
    qspi.Instance = QUADSPI;
}

int main(void)
{
    const xy_fota_flash_ops_t *ops;
    uint8_t verify[256];

    SCB->VTOR = 0x08000000U;
    HAL_Init();
    clock_init();
    peripherals_init();
    uart_text("PANDORA FOTA CANDIDATE PROGRAMMER READY\r\n");
    uart_text("PROGRAMMER_COMMIT " XINYI_FIRMWARE_COMMIT "\r\n");
    uart_text("CANDIDATE_SOURCE_COMMIT " PANDORA_FOTA_CANDIDATE_SOURCE_COMMIT "\r\n");

    if (PANDORA_FOTA_CANDIDATE_BLOB_SIZE > W25Q128_FOTA_IMAGE_SIZE ||
        xy_device_init() != XY_DEVICE_OK || xy_hal_qspi_init(&qspi, &qspi_config) != XY_HAL_OK ||
        xy_w25q128_init(&w25q128, &qspi, "pandora-candidate-programmer") != XY_W25Q128_OK ||
        xy_fota_w25q128_bind(&w25q128, W25Q128_FOTA_IMAGE_ADDRESS, W25Q128_FOTA_IMAGE_SIZE,
                             1000U) != XY_FOTA_OK ||
        (ops = xy_fota_w25q128_ops()) == NULL || ops->init() != XY_FOTA_OK ||
        ops->erase(W25Q128_FOTA_IMAGE_ADDRESS, PANDORA_FOTA_CANDIDATE_BLOB_SIZE) != XY_FOTA_OK) {
        uart_text("PANDORA_FOTA_CANDIDATE_PROGRAM_ERROR\r\n");
        stop();
    }
    for (uint32_t offset = 0U; offset < PANDORA_FOTA_CANDIDATE_BLOB_SIZE; offset += 256U) {
        uint32_t chunk = PANDORA_FOTA_CANDIDATE_BLOB_SIZE - offset;
        if (chunk > sizeof(verify)) {
            chunk = sizeof(verify);
        }
        if (ops->write(W25Q128_FOTA_IMAGE_ADDRESS + offset,
                       pandora_fota_candidate_blob + offset, chunk) != XY_FOTA_OK ||
            ops->read(W25Q128_FOTA_IMAGE_ADDRESS + offset, verify, chunk) != XY_FOTA_OK) {
            uart_text("PANDORA_FOTA_CANDIDATE_PROGRAM_ERROR\r\n");
            stop();
        }
        for (uint32_t index = 0U; index < chunk; ++index) {
            if (verify[index] != pandora_fota_candidate_blob[offset + index]) {
                uart_text("PANDORA_FOTA_CANDIDATE_VERIFY_ERROR\r\n");
                stop();
            }
        }
    }
    uart_text("PANDORA_FOTA_CANDIDATE_PROGRAMMED_VERIFIED\r\n");
    stop();
}
