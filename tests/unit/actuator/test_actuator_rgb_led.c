#include "unity.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "xy_actuator_rgb_led.h"

static xy_hal_error_t g_init_results[3];
static unsigned g_init_calls;
static xy_hal_error_t g_write_results[64];
static unsigned g_write_result_count;
static unsigned g_write_calls;
static uint8_t g_write_pins[64];
static uint8_t g_write_levels[64];
static unsigned g_delay_calls;
static uint32_t g_delays[16];

xy_hal_error_t xy_hal_gpio_init(xy_hal_gpio_port_t port, uint8_t pin,
                                const xy_hal_gpio_config_t *config)
{
    TEST_ASSERT_EQUAL_PTR((void *)0x1234, port);
    TEST_ASSERT_TRUE(pin >= 7U && pin <= 9U);
    TEST_ASSERT_NOT_NULL(config);
    TEST_ASSERT_EQUAL(XY_HAL_GPIO_MODE_OUTPUT, config->mode);
    return g_init_calls < 3U ? g_init_results[g_init_calls++] : XY_HAL_ERROR_IO;
}

xy_hal_error_t xy_hal_gpio_write(xy_hal_gpio_port_t port, uint8_t pin, uint8_t value)
{
    xy_hal_error_t result = XY_HAL_OK;
    TEST_ASSERT_EQUAL_PTR((void *)0x1234, port);
    TEST_ASSERT_LESS_THAN_UINT(64U, g_write_calls);
    g_write_pins[g_write_calls] = pin;
    g_write_levels[g_write_calls] = value;
    if (g_write_calls < g_write_result_count) {
        result = g_write_results[g_write_calls];
    }
    ++g_write_calls;
    return result;
}

static void fake_delay(uint32_t milliseconds)
{
    TEST_ASSERT_LESS_THAN_UINT(16U, g_delay_calls);
    g_delays[g_delay_calls++] = milliseconds;
}

void setUp(void)
{
    memset(g_init_results, 0, sizeof(g_init_results));
    g_init_calls = 0U;
    memset(g_write_results, 0, sizeof(g_write_results));
    g_write_result_count = 0U;
    g_write_calls = 0U;
    memset(g_write_pins, 0, sizeof(g_write_pins));
    memset(g_write_levels, 0, sizeof(g_write_levels));
    g_delay_calls = 0U;
    memset(g_delays, 0, sizeof(g_delays));
}

void tearDown(void) {}

static xy_actuator_rgb_led_t make_led(bool active_low)
{
    xy_actuator_rgb_led_t led = {
        .port = (xy_hal_gpio_port_t)0x1234,
        .red_pin = 7U,
        .green_pin = 8U,
        .blue_pin = 9U,
        .active_low = active_low,
        .delay_ms = fake_delay,
    };
    return led;
}

static void assert_last_three_levels(uint8_t red, uint8_t green, uint8_t blue)
{
    TEST_ASSERT_GREATER_OR_EQUAL_UINT(3U, g_write_calls);
    TEST_ASSERT_EQUAL_UINT8(7U, g_write_pins[g_write_calls - 3U]);
    TEST_ASSERT_EQUAL_UINT8(red, g_write_levels[g_write_calls - 3U]);
    TEST_ASSERT_EQUAL_UINT8(8U, g_write_pins[g_write_calls - 2U]);
    TEST_ASSERT_EQUAL_UINT8(green, g_write_levels[g_write_calls - 2U]);
    TEST_ASSERT_EQUAL_UINT8(9U, g_write_pins[g_write_calls - 1U]);
    TEST_ASSERT_EQUAL_UINT8(blue, g_write_levels[g_write_calls - 1U]);
}

static void test_init_forces_all_channels_inactive(void)
{
    xy_actuator_rgb_led_t led = make_led(true);

    TEST_ASSERT_EQUAL(ACTUATOR_EOK, xy_actuator_rgb_led_init(&led));
    TEST_ASSERT_TRUE(led.initialized);
    TEST_ASSERT_EQUAL(XY_ACTUATOR_RGB_OFF, led.color);
    TEST_ASSERT_EQUAL_UINT(3U, g_init_calls);
    assert_last_three_levels(1U, 1U, 1U);
}

