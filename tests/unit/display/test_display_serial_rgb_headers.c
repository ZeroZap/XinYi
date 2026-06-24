/*
 * Public include guards for dormant serial RGB helper headers.
 */
#include "xy_rgb_color.h"
#include "xy_rgb_fx.h"
#include "xy_rgb_line.h"
#include "xy_rgb_circle.h"
#include "xy_rgb_segment.h"

#include "unity.h"

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_serial_rgb_public_headers_expose_expected_types(void)
{
    rgb_color_t red = RGB_COLOR_RED;
    hsv_color_t hsv = {0};
    xy_rgb_segment_t segment = {0};
    xy_rgb_circle_config_t circle = {0};
    point2d_t point = {0};
    const xy_rgb_fx_info_t info = {
        .id = FX_STATIC,
        .name = "static",
        .handler = 0,
        .min_speed = 0,
        .max_speed = 255,
    };

    segment.start = 1;
    segment.stop = 3;
    segment.color1 = red;
    segment.effect = FX_RAINBOW;
    segment.enabled = true;

    circle.num_leds = 16;
    circle.center_x = 8;
    circle.center_y = 8;
    circle.radius = 7;

    point.x = 1;
    point.y = -1;
    hsv.h = 0;
    hsv.s = 255;
    hsv.v = 255;

    TEST_ASSERT_EQUAL_UINT8(255U, red.r);
    TEST_ASSERT_EQUAL_UINT8(0U, red.g);
    TEST_ASSERT_EQUAL_UINT8(0U, red.b);
    TEST_ASSERT_EQUAL_UINT16(1U, segment.start);
    TEST_ASSERT_EQUAL_UINT16(3U, segment.stop);
    TEST_ASSERT_EQUAL_UINT16(16U, circle.num_leds);
    TEST_ASSERT_EQUAL_INT16(1, point.x);
    TEST_ASSERT_EQUAL_INT16(-1, point.y);
    TEST_ASSERT_EQUAL(FX_STATIC, info.id);
    TEST_ASSERT_EQUAL_UINT8(255U, hsv.s);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_serial_rgb_public_headers_expose_expected_types);
    return UNITY_END();
}
