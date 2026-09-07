#include "pandora_fota_install_flash.h"
#include "pandora_fota_flash.h"

#include "stm32l4xx_hal.h"
#include "xy_device.h"
#include "xy_fota_boot.h"
#include "xy_fota_w25q128.h"
#include "xy_hal_gpio.h"
#include "xy_hal_qspi.h"
#include "xy_hal_uart.h"
#include "xy_w25q128.h"

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the bootloader source commit"
#endif

#define W25Q128_FOTA_IMAGE_ADDRESS 0x00F00000U
#define W25Q128_FOTA_IMAGE_SIZE 0x00080000U
#define PANDORA_FOTA_CONFIRM_REQUEST_MAGIC 0x46525132U
#define PANDORA_FOTA_CONFIRM_ACK_MAGIC 0x46414332U
#define PANDORA_FOTA_MAX_ATTEMPTS 3U
#define PANDORA_FOTA_RESTAGE_DONE_MAGIC 0x52535444U

#ifdef PANDORA_FOTA_AUTHORIZE_RESTAGE
#ifndef PANDORA_FOTA_RESTAGE_SOURCE_COMMIT
#error "Reviewed restage requires an exact approved source commit"
#endif
#endif

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

static int parse_hex_word(const char *text, uint32_t *value)
{
    uint32_t result = 0U;

    for (uint32_t index = 0U; index < 8U; ++index) {
        char digit = text[index];
        uint32_t nibble;
        if (digit >= '0' && digit <= '9') {
            nibble = (uint32_t)(digit - '0');
        } else if (digit >= 'a' && digit <= 'f') {
            nibble = (uint32_t)(digit - 'a' + 10);
        } else {
            return 0;
        }
        result = (result << 4) | nibble;
    }
    *value = result;
    return 1;
}

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
    size_t length = 0U;
    while (text[length] != '\0') {
        ++length;
    }
    (void)xy_hal_uart_send(&uart1, (const uint8_t *)text, length, 200U);
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
    const xy_hal_gpio_config_t uart_gpio = {
        XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_UP, XY_HAL_GPIO_OTYPE_PP,
        XY_HAL_GPIO_SPEED_VERY_HIGH, GPIO_AF7_USART1,
    };
    const xy_hal_gpio_config_t qspi_gpio = {
        XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_UP, XY_HAL_GPIO_OTYPE_PP,
        XY_HAL_GPIO_SPEED_VERY_HIGH, GPIO_AF10_QUADSPI,
    };
    const xy_hal_uart_config_t uart_config = {
        115200U, XY_HAL_UART_WORDLEN_8B, XY_HAL_UART_STOPBITS_1, XY_HAL_UART_PARITY_NONE,
        XY_HAL_UART_FLOWCTRL_NONE, XY_HAL_UART_MODE_TX_RX,
    };

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_QSPI_CLK_ENABLE();
    uart1.Instance = USART1;
    if (xy_hal_gpio_init(GPIOA, 9U, &uart_gpio) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &uart_gpio) != XY_HAL_OK ||
        xy_hal_uart_init(&uart1, &uart_config) != XY_HAL_OK) {
        stop();
    }
    for (uint8_t pin = 10U; pin <= 15U; ++pin) {
        if (xy_hal_gpio_init(GPIOE, pin, &qspi_gpio) != XY_HAL_OK) {
            stop();
        }
    }
    qspi.Instance = QUADSPI;
}

static void jump_to_application(void)
{
    void (*reset_handler)(void) = (void (*)(void))(uintptr_t)app_vectors[1];

    uart_text("PANDORA_BOOT_JUMP_APP\r\n");
    (void)xy_hal_uart_deinit(&uart1);
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
    HAL_PWR_EnableBkUpAccess();

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
#ifdef PANDORA_FOTA_AUTHORIZE_RESTAGE
        if (RTC->BKP2R != PANDORA_FOTA_RESTAGE_DONE_MAGIC) {
            xy_fota_boot_reviewed_restage_authorization_t authorization = {
                .candidate = {
                    .magic = XY_FOTA_BOOT_RESTAGE_AUTHORIZATION_MAGIC,
                    .format_version = XY_FOTA_BOOT_RESTAGE_AUTHORIZATION_VERSION,
                    .size = sizeof(xy_fota_boot_restage_authorization_t),
                    .image_version = header.image_version,
                    .image_size = header.image_size,
                    .image_crc32 = header.image_crc32,
                },
            };
            uint32_t expected_source_commit[5];

            for (uint32_t index = 0U; index < 5U; ++index) {
                if (!parse_hex_word(PANDORA_FOTA_RESTAGE_SOURCE_COMMIT + index * 8U,
                                    &expected_source_commit[index])) {
                    uart_text("PANDORA_BOOT_RESTAGE_AUTHORIZATION_ERROR\r\n");
                    stop();
                }
                authorization.source_commit[index] = expected_source_commit[index];
            }

            if (xy_fota_boot_candidate_authorize_reviewed_restage(
                    &candidate, pandora_fota_install_ops(), &journal, &authorization,
                    expected_source_commit) != XY_FOTA_OK) {
                uart_text("PANDORA_BOOT_RESTAGE_AUTHORIZATION_ERROR\r\n");
                stop();
            }
            RTC->BKP2R = PANDORA_FOTA_RESTAGE_DONE_MAGIC;
            uart_text("PANDORA_BOOT_RESTAGE_AUTHORIZED\r\n");
        }
#endif
        if (xy_fota_boot_candidate_install_once(&candidate, pandora_fota_install_ops(), &journal,
                                                &installed) != XY_FOTA_OK) {
            uart_text("PANDORA_BOOT_INSTALL_ERROR\r\n");
            stop();
        }
        uart_text(installed ? "PANDORA_BOOT_CANDIDATE_INSTALLED\r\n"
                            : "PANDORA_BOOT_CANDIDATE_ALREADY_INSTALLED\r\n");
        if (RTC->BKP1R == PANDORA_FOTA_CONFIRM_REQUEST_MAGIC) {
            if (xy_fota_boot_candidate_confirm(&candidate, pandora_fota_install_ops(), &journal) !=
                XY_FOTA_OK) {
                uart_text("PANDORA_BOOT_CONFIRM_ERROR\r\n");
                stop();
            }
            RTC->BKP1R = PANDORA_FOTA_CONFIRM_ACK_MAGIC;
            uart_text("PANDORA_BOOT_CANDIDATE_CONFIRMED\r\n");
        } else {
            int rollback_required = 0;

            if (xy_fota_boot_candidate_record_attempt(&candidate, pandora_fota_install_ops(),
                                                      &journal, PANDORA_FOTA_MAX_ATTEMPTS,
                                                      &rollback_required) != XY_FOTA_OK) {
                uart_text("PANDORA_BOOT_ATTEMPT_ERROR\r\n");
                stop();
            }
            if (rollback_required) {
                uart_text("PANDORA_BOOT_ROLLBACK_REQUIRED\r\n");
                stop();
            }
            uart_text("PANDORA_BOOT_ATTEMPT_COMMITTED\r\n");
        }
    } else if (!pandora_fota_application_vectors_valid()) {
        uart_text("PANDORA_BOOT_NO_VALID_IMAGE\r\n");
        stop();
    }
    jump_to_application();
}
