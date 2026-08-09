#include "xy_gui_ssd1306_adapter.h"
#include "unity.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define TEST_OLED_WIDTH       8U
#define TEST_OLED_HEIGHT      16U
#define TEST_OLED_BUFFER_SIZE ((TEST_OLED_WIDTH * TEST_OLED_HEIGHT) / 8U)
#define MAX_I2C_WRITES        8U
#define MAX_I2C_BYTES         256U

typedef struct {
    uint8_t bytes[MAX_I2C_BYTES];
    size_t len;
} i2c_write_t;

static i2c_write_t g_i2c_writes[MAX_I2C_WRITES];
static size_t g_i2c_write_count;

static void reset_i2c_writes(void)
{
    memset(g_i2c_writes, 0, sizeof(g_i2c_writes));
    g_i2c_write_count = 0;
}

void setUp(void)
{
    reset_i2c_writes();
    xy_gui_ssd1306_adapter_reset();
}

void tearDown(void)
{
}

xy_error_t xy_i2c_device_init(xy_i2c_device_t *dev, void *i2c_handle, uint16_t addr,
                              uint32_t timeout)
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

xy_error_t xy_i2c_device_write(xy_i2c_device_t *dev, const uint8_t *data, size_t len)
{
    (void)dev;
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_LESS_THAN_UINT(MAX_I2C_WRITES, g_i2c_write_count);
    TEST_ASSERT_LESS_OR_EQUAL_UINT(MAX_I2C_BYTES, len);

    memcpy(g_i2c_writes[g_i2c_write_count].bytes, data, len);
    g_i2c_writes[g_i2c_write_count].len = len;
    g_i2c_write_count++;
    return XY_DEVICE_OK;
}

void xy_hal_delay_ms(uint32_t ms)
{
    (void)ms;
}

static void init_oled_fixture(xy_oled_ssd1306_t *oled, uint8_t *buffer, uint16_t width,
                              uint16_t height)
{
    memset(oled, 0, sizeof(*oled));
    memset(buffer, 0, (width * height) / 8U);
    oled->width = width;
    oled->height = height;
    oled->buffer = buffer;
}

static void test_bind_rejects_null_inputs_without_mutating_existing_driver(void)
{
    xy_oled_ssd1306_t oled;
    uint8_t buffer[TEST_OLED_BUFFER_SIZE];
    xy_gui_disp_drv_t drv;

    init_oled_fixture(&oled, buffer, TEST_OLED_WIDTH, TEST_OLED_HEIGHT);
    memset(&drv, 0xA5, sizeof(drv));

    TEST_ASSERT_EQUAL_INT(XY_GUI_INVALID_PARAM, xy_gui_ssd1306_bind(NULL, &oled));
    TEST_ASSERT_EQUAL_INT(XY_GUI_INVALID_PARAM, xy_gui_ssd1306_bind(&drv, NULL));
    TEST_ASSERT_NOT_NULL(drv.draw_pixel);
    TEST_ASSERT_NOT_NULL(drv.fill_rect);
    TEST_ASSERT_NOT_NULL(drv.flush);
}

static void test_draw_pixel_maps_rgb565_to_mono_bits(void)
{
    xy_oled_ssd1306_t oled;
    uint8_t buffer[TEST_OLED_BUFFER_SIZE];
    xy_gui_disp_drv_t drv;

    init_oled_fixture(&oled, buffer, TEST_OLED_WIDTH, TEST_OLED_HEIGHT);
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_ssd1306_bind(&drv, &oled));
    TEST_ASSERT_NOT_NULL(drv.init);
    TEST_ASSERT_NOT_NULL(drv.draw_pixel);
    TEST_ASSERT_NULL(drv.draw_line);
    TEST_ASSERT_NULL(drv.draw_rect);
    TEST_ASSERT_NULL(drv.draw_char);
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, drv.init());

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, drv.draw_pixel(0, 0, XY_GUI_COLOR_RED));
    TEST_ASSERT_EQUAL_HEX8(0x01U, buffer[0]);

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, drv.draw_pixel(1, 7, XY_GUI_COLOR_WHITE));
    TEST_ASSERT_EQUAL_HEX8(0x80U, buffer[1]);

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, drv.draw_pixel(1, 7, XY_GUI_COLOR_BLACK));
    TEST_ASSERT_EQUAL_HEX8(0x00U, buffer[1]);

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, drv.draw_pixel(-1, 0, XY_GUI_COLOR_WHITE));
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, drv.draw_pixel((int16_t)TEST_OLED_WIDTH, 0, XY_GUI_COLOR_WHITE));
    TEST_ASSERT_EQUAL_HEX8(0x01U, buffer[0]);
}

