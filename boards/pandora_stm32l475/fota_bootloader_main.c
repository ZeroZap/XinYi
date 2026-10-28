#include "pandora_fota_boot_flow.h"
#include "pandora_fota_flash.h"
#include "pandora_fota_install_flash.h"

#include "pandora_platform_startup.h"
#include "stm32l4xx_hal.h"
#include "xy_device.h"
#include "xy_fota_boot.h"
#include "xy_fota_w25q128.h"
#include "xy_hal_gpio.h"
#include "xy_hal_qspi.h"
#include "xy_hal_rtc.h"
#include "xy_hal_sys.h"
#include "xy_hal_uart.h"
#include "xy_w25q128.h"

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the bootloader source commit"
#endif

#define W25Q128_FOTA_IMAGE_ADDRESS 0x00F00000U
#define W25Q128_FOTA_IMAGE_SIZE 0x00080000U

#ifdef PANDORA_FOTA_AUTHORIZE_RESTAGE
#ifndef PANDORA_FOTA_RESTAGE_SOURCE_COMMIT
#error "Reviewed restage requires an exact approved source commit"
#endif
#endif

static UART_HandleTypeDef uart1;
static QSPI_HandleTypeDef qspi;
static xy_w25q128_t w25q128;
static const uint32_t* const app_vectors = (const uint32_t*)PANDORA_FOTA_APP_BASE;
static const xy_fota_boot_install_ops_t* install_ops;
static xy_fota_boot_journal_config_t journal_config;
static xy_fota_boot_candidate_header_t candidate_header;
static xy_fota_boot_candidate_config_t candidate_config = {
    .storage_address = W25Q128_FOTA_IMAGE_ADDRESS,
    .storage_size = W25Q128_FOTA_IMAGE_SIZE,
    .execution_base = PANDORA_FOTA_APP_BASE,
    .execution_limit = PANDORA_FOTA_EXECUTION_LIMIT,
    .sram_base = 0x20000000U,
    .sram_limit = 0x20018000U,
    .sram2_base = 0x10000000U,
    .sram2_limit = 0x10008000U,
};

static const xy_hal_qspi_config_t qspi_config = {
    .clock_prescaler = 3U,
    .fifo_threshold = 1U,
    .flash_size_bits = 24U,
    .chip_select_high_cycles = 2U,
};

