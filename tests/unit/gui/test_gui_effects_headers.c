#include "unity.h"

#include "xy_gui_effects.h"
#include "xy_gui_effect_blink.h"
#include "xy_gui_effect_breath.h"
#include "xy_gui_effect_fade.h"
#include "xy_gui_effect_rotate.h"
#include "xy_gui_effect_slide.h"

#include <stdbool.h>
#include <stdint.h>

void setUp(void) {}
void tearDown(void) {}

static void test_effects_headers_expose_public_types(void)
{
    xy_effect_t effect = {
        .type = XY_EFFECT_TYPE_NONE,
        .state = XY_EFFECT_STATE_STOPPED,
        .duration = 100U,
        .elapsed = 0U,
        .repeat = 1U,
        .current_repeat = 0U,
        .progress = 0.0f,
    };
    xy_effect_fade_t fade = {.base = effect, .start_alpha = 0.0f, .end_alpha = 1.0f, .fade_in = true};
    xy_effect_blink_t blink = {
        .base = effect,
        .on_time = 10U,
        .off_time = 20U,
        .min_brightness = 1U,
        .max_brightness = 255U,
    };
    xy_effect_breath_t breath = {
        .base = effect,
        .period = 1000U,
        .min_brightness = 2U,
        .max_brightness = 200U,
        .smooth = true,
    };
    xy_effect_slide_t slide = {
        .base = effect,
        .direction = XY_EFFECT_DIR_LEFT,
        .start_offset = 0,
        .end_offset = -16,
    };
    xy_effect_rotate_t rotate = {
        .base = effect,
        .start_angle = 0.0f,
        .end_angle = 90.0f,
        .center_x = 0.5f,
        .center_y = 0.5f,
        .clockwise = true,
    };

    TEST_ASSERT_EQUAL_INT(XY_EFFECT_TYPE_NONE, effect.type);
    TEST_ASSERT_EQUAL_INT(XY_EFFECT_TYPE_FADE, XY_EFFECT_TYPE_FADE);
    TEST_ASSERT_EQUAL_INT(XY_EFFECT_TYPE_BLINK, XY_EFFECT_TYPE_BLINK);
    TEST_ASSERT_EQUAL_INT(XY_EFFECT_TYPE_BREATH, XY_EFFECT_TYPE_BREATH);
    TEST_ASSERT_EQUAL_INT(XY_EFFECT_TYPE_SLIDE, XY_EFFECT_TYPE_SLIDE);
    TEST_ASSERT_EQUAL_INT(XY_EFFECT_TYPE_ROTATE, XY_EFFECT_TYPE_ROTATE);
    TEST_ASSERT_TRUE(fade.fade_in);
    TEST_ASSERT_EQUAL_UINT16(10U, blink.on_time);
    TEST_ASSERT_EQUAL_UINT16(1000U, breath.period);
    TEST_ASSERT_EQUAL_INT(XY_EFFECT_DIR_LEFT, slide.direction);
    TEST_ASSERT_TRUE(rotate.clockwise);
}

static void test_effects_headers_expose_public_function_signatures(void)
{
    typedef int (*fade_create_fn_t)(xy_effect_fade_t *, bool, uint32_t);
    typedef void (*fade_update_fn_t)(xy_effect_fade_t *, uint32_t);
    typedef float (*fade_alpha_fn_t)(xy_effect_fade_t *);
    typedef int (*blink_create_fn_t)(xy_effect_blink_t *, uint16_t, uint16_t, uint8_t, uint8_t);
    typedef uint8_t (*blink_brightness_fn_t)(xy_effect_blink_t *);
    typedef int (*breath_create_fn_t)(xy_effect_breath_t *, uint16_t, uint8_t, uint8_t);
    typedef uint8_t (*breath_brightness_fn_t)(xy_effect_breath_t *);
    typedef int (*slide_create_fn_t)(xy_effect_slide_t *, xy_effect_dir_t, int16_t, uint32_t);
    typedef int16_t (*slide_offset_fn_t)(xy_effect_slide_t *);
    typedef int (*rotate_create_fn_t)(xy_effect_rotate_t *, float, float, uint32_t);
    typedef float (*rotate_angle_fn_t)(xy_effect_rotate_t *);

    TEST_ASSERT_GREATER_THAN_UINT(0U, sizeof(fade_create_fn_t));
    TEST_ASSERT_GREATER_THAN_UINT(0U, sizeof(fade_update_fn_t));
    TEST_ASSERT_GREATER_THAN_UINT(0U, sizeof(fade_alpha_fn_t));
    TEST_ASSERT_GREATER_THAN_UINT(0U, sizeof(blink_create_fn_t));
    TEST_ASSERT_GREATER_THAN_UINT(0U, sizeof(blink_brightness_fn_t));
    TEST_ASSERT_GREATER_THAN_UINT(0U, sizeof(breath_create_fn_t));
    TEST_ASSERT_GREATER_THAN_UINT(0U, sizeof(breath_brightness_fn_t));
    TEST_ASSERT_GREATER_THAN_UINT(0U, sizeof(slide_create_fn_t));
    TEST_ASSERT_GREATER_THAN_UINT(0U, sizeof(slide_offset_fn_t));
    TEST_ASSERT_GREATER_THAN_UINT(0U, sizeof(rotate_create_fn_t));
    TEST_ASSERT_GREATER_THAN_UINT(0U, sizeof(rotate_angle_fn_t));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_effects_headers_expose_public_types);
    RUN_TEST(test_effects_headers_expose_public_function_signatures);
    return UNITY_END();
}