static void test_fill_rect_clips_through_ssd1306_driver_and_ignores_empty_rects(void)
{
    xy_oled_ssd1306_t oled;
    uint8_t buffer[TEST_OLED_BUFFER_SIZE];
    xy_gui_disp_drv_t drv;

    init_oled_fixture(&oled, buffer, TEST_OLED_WIDTH, TEST_OLED_HEIGHT);
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_ssd1306_bind(&drv, &oled));

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, drv.fill_rect(-1, -1, 3, 3, XY_GUI_COLOR_GREEN));
    TEST_ASSERT_EQUAL_HEX8(0x03U, buffer[0]);
    TEST_ASSERT_EQUAL_HEX8(0x03U, buffer[1]);
    TEST_ASSERT_EQUAL_HEX8(0x00U, buffer[2]);

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, drv.fill_rect(0, 0, 0, 5, XY_GUI_COLOR_WHITE));
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, drv.fill_rect(0, 0, 5, -1, XY_GUI_COLOR_WHITE));
    TEST_ASSERT_EQUAL_HEX8(0x03U, buffer[0]);
    TEST_ASSERT_EQUAL_HEX8(0x03U, buffer[1]);

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, drv.fill_rect(0, 0, 2, 2, XY_GUI_COLOR_BLACK));
    TEST_ASSERT_EQUAL_HEX8(0x00U, buffer[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00U, buffer[1]);
}

static void test_flush_forwards_to_ssd1306_refresh_transactions(void)
{
    xy_oled_ssd1306_t oled;
    uint8_t buffer[TEST_OLED_BUFFER_SIZE];
    xy_gui_disp_drv_t drv;

    init_oled_fixture(&oled, buffer, TEST_OLED_WIDTH, TEST_OLED_HEIGHT);
    buffer[0] = 0xA5U;
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_ssd1306_bind(&drv, &oled));

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, drv.flush());
    TEST_ASSERT_EQUAL_UINT(4U, g_i2c_write_count);
    TEST_ASSERT_EQUAL_UINT(6U, g_i2c_writes[0].len);
    TEST_ASSERT_EQUAL_HEX8(0x00U, g_i2c_writes[0].bytes[0]);
    TEST_ASSERT_EQUAL_HEX8(0x21U, g_i2c_writes[0].bytes[1]);
    TEST_ASSERT_EQUAL_HEX8((uint8_t)(TEST_OLED_WIDTH - 1U), g_i2c_writes[0].bytes[5]);
    TEST_ASSERT_EQUAL_HEX8(0x22U, g_i2c_writes[1].bytes[1]);
    TEST_ASSERT_EQUAL_HEX8(1U, g_i2c_writes[1].bytes[5]);
    TEST_ASSERT_EQUAL_UINT(1U, g_i2c_writes[2].len);
    TEST_ASSERT_EQUAL_HEX8(0x40U, g_i2c_writes[2].bytes[0]);
    TEST_ASSERT_EQUAL_UINT(TEST_OLED_BUFFER_SIZE, g_i2c_writes[3].len);
    TEST_ASSERT_EQUAL_HEX8(0xA5U, g_i2c_writes[3].bytes[0]);
}

