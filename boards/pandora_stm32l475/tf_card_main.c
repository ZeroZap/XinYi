#include "pandora_platform_startup.h"
#include "stm32l4xx_hal.h"
#include "xy_hal_delay.h"
#include "xy_hal_gpio.h"
#include "xy_hal_spi.h"
#include "xy_hal_sys.h"
#include "xy_hal_uart.h"
#include "xy_sd_spi.h"

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the source commit used for this image"
#endif

static UART_HandleTypeDef uart1;
static SPI_HandleTypeDef spi1;
static xy_sd_spi_t card;

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

static void uart_u64(uint64_t value)
{
    char text[24];
    size_t pos = sizeof(text);
    text[--pos] = '\0';
    do {
        text[--pos] = (char)('0' + value % 10U);
        value /= 10U;
    } while (value != 0U);
    uart_text(&text[pos]);
}

static void fail(const char *marker, xy_hal_error_t error)
{
    uart_text(marker);
    uart_text(" error=");
    uart_u64((uint64_t)(uint32_t)(-error));
    uart_text("\r\n");
    stop();
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

static xy_hal_error_t sd_cs(void *arg, uint8_t level)
{
    (void)arg;
    return xy_hal_gpio_write(GPIOC, 3U, level);
}

static xy_hal_error_t sd_transfer(void *spi, const uint8_t *tx, uint8_t *rx, size_t length,
                                  uint32_t timeout_ms)
{
    return xy_hal_spi_transmit_receive(spi, tx, rx, length, timeout_ms);
}

static void sd_bus_init(void)
{
    const xy_hal_gpio_config_t alternate = {XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_UP,
                                             XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_HIGH,
                                             GPIO_AF5_SPI1};
    const xy_hal_gpio_config_t output = {XY_HAL_GPIO_MODE_OUTPUT, XY_HAL_GPIO_PULL_UP,
                                          XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_HIGH, 0U};
    const xy_hal_spi_config_t config = {
        .mode = XY_HAL_SPI_MODE_0,
        .direction = XY_HAL_SPI_DIR_2LINES,
        .datasize = XY_HAL_SPI_DATASIZE_8BIT,
        .firstbit = XY_HAL_SPI_FIRSTBIT_MSB,
        .nss = XY_HAL_SPI_NSS_SOFT,
        .baudrate_prescaler = SPI_BAUDRATEPRESCALER_256,
        .is_master = 1U,
    };
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();
    if (xy_hal_gpio_init(GPIOA, 5U, &alternate) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 6U, &alternate) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 7U, &alternate) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOC, 3U, &output) != XY_HAL_OK) stop();
    (void)xy_hal_gpio_write(GPIOC, 3U, 1U);
    spi1.Instance = SPI1;
    if (xy_hal_spi_init(&spi1, &config) != XY_HAL_OK) stop();
}

int main(void)
{
    uint8_t block[XY_SD_SPI_BLOCK_SIZE];
    const xy_sd_spi_config_t config = {
        .spi = &spi1,
        .transfer = sd_transfer,
        .set_cs = sd_cs,
        .delay_ms = xy_hal_delay_ms,
        .timeout_ms = 100U,
    };
    if (xy_hal_sys_init() != XY_HAL_OK || pandora_platform_startup() != 0) stop();
    uart_init();
    sd_bus_init();
    uart_text("PANDORA TF SPI1 PROBE\r\nFIRMWARE_COMMIT " XINYI_FIRMWARE_COMMIT "\r\n");
    xy_hal_error_t result = xy_sd_spi_init(&card, &config);
    if (result != XY_HAL_OK) {
        fail("PANDORA_TF_INIT_ERROR", result);
    }
    uart_text("PANDORA_TF_READY type=");
    uart_text(card.type == XY_SD_SPI_CARD_SDHC ? "SDHC" : "SDSC");
    uart_text(" blocks=");
    uart_u64(card.block_count);
    uart_text(" bytes=");
    uart_u64(card.capacity_bytes);
    uart_text("\r\n");
    result = xy_sd_spi_read_block(&card, 0U, block);
    if (result != XY_HAL_OK) {
        fail("PANDORA_TF_READ_ERROR", result);
    }
    uart_text("PANDORA_TF_BLOCK0_OK signature=");
    uart_text(block[510] == 0x55U && block[511] == 0xAAU ? "55AA" : "OTHER");
    uart_text("\r\nPANDORA_TF_PROBE_DONE\r\n");
    for (;;) xy_hal_delay_ms(1000U);
}
