#include "xy_hal_gpio.h"
#include "xy_hal_spi.h"
#include "xy_hal_uart.h"

#include <stddef.h>
#include <stdint.h>

#include "ch32v30x.h"

static void uart_write(const char* text) {
    size_t len = 0U;
    while (text[len] != '\0')
        ++len;
    (void)xy_hal_uart_send(USART1, (const uint8_t*)text, len, 1000U);
}

static int flash_read_jedec(uint8_t id[3]) {
    const uint8_t command = 0x9FU;
    if (xy_hal_gpio_write(GPIOA, 15U, 0U) != XY_HAL_OK)
        return 0;
    if (xy_hal_spi_transmit(SPI3, &command, 1U, 100U) != XY_HAL_OK ||
        xy_hal_spi_receive(SPI3, id, 3U, 100U) != XY_HAL_OK) {
        (void)xy_hal_gpio_write(GPIOA, 15U, 1U);
        return 0;
    }
    (void)xy_hal_gpio_write(GPIOA, 15U, 1U);
    return 1;
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
    uint8_t id[3];

    if (xy_hal_gpio_init(GPIOA, 9U, &uart_tx) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &uart_rx) != XY_HAL_OK ||
        xy_hal_uart_init(USART1, &uart) != XY_HAL_OK) {
        for (;;) {
        }
    }
    if (xy_hal_gpio_init(GPIOB, 3U, &spi_af) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOB, 4U, &spi_miso) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOB, 5U, &spi_af) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 15U, &cs) != XY_HAL_OK ||
        xy_hal_gpio_write(GPIOA, 15U, 1U) != XY_HAL_OK ||
        xy_hal_spi_init(SPI3, &spi) != XY_HAL_OK) {
        uart_write("OPENCH_SPI3_INIT_ERROR\r\n");
        for (;;) {
        }
    }

    for (;;) {
        if (flash_read_jedec(id) && id[0] == 0xEFU && id[1] == 0x40U && id[2] == 0x18U) {
            uart_write("OPENCH_SPI3_W25Q128_JEDEC_OK EF4018\r\n");
        } else {
            uart_write("OPENCH_SPI3_W25Q128_JEDEC_ERROR\r\n");
        }
        for (volatile uint32_t delay = 0; delay < 300000U; ++delay) {
            ;
        }
    }
}
