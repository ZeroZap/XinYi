#include "unity.h"

#include "xy_gui_effect_blink.h"
#include "xy_gui_effect_breath.h"
#include "xy_gui_effect_fade.h"
#include "xy_gui_effect_rotate.h"
#include "xy_gui_effect_slide.h"

#include <stdbool.h>
#include <stdint.h>

void setUp(void) {}
void tearDown(void) {}

static void test_effect_lifecycle_handles_null_and_state_transitions(void)
{
    xy_effect_t effect = {
        .type = XY_EFFECT_TYPE_FADE,
        .state = XY_EFFECT_STATE_STOPPED,
        .duration = 100U,
        .elapsed = 40U,
        .repeat = 2U,
        .current_repeat = 1U,
        .progress = 0.4f,
    };

    xy_effect_start(NULL);
    xy_effect_stop(NULL);
    xy_effect_pause(NULL);
    xy_effect_resume(NULL);
    xy_effect_reset(NULL);

    xy_effect_start(&effect);
    TEST_ASSERT_TRUE(xy_effect_is_running(&effect));

    xy_effect_pause(&effect);
    TEST_ASSERT_EQUAL_INT(XY_EFFECT_STATE_PAUSED, effect.state);

    xy_effect_resume(&effect);
    TEST_ASSERT_EQUAL_INT(XY_EFFECT_STATE_RUNNING, effect.state);

    xy_effect_stop(&effect);
    TEST_ASSERT_EQUAL_INT(XY_EFFECT_STATE_STOPPED, effect.state);
    TEST_ASSERT_EQUAL_UINT32(0U, effect.elapsed);
    TEST_ASSERT_EQUAL_UINT16(0U, effect.current_repeat);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, effect.progress);

    effect.elapsed = 77U;
    effect.current_repeat = 1U;
    effect.progress = 0.77f;
    effect.state = XY_EFFECT_STATE_RUNNING;
    xy_effect_reset(&effect);
    TEST_ASSERT_EQUAL_INT(XY_EFFECT_STATE_STOPPED, effect.state);
    TEST_ASSERT_EQUAL_UINT32(0U, effect.elapsed);
    TEST_ASSERT_EQUAL_UINT16(0U, effect.current_repeat);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, xy_effect_get_progress(&effect));
    TEST_ASSERT_FALSE(xy_effect_is_running(NULL));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, xy_effect_get_progress(NULL));
}

static void test_fade_progress_alpha_and_repeat_contracts(void)
{
    xy_effect_fade_t fade;

    TEST_ASSERT_EQUAL_INT(-1, xy_effect_fade_create(NULL, true, 100U));
    TEST_ASSERT_EQUAL_INT(0, xy_effect_fade_create(&fade, true, 100U));
    TEST_ASSERT_EQUAL_INT(XY_EFFECT_TYPE_FADE, fade.base.type);
    TEST_ASSERT_TRUE(fade.fade_in);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, fade.start_alpha);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, fade.end_alpha);

    xy_effect_fade_update(&fade, 50U);
    TEST_ASSERT_EQUAL_UINT32(0U, fade.base.elapsed);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, fade.base.progress);

    xy_effect_start(&fade.base);
    xy_effect_fade_update(&fade, 50U);
    TEST_ASSERT_EQUAL_UINT32(50U, fade.base.elapsed);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.5f, fade.base.progress);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.75f, xy_effect_fade_get_alpha(&fade));

    xy_effect_fade_update(&fade, 75U);
    TEST_ASSERT_EQUAL_INT(XY_EFFECT_STATE_STOPPED, fade.base.state);
    TEST_ASSERT_EQUAL_UINT16(1U, fade.base.current_repeat);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, fade.base.progress);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, xy_effect_fade_get_alpha(&fade));

    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, xy_effect_fade_get_alpha(NULL));

    TEST_ASSERT_EQUAL_INT(0, xy_effect_fade_create(&fade, false, 0U));
    xy_effect_start(&fade.base);
    xy_effect_fade_update(&fade, 1U);
    TEST_ASSERT_EQUAL_INT(XY_EFFECT_STATE_STOPPED, fade.base.state);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, fade.base.progress);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, xy_effect_fade_get_alpha(&fade));
}

