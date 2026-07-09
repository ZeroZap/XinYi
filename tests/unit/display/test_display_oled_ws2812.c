#include "xy_oled_ssd1306.h"
#include "xy_ws2812.h"
#include "xy_device_core.h"
#include "fff.h"
#include "unity.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_I2C_WRITES 64
#define MAX_I2C_BYTES 2048
#define MAX_GPIO_EVENTS 4096

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(xy_error_t, xy_i2c_device_write, xy_i2c_device_t *, const uint8_t *, size_t)
FAKE_VOID_FUNC(xy_hal_delay_ms, uint32_t)
FAKE_VOID_FUNC(gpio_callback, uint8_t, uint8_t)

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

static xy_error_t fake_i2c_device_write(xy_i2c_device_t *dev, const uint8_t *data, size_t len);
static void fake_hal_delay_ms(uint32_t ms);
static void gpio_callback_impl(uint8_t pin, uint8_t value);

void setUp(void)
{
}

void tearDown(void)
{
}

static void reset_i2c_log(void)
{
    RESET_FAKE(xy_i2c_device_write);
    RESET_FAKE(xy_hal_delay_ms);
    FFF_RESET_HISTORY();

    xy_i2c_device_write_fake.custom_fake = fake_i2c_device_write;
    xy_hal_delay_ms_fake.custom_fake = fake_hal_delay_ms;

    memset(i2c_writes, 0, sizeof(i2c_writes));
    i2c_write_count = 0;
    delay_ms_total = 0;
}

