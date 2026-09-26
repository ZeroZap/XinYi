#include "pandora_platform_startup.h"
#include "stm32l4xx_hal.h"
#include "xy_hal_delay.h"
#include "xy_hal_gpio.h"
#include "xy_hal_spi.h"
#include "xy_hal_sys.h"
#include "xy_hal_uart.h"
#include "xy_nrf24l01.h"

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the source commit used for this image"
#endif

static UART_HandleTypeDef uart1;
static SPI_HandleTypeDef spi2;

void _init(void) {}
void _fini(void) {}
void SysTick_Handler(void) { xy_hal_sys_tick_irq_handler(); }

static void stop(void)
{
    __disable_irq();
    for (;;) {}
}

static void uart_text(const char *text)
{
    uint16_t length = 0U;
    while (text[length] != '\0') ++length;
    (void)xy_hal_uart_send(&uart1, (const uint8_t *)text, length, 100U);
}

static void uart_hex8(uint8_t value)
{
    static const char digits[] = "0123456789ABCDEF";
    char text[3] = {digits[value >> 4], digits[value & 0x0FU], '\0'};
    uart_text(text);
}

static void uart_error(xy_hal_error_t error)
{
    char text[5];
    uint32_t value = (uint32_t)(-error);
    size_t pos = sizeof(text);
    text[--pos] = '\0';
    do {
        text[--pos] = (char)('0' + value % 10U);
        value /= 10U;
    } while (value != 0U);
    uart_text(&text[pos]);
}

static void uart_init(void)
{
    const xy_hal_gpio_config_t pins = {XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_UP,
                                       XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_VERY_HIGH,
                                       GPIO_AF7_USART1};
    const xy_hal_uart_config_t config = {115200U, XY_HAL_UART_WORDLEN_8B,
                                         XY_HAL_UART_STOPBITS_1, XY_HAL_UART_PARITY_NONE,
                                         XY_HAL_UART_FLOWCTRL_NONE, XY_HAL_UART_MODE_TX_RX};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();
    if (xy_hal_gpio_init(GPIOA, 9U, &pins) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &pins) != XY_HAL_OK) stop();
    uart1.Instance = USART1;
    if (xy_hal_uart_init(&uart1, &config) != XY_HAL_OK) stop();
}

static xy_hal_error_t nrf_transfer(void *spi, const uint8_t *tx, uint8_t *rx,
                                   size_t length, uint32_t timeout_ms)
{
    return xy_hal_spi_transmit_receive(spi, tx, rx, length, timeout_ms);
}

static xy_hal_error_t nrf_csn(void *arg, uint8_t level)
{
    (void)arg;
    return xy_hal_gpio_write(GPIOD, 6U, level);
}

static xy_hal_error_t nrf_ce(void *arg, uint8_t level)
{
    (void)arg;
    return xy_hal_gpio_write(GPIOD, 5U, level);
}

static void nrf_bus_init(void)
{
    const xy_hal_gpio_config_t alternate = {XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_NONE,
                                             XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_HIGH,
                                             GPIO_AF5_SPI2};
    const xy_hal_gpio_config_t output = {XY_HAL_GPIO_MODE_OUTPUT, XY_HAL_GPIO_PULL_NONE,
                                         XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_HIGH, 0U};
    const xy_hal_gpio_config_t input = {XY_HAL_GPIO_MODE_INPUT, XY_HAL_GPIO_PULL_UP,
                                        XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_LOW, 0U};
    const xy_hal_spi_config_t config = {
        .mode = XY_HAL_SPI_MODE_0,
        .direction = XY_HAL_SPI_DIR_2LINES,
        .datasize = XY_HAL_SPI_DATASIZE_8BIT,
        .firstbit = XY_HAL_SPI_FIRSTBIT_MSB,
        .nss = XY_HAL_SPI_NSS_SOFT,
        .baudrate_prescaler = SPI_BAUDRATEPRESCALER_32,
        .is_master = 1U,
    };
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_SPI2_CLK_ENABLE();
    if (xy_hal_gpio_init(GPIOD, 5U, &output) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOD, 6U, &output) != XY_HAL_OK ||
        xy_hal_gpio_write(GPIOD, 5U, 0U) != XY_HAL_OK ||
        xy_hal_gpio_write(GPIOD, 6U, 1U) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOD, 4U, &input) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOB, 13U, &alternate) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOB, 14U, &alternate) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOB, 15U, &alternate) != XY_HAL_OK) stop();
    spi2.Instance = SPI2;
    if (xy_hal_spi_init(&spi2, &config) != XY_HAL_OK) stop();
}

int main(void)
{
    xy_nrf24l01_t radio;
    xy_hal_error_t result;
    const xy_nrf24l01_config_t config = {
        .spi = &spi2,
        .transfer = nrf_transfer,
        .set_csn = nrf_csn,
        .set_ce = nrf_ce,
        .timeout_ms = 100U,
    };
    if (xy_hal_sys_init() != XY_HAL_OK || pandora_platform_startup() != 0) stop();
    uart_init();
    nrf_bus_init();
    uart_text("PANDORA NRF24L01 SPI2 PROBE\r\nFIRMWARE_COMMIT " XINYI_FIRMWARE_COMMIT "\r\n");
    result = xy_nrf24l01_probe(&radio, &config);
    if (result != XY_HAL_OK) {
        uart_text("NRF24_NOT_DETECTED error=");
        uart_error(result);
        uart_text(" irq=");
        uart_text(xy_hal_gpio_read(GPIOD, 4U) == 0 ? "LOW" : "HIGH");
        uart_text("\r\n");
        stop();
    }
    uart_text("NRF24_DETECTED status=0x"); uart_hex8(radio.status);
    uart_text(" config=0x"); uart_hex8(radio.config_reg);
    uart_text(" en_aa=0x"); uart_hex8(radio.en_aa);
    uart_text(" setup_aw=0x"); uart_hex8(radio.setup_aw);
    uart_text(" rf_ch=0x"); uart_hex8(radio.rf_ch);
    uart_text(" rf_setup=0x"); uart_hex8(radio.rf_setup);
    uart_text(" fifo=0x"); uart_hex8(radio.fifo_status);
    uart_text(" irq="); uart_text(xy_hal_gpio_read(GPIOD, 4U) == 0 ? "LOW" : "HIGH");
    uart_text("\r\nNRF24_PROBE_DONE\r\n");
    for (;;) xy_hal_delay_ms(1000U);
}
