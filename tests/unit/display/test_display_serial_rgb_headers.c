/*
 * Public include guards for dormant serial RGB helper headers.
 */
#include "xy_rgb_color.h"
#include "xy_rgb_fx.h"
#include "xy_rgb_line.h"
#include "xy_rgb_circle.h"
#include "xy_rgb_segment.h"

#include <assert.h>

int main(void)
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

    assert(red.r == 255 && red.g == 0 && red.b == 0);
    assert(segment.start == 1 && segment.stop == 3);
    assert(circle.num_leds == 16);
    assert(point.x == 1 && point.y == -1);
    assert(info.id == FX_STATIC);
    assert(hsv.s == 255);
    return 0;
}
