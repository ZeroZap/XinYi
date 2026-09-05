#include "pandora_fota_install_flash.h"
#include "pandora_fota_flash.h"

#include "stm32l4xx_hal.h"
#include "xy_device.h"
#include "xy_fota_boot.h"
#include "xy_fota_w25q128.h"
#include "xy_hal_qspi.h"
#include "xy_w25q128.h"

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the bootloader source commit"
#endif

#define W25Q128_FOTA_IMAGE_ADDRESS 0x00F00000U
#define W25Q128_FOTA_IMAGE_SIZE 0x00080000U

static UART_HandleTypeDef uart1;
static QSPI_HandleTypeDef qspi;
static xy_w25q128_t w25q128;
static const uint32_t *const app_vectors = (const uint32_t *)PANDORA_FOTA_APP_BASE;

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

static void jump_to_application(void)
{
    void (*reset_handler)(void) = (void (*)(void))(uintptr_t)app_vectors[1];

    uart_text("PANDORA_BOOT_JUMP_APP\r\n");
    (void)HAL_UART_DeInit(&uart1);
    (void)xy_hal_qspi_deinit(&qspi);
    HAL_DeInit();
    __disable_irq();
    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL = 0U;
    for (uint32_t index = 0U; index < 8U; ++index) {
        NVIC->ICER[index] = UINT32_MAX;
        NVIC->ICPR[index] = UINT32_MAX;
    }
    SCB->VTOR = PANDORA_FOTA_APP_BASE;
    __DSB();
    __ISB();
    __set_MSP(app_vectors[0]);
    reset_handler();
    stop();
}

int main(void)
{
    const xy_fota_flash_ops_t *candidate_ops;
    xy_fota_boot_candidate_header_t header;
    xy_fota_boot_journal_config_t journal = pandora_fota_boot_journal_config();
    int installed = 0;
    xy_fota_boot_candidate_config_t candidate = {
        .storage_address = W25Q128_FOTA_IMAGE_ADDRESS,
        .storage_size = W25Q128_FOTA_IMAGE_SIZE,
        .execution_base = PANDORA_FOTA_APP_BASE,
        .execution_limit = PANDORA_FOTA_EXECUTION_LIMIT,
        .sram_base = 0x20000000U,
        .sram_limit = 0x20018000U,
        .sram2_base = 0x10000000U,
        .sram2_limit = 0x10008000U,
    };

    SCB->VTOR = 0x08000000U;
    HAL_Init();
    clock_init();
    peripherals_init();

    uart_text("PANDORA FOTA BOOTLOADER READY\r\n");
    uart_text("BOOTLOADER_COMMIT " XINYI_FIRMWARE_COMMIT "\r\n");
    if (xy_device_init() != XY_DEVICE_OK || xy_hal_qspi_init(&qspi, &qspi_config) != XY_HAL_OK ||
        xy_w25q128_init(&w25q128, &qspi, "pandora-boot-candidate") != XY_W25Q128_OK ||
        xy_fota_w25q128_bind(&w25q128, W25Q128_FOTA_IMAGE_ADDRESS, W25Q128_FOTA_IMAGE_SIZE,
                             1000U) != XY_FOTA_OK ||
        (candidate_ops = xy_fota_w25q128_ops()) == NULL || candidate_ops->init() != XY_FOTA_OK) {
        uart_text("PANDORA_BOOT_INIT_ERROR\r\n");
        stop();
    }
    candidate.read = candidate_ops->read;
    if (xy_fota_boot_candidate_validate(&candidate, &header) == XY_FOTA_OK) {
        if (xy_fota_boot_candidate_install_once(&candidate, pandora_fota_install_ops(), &journal,
                                                &installed) != XY_FOTA_OK) {
            uart_text("PANDORA_BOOT_INSTALL_ERROR\r\n");
            stop();
        }
        uart_text(installed ? "PANDORA_BOOT_CANDIDATE_INSTALLED\r\n"
                            : "PANDORA_BOOT_CANDIDATE_ALREADY_INSTALLED\r\n");
    } else if (!pandora_fota_application_vectors_valid()) {
        uart_text("PANDORA_BOOT_NO_VALID_IMAGE\r\n");
        stop();
    }
    jump_to_application();
}
