#include "xy_oled_ssd1306.h"
#include "xy_ws2812.h"
#include "xy_device_core.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_I2C_WRITES 64
#define MAX_I2C_BYTES 2048
#define MAX_GPIO_EVENTS 4096

typedef struct {
    uint8_t bytes[MAX_I2C_BYTES];
    size_t len;
} i2c_write_t;

typedef struct {
    uint8_t pin;
    uint8_t value;
} gpio_event_t;

static i2c_write_t i2c_writes[MAX_I2C_WRITES];
static size_t i2c_write_count;
static uint32_t delay_ms_total;
static gpio_event_t gpio_events[MAX_GPIO_EVENTS];
static size_t gpio_event_count;

static void reset_i2c_log(void)
{
    memset(i2c_writes, 0, sizeof(i2c_writes));
    i2c_write_count = 0;
    delay_ms_total = 0;
}

static void reset_gpio_log(void)
{
    memset(gpio_events, 0, sizeof(gpio_events));
    gpio_event_count = 0;
}

xy_error_t xy_i2c_device_init(xy_i2c_device_t *dev, void *i2c_handle,
                              uint16_t addr, uint32_t timeout)
{
    if (!dev || !i2c_handle) {
        return XY_DEVICE_INVALID_PARAM;
    }
    memset(dev, 0, sizeof(*dev));
    dev->i2c_handle = i2c_handle;
    dev->dev_addr = addr;
    dev->timeout = timeout;
    dev->base.initialized = 1;
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_read_reg(xy_i2c_device_t *dev, uint8_t reg,
                                  uint8_t *data, size_t len)
{
    (void)dev;
    (void)reg;
    memset(data, 0, len);
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_write_reg(xy_i2c_device_t *dev, uint8_t reg,
                                   const uint8_t *data, size_t len)
{
    (void)dev;
    (void)reg;
    (void)data;
    (void)len;
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_read(xy_i2c_device_t *dev, uint8_t *data, size_t len)
{
    (void)dev;
    memset(data, 0, len);
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_write(xy_i2c_device_t *dev, const uint8_t *data, size_t len)
{
    (void)dev;
    assert(i2c_write_count < MAX_I2C_WRITES);
    assert(len <= MAX_I2C_BYTES);
    memcpy(i2c_writes[i2c_write_count].bytes, data, len);
    i2c_writes[i2c_write_count].len = len;
    i2c_write_count++;
    return XY_DEVICE_OK;
}

void xy_hal_delay_ms(uint32_t ms)
{
    delay_ms_total += ms;
}

int xy_device_registry_register(xy_device_t *dev)
{
    (void)dev;
    return XY_DEVICE_OK;
}

int xy_device_registry_unregister(xy_device_t *dev)
{
    (void)dev;
    return XY_DEVICE_OK;
}

static void gpio_callback(uint8_t pin, uint8_t value)
{
    assert(gpio_event_count < MAX_GPIO_EVENTS);
    gpio_events[gpio_event_count].pin = pin;
    gpio_events[gpio_event_count].value = value;
    gpio_event_count++;
}

static void test_oled_init_pixel_line_refresh(void)
{
    xy_oled_ssd1306_t oled;

    reset_i2c_log();
    assert(xy_oled_ssd1306_init(NULL, (void *)0x1, 128, 64) == XY_DEVICE_INVALID_PARAM);
    assert(xy_oled_ssd1306_init(&oled, NULL, 128, 64) == XY_DEVICE_INVALID_PARAM);
    assert(xy_oled_ssd1306_init(&oled, (void *)0x1, 128, 64) == XY_DEVICE_OK);
    assert(oled.width == 128U);
    assert(oled.height == 64U);
    assert(oled.i2c_dev.dev_addr == 0x3CU);
    assert(oled.buffer != NULL);
    assert(i2c_write_count == 14U);
    assert(i2c_writes[0].len == 2U);
    assert(i2c_writes[0].bytes[0] == 0x00U);
    assert(i2c_writes[0].bytes[1] == 0xAEU);
    assert(delay_ms_total == 14U);

    xy_oled_ssd1306_draw_pixel(&oled, 0, 0, true);
    xy_oled_ssd1306_draw_pixel(&oled, 1, 7, true);
    xy_oled_ssd1306_draw_pixel(&oled, 2, 8, true);
    xy_oled_ssd1306_draw_pixel(&oled, -1, 0, true);
    xy_oled_ssd1306_draw_pixel(&oled, 128, 0, true);
    assert(oled.buffer[0] == 0x01U);
    assert(oled.buffer[1] == 0x80U);
    assert(oled.buffer[128 + 2] == 0x01U);
    xy_oled_ssd1306_draw_pixel(&oled, 1, 7, false);
    assert(oled.buffer[1] == 0x00U);

    xy_oled_ssd1306_draw_line(&oled, 0, 0, 3, 0, true);
    assert((oled.buffer[0] & 0x01U) != 0U);
    assert((oled.buffer[1] & 0x01U) != 0U);
    assert((oled.buffer[2] & 0x01U) != 0U);
    assert((oled.buffer[3] & 0x01U) != 0U);

    reset_i2c_log();
    xy_oled_ssd1306_refresh(&oled);
    assert(i2c_write_count == 4U);
    assert(i2c_writes[0].len == 6U);
    assert(i2c_writes[0].bytes[0] == 0x00U);
    assert(i2c_writes[0].bytes[1] == 0x21U);
    assert(i2c_writes[0].bytes[5] == 127U);
    assert(i2c_writes[1].bytes[1] == 0x22U);
    assert(i2c_writes[1].bytes[5] == 7U);
    assert(i2c_writes[2].len == 1U && i2c_writes[2].bytes[0] == 0x40U);
    assert(i2c_writes[3].len == 1024U);

    xy_oled_ssd1306_clear(&oled);
    for (size_t i = 0; i < 1024U; i++) {
        assert(oled.buffer[i] == 0U);
    }
    free(oled.buffer);
}

static xy_ws2812_config_t make_ws_config(xy_ws2812_color_order_t order)
{
    xy_ws2812_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.num_leds = 3;
    cfg.led_type = WS2812_TYPE_WS2812B;
    cfg.data_pin = 7;
    cfg.color_order = order;
    cfg.brightness = 255;
    return cfg;
}

static void test_ws2812_pixels_show_and_order(void)
{
    xy_ws2812_handle_t strip;
    xy_ws2812_config_t cfg = make_ws_config(WS2812_COLOR_GRB);
    xy_ws2812_color_t c = { .r = 0x10, .g = 0x20, .b = 0x40, .w = 0x80 };

    assert(xy_ws2812_init(NULL, &cfg) == WS2812_ERROR_INVALID_PARAM);
    assert(xy_ws2812_init(&strip, NULL) == WS2812_ERROR_INVALID_PARAM);
    cfg.num_leds = 0;
    assert(xy_ws2812_init(&strip, &cfg) == WS2812_ERROR_INVALID_PARAM);
    cfg.num_leds = 3;

    assert(xy_ws2812_init(&strip, &cfg) == WS2812_OK);
    assert(strip.initialized);
    assert(strip.buffer_size == 9U);
    assert(strip.base.initialized == 1U);

    xy_ws2812_set_pixel(&strip, 1, c);
    assert(strip.dirty);
    assert(strip.leds[1].r == 0x0FU);
    assert(strip.leds[1].g == 0x1FU);
    assert(strip.leds[1].b == 0x3FU);
    xy_ws2812_set_pixel(&strip, 99, c);
    assert(xy_ws2812_get_pixel(&strip, 99).r == 0U);

    xy_ws2812_set_gpio_callback(gpio_callback);
    reset_gpio_log();
    xy_ws2812_show(&strip);
    assert(!strip.dirty);
    assert(strip.tx_buffer[3] == 0x1FU);
    assert(strip.tx_buffer[4] == 0x0FU);
    assert(strip.tx_buffer[5] == 0x3FU);
    assert(gpio_event_count == (size_t)(strip.buffer_size * 16U + 1U));
    assert(gpio_events[0].pin == cfg.data_pin);
    assert(gpio_events[0].value == 1U);
    assert(gpio_events[gpio_event_count - 1U].value == 0U);

    xy_ws2812_deinit(&strip);
    assert(!strip.initialized);
    assert(strip.leds == NULL);
    assert(strip.tx_buffer == NULL);
}

static void test_ws2812_rgbw_and_color_math(void)
{
    xy_ws2812_handle_t strip;
    xy_ws2812_config_t cfg = make_ws_config(WS2812_COLOR_RGBW);
    xy_ws2812_color_t colors[2] = {
        { .r = 100, .g = 50, .b = 25, .w = 10 },
        { .r = 1, .g = 2, .b = 3, .w = 4 },
    };
    uint8_t h = 0, s = 0, v = 0;

    assert(xy_ws2812_init(&strip, &cfg) == WS2812_OK);
    assert(strip.buffer_size == 12U);
    xy_ws2812_set_pixels(&strip, colors, 2, 1);
    assert(strip.leds[1].r == 99U);
    assert(strip.leds[1].g == 49U);
    assert(strip.leds[1].b == 24U);
    assert(strip.leds[1].w == 9U);
    assert(strip.leds[2].r == 0U);
    assert(strip.leds[2].g == 1U);
    assert(strip.leds[2].b == 2U);
    assert(strip.leds[2].w == 3U);

    xy_ws2812_show(&strip);
    assert(strip.tx_buffer[4] == 99U);
    assert(strip.tx_buffer[5] == 49U);
    assert(strip.tx_buffer[6] == 24U);
    assert(strip.tx_buffer[7] == 9U);

    xy_ws2812_set_brightness(&strip, 128);
    assert(xy_ws2812_get_brightness(&strip) == 128U);
    xy_ws2812_fill(&strip, (xy_ws2812_color_t){ .r = 64, .g = 32, .b = 16, .w = 8 });
    assert(strip.leds[0].r == 32U);
    assert(strip.leds[0].g == 16U);
    assert(strip.leds[0].b == 8U);
    assert(strip.leds[0].w == 4U);

    xy_ws2812_color_t red = xy_ws2812_hsv_to_rgb(0, 255, 255);
    assert(red.r == 255U && red.g == 0U && red.b == 0U);
    xy_ws2812_rgb_to_hsv((xy_ws2812_color_t){ .r = 5, .g = 5, .b = 5 }, &h, &s, &v);
    assert(h == 0U && s == 0U && v == 5U);
    xy_ws2812_color_t blend = xy_ws2812_color_blend(
        (xy_ws2812_color_t){ .r = 0, .g = 0, .b = 0, .w = 0 },
        (xy_ws2812_color_t){ .r = 255, .g = 128, .b = 64, .w = 32 },
        128);
    assert(blend.r == 127U && blend.g == 64U && blend.b == 32U && blend.w == 16U);

    xy_ws2812_deinit(&strip);
}

int main(void)
{
    test_oled_init_pixel_line_refresh();
    test_ws2812_pixels_show_and_order();
    test_ws2812_rgbw_and_color_math();
    return 0;
}
