#include "unity.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "xy_oled_ssd1306.h"
#include "xy_device.h"
#include "xy_os_delay.h"

#define MAX_WRITES 64U
#define MAX_CAPTURE 1030U

static uint8_t g_write_data[MAX_WRITES][MAX_CAPTURE];
static size_t g_write_len[MAX_WRITES];
static int g_write_ret[MAX_WRITES];
static size_t g_write_count;
static size_t g_write_index;
static uint32_t g_delay_total;
static uint8_t g_last_addr;

xy_error_t xy_i2c_device_init(xy_i2c_device_t *dev, void *i2c_handle, uint16_t addr, uint32_t timeout)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_NOT_NULL(i2c_handle);
    memset(dev, 0, sizeof(*dev));
    dev->base.initialized = 1;
    dev->i2c_handle = i2c_handle;
    dev->dev_addr = addr;
    dev->timeout = timeout;
    g_last_addr = (uint8_t)addr;
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_write(xy_i2c_device_t *dev, const uint8_t *data, size_t len)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_LESS_THAN_UINT(MAX_WRITES, g_write_index);
    TEST_ASSERT_LESS_OR_EQUAL_UINT(MAX_CAPTURE, len);
    memcpy(g_write_data[g_write_index], data, len);
    g_write_len[g_write_index] = len;
    int ret = g_write_ret[g_write_index];
    g_write_index++;
    return ret;
}

xy_os_status_t xy_os_delay(uint32_t ticks)
{
    g_delay_total += ticks;
    return 0;
}

int xy_printf(const char *fmt, ...)
{
    (void)fmt;
    return 0;
}

static void queue_write_ret(int ret)
{
    TEST_ASSERT_LESS_THAN_UINT(MAX_WRITES, g_write_count);
    g_write_ret[g_write_count++] = ret;
}

static void queue_success_writes(size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        queue_write_ret(XY_DEVICE_OK);
    }
}

void setUp(void)
{
    memset(g_write_data, 0, sizeof(g_write_data));
    memset(g_write_len, 0, sizeof(g_write_len));
    memset(g_write_ret, 0, sizeof(g_write_ret));
    g_write_count = 0;
    g_write_index = 0;
    g_delay_total = 0;
    g_last_addr = 0;
}

void tearDown(void)
{
}