static void reset_gpio_log(void)
{
    RESET_FAKE(gpio_callback);
    FFF_RESET_HISTORY();
    gpio_callback_fake.custom_fake = gpio_callback_impl;

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

static xy_error_t fake_i2c_device_write(xy_i2c_device_t *dev, const uint8_t *data, size_t len)
{
    (void)dev;
    TEST_ASSERT_LESS_THAN_UINT32(MAX_I2C_WRITES, i2c_write_count);
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(MAX_I2C_BYTES, len);
    memcpy(i2c_writes[i2c_write_count].bytes, data, len);
    i2c_writes[i2c_write_count].len = len;
    i2c_write_count++;
    return XY_DEVICE_OK;
}

static void fake_hal_delay_ms(uint32_t ms)
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

static void gpio_callback_impl(uint8_t pin, uint8_t value)
{
    TEST_ASSERT_LESS_THAN_UINT32(MAX_GPIO_EVENTS, gpio_event_count);
    gpio_events[gpio_event_count].pin = pin;
    gpio_events[gpio_event_count].value = value;
    gpio_event_count++;
}

static void test_oled_init_pixel_line_refresh(void)
{
    xy_oled_ssd1306_t oled;

    reset_i2c_log();
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_oled_ssd1306_init(NULL, (void *)0x1, 128, 64));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_oled_ssd1306_init(&oled, NULL, 128, 64));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_oled_ssd1306_init(&oled, (void *)0x1, 128, 64));
    TEST_ASSERT_EQUAL_UINT16(128U, oled.width);
    TEST_ASSERT_EQUAL_UINT16(64U, oled.height);
    TEST_ASSERT_EQUAL_UINT16(0x3CU, oled.i2c_dev.dev_addr);
    TEST_ASSERT_NOT_NULL(oled.buffer);
    TEST_ASSERT_EQUAL_UINT32(14U, i2c_write_count);
    TEST_ASSERT_EQUAL_UINT(14U, xy_i2c_device_write_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&oled.i2c_dev, xy_i2c_device_write_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT(14U, xy_hal_delay_ms_fake.call_count);
    TEST_ASSERT_EQUAL_UINT32(2U, i2c_writes[0].len);
    TEST_ASSERT_EQUAL_HEX8(0x00U, i2c_writes[0].bytes[0]);
    TEST_ASSERT_EQUAL_HEX8(0xAEU, i2c_writes[0].bytes[1]);
    TEST_ASSERT_EQUAL_UINT32(14U, delay_ms_total);

    xy_oled_ssd1306_draw_pixel(&oled, 0, 0, true);
    xy_oled_ssd1306_draw_pixel(&oled, 1, 7, true);
    xy_oled_ssd1306_draw_pixel(&oled, 2, 8, true);
    xy_oled_ssd1306_draw_pixel(&oled, -1, 0, true);
    xy_oled_ssd1306_draw_pixel(&oled, 128, 0, true);
    TEST_ASSERT_EQUAL_HEX8(0x01U, oled.buffer[0]);
    TEST_ASSERT_EQUAL_HEX8(0x80U, oled.buffer[1]);
    TEST_ASSERT_EQUAL_HEX8(0x01U, oled.buffer[128 + 2]);
    xy_oled_ssd1306_draw_pixel(&oled, 1, 7, false);
    TEST_ASSERT_EQUAL_HEX8(0x00U, oled.buffer[1]);

    xy_oled_ssd1306_draw_line(&oled, 0, 0, 3, 0, true);
    TEST_ASSERT_TRUE((oled.buffer[0] & 0x01U) != 0U);
    TEST_ASSERT_TRUE((oled.buffer[1] & 0x01U) != 0U);
    TEST_ASSERT_TRUE((oled.buffer[2] & 0x01U) != 0U);
    TEST_ASSERT_TRUE((oled.buffer[3] & 0x01U) != 0U);

    reset_i2c_log();
    xy_oled_ssd1306_refresh(&oled);
    TEST_ASSERT_EQUAL_UINT32(4U, i2c_write_count);
    TEST_ASSERT_EQUAL_UINT(4U, xy_i2c_device_write_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&oled.i2c_dev, xy_i2c_device_write_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT32(6U, i2c_writes[0].len);
    TEST_ASSERT_EQUAL_HEX8(0x00U, i2c_writes[0].bytes[0]);
    TEST_ASSERT_EQUAL_HEX8(0x21U, i2c_writes[0].bytes[1]);
    TEST_ASSERT_EQUAL_UINT8(127U, i2c_writes[0].bytes[5]);
    TEST_ASSERT_EQUAL_HEX8(0x22U, i2c_writes[1].bytes[1]);
    TEST_ASSERT_EQUAL_UINT8(7U, i2c_writes[1].bytes[5]);
    TEST_ASSERT_EQUAL_UINT32(1U, i2c_writes[2].len);
    TEST_ASSERT_EQUAL_HEX8(0x40U, i2c_writes[2].bytes[0]);
    TEST_ASSERT_EQUAL_UINT32(1024U, i2c_writes[3].len);

    xy_oled_ssd1306_clear(&oled);
    for (size_t i = 0; i < 1024U; i++) {
        TEST_ASSERT_EQUAL_HEX8(0U, oled.buffer[i]);
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

    TEST_ASSERT_EQUAL_INT(WS2812_ERROR_INVALID_PARAM, xy_ws2812_init(NULL, &cfg));
    TEST_ASSERT_EQUAL_INT(WS2812_ERROR_INVALID_PARAM, xy_ws2812_init(&strip, NULL));
    cfg.num_leds = 0;
    TEST_ASSERT_EQUAL_INT(WS2812_ERROR_INVALID_PARAM, xy_ws2812_init(&strip, &cfg));
    cfg.num_leds = 3;

    TEST_ASSERT_EQUAL_INT(WS2812_OK, xy_ws2812_init(&strip, &cfg));
    TEST_ASSERT_TRUE(strip.initialized);
    TEST_ASSERT_EQUAL_UINT32(9U, strip.buffer_size);
    TEST_ASSERT_EQUAL_UINT8(1U, strip.base.initialized);

    xy_ws2812_set_pixel(&strip, 1, c);
    TEST_ASSERT_TRUE(strip.dirty);
    TEST_ASSERT_EQUAL_HEX8(0x0FU, strip.leds[1].r);
    TEST_ASSERT_EQUAL_HEX8(0x1FU, strip.leds[1].g);
    TEST_ASSERT_EQUAL_HEX8(0x3FU, strip.leds[1].b);
    xy_ws2812_set_pixel(&strip, 99, c);
    TEST_ASSERT_EQUAL_HEX8(0U, xy_ws2812_get_pixel(&strip, 99).r);

    xy_ws2812_set_gpio_callback(gpio_callback);
    reset_gpio_log();
    xy_ws2812_show(&strip);
    TEST_ASSERT_FALSE(strip.dirty);
    TEST_ASSERT_EQUAL_HEX8(0x1FU, strip.tx_buffer[3]);
    TEST_ASSERT_EQUAL_HEX8(0x0FU, strip.tx_buffer[4]);
    TEST_ASSERT_EQUAL_HEX8(0x3FU, strip.tx_buffer[5]);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)(strip.buffer_size * 16U + 1U), gpio_event_count);
    TEST_ASSERT_EQUAL_UINT(gpio_event_count, gpio_callback_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8(cfg.data_pin, gpio_callback_fake.arg0_history[0]);
    TEST_ASSERT_EQUAL_UINT8(1U, gpio_callback_fake.arg1_history[0]);
    TEST_ASSERT_EQUAL_UINT8(cfg.data_pin, gpio_events[0].pin);
    TEST_ASSERT_EQUAL_UINT8(1U, gpio_events[0].value);
    TEST_ASSERT_EQUAL_UINT8(cfg.data_pin, gpio_callback_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT8(0U, gpio_callback_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT8(0U, gpio_events[gpio_event_count - 1U].value);

    xy_ws2812_deinit(&strip);
    TEST_ASSERT_FALSE(strip.initialized);
    TEST_ASSERT_NULL(strip.leds);
    TEST_ASSERT_NULL(strip.tx_buffer);
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

    TEST_ASSERT_EQUAL_INT(WS2812_OK, xy_ws2812_init(&strip, &cfg));
    TEST_ASSERT_EQUAL_UINT32(12U, strip.buffer_size);
    xy_ws2812_set_pixels(&strip, colors, 2, 1);
    TEST_ASSERT_EQUAL_UINT8(99U, strip.leds[1].r);
    TEST_ASSERT_EQUAL_UINT8(49U, strip.leds[1].g);
    TEST_ASSERT_EQUAL_UINT8(24U, strip.leds[1].b);
    TEST_ASSERT_EQUAL_UINT8(9U, strip.leds[1].w);
    TEST_ASSERT_EQUAL_UINT8(0U, strip.leds[2].r);
    TEST_ASSERT_EQUAL_UINT8(1U, strip.leds[2].g);
    TEST_ASSERT_EQUAL_UINT8(2U, strip.leds[2].b);
    TEST_ASSERT_EQUAL_UINT8(3U, strip.leds[2].w);

    xy_ws2812_show(&strip);
    TEST_ASSERT_EQUAL_UINT8(99U, strip.tx_buffer[4]);
    TEST_ASSERT_EQUAL_UINT8(49U, strip.tx_buffer[5]);
    TEST_ASSERT_EQUAL_UINT8(24U, strip.tx_buffer[6]);
    TEST_ASSERT_EQUAL_UINT8(9U, strip.tx_buffer[7]);

    xy_ws2812_set_brightness(&strip, 128);
    TEST_ASSERT_EQUAL_UINT8(128U, xy_ws2812_get_brightness(&strip));
    xy_ws2812_fill(&strip, (xy_ws2812_color_t){ .r = 64, .g = 32, .b = 16, .w = 8 });
    TEST_ASSERT_EQUAL_UINT8(32U, strip.leds[0].r);
    TEST_ASSERT_EQUAL_UINT8(16U, strip.leds[0].g);
    TEST_ASSERT_EQUAL_UINT8(8U, strip.leds[0].b);
    TEST_ASSERT_EQUAL_UINT8(4U, strip.leds[0].w);

    xy_ws2812_color_t red = xy_ws2812_hsv_to_rgb(0, 255, 255);
    TEST_ASSERT_EQUAL_UINT8(255U, red.r);
    TEST_ASSERT_EQUAL_UINT8(0U, red.g);
    TEST_ASSERT_EQUAL_UINT8(0U, red.b);
    xy_ws2812_rgb_to_hsv((xy_ws2812_color_t){ .r = 5, .g = 5, .b = 5 }, &h, &s, &v);
    TEST_ASSERT_EQUAL_UINT8(0U, h);
    TEST_ASSERT_EQUAL_UINT8(0U, s);
    TEST_ASSERT_EQUAL_UINT8(5U, v);
    xy_ws2812_color_t blend = xy_ws2812_color_blend(
        (xy_ws2812_color_t){ .r = 0, .g = 0, .b = 0, .w = 0 },
        (xy_ws2812_color_t){ .r = 255, .g = 128, .b = 64, .w = 32 },
        128);
    TEST_ASSERT_EQUAL_UINT8(127U, blend.r);
    TEST_ASSERT_EQUAL_UINT8(64U, blend.g);
    TEST_ASSERT_EQUAL_UINT8(32U, blend.b);
    TEST_ASSERT_EQUAL_UINT8(16U, blend.w);

    xy_ws2812_deinit(&strip);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_oled_init_pixel_line_refresh);
    RUN_TEST(test_ws2812_pixels_show_and_order);
    RUN_TEST(test_ws2812_rgbw_and_color_math);
    return UNITY_END();
}