#ifdef PANDORA_FOTA_AUTHORIZE_RESTAGE
static int parse_hex_word(const char* text, uint32_t* value) {
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
#endif

void _init(void) {}
void _fini(void) {}

void SysTick_Handler(void) {
    xy_hal_sys_tick_irq_handler();
}

static void stop(void) {
    __disable_irq();
    for (;;) {
    }
}

static void uart_text(const char* text) {
    size_t length = 0U;
    while (text[length] != '\0') {
        ++length;
    }
    (void)xy_hal_uart_send(&uart1, (const uint8_t*)text, length, 200U);
}

static void peripherals_init(void) {
    const xy_hal_gpio_config_t uart_gpio = {
        XY_HAL_GPIO_MODE_AF,         XY_HAL_GPIO_PULL_UP, XY_HAL_GPIO_OTYPE_PP,
        XY_HAL_GPIO_SPEED_VERY_HIGH, GPIO_AF7_USART1,
    };
    const xy_hal_gpio_config_t qspi_gpio = {
        XY_HAL_GPIO_MODE_AF,         XY_HAL_GPIO_PULL_UP, XY_HAL_GPIO_OTYPE_PP,
        XY_HAL_GPIO_SPEED_VERY_HIGH, GPIO_AF10_QUADSPI,
    };
    const xy_hal_uart_config_t uart_config = {
        115200U,
        XY_HAL_UART_WORDLEN_8B,
        XY_HAL_UART_STOPBITS_1,
        XY_HAL_UART_PARITY_NONE,
        XY_HAL_UART_FLOWCTRL_NONE,
        XY_HAL_UART_MODE_TX_RX,
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

static void jump_to_application(void) {
    void (*reset_handler)(void) = (void (*)(void))(uintptr_t)app_vectors[1];

    uart_text("PANDORA_BOOT_JUMP_APP\r\n");
    (void)xy_hal_uart_deinit(&uart1);
    (void)xy_hal_qspi_deinit(&qspi);
    if (xy_hal_sys_deinit() != XY_HAL_OK) {
        stop();
    }
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

static void boot_mark(const char* marker) {
    uart_text(marker);
}

static int boot_rtc_backup_read(uint8_t slot, uint32_t* value) {
    return xy_hal_rtc_backup_read(slot, value) == XY_HAL_OK ? 0 : -1;
}

static int boot_rtc_backup_write(uint8_t slot, uint32_t value) {
    return xy_hal_rtc_backup_write(slot, value) == XY_HAL_OK ? 0 : -1;
}

static int boot_candidate_valid(void) {
    return xy_fota_boot_candidate_validate(&candidate_config, &candidate_header) == XY_FOTA_OK;
}

static int boot_application_vectors_valid(void) {
    return pandora_fota_application_vectors_valid() != 0;
}

static int boot_install_once(int* installed) {
    return xy_fota_boot_candidate_install_once(&candidate_config, install_ops, &journal_config,
                                               installed) == XY_FOTA_OK
               ? 0
               : -1;
}

static int boot_confirm(void) {
    return xy_fota_boot_candidate_confirm(&candidate_config, install_ops, &journal_config) ==
                   XY_FOTA_OK
               ? 0
               : -1;
}

static int boot_record_attempt(int* rollback_required) {
    return xy_fota_boot_candidate_record_attempt(&candidate_config, install_ops, &journal_config,
                                                 PANDORA_FOTA_FLOW_MAX_ATTEMPTS,
                                                 rollback_required) == XY_FOTA_OK
               ? 0
               : -1;
}

#ifdef PANDORA_FOTA_AUTHORIZE_RESTAGE
static int boot_authorize_restage(void) {
    xy_fota_boot_reviewed_restage_authorization_t authorization = {
        .candidate =
            {
                .magic = XY_FOTA_BOOT_RESTAGE_AUTHORIZATION_MAGIC,
                .format_version = XY_FOTA_BOOT_RESTAGE_AUTHORIZATION_VERSION,
                .size = sizeof(xy_fota_boot_restage_authorization_t),
                .image_version = candidate_header.image_version,
                .image_size = candidate_header.image_size,
                .image_crc32 = candidate_header.image_crc32,
            },
    };
    uint32_t expected_source_commit[5];

    for (uint32_t index = 0U; index < 5U; ++index) {
        if (!parse_hex_word(PANDORA_FOTA_RESTAGE_SOURCE_COMMIT + index * 8U,
                            &expected_source_commit[index])) {
            uart_text("PANDORA_BOOT_RESTAGE_AUTHORIZATION_ERROR\r\n");
            return -1;
        }
        authorization.source_commit[index] = expected_source_commit[index];
    }
    if (xy_fota_boot_candidate_authorize_reviewed_restage(&candidate_config, install_ops,
                                                          &journal_config, &authorization,
                                                          expected_source_commit) != XY_FOTA_OK) {
        uart_text("PANDORA_BOOT_RESTAGE_AUTHORIZATION_ERROR\r\n");
        return -1;
    }
    return 0;
}
#endif

static const pandora_fota_boot_flow_ops_t boot_flow_ops = {
    .rtc_backup_read = boot_rtc_backup_read,
    .rtc_backup_write = boot_rtc_backup_write,
    .candidate_valid = boot_candidate_valid,
    .application_vectors_valid = boot_application_vectors_valid,
    .install_once = boot_install_once,
    .confirm = boot_confirm,
    .record_attempt = boot_record_attempt,
#ifdef PANDORA_FOTA_AUTHORIZE_RESTAGE
    .authorize_restage = boot_authorize_restage,
#endif
    .mark = boot_mark,
};

int main(void) {
    const xy_fota_flash_ops_t* candidate_ops;

    journal_config = pandora_fota_boot_journal_config();
    SCB->VTOR = 0x08000000U;
    if (xy_hal_sys_init() != XY_HAL_OK) {
        stop();
    }
    if (pandora_platform_startup() != 0) {
        stop();
    }
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
    candidate_config.read = candidate_ops->read;
    install_ops = pandora_fota_install_ops();
    if (pandora_fota_boot_flow_run(&boot_flow_ops) == PANDORA_FOTA_BOOT_OUTCOME_HALT) {
        stop();
    }
    jump_to_application();
}
