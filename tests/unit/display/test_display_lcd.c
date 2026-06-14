#include "xy_lcd_spi.h"
#include "xy_lcd_i8080.h"
#include "xy_lcd_st7789.h"
#include "xy_hal_error.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#define MAX_SPI_OPS 256
#define MAX_GPIO_OPS 1024

typedef struct {
    const uint8_t *data;
    uint8_t bytes[8];
    size_t len;
    int dma;
} spi_op_t;

typedef struct {
    xy_hal_gpio_port_t port;
    uint8_t pin;
    uint8_t value;
} gpio_op_t;

static spi_op_t spi_ops[MAX_SPI_OPS];
static size_t spi_op_count;
static gpio_op_t gpio_ops[MAX_GPIO_OPS];
static size_t gpio_op_count;
static uint32_t delay_ms_total;
static uint32_t delay_us_total;
static uint32_t read_pattern;

static void reset_logs(void)
{
    memset(spi_ops, 0, sizeof(spi_ops));
    memset(gpio_ops, 0, sizeof(gpio_ops));
    spi_op_count = 0;
    gpio_op_count = 0;
    delay_ms_total = 0;
    delay_us_total = 0;
    read_pattern = 0;
}

xy_hal_error_t xy_hal_spi_transmit(void *spi, const uint8_t *data, size_t len,
                                   uint32_t timeout)
{
    (void)spi;
    (void)timeout;
    assert(spi_op_count < MAX_SPI_OPS);
    spi_ops[spi_op_count].data = data;
    spi_ops[spi_op_count].len = len;
    spi_ops[spi_op_count].dma = 0;
    size_t copy = len < sizeof(spi_ops[spi_op_count].bytes)
                    ? len : sizeof(spi_ops[spi_op_count].bytes);
    memcpy(spi_ops[spi_op_count].bytes, data, copy);
    spi_op_count++;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_receive(void *spi, uint8_t *data, size_t len,
                                  uint32_t timeout)
{
    (void)spi;
    (void)timeout;
    memset(data, 0x5A, len);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_transmit_receive(void *spi, const uint8_t *tx_data,
                                           uint8_t *rx_data, size_t len,
                                           uint32_t timeout)
{
    (void)spi;
    (void)timeout;
    memcpy(rx_data, tx_data, len);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_transmit_dma(void *spi, const uint8_t *data, size_t len)
{
    (void)spi;
    assert(spi_op_count < MAX_SPI_OPS);
    spi_ops[spi_op_count].data = data;
    spi_ops[spi_op_count].len = len;
    spi_ops[spi_op_count].dma = 1;
    size_t copy = len < sizeof(spi_ops[spi_op_count].bytes)
                    ? len : sizeof(spi_ops[spi_op_count].bytes);
    memcpy(spi_ops[spi_op_count].bytes, data, copy);
    spi_op_count++;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_receive_dma(void *spi, uint8_t *data, size_t len)
{
    (void)spi;
    memset(data, 0xA5, len);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_transmit_receive_dma(void *spi,
                                               const uint8_t *tx_data,
                                               uint8_t *rx_data, size_t len)
{
    (void)spi;
    memcpy(rx_data, tx_data, len);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_write(xy_hal_gpio_port_t port, uint8_t pin, uint8_t value)
{
    assert(gpio_op_count < MAX_GPIO_OPS);
    gpio_ops[gpio_op_count].port = port;
    gpio_ops[gpio_op_count].pin = pin;
    gpio_ops[gpio_op_count].value = value;
    gpio_op_count++;
    return XY_HAL_OK;
}

int32_t xy_hal_gpio_read(xy_hal_gpio_port_t port, uint8_t pin)
{
    (void)port;
    return (read_pattern >> pin) & 1U;
}

void xy_hal_delay_ms(uint32_t ms)
{
    delay_ms_total += ms;
}

void xy_hal_delay_us(uint32_t us)
{
    delay_us_total += us;
}

static xy_lcd_spi_config_t make_spi_config(void)
{
    xy_lcd_spi_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.base.width = 4;
    cfg.base.height = 3;
    cfg.base.color_fmt = XY_LCD_COLOR_FORMAT_RGB565;
    cfg.base.rotation = XY_LCD_ROTATION_0;
    cfg.spi_handle = (void *)0x1234;
    cfg.dc_port = (xy_hal_gpio_port_t)0x10;
    cfg.cs_port = (xy_hal_gpio_port_t)0x20;
    cfg.rst_port = (xy_hal_gpio_port_t)0x30;
    cfg.bl_port = (xy_hal_gpio_port_t)0x40;
    cfg.dc_pin = 1;
    cfg.cs_pin = 2;
    cfg.rst_pin = 3;
    cfg.bl_pin = 4;
    return cfg;
}

static void test_spi_init_window_and_pixel_endian(void)
{
    xy_lcd_spi_device_t lcd;
    xy_lcd_spi_config_t cfg = make_spi_config();
    uint16_t pixels[] = {0x1234, 0xABCD};

    assert(xy_lcd_spi_init(&lcd, &cfg) == XY_ERR_OK);
    assert(lcd.initialized);
    assert(lcd.base.width == 4U);
    assert(lcd.base.height == 3U);
    assert(lcd.base.ops == &xy_lcd_spi_ops);

    reset_logs();
    xy_lcd_spi_set_window(&lcd, 1, 2, 3, 4);
    assert(spi_op_count == 7U);
    assert(spi_ops[0].bytes[0] == 0x2A);
    assert(spi_ops[1].bytes[0] == 0x00 && spi_ops[1].bytes[1] == 0x01);
    assert(spi_ops[2].bytes[0] == 0x00 && spi_ops[2].bytes[1] == 0x03);
    assert(spi_ops[3].bytes[0] == 0x2B);
    assert(spi_ops[4].bytes[0] == 0x00 && spi_ops[4].bytes[1] == 0x02);
    assert(spi_ops[5].bytes[0] == 0x00 && spi_ops[5].bytes[1] == 0x05);
    assert(spi_ops[6].bytes[0] == 0x2C);

    reset_logs();
    xy_lcd_spi_write_pixel(&lcd, pixels, 2);
    assert(spi_op_count == 1U);
    assert(spi_ops[0].len == 4U);
    assert(spi_ops[0].bytes[0] == 0x12);
    assert(spi_ops[0].bytes[1] == 0x34);
    assert(spi_ops[0].bytes[2] == 0xAB);
    assert(spi_ops[0].bytes[3] == 0xCD);

    xy_lcd_spi_deinit(&lcd);
    assert(!lcd.initialized);
}

static void test_spi_reset_backlight_and_dma(void)
{
    xy_lcd_spi_device_t lcd;
    xy_lcd_spi_config_t cfg = make_spi_config();
    uint8_t payload[] = {1, 2, 3};

    assert(xy_lcd_spi_init(&lcd, &cfg) == XY_ERR_OK);

    reset_logs();
    xy_lcd_spi_reset(&lcd);
    assert(delay_ms_total == 140U);
    assert(gpio_op_count == 3U);
    assert(gpio_ops[0].pin == cfg.rst_pin && gpio_ops[0].value == 1U);
    assert(gpio_ops[1].pin == cfg.rst_pin && gpio_ops[1].value == 0U);
    assert(gpio_ops[2].pin == cfg.rst_pin && gpio_ops[2].value == 1U);

    reset_logs();
    xy_lcd_spi_set_backlight(&lcd, 0);
    xy_lcd_spi_set_backlight(&lcd, 100);
    assert(gpio_op_count == 2U);
    assert(gpio_ops[0].pin == cfg.bl_pin && gpio_ops[0].value == 0U);
    assert(gpio_ops[1].pin == cfg.bl_pin && gpio_ops[1].value == 1U);

    lcd.use_dma = true;
    reset_logs();
    xy_lcd_spi_write_data(&lcd, payload, sizeof(payload));
    assert(spi_op_count == 1U);
    assert(spi_ops[0].dma == 1);
    assert(spi_ops[0].len == sizeof(payload));

    xy_lcd_spi_deinit(&lcd);
}

static xy_lcd_i8080_config_t make_i8080_config(void)
{
    xy_lcd_i8080_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.base.width = 5;
    cfg.base.height = 4;
    cfg.base.color_fmt = XY_LCD_COLOR_FORMAT_RGB565;
    cfg.data_width = XY_LCD_I8080_16BIT;
    for (uint8_t i = 0; i < 16; i++) {
        cfg.db_ports[i] = (xy_hal_gpio_port_t)(uintptr_t)(0x100U + i);
    }
    cfg.db0_pin = 0;
    cfg.db1_pin = 1;
    cfg.db2_pin = 2;
    cfg.db3_pin = 3;
    cfg.db4_pin = 4;
    cfg.db5_pin = 5;
    cfg.db6_pin = 6;
    cfg.db7_pin = 7;
    cfg.db8_pin = 8;
    cfg.db9_pin = 9;
    cfg.db10_pin = 10;
    cfg.db11_pin = 11;
    cfg.db12_pin = 12;
    cfg.db13_pin = 13;
    cfg.db14_pin = 14;
    cfg.db15_pin = 15;
    cfg.wr_port = (xy_hal_gpio_port_t)0x50;
    cfg.rd_port = (xy_hal_gpio_port_t)0x60;
    cfg.cs_port = (xy_hal_gpio_port_t)0x70;
    cfg.rs_port = (xy_hal_gpio_port_t)0x80;
    cfg.rst_port = (xy_hal_gpio_port_t)0x90;
    cfg.wr_pin = 20;
    cfg.rd_pin = 21;
    cfg.cs_pin = 22;
    cfg.rs_pin = 23;
    cfg.rst_pin = 24;
    return cfg;
}

static void test_i8080_bus_write_read_and_window(void)
{
    xy_lcd_i8080_device_t lcd;
    xy_lcd_i8080_config_t cfg = make_i8080_config();

    assert(xy_lcd_i8080_init(&lcd, &cfg) == XY_ERR_OK);
    assert(lcd.initialized);
    assert(lcd.base.ops == &xy_lcd_i8080_ops);

    reset_logs();
    xy_lcd_i8080_write_data16(&lcd, 0xA55A);
    assert(gpio_op_count >= 20U);
    assert(gpio_ops[0].pin == cfg.rs_pin && gpio_ops[0].value == 1U);
    assert(gpio_ops[1].pin == cfg.cs_pin && gpio_ops[1].value == 0U);
    assert(gpio_ops[2 + 0].pin == 0U && gpio_ops[2 + 0].value == 0U);
    assert(gpio_ops[2 + 1].pin == 1U && gpio_ops[2 + 1].value == 1U);
    assert(gpio_ops[2 + 8].pin == 8U && gpio_ops[2 + 8].value == 1U);
    assert(gpio_ops[18].pin == cfg.wr_pin && gpio_ops[18].value == 0U);
    assert(gpio_ops[19].pin == cfg.wr_pin && gpio_ops[19].value == 1U);
    assert(gpio_ops[20].pin == cfg.cs_pin && gpio_ops[20].value == 1U);
    assert(delay_us_total == 1U);

    read_pattern = 0xBEEF;
    assert(xy_lcd_i8080_read_data(&lcd) == 0xBEEFU);

    reset_logs();
    xy_lcd_i8080_set_window(&lcd, 1, 1, 2, 2);
    assert(gpio_op_count > 0U);

    xy_lcd_i8080_deinit(&lcd);
    assert(!lcd.initialized);
}

static xy_lcd_st7789_config_t make_st7789_config(void)
{
    xy_lcd_st7789_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.spi = make_spi_config();
    cfg.spi.base.width = 8;
    cfg.spi.base.height = 6;
    cfg.offset_x = 2;
    cfg.offset_y = 3;
    cfg.rgb_order = true;
    return cfg;
}

static void test_st7789_offsets_rotation_and_ops(void)
{
    xy_lcd_st7789_device_t lcd;
    xy_lcd_st7789_config_t cfg = make_st7789_config();

    assert(xy_lcd_st7789_init(&lcd, &cfg) == XY_ERR_OK);
    assert(lcd.initialized);
    assert(lcd.spi_dev.base.ops == &xy_lcd_st7789_ops);

    reset_logs();
    xy_lcd_st7789_set_column(&lcd, 4, 3);
    assert(spi_op_count == 5U);
    assert(spi_ops[0].bytes[0] == ST7789_CMD_CASET);
    assert(spi_ops[1].bytes[0] == 0x00 && spi_ops[2].bytes[0] == 0x06);
    assert(spi_ops[3].bytes[0] == 0x00 && spi_ops[4].bytes[0] == 0x08);

    reset_logs();
    xy_lcd_st7789_set_row(&lcd, 5, 2);
    assert(spi_op_count == 5U);
    assert(spi_ops[0].bytes[0] == ST7789_CMD_RASET);
    assert(spi_ops[1].bytes[0] == 0x00 && spi_ops[2].bytes[0] == 0x08);
    assert(spi_ops[3].bytes[0] == 0x00 && spi_ops[4].bytes[0] == 0x09);

    reset_logs();
    xy_lcd_st7789_draw_pixel(&lcd, 1, 2, 0x1357);
    assert(spi_ops[0].bytes[0] == ST7789_CMD_CASET);
    assert(spi_ops[1].bytes[0] == 0x00 && spi_ops[2].bytes[0] == 0x03);
    assert(spi_ops[3].bytes[0] == 0x00 && spi_ops[4].bytes[0] == 0x03);
    assert(spi_ops[5].bytes[0] == ST7789_CMD_RASET);
    assert(spi_ops[6].bytes[0] == 0x00 && spi_ops[7].bytes[0] == 0x05);
    assert(spi_ops[8].bytes[0] == 0x00 && spi_ops[9].bytes[0] == 0x05);
    assert(spi_ops[11].bytes[0] == 0x13 && spi_ops[11].bytes[1] == 0x57);

    reset_logs();
    xy_lcd_st7789_set_inversion(&lcd, true);
    xy_lcd_st7789_set_inversion(&lcd, false);
    assert(spi_ops[0].bytes[0] == ST7789_CMD_INVON);
    assert(spi_ops[1].bytes[0] == ST7789_CMD_INVOFF);

    xy_lcd_st7789_deinit(&lcd);
    assert(!lcd.initialized);
}

int main(void)
{
    test_spi_init_window_and_pixel_endian();
    test_spi_reset_backlight_and_dma();
    test_i8080_bus_write_read_and_window();
    test_st7789_offsets_rotation_and_ops();
    return 0;
}