static void test_set_maps_active_low_colors_and_disables_first(void)
{
    xy_actuator_rgb_led_t led = make_led(true);

    TEST_ASSERT_EQUAL(ACTUATOR_EOK, xy_actuator_rgb_led_init(&led));
    g_write_calls = 0U;
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, xy_actuator_rgb_led_set(&led, XY_ACTUATOR_RGB_MAGENTA));
    TEST_ASSERT_EQUAL_UINT(5U, g_write_calls);
    TEST_ASSERT_EQUAL_UINT8(1U, g_write_levels[0]);
    TEST_ASSERT_EQUAL_UINT8(1U, g_write_levels[1]);
    TEST_ASSERT_EQUAL_UINT8(1U, g_write_levels[2]);
    TEST_ASSERT_EQUAL_UINT8(7U, g_write_pins[3]);
    TEST_ASSERT_EQUAL_UINT8(0U, g_write_levels[3]);
    TEST_ASSERT_EQUAL_UINT8(9U, g_write_pins[4]);
    TEST_ASSERT_EQUAL_UINT8(0U, g_write_levels[4]);
    TEST_ASSERT_EQUAL(XY_ACTUATOR_RGB_MAGENTA, led.color);
}

static void test_pattern_finishes_off(void)
{
    static const xy_actuator_rgb_led_step_t pattern[] = {
        {XY_ACTUATOR_RGB_RED, 200U},
        {XY_ACTUATOR_RGB_GREEN, 200U},
        {XY_ACTUATOR_RGB_BLUE, 200U},
        {XY_ACTUATOR_RGB_WHITE, 400U},
    };
    xy_actuator_rgb_led_t led = make_led(true);

    TEST_ASSERT_EQUAL(ACTUATOR_EOK, xy_actuator_rgb_led_init(&led));
    TEST_ASSERT_EQUAL(ACTUATOR_EOK,
                      xy_actuator_rgb_led_play(&led, pattern,
                                               sizeof(pattern) / sizeof(pattern[0])));
    TEST_ASSERT_EQUAL_UINT(4U, g_delay_calls);
    TEST_ASSERT_EQUAL_UINT32(200U, g_delays[0]);
    TEST_ASSERT_EQUAL_UINT32(400U, g_delays[3]);
    TEST_ASSERT_EQUAL(XY_ACTUATOR_RGB_OFF, led.color);
    assert_last_three_levels(1U, 1U, 1U);
}

static void test_invalid_parameters_have_no_side_effects(void)
{
    xy_actuator_rgb_led_t led = make_led(true);
    xy_actuator_rgb_led_step_t invalid = {XY_ACTUATOR_RGB_RED, 0U};

    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, xy_actuator_rgb_led_init(NULL));
    led.green_pin = led.red_pin;
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, xy_actuator_rgb_led_init(&led));
    TEST_ASSERT_EQUAL_UINT(0U, g_write_calls);
    led = make_led(true);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, xy_actuator_rgb_led_set(&led, XY_ACTUATOR_RGB_RED));
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, xy_actuator_rgb_led_play(&led, NULL, 1U));
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, xy_actuator_rgb_led_play(&led, &invalid, 1U));
}

static void test_init_and_write_failures_leave_fail_safe_off(void)
{
    xy_actuator_rgb_led_t led = make_led(true);

    g_init_results[1] = XY_HAL_ERROR_IO;
    TEST_ASSERT_EQUAL(ACTUATOR_EIO, xy_actuator_rgb_led_init(&led));
    TEST_ASSERT_FALSE(led.initialized);
    TEST_ASSERT_EQUAL(XY_ACTUATOR_RGB_OFF, led.color);
    assert_last_three_levels(1U, 1U, 1U);

    setUp();
    led = make_led(true);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, xy_actuator_rgb_led_init(&led));
    g_write_calls = 0U;
    g_write_result_count = 2U;
    g_write_results[1] = XY_HAL_ERROR_IO;
    TEST_ASSERT_EQUAL(ACTUATOR_EIO, xy_actuator_rgb_led_set(&led, XY_ACTUATOR_RGB_RED));
    TEST_ASSERT_EQUAL(XY_ACTUATOR_RGB_OFF, led.color);
    assert_last_three_levels(1U, 1U, 1U);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_forces_all_channels_inactive);
    RUN_TEST(test_set_maps_active_low_colors_and_disables_first);
    RUN_TEST(test_pattern_finishes_off);
    RUN_TEST(test_invalid_parameters_have_no_side_effects);
    RUN_TEST(test_init_and_write_failures_leave_fail_safe_off);
    return UNITY_END();
}
