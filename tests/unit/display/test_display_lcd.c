#include "xy_lcd_spi.h"
#include "xy_lcd_i8080.h"
#include "xy_lcd_st7789.h"
#include "xy_hal_error.h"
#include "unity.h"

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

void setUp(void)
{
}

void tearDown(void)
{
}

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
    TEST_ASSERT_LESS_THAN_UINT32(MAX_SPI_OPS, spi_op_count);
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
    TEST_ASSERT_LESS_THAN_UINT32(MAX_SPI_OPS, spi_op_count);
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
    TEST_ASSERT_LESS_THAN_UINT32(MAX_GPIO_OPS, gpio_op_count);
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

    TEST_ASSERT_EQUAL_INT(XY_ERR_OK, xy_lcd_spi_init(&lcd, &cfg));
    TEST_ASSERT_TRUE(lcd.initialized);
    TEST_ASSERT_EQUAL_UINT16(4U, lcd.base.width);
    TEST_ASSERT_EQUAL_UINT16(3U, lcd.base.height);
    TEST_ASSERT_EQUAL_PTR(&xy_lcd_spi_ops, lcd.base.ops);

    reset_logs();
    xy_lcd_spi_set_window(&lcd, 1, 2, 3, 4);
    TEST_ASSERT_EQUAL_UINT32(7U, spi_op_count);
    TEST_ASSERT_EQUAL_HEX8(0x2A, spi_ops[0].bytes[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00, spi_ops[1].bytes[0]);
    TEST_ASSERT_EQUAL_HEX8(0x01, spi_ops[1].bytes[1]);
    TEST_ASSERT_EQUAL_HEX8(0x00, spi_ops[2].bytes[0]);
    TEST_ASSERT_EQUAL_HEX8(0x03, spi_ops[2].bytes[1]);
    TEST_ASSERT_EQUAL_HEX8(0x2B, spi_ops[3].bytes[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00, spi_ops[4].bytes[0]);
    TEST_ASSERT_EQUAL_HEX8(0x02, spi_ops[4].bytes[1]);
    TEST_ASSERT_EQUAL_HEX8(0x00, spi_ops[5].bytes[0]);
    TEST_ASSERT_EQUAL_HEX8(0x05, spi_ops[5].bytes[1]);
    TEST_ASSERT_EQUAL_HEX8(0x2C, spi_ops[6].bytes[0]);

    reset_logs();
    xy_lcd_spi_write_pixel(&lcd, pixels, 2);
    TEST_ASSERT_EQUAL_UINT32(1U, spi_op_count);
    TEST_ASSERT_EQUAL_UINT32(4U, spi_ops[0].len);
    TEST_ASSERT_EQUAL_HEX8(0x12, spi_ops[0].bytes[0]);
    TEST_ASSERT_EQUAL_HEX8(0x34, spi_ops[0].bytes[1]);
    TEST_ASSERT_EQUAL_HEX8(0xAB, spi_ops[0].bytes[2]);
    TEST_ASSERT_EQUAL_HEX8(0xCD, spi_ops[0].bytes[3]);

    xy_lcd_spi_deinit(&lcd);
    TEST_ASSERT_FALSE(lcd.initialized);
}

static void test_spi_reset_backlight_and_dma(void)
{
    xy_lcd_spi_device_t lcd;
    xy_lcd_spi_config_t cfg = make_spi_config();
    uint8_t payload[] = {1, 2, 3};

    TEST_ASSERT_EQUAL_INT(XY_ERR_OK, xy_lcd_spi_init(&lcd, &cfg));

    reset_logs();
    xy_lcd_spi_reset(&lcd);
    TEST_ASSERT_EQUAL_UINT32(140U, delay_ms_total);
    TEST_ASSERT_EQUAL_UINT32(3U, gpio_op_count);
    TEST_ASSERT_EQUAL_UINT8(cfg.rst_pin, gpio_ops[0].pin);
    TEST_ASSERT_EQUAL_UINT8(1U, gpio_ops[0].value);
    TEST_ASSERT_EQUAL_UINT8(cfg.rst_pin, gpio_ops[1].pin);
    TEST_ASSERT_EQUAL_UINT8(0U, gpio_ops[1].value);
    TEST_ASSERT_EQUAL_UINT8(cfg.rst_pin, gpio_ops[2].pin);
    TEST_ASSERT_EQUAL_UINT8(1U, gpio_ops[2].value);

    reset_logs();
    xy_lcd_spi_set_backlight(&lcd, 0);
    xy_lcd_spi_set_backlight(&lcd, 100);
    TEST_ASSERT_EQUAL_UINT32(2U, gpio_op_count);
    TEST_ASSERT_EQUAL_UINT8(cfg.bl_pin, gpio_ops[0].pin);
    TEST_ASSERT_EQUAL_UINT8(0U, gpio_ops[0].value);
    TEST_ASSERT_EQUAL_UINT8(cfg.bl_pin, gpio_ops[1].pin);
    TEST_ASSERT_EQUAL_UINT8(1U, gpio_ops[1].value);

    lcd.use_dma = true;
    reset_logs();
    xy_lcd_spi_write_data(&lcd, payload, sizeof(payload));
    TEST_ASSERT_EQUAL_UINT32(1U, spi_op_count);
    TEST_ASSERT_EQUAL_INT(1, spi_ops[0].dma);
    TEST_ASSERT_EQUAL_UINT32(sizeof(payload), spi_ops[0].len);

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

    TEST_ASSERT_EQUAL_INT(XY_ERR_OK, xy_lcd_i8080_init(&lcd, &cfg));
    TEST_ASSERT_TRUE(lcd.initialized);
    TEST_ASSERT_EQUAL_PTR(&xy_lcd_i8080_ops, lcd.base.ops);

    reset_logs();
    xy_lcd_i8080_write_data16(&lcd, 0xA55A);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(20U, gpio_op_count);
    TEST_ASSERT_EQUAL_UINT8(cfg.rs_pin, gpio_ops[0].pin);
    TEST_ASSERT_EQUAL_UINT8(1U, gpio_ops[0].value);
    TEST_ASSERT_EQUAL_UINT8(cfg.cs_pin, gpio_ops[1].pin);
    TEST_ASSERT_EQUAL_UINT8(0U, gpio_ops[1].value);
    TEST_ASSERT_EQUAL_UINT8(0U, gpio_ops[2 + 0].pin);
    TEST_ASSERT_EQUAL_UINT8(0U, gpio_ops[2 + 0].value);
    TEST_ASSERT_EQUAL_UINT8(1U, gpio_ops[2 + 1].pin);
    TEST_ASSERT_EQUAL_UINT8(1U, gpio_ops[2 + 1].value);
    TEST_ASSERT_EQUAL_UINT8(8U, gpio_ops[2 + 8].pin);
    TEST_ASSERT_EQUAL_UINT8(1U, gpio_ops[2 + 8].value);
    TEST_ASSERT_EQUAL_UINT8(cfg.wr_pin, gpio_ops[18].pin);
    TEST_ASSERT_EQUAL_UINT8(0U, gpio_ops[18].value);
    TEST_ASSERT_EQUAL_UINT8(cfg.wr_pin, gpio_ops[19].pin);
    TEST_ASSERT_EQUAL_UINT8(1U, gpio_ops[19].value);
    TEST_ASSERT_EQUAL_UINT8(cfg.cs_pin, gpio_ops[20].pin);
    TEST_ASSERT_EQUAL_UINT8(1U, gpio_ops[20].value);
    TEST_ASSERT_EQUAL_UINT32(1U, delay_us_total);

    read_pattern = 0xBEEF;
    TEST_ASSERT_EQUAL_HEX16(0xBEEFU, xy_lcd_i8080_read_data(&lcd));

    reset_logs();
    xy_lcd_i8080_set_window(&lcd, 1, 1, 2, 2);
    TEST_ASSERT_GREATER_THAN_UINT32(0U, gpio_op_count);

    xy_lcd_i8080_deinit(&lcd);
    TEST_ASSERT_FALSE(lcd.initialized);
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

    TEST_ASSERT_EQUAL_INT(XY_ERR_OK, xy_lcd_st7789_init(&lcd, &cfg));
    TEST_ASSERT_TRUE(lcd.initialized);
    TEST_ASSERT_EQUAL_PTR(&xy_lcd_st7789_ops, lcd.spi_dev.base.ops);

    reset_logs();
    xy_lcd_st7789_set_column(&lcd, 4, 3);
    TEST_ASSERT_EQUAL_UINT32(5U, spi_op_count);
    TEST_ASSERT_EQUAL_HEX8(ST7789_CMD_CASET, spi_ops[0].bytes[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00, spi_ops[1].bytes[0]);
    TEST_ASSERT_EQUAL_HEX8(0x06, spi_ops[2].bytes[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00, spi_ops[3].bytes[0]);
    TEST_ASSERT_EQUAL_HEX8(0x08, spi_ops[4].bytes[0]);

    reset_logs();
    xy_lcd_st7789_set_row(&lcd, 5, 2);
    TEST_ASSERT_EQUAL_UINT32(5U, spi_op_count);
    TEST_ASSERT_EQUAL_HEX8(ST7789_CMD_RASET, spi_ops[0].bytes[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00, spi_ops[1].bytes[0]);
    TEST_ASSERT_EQUAL_HEX8(0x08, spi_ops[2].bytes[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00, spi_ops[3].bytes[0]);
    TEST_ASSERT_EQUAL_HEX8(0x09, spi_ops[4].bytes[0]);

    reset_logs();
    xy_lcd_st7789_draw_pixel(&lcd, 1, 2, 0x1357);
    TEST_ASSERT_EQUAL_HEX8(ST7789_CMD_CASET, spi_ops[0].bytes[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00, spi_ops[1].bytes[0]);
    TEST_ASSERT_EQUAL_HEX8(0x03, spi_ops[2].bytes[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00, spi_ops[3].bytes[0]);
    TEST_ASSERT_EQUAL_HEX8(0x03, spi_ops[4].bytes[0]);
    TEST_ASSERT_EQUAL_HEX8(ST7789_CMD_RASET, spi_ops[5].bytes[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00, spi_ops[6].bytes[0]);
    TEST_ASSERT_EQUAL_HEX8(0x05, spi_ops[7].bytes[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00, spi_ops[8].bytes[0]);
    TEST_ASSERT_EQUAL_HEX8(0x05, spi_ops[9].bytes[0]);
    TEST_ASSERT_EQUAL_HEX8(0x13, spi_ops[11].bytes[0]);
    TEST_ASSERT_EQUAL_HEX8(0x57, spi_ops[11].bytes[1]);

    reset_logs();
    xy_lcd_st7789_set_inversion(&lcd, true);
    xy_lcd_st7789_set_inversion(&lcd, false);
    TEST_ASSERT_EQUAL_HEX8(ST7789_CMD_INVON, spi_ops[0].bytes[0]);
    TEST_ASSERT_EQUAL_HEX8(ST7789_CMD_INVOFF, spi_ops[1].bytes[0]);

    xy_lcd_st7789_deinit(&lcd);
    TEST_ASSERT_FALSE(lcd.initialized);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_spi_init_window_and_pixel_endian);
    RUN_TEST(test_spi_reset_backlight_and_dma);
    RUN_TEST(test_i8080_bus_write_read_and_window);
    RUN_TEST(test_st7789_offsets_rotation_and_ops);
    return UNITY_END();
}