static void test_multiple_bound_oled_instances_keep_isolated_slots(void)
{
    xy_oled_ssd1306_t first;
    xy_oled_ssd1306_t second;
    uint8_t first_buffer[TEST_OLED_BUFFER_SIZE];
    uint8_t second_buffer[TEST_OLED_BUFFER_SIZE];
    xy_gui_disp_drv_t first_drv;
    xy_gui_disp_drv_t second_drv;

    init_oled_fixture(&first, first_buffer, TEST_OLED_WIDTH, TEST_OLED_HEIGHT);
    init_oled_fixture(&second, second_buffer, TEST_OLED_WIDTH, TEST_OLED_HEIGHT);

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_ssd1306_bind(&first_drv, &first));
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_ssd1306_bind(&second_drv, &second));

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, first_drv.draw_pixel(2, 0, XY_GUI_COLOR_WHITE));
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, second_drv.draw_pixel(3, 0, XY_GUI_COLOR_WHITE));
    TEST_ASSERT_EQUAL_HEX8(0x01U, first_buffer[2]);
    TEST_ASSERT_EQUAL_HEX8(0x00U, first_buffer[3]);
    TEST_ASSERT_EQUAL_HEX8(0x00U, second_buffer[2]);
    TEST_ASSERT_EQUAL_HEX8(0x01U, second_buffer[3]);

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, first_drv.flush());
    TEST_ASSERT_EQUAL_UINT(4U, g_i2c_write_count);
    TEST_ASSERT_EQUAL_HEX8(0x01U, g_i2c_writes[3].bytes[2]);

    reset_i2c_writes();
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, second_drv.flush());
    TEST_ASSERT_EQUAL_UINT(4U, g_i2c_write_count);
    TEST_ASSERT_EQUAL_HEX8(0x01U, g_i2c_writes[3].bytes[3]);
}

static void test_slot_exhaustion_clears_output_driver_until_reset_releases_slots(void)
{
    xy_oled_ssd1306_t oleds[5];
    uint8_t buffers[5][TEST_OLED_BUFFER_SIZE];
    xy_gui_disp_drv_t drivers[5];

    for (size_t index = 0; index < 5U; ++index) {
        init_oled_fixture(&oleds[index], buffers[index], TEST_OLED_WIDTH, TEST_OLED_HEIGHT);
        memset(&drivers[index], 0xA5, sizeof(drivers[index]));
    }

    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_ssd1306_bind(&drivers[0], &oleds[0]));
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_ssd1306_bind(&drivers[1], &oleds[1]));
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_ssd1306_bind(&drivers[2], &oleds[2]));
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_ssd1306_bind(&drivers[3], &oleds[3]));

    TEST_ASSERT_EQUAL_INT(XY_GUI_NO_MEM, xy_gui_ssd1306_bind(&drivers[4], &oleds[4]));
    TEST_ASSERT_NULL(drivers[4].init);
    TEST_ASSERT_NULL(drivers[4].draw_pixel);
    TEST_ASSERT_NULL(drivers[4].fill_rect);
    TEST_ASSERT_NULL(drivers[4].flush);

    xy_gui_ssd1306_adapter_reset();
    TEST_ASSERT_EQUAL_INT(XY_GUI_OK, xy_gui_ssd1306_bind(&drivers[4], &oleds[4]));
    TEST_ASSERT_NOT_NULL(drivers[4].draw_pixel);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_bind_rejects_null_inputs_without_mutating_existing_driver);
    RUN_TEST(test_draw_pixel_maps_rgb565_to_mono_bits);
    RUN_TEST(test_fill_rect_clips_through_ssd1306_driver_and_ignores_empty_rects);
    RUN_TEST(test_flush_forwards_to_ssd1306_refresh_transactions);
    RUN_TEST(test_multiple_bound_oled_instances_keep_isolated_slots);
    RUN_TEST(test_slot_exhaustion_clears_output_driver_until_reset_releases_slots);
    return UNITY_END();
}
