#include "stm32l4xx_hal.h"
#include "xy_hal_gpio.h"
#include "xy_hal_spi.h"
#include "xy_hal_uart.h"
#include "xy_lcd_st7789.h"

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the source commit used for this image"
#endif

static UART_HandleTypeDef uart1;
static SPI_HandleTypeDef spi3;
static xy_lcd_st7789_device_t lcd;

void _init(void) {}
void _fini(void) {}
void SysTick_Handler(void) { HAL_IncTick(); }
void xy_hal_delay_ms(uint32_t ms) { HAL_Delay(ms); }
void xy_hal_delay_us(uint32_t us)
{
    uint32_t cycles = (SystemCoreClock / 1000000U) * us;
    while (cycles-- != 0U) {
        __NOP();
    }
}

static void backlight(uint8_t on)
{
    (void)xy_hal_gpio_write(GPIOB, 7U, on);
}

static void fail(void)
{
    backlight(0U);
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
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) fail();
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM = 1U;
    osc.PLL.PLLN = 20U;
    osc.PLL.PLLP = RCC_PLLP_DIV7;
    osc.PLL.PLLQ = RCC_PLLQ_DIV2;
    osc.PLL.PLLR = RCC_PLLR_DIV2;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) fail();
    clk.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 |
                    RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_4) != HAL_OK) fail();
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
    if (xy_hal_uart_init(&uart1, &uart) != XY_HAL_OK) fail();
}

static void display_bus_init(void)
{
    const xy_hal_gpio_config_t alternate = {
        XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_NONE, XY_HAL_GPIO_OTYPE_PP,
        XY_HAL_GPIO_SPEED_VERY_HIGH, GPIO_AF6_SPI3,
    };
    xy_hal_gpio_config_t output = {
        .mode = XY_HAL_GPIO_MODE_OUTPUT,
        .otype = XY_HAL_GPIO_OTYPE_PP,
        .pull = XY_HAL_GPIO_PULL_NONE,
        .speed = XY_HAL_GPIO_SPEED_VERY_HIGH,
    };
    xy_hal_spi_config_t config = {
        .mode = XY_HAL_SPI_MODE_0,
        .direction = XY_HAL_SPI_DIR_1LINE,
        .datasize = XY_HAL_SPI_DATASIZE_8BIT,
        .firstbit = XY_HAL_SPI_FIRSTBIT_MSB,
        .nss = XY_HAL_SPI_NSS_SOFT,
        .baudrate_prescaler = SPI_BAUDRATEPRESCALER_8,
        .is_master = 1U,
    };

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_SPI3_CLK_ENABLE();
    if (xy_hal_gpio_init(GPIOB, 3U, &alternate) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOB, 5U, &alternate) != XY_HAL_OK) fail();
    if (xy_hal_gpio_init(GPIOB, 4U, &output) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOD, 7U, &output) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOB, 6U, &output) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOB, 7U, &output) != XY_HAL_OK) fail();
    (void)xy_hal_gpio_write(GPIOD, 7U, 1U);
    backlight(0U);
    spi3.Instance = SPI3;
    if (xy_hal_spi_init(&spi3, &config) != XY_HAL_OK) fail();
}

static void fill(uint16_t color, const char *marker)
{
    if (xy_lcd_st7789_fill_checked(&lcd, 0U, 0U, 240U, 240U, color) != XY_ERR_OK) {
        uart_text("PANDORA_ST7789_IO_ERROR\r\n");
        fail();
    }
    uart_text(marker);
    HAL_Delay(500U);
}

int main(void)
{
    xy_lcd_st7789_config_t config = {0};
    HAL_Init();
    clock_init();
    uart_init();
    display_bus_init();
    uart_text("PANDORA ST7789 SPI3 READY\r\nFIRMWARE_COMMIT " XINYI_FIRMWARE_COMMIT "\r\n");

    config.spi.base.width = 240U;
    config.spi.base.height = 240U;
    config.spi.base.color_fmt = XY_LCD_COLOR_FORMAT_RGB565;
    config.spi.base.rotation = XY_LCD_ROTATION_0;
    config.spi.base.disable_framebuffer = true;
    config.spi.spi_handle = &spi3;
    config.spi.spi_speed = 10000000U;
    config.spi.spi_mode = 0U;
    config.spi.dc_port = GPIOB;
    config.spi.dc_pin = 4U;
    config.spi.cs_port = GPIOD;
    config.spi.cs_pin = 7U;
    config.spi.rst_port = GPIOB;
    config.spi.rst_pin = 6U;
    config.spi.bl_port = GPIOB;
    config.spi.bl_pin = 7U;
    /* This Pandora panel accepts RGB565 when MADCTL BGR is clear. */
    config.rgb_order = true;
    config.invert_on_init = true;
    if (xy_lcd_st7789_init(&lcd, &config) != XY_ERR_OK) {
        uart_text("PANDORA_ST7789_INIT_ERROR\r\n");
        fail();
    }
    backlight(1U);
    fill(0xF800U, "PANDORA_ST7789_COLOR RED\r\n");
    fill(0x07E0U, "PANDORA_ST7789_COLOR GREEN\r\n");
    fill(0x001FU, "PANDORA_ST7789_COLOR BLUE\r\n");
    fill(0xFFFFU, "PANDORA_ST7789_COLOR WHITE\r\n");
    fill(0x0000U, "PANDORA_ST7789_COLOR BLACK\r\n");
    if (xy_lcd_st7789_fill_checked(&lcd, 0U, 0U, 120U, 120U, 0xF800U) != XY_ERR_OK ||
        xy_lcd_st7789_fill_checked(&lcd, 120U, 0U, 120U, 120U, 0x07E0U) != XY_ERR_OK ||
        xy_lcd_st7789_fill_checked(&lcd, 0U, 120U, 120U, 120U, 0x001FU) != XY_ERR_OK ||
        xy_lcd_st7789_fill_checked(&lcd, 120U, 120U, 120U, 120U, 0xFFFFU) != XY_ERR_OK ||
        xy_lcd_st7789_fill_checked(&lcd, 56U, 56U, 128U, 128U, 0x0000U) != XY_ERR_OK) {
        uart_text("PANDORA_ST7789_PATTERN_ERROR\r\n");
        fail();
    }
    uart_text("PANDORA_ST7789_PATTERN_DONE\r\nPANDORA_ST7789_FINAL_SAFE backlight=ON pattern=HELD\r\n");
    for (;;) HAL_Delay(1000U);
}