static void test_blink_brightness_and_zero_cycle_guards(void)
{
    xy_effect_blink_t blink;

    TEST_ASSERT_EQUAL_INT(-1, xy_effect_blink_create(NULL, 10U, 20U, 1U, 9U));
    TEST_ASSERT_EQUAL_INT(0, xy_effect_blink_create(&blink, 10U, 20U, 1U, 9U));
    TEST_ASSERT_EQUAL_INT(XY_EFFECT_TYPE_BLINK, blink.base.type);
    TEST_ASSERT_FALSE(xy_effect_blink_is_on(&blink));
    TEST_ASSERT_EQUAL_UINT8(1U, xy_effect_blink_get_brightness(&blink));

    xy_effect_start(&blink.base);
    TEST_ASSERT_TRUE(xy_effect_blink_is_on(&blink));
    TEST_ASSERT_EQUAL_UINT8(9U, xy_effect_blink_get_brightness(&blink));

    xy_effect_blink_update(&blink, 10U);
    TEST_ASSERT_FALSE(xy_effect_blink_is_on(&blink));
    TEST_ASSERT_EQUAL_UINT8(1U, xy_effect_blink_get_brightness(&blink));

    xy_effect_blink_update(&blink, 20U);
    TEST_ASSERT_TRUE(xy_effect_blink_is_on(&blink));
    TEST_ASSERT_EQUAL_UINT8(9U, xy_effect_blink_get_brightness(&blink));

    TEST_ASSERT_FALSE(xy_effect_blink_is_on(NULL));
    TEST_ASSERT_EQUAL_UINT8(0U, xy_effect_blink_get_brightness(NULL));

    TEST_ASSERT_EQUAL_INT(0, xy_effect_blink_create(&blink, 0U, 0U, 3U, 7U));
    xy_effect_start(&blink.base);
    TEST_ASSERT_FALSE(xy_effect_blink_is_on(&blink));
    TEST_ASSERT_EQUAL_UINT8(3U, xy_effect_blink_get_brightness(&blink));
}

static void test_breath_period_smooth_and_linear_contracts(void)
{
    xy_effect_breath_t breath;

    TEST_ASSERT_EQUAL_INT(-1, xy_effect_breath_create(NULL, 1000U, 10U, 110U));
    TEST_ASSERT_EQUAL_INT(0, xy_effect_breath_create(&breath, 1000U, 10U, 110U));
    TEST_ASSERT_EQUAL_INT(XY_EFFECT_TYPE_BREATH, breath.base.type);
    TEST_ASSERT_TRUE(breath.smooth);

    xy_effect_start(&breath.base);
    xy_effect_breath_update(&breath, 250U);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.25f, breath.base.progress);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, xy_effect_breath_get_sine_value(&breath));
    TEST_ASSERT_EQUAL_UINT8(110U, xy_effect_breath_get_brightness(&breath));

    breath.smooth = false;
    breath.base.progress = 0.25f;
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.5f, xy_effect_breath_get_sine_value(&breath));
    breath.base.progress = 0.75f;
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.5f, xy_effect_breath_get_sine_value(&breath));

    TEST_ASSERT_EQUAL_INT(0, xy_effect_breath_create(&breath, 0U, 4U, 8U));
    xy_effect_start(&breath.base);
    xy_effect_breath_update(&breath, 50U);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, breath.base.progress);

    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, xy_effect_breath_get_sine_value(NULL));
    TEST_ASSERT_EQUAL_UINT8(0U, xy_effect_breath_get_brightness(NULL));
}

