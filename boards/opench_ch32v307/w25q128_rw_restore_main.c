#include "ch32v30x.h"
#include "xy_hal_gpio.h"
#include "xy_hal_spi.h"
#include "xy_hal_uart.h"
#include <stddef.h>
#include <string.h>

#define TEST_SECTOR_ADDRESS 0x00EFF000UL
#define SECTOR_SIZE 4096U
#define PAGE_SIZE 256U

static uint8_t backup[SECTOR_SIZE];
static uint8_t verify[SECTOR_SIZE];
static uint8_t pattern[PAGE_SIZE];

static void write_marker(const char* text) {
    size_t length = 0U;
    while (text[length] != '\0')
        ++length;
    (void)xy_hal_uart_send(USART1, (const uint8_t*)text, length, 1000U);
}

static int flash_transaction(const uint8_t* tx, size_t tx_len, uint8_t* rx, size_t rx_len) {
    if (xy_hal_gpio_write(GPIOA, 15U, 0U) != XY_HAL_OK)
        return 0;
    if ((tx_len != 0U && xy_hal_spi_transmit(SPI3, tx, tx_len, 1000U) != XY_HAL_OK) ||
        (rx_len != 0U && xy_hal_spi_receive(SPI3, rx, rx_len, 1000U) != XY_HAL_OK)) {
        (void)xy_hal_gpio_write(GPIOA, 15U, 1U);
        return 0;
    }
    return xy_hal_gpio_write(GPIOA, 15U, 1U) == XY_HAL_OK;
}

static int flash_write_enable(void) {
    const uint8_t command = 0x06U;
    return flash_transaction(&command, 1U, NULL, 0U);
}

static int flash_wait_ready(void) {
    for (uint32_t attempt = 0U; attempt < 100000U; ++attempt) {
        const uint8_t command = 0x05U;
        uint8_t status = 0xFFU;
        if (!flash_transaction(&command, 1U, &status, 1U))
            return 0;
        if ((status & 1U) == 0U)
            return 1;
    }
    return 0;
}

static int flash_read(uint32_t address, uint8_t* data, size_t length) {
    uint8_t command[4] = {0x03U, (uint8_t)(address >> 16), (uint8_t)(address >> 8),
                          (uint8_t)address};
    return flash_transaction(command, sizeof(command), data, length);
}

static int flash_erase_sector(uint32_t address) {
    uint8_t command[4] = {0x20U, (uint8_t)(address >> 16), (uint8_t)(address >> 8),
                          (uint8_t)address};
    return flash_write_enable() && flash_transaction(command, sizeof(command), NULL, 0U) &&
           flash_wait_ready();
}

static int flash_program_page(uint32_t address, const uint8_t* data) {
    uint8_t command[4] = {0x02U, (uint8_t)(address >> 16), (uint8_t)(address >> 8),
                          (uint8_t)address};
    if (!flash_write_enable() || xy_hal_gpio_write(GPIOA, 15U, 0U) != XY_HAL_OK ||
        xy_hal_spi_transmit(SPI3, command, sizeof(command), 1000U) != XY_HAL_OK ||
        xy_hal_spi_transmit(SPI3, data, PAGE_SIZE, 1000U) != XY_HAL_OK) {
        (void)xy_hal_gpio_write(GPIOA, 15U, 1U);
        return 0;
    }
    return xy_hal_gpio_write(GPIOA, 15U, 1U) == XY_HAL_OK && flash_wait_ready();
}

static int restore_backup(void) {
    if (!flash_erase_sector(TEST_SECTOR_ADDRESS))
        return 0;
    for (size_t offset = 0U; offset < SECTOR_SIZE; offset += PAGE_SIZE) {
        int all_erased = 1;
        for (size_t index = 0U; index < PAGE_SIZE; ++index) {
            if (backup[offset + index] != 0xFFU) {
                all_erased = 0;
                break;
            }
        }
        if (!all_erased &&
            !flash_program_page(TEST_SECTOR_ADDRESS + (uint32_t)offset, &backup[offset]))
            return 0;
    }
    return flash_read(TEST_SECTOR_ADDRESS, verify, sizeof(verify)) &&
           memcmp(backup, verify, sizeof(backup)) == 0;
}

int main(void) {
    const xy_hal_gpio_config_t uart_tx = {XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_NONE,
                                          XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_VERY_HIGH, 0U};
    const xy_hal_gpio_config_t uart_rx = {XY_HAL_GPIO_MODE_INPUT, XY_HAL_GPIO_PULL_NONE,
                                          XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_LOW, 0U};
    const xy_hal_gpio_config_t spi_af = {XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_NONE,
                                         XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_VERY_HIGH, 0U};
    const xy_hal_gpio_config_t spi_miso = {XY_HAL_GPIO_MODE_INPUT, XY_HAL_GPIO_PULL_UP,
                                           XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_HIGH, 0U};
    const xy_hal_gpio_config_t cs = {XY_HAL_GPIO_MODE_OUTPUT, XY_HAL_GPIO_PULL_UP,
                                     XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_HIGH, 0U};
    const xy_hal_uart_config_t uart = {115200U,
                                       XY_HAL_UART_WORDLEN_8B,
                                       XY_HAL_UART_STOPBITS_1,
                                       XY_HAL_UART_PARITY_NONE,
                                       XY_HAL_UART_FLOWCTRL_NONE,
                                       XY_HAL_UART_MODE_TX_RX};
    const xy_hal_spi_config_t spi = {XY_HAL_SPI_MODE_0,
                                     XY_HAL_SPI_DIR_2LINES,
                                     XY_HAL_SPI_DATASIZE_8BIT,
                                     XY_HAL_SPI_FIRSTBIT_MSB,
                                     XY_HAL_SPI_NSS_SOFT,
                                     SPI_BaudRatePrescaler_8,
                                     1U};

    SystemCoreClockUpdate();
    if (xy_hal_gpio_init(GPIOA, 9U, &uart_tx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &uart_rx) != XY_HAL_OK ||
        xy_hal_uart_init(USART1, &uart) != XY_HAL_OK)
        for (;;) {
        }
    write_marker("OPENCH_W25Q128_RW_BOOT_20261012_B\r\n");
    if (xy_hal_gpio_init(GPIOB, 3U, &spi_af) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOB, 4U, &spi_miso) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOB, 5U, &spi_af) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 15U, &cs) != XY_HAL_OK ||
        xy_hal_gpio_write(GPIOA, 15U, 1U) != XY_HAL_OK ||
        xy_hal_spi_init(SPI3, &spi) != XY_HAL_OK) {
        write_marker("OPENCH_W25Q128_RW_ERROR_B\r\n");
        for (;;) {
        }
    }
    for (size_t index = 0U; index < PAGE_SIZE; ++index)
        pattern[index] = (uint8_t)(index ^ 0xA5U);
    if (!flash_read(TEST_SECTOR_ADDRESS, backup, sizeof(backup)) ||
        !flash_erase_sector(TEST_SECTOR_ADDRESS) ||
        !flash_program_page(TEST_SECTOR_ADDRESS, pattern) ||
        !flash_read(TEST_SECTOR_ADDRESS, verify, PAGE_SIZE) ||
        memcmp(pattern, verify, PAGE_SIZE) != 0) {
        write_marker("OPENCH_W25Q128_RW_ERROR_B\r\n");
        for (;;) {
        }
    }
    if (!restore_backup()) {
        write_marker("OPENCH_W25Q128_RESTORE_ERROR_B\r\n");
        for (;;) {
        }
    }
    write_marker("OPENCH_W25Q128_RW_RESTORE_OK_B\r\n");
    for (;;) {
    }
}