static void test_init_rejects_invalid_inputs_and_runs_full_init_refresh(void)
{
    xy_oled_ssd1306_t oled;
    int fake_bus;

    TEST_ASSERT_EQUAL_INT(XY_OLED_INVALID_PARAM, xy_oled_ssd1306_init(NULL, &fake_bus));
    TEST_ASSERT_EQUAL_INT(XY_OLED_INVALID_PARAM, xy_oled_ssd1306_init(&oled, NULL));

    queue_success_writes(31U); /* 24 init commands + 6 address commands + 1 framebuffer data write */
    TEST_ASSERT_EQUAL_INT(XY_OLED_OK, xy_oled_ssd1306_init(&oled, &fake_bus));
    TEST_ASSERT_TRUE(oled.initialized);
    TEST_ASSERT_EQUAL_UINT16(128U, oled.width);
    TEST_ASSERT_EQUAL_UINT16(64U, oled.height);
    TEST_ASSERT_EQUAL_UINT16(1024U, oled.buffer_size);
    TEST_ASSERT_EQUAL_UINT8(SSD1306_ADDR, g_last_addr);
    TEST_ASSERT_NOT_NULL(oled.buffer);
    TEST_ASSERT_EQUAL_UINT(31U, g_write_index);
    TEST_ASSERT_EQUAL_UINT32(124U, g_delay_total);

    TEST_ASSERT_EQUAL_UINT(2U, g_write_len[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00U, g_write_data[0][0]);
    TEST_ASSERT_EQUAL_HEX8(SSD1306_CMD_DISPLAY_OFF, g_write_data[0][1]);
    TEST_ASSERT_EQUAL_HEX8(SSD1306_CMD_COLUMN_ADDR, g_write_data[24][1]);
    TEST_ASSERT_EQUAL_HEX8(SSD1306_CMD_PAGE_ADDR, g_write_data[27][1]);
    TEST_ASSERT_EQUAL_UINT(1025U, g_write_len[30]);
    TEST_ASSERT_EQUAL_HEX8(0x40U, g_write_data[30][0]);
    for (size_t i = 1; i < g_write_len[30]; ++i) {
        TEST_ASSERT_EQUAL_HEX8(0x00U, g_write_data[30][i]);
    }

    free(oled.buffer);
}

static void test_init_reports_command_and_refresh_failures(void)
{
    xy_oled_ssd1306_t oled;
    int fake_bus;

    queue_write_ret(XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_OLED_NOT_FOUND, xy_oled_ssd1306_init(&oled, &fake_bus));
    TEST_ASSERT_NULL(oled.buffer);

    queue_success_writes(30U);
    queue_write_ret(XY_DEVICE_ERROR); /* framebuffer data write fails during init refresh */
    TEST_ASSERT_EQUAL_INT(XY_OLED_ERROR, xy_oled_ssd1306_init(&oled, &fake_bus));
    TEST_ASSERT_NULL(oled.buffer);
    TEST_ASSERT_FALSE(oled.initialized);
}

static void test_draw_pixel_lines_rectangles_and_clear(void)
{
    uint8_t buffer[1024];
    xy_oled_ssd1306_t oled;
    memset(buffer, 0, sizeof(buffer));
    memset(&oled, 0, sizeof(oled));
    oled.width = 128;
    oled.height = 64;
    oled.buffer_size = sizeof(buffer);
    oled.buffer = buffer;

    TEST_ASSERT_EQUAL_INT(XY_OLED_INVALID_PARAM, xy_oled_ssd1306_clear(NULL));
    TEST_ASSERT_EQUAL_INT(XY_OLED_INVALID_PARAM, xy_oled_ssd1306_draw_pixel(NULL, 0, 0, OLED_COLOR_WHITE));
    TEST_ASSERT_EQUAL_INT(XY_OLED_INVALID_PARAM, xy_oled_ssd1306_draw_pixel(&oled, -1, 0, OLED_COLOR_WHITE));
    TEST_ASSERT_EQUAL_INT(XY_OLED_INVALID_PARAM, xy_oled_ssd1306_draw_pixel(&oled, 128, 0, OLED_COLOR_WHITE));
    TEST_ASSERT_EQUAL_INT(XY_OLED_INVALID_PARAM, xy_oled_ssd1306_draw_pixel(&oled, 0, 64, OLED_COLOR_WHITE));

    TEST_ASSERT_EQUAL_INT(XY_OLED_OK, xy_oled_ssd1306_draw_pixel(&oled, 0, 0, OLED_COLOR_WHITE));
    TEST_ASSERT_EQUAL_INT(XY_OLED_OK, xy_oled_ssd1306_draw_pixel(&oled, 1, 7, OLED_COLOR_WHITE));
    TEST_ASSERT_EQUAL_INT(XY_OLED_OK, xy_oled_ssd1306_draw_pixel(&oled, 2, 8, OLED_COLOR_WHITE));
    TEST_ASSERT_EQUAL_HEX8(0x01U, buffer[0]);
    TEST_ASSERT_EQUAL_HEX8(0x80U, buffer[1]);
    TEST_ASSERT_EQUAL_HEX8(0x01U, buffer[128 + 2]);

    TEST_ASSERT_EQUAL_INT(XY_OLED_OK, xy_oled_ssd1306_draw_pixel(&oled, 1, 7, OLED_COLOR_BLACK));
    TEST_ASSERT_EQUAL_HEX8(0x00U, buffer[1]);

    TEST_ASSERT_EQUAL_INT(XY_OLED_OK, xy_oled_ssd1306_draw_hline(&oled, 4, 1, 2, OLED_COLOR_WHITE));
    TEST_ASSERT_BITS_HIGH(0x02U, buffer[2]);
    TEST_ASSERT_BITS_HIGH(0x02U, buffer[3]);
    TEST_ASSERT_BITS_HIGH(0x02U, buffer[4]);

    TEST_ASSERT_EQUAL_INT(XY_OLED_OK, xy_oled_ssd1306_draw_vline(&oled, 5, 10, 8, OLED_COLOR_WHITE));
    TEST_ASSERT_BITS_HIGH(0x07U, buffer[128 + 5]);

    TEST_ASSERT_EQUAL_INT(XY_OLED_OK, xy_oled_ssd1306_draw_rect(&oled, 10, 0, 3, 3, OLED_COLOR_WHITE));
    TEST_ASSERT_BITS_HIGH(0x01U, buffer[10]);
    TEST_ASSERT_BITS_HIGH(0x04U, buffer[12]);

    TEST_ASSERT_EQUAL_INT(XY_OLED_OK, xy_oled_ssd1306_fill_rect(&oled, 20, 0, 2, 2, OLED_COLOR_WHITE));
    TEST_ASSERT_BITS_HIGH(0x03U, buffer[20]);
    TEST_ASSERT_BITS_HIGH(0x03U, buffer[21]);

    TEST_ASSERT_EQUAL_INT(XY_OLED_OK, xy_oled_ssd1306_clear(&oled));
    for (size_t i = 0; i < sizeof(buffer); ++i) {
        TEST_ASSERT_EQUAL_HEX8(0x00U, buffer[i]);
    }

    TEST_ASSERT_EQUAL_INT(XY_OLED_OK, xy_oled_ssd1306_draw_hline(&oled, 6, 2, 4, OLED_COLOR_WHITE));
    TEST_ASSERT_BITS_HIGH(0x04U, buffer[4]);
    TEST_ASSERT_BITS_HIGH(0x04U, buffer[5]);
    TEST_ASSERT_BITS_HIGH(0x04U, buffer[6]);

    TEST_ASSERT_EQUAL_INT(XY_OLED_OK, xy_oled_ssd1306_draw_vline(&oled, 7, 12, 10, OLED_COLOR_WHITE));
    TEST_ASSERT_BITS_HIGH(0x1CU, buffer[128 + 7]);
}

static void test_draw_string_and_char_guards(void)
{
    uint8_t buffer[1024];
    xy_oled_ssd1306_t oled;
    memset(buffer, 0, sizeof(buffer));
    memset(&oled, 0, sizeof(oled));
    oled.width = 128;
    oled.height = 64;
    oled.buffer_size = sizeof(buffer);
    oled.buffer = buffer;

    TEST_ASSERT_EQUAL_INT(XY_OLED_INVALID_PARAM, xy_oled_ssd1306_draw_char(NULL, 0, 0, 'A', OLED_COLOR_WHITE));
    TEST_ASSERT_EQUAL_INT(XY_OLED_INVALID_PARAM, xy_oled_ssd1306_draw_string(NULL, 0, 0, "A", OLED_COLOR_WHITE));
    TEST_ASSERT_EQUAL_INT(XY_OLED_INVALID_PARAM, xy_oled_ssd1306_draw_string(&oled, 0, 0, NULL, OLED_COLOR_WHITE));

    TEST_ASSERT_EQUAL_INT(XY_OLED_OK, xy_oled_ssd1306_draw_char(&oled, 0, 0, ' ', OLED_COLOR_WHITE));
    for (size_t i = 0; i < sizeof(buffer); ++i) {
        TEST_ASSERT_EQUAL_HEX8(0x00U, buffer[i]);
    }

    TEST_ASSERT_EQUAL_INT(XY_OLED_OK, xy_oled_ssd1306_draw_char(&oled, 0, 0, 0x7F, OLED_COLOR_WHITE));
    for (size_t i = 0; i < sizeof(buffer); ++i) {
        TEST_ASSERT_EQUAL_HEX8(0x00U, buffer[i]);
    }

    TEST_ASSERT_EQUAL_INT(XY_OLED_OK, xy_oled_ssd1306_draw_string(&oled, 0, 0, "!!", OLED_COLOR_WHITE));
    TEST_ASSERT_EQUAL_HEX8(0x5FU, buffer[2]);
    TEST_ASSERT_EQUAL_HEX8(0x5FU, buffer[8]);
}


static void test_refresh_data_failure_preserves_framebuffer(void)
{
    uint8_t buffer[1024];
    xy_oled_ssd1306_t oled;
    memset(buffer, 0x3C, sizeof(buffer));
    memset(&oled, 0, sizeof(oled));
    oled.width = 128;
    oled.height = 64;
    oled.buffer_size = sizeof(buffer);
    oled.buffer = buffer;
    oled.i2c_dev.base.initialized = 1;
    oled.initialized = 1;

    queue_success_writes(6U);
    queue_write_ret(XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_oled_ssd1306_refresh(&oled));
    TEST_ASSERT_EQUAL_UINT(7U, g_write_index);
    TEST_ASSERT_EQUAL_HEX8(0x40U, g_write_data[6][0]);
    for (size_t i = 0; i < sizeof(buffer); ++i) {
        TEST_ASSERT_EQUAL_HEX8(0x3CU, buffer[i]);
    }
}

static void test_display_controls_propagate_write_failures(void)
{
    xy_oled_ssd1306_t oled;
    memset(&oled, 0, sizeof(oled));
    oled.i2c_dev.base.initialized = 1;

    queue_write_ret(XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_oled_ssd1306_display_on(&oled));

    queue_write_ret(XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_oled_ssd1306_display_off(&oled));

    queue_write_ret(XY_DEVICE_OK);
    queue_write_ret(XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_oled_ssd1306_set_contrast(&oled, 0x22U));
    TEST_ASSERT_EQUAL_HEX8(SSD1306_CMD_SET_CONTRAST, g_write_data[2][1]);
    TEST_ASSERT_EQUAL_HEX8(0x22U, g_write_data[3][1]);
}

static void test_refresh_commands_controls_and_deinit(void)
{
    uint8_t buffer[1024];
    xy_oled_ssd1306_t oled;
    memset(buffer, 0xA5, sizeof(buffer));
    memset(&oled, 0, sizeof(oled));
    oled.width = 128;
    oled.height = 64;
    oled.buffer_size = sizeof(buffer);
    oled.buffer = buffer;
    oled.i2c_dev.base.initialized = 1;

    TEST_ASSERT_EQUAL_INT(XY_OLED_INVALID_PARAM, xy_oled_ssd1306_refresh(NULL));
    TEST_ASSERT_EQUAL_INT(XY_OLED_INVALID_PARAM, xy_oled_ssd1306_refresh(&oled));
    oled.initialized = 1;

    queue_success_writes(7U);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_oled_ssd1306_refresh(&oled));
    TEST_ASSERT_EQUAL_HEX8(SSD1306_CMD_COLUMN_ADDR, g_write_data[0][1]);
    TEST_ASSERT_EQUAL_HEX8(0U, g_write_data[1][1]);
    TEST_ASSERT_EQUAL_HEX8(127U, g_write_data[2][1]);
    TEST_ASSERT_EQUAL_HEX8(SSD1306_CMD_PAGE_ADDR, g_write_data[3][1]);
    TEST_ASSERT_EQUAL_HEX8(7U, g_write_data[5][1]);
    TEST_ASSERT_EQUAL_UINT(1025U, g_write_len[6]);
    TEST_ASSERT_EQUAL_HEX8(0x40U, g_write_data[6][0]);
    TEST_ASSERT_EQUAL_HEX8(0xA5U, g_write_data[6][1]);

    queue_success_writes(2U);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_oled_ssd1306_set_contrast(&oled, 0x55));
    TEST_ASSERT_EQUAL_HEX8(SSD1306_CMD_SET_CONTRAST, g_write_data[7][1]);
    TEST_ASSERT_EQUAL_HEX8(0x55U, g_write_data[8][1]);

    queue_success_writes(2U);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_oled_ssd1306_display_on(&oled));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_oled_ssd1306_display_off(&oled));
    TEST_ASSERT_EQUAL_HEX8(SSD1306_CMD_DISPLAY_ON, g_write_data[9][1]);
    TEST_ASSERT_EQUAL_HEX8(SSD1306_CMD_DISPLAY_OFF, g_write_data[10][1]);

    oled.buffer = malloc(16U);
    TEST_ASSERT_NOT_NULL(oled.buffer);
    queue_success_writes(1U);
    TEST_ASSERT_EQUAL_INT(XY_OLED_OK, xy_oled_ssd1306_deinit(&oled));
    TEST_ASSERT_NULL(oled.buffer);
    TEST_ASSERT_FALSE(oled.initialized);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_rejects_invalid_inputs_and_runs_full_init_refresh);
    RUN_TEST(test_init_reports_command_and_refresh_failures);
    RUN_TEST(test_draw_pixel_lines_rectangles_and_clear);
    RUN_TEST(test_draw_string_and_char_guards);
    RUN_TEST(test_refresh_data_failure_preserves_framebuffer);
    RUN_TEST(test_display_controls_propagate_write_failures);
    RUN_TEST(test_refresh_commands_controls_and_deinit);
    return UNITY_END();
}