static void test_slide_offset_and_completion_contracts(void)
{
    xy_effect_slide_t slide;

    TEST_ASSERT_EQUAL_INT(-1, xy_effect_slide_create(NULL, XY_EFFECT_DIR_LEFT, -100, 100U));
    TEST_ASSERT_EQUAL_INT(0, xy_effect_slide_create(&slide, XY_EFFECT_DIR_LEFT, -100, 100U));
    TEST_ASSERT_EQUAL_INT(XY_EFFECT_TYPE_SLIDE, slide.base.type);

    xy_effect_start(&slide.base);
    xy_effect_slide_update(&slide, 50U);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.5f, slide.base.progress);
    TEST_ASSERT_EQUAL_INT(-75, xy_effect_slide_get_offset(&slide));

    xy_effect_slide_update(&slide, 60U);
    TEST_ASSERT_EQUAL_INT(XY_EFFECT_STATE_STOPPED, slide.base.state);
    TEST_ASSERT_EQUAL_INT(-100, xy_effect_slide_get_offset(&slide));
    TEST_ASSERT_EQUAL_INT(0, xy_effect_slide_get_offset(NULL));

    TEST_ASSERT_EQUAL_INT(0, xy_effect_slide_create(&slide, XY_EFFECT_DIR_NONE, 40, 0U));
    xy_effect_start(&slide.base);
    xy_effect_slide_update(&slide, 1U);
    TEST_ASSERT_EQUAL_INT(XY_EFFECT_STATE_STOPPED, slide.base.state);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, slide.base.progress);
    TEST_ASSERT_EQUAL_INT(0, xy_effect_slide_get_offset(&slide));
}

static void test_rotate_angle_normalization_and_direction_contracts(void)
{
    xy_effect_rotate_t rotate;

    TEST_ASSERT_EQUAL_INT(-1, xy_effect_rotate_create(NULL, 0.0f, 90.0f, 100U));
    TEST_ASSERT_EQUAL_INT(0, xy_effect_rotate_create(&rotate, 0.0f, 90.0f, 100U));
    TEST_ASSERT_EQUAL_INT(XY_EFFECT_TYPE_ROTATE, rotate.base.type);
    TEST_ASSERT_TRUE(rotate.clockwise);

    xy_effect_start(&rotate.base);
    xy_effect_rotate_update(&rotate, 50U);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.5f, rotate.base.progress);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 67.5f, xy_effect_rotate_get_angle(&rotate));

    rotate.clockwise = false;
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 22.5f, xy_effect_rotate_get_angle(&rotate));

    TEST_ASSERT_EQUAL_INT(0, xy_effect_rotate_create(&rotate, 350.0f, 370.0f, 100U));
    xy_effect_start(&rotate.base);
    xy_effect_rotate_update(&rotate, 100U);
    TEST_ASSERT_EQUAL_INT(XY_EFFECT_STATE_STOPPED, rotate.base.state);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 10.0f, xy_effect_rotate_get_angle(&rotate));

    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, xy_effect_rotate_get_angle(NULL));
}

static void test_effect_utility_functions_are_bounded(void)
{
    TEST_ASSERT_EQUAL_INT(0, xy_effect_init());
    TEST_ASSERT_EQUAL_INT(0, xy_effect_init());
    TEST_ASSERT_EQUAL_INT(0, xy_effect_deinit());

    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 10.0f, xy_effect_lerp(10.0f, 20.0f, -0.1f));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 20.0f, xy_effect_lerp(10.0f, 20.0f, 1.1f));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 15.0f, xy_effect_lerp(10.0f, 20.0f, 0.5f));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.25f, xy_effect_ease_in(0.5f));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.75f, xy_effect_ease_out(0.5f));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.5f, xy_effect_ease_in_out(0.5f));
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(0U, xy_effect_get_time_ms());
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_effect_lifecycle_handles_null_and_state_transitions);
    RUN_TEST(test_fade_progress_alpha_and_repeat_contracts);
    RUN_TEST(test_blink_brightness_and_zero_cycle_guards);
    RUN_TEST(test_breath_period_smooth_and_linear_contracts);
    RUN_TEST(test_slide_offset_and_completion_contracts);
    RUN_TEST(test_rotate_angle_normalization_and_direction_contracts);
    RUN_TEST(test_effect_utility_functions_are_bounded);
    return UNITY_END();
}
