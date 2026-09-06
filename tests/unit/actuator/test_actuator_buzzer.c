#include "unity.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "xy_actuator_buzzer.h"

static xy_hal_error_t g_init_result;
static xy_hal_error_t g_write_results[16];
static unsigned g_write_result_count;
static unsigned g_write_calls;
static uint8_t g_write_levels[16];
static unsigned g_delay_calls;
static uint32_t g_delays[16];

xy_hal_error_t xy_hal_gpio_init(xy_hal_gpio_port_t port, uint8_t pin,
                                const xy_hal_gpio_config_t *config)
{
    TEST_ASSERT_EQUAL_PTR((void *)0x1234, port);
    TEST_ASSERT_EQUAL_UINT8(2U, pin);
    TEST_ASSERT_NOT_NULL(config);
    TEST_ASSERT_EQUAL(XY_HAL_GPIO_MODE_OUTPUT, config->mode);
    return g_init_result;
}

xy_hal_error_t xy_hal_gpio_write(xy_hal_gpio_port_t port, uint8_t pin, uint8_t value)
{
    TEST_ASSERT_EQUAL_PTR((void *)0x1234, port);
    TEST_ASSERT_EQUAL_UINT8(2U, pin);
    TEST_ASSERT_LESS_THAN_UINT(16U, g_write_calls);
    g_write_levels[g_write_calls] = value;
    return g_write_calls < g_write_result_count ? g_write_results[g_write_calls++]
                                                 : (g_write_calls++, XY_HAL_OK);
}

static void fake_delay(uint32_t milliseconds)
{
    TEST_ASSERT_LESS_THAN_UINT(16U, g_delay_calls);
    g_delays[g_delay_calls++] = milliseconds;
}

void setUp(void)
{
    g_init_result = XY_HAL_OK;
    memset(g_write_results, 0, sizeof(g_write_results));
    g_write_result_count = 0U;
    g_write_calls = 0U;
    memset(g_write_levels, 0, sizeof(g_write_levels));
    g_delay_calls = 0U;
    memset(g_delays, 0, sizeof(g_delays));
}

void tearDown(void) {}

static xy_actuator_buzzer_t make_buzzer(bool active_high)
{
    xy_actuator_buzzer_t buzzer = {0};
    buzzer.port = (xy_hal_gpio_port_t)0x1234;
    buzzer.pin = 2U;
    buzzer.active_high = active_high;
    buzzer.delay_ms = fake_delay;
    return buzzer;
}

static void test_init_forces_safe_off_before_and_after_gpio_configuration(void)
{
    xy_actuator_buzzer_t buzzer = make_buzzer(true);

    TEST_ASSERT_EQUAL(ACTUATOR_EOK, xy_actuator_buzzer_init(&buzzer));
    TEST_ASSERT_EQUAL_UINT(2U, g_write_calls);
    TEST_ASSERT_EQUAL_UINT8(0U, g_write_levels[0]);
    TEST_ASSERT_EQUAL_UINT8(0U, g_write_levels[1]);
    TEST_ASSERT_FALSE(buzzer.is_on);
    TEST_ASSERT_TRUE(buzzer.initialized);
}

static void test_on_off_and_active_low_mapping(void)
{
    xy_actuator_buzzer_t buzzer = make_buzzer(false);

    TEST_ASSERT_EQUAL(ACTUATOR_EOK, xy_actuator_buzzer_init(&buzzer));
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, xy_actuator_buzzer_on(&buzzer));
    TEST_ASSERT_TRUE(buzzer.is_on);
    TEST_ASSERT_EQUAL_UINT8(0U, g_write_levels[2]);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, xy_actuator_buzzer_off(&buzzer));
    TEST_ASSERT_FALSE(buzzer.is_on);
    TEST_ASSERT_EQUAL_UINT8(1U, g_write_levels[3]);
}

static void test_pulse_and_pattern_finish_off(void)
{
    static const xy_actuator_buzzer_step_t pattern[] = {
        {true, 100U}, {false, 80U}, {true, 100U}, {false, 80U}, {true, 300U},
    };
    xy_actuator_buzzer_t buzzer = make_buzzer(true);

    TEST_ASSERT_EQUAL(ACTUATOR_EOK, xy_actuator_buzzer_init(&buzzer));
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, xy_actuator_buzzer_pulse(&buzzer, 25U));
    TEST_ASSERT_EQUAL_UINT32(25U, g_delays[0]);
    TEST_ASSERT_FALSE(buzzer.is_on);

    TEST_ASSERT_EQUAL(ACTUATOR_EOK,
                      xy_actuator_buzzer_play(&buzzer, pattern,
                                              sizeof(pattern) / sizeof(pattern[0])));
    TEST_ASSERT_EQUAL_UINT(6U, g_delay_calls);
    TEST_ASSERT_EQUAL_UINT32(300U, g_delays[5]);
    TEST_ASSERT_FALSE(buzzer.is_on);
    TEST_ASSERT_EQUAL_UINT8(0U, g_write_levels[g_write_calls - 1U]);
}

static void test_invalid_parameters_have_no_side_effects(void)
{
    xy_actuator_buzzer_t buzzer = make_buzzer(true);
    xy_actuator_buzzer_step_t invalid = {true, 0U};

    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, xy_actuator_buzzer_init(NULL));
    buzzer.port = NULL;
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, xy_actuator_buzzer_init(&buzzer));
    TEST_ASSERT_EQUAL_UINT(0U, g_write_calls);

    buzzer = make_buzzer(true);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, xy_actuator_buzzer_on(&buzzer));
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, xy_actuator_buzzer_play(&buzzer, NULL, 1U));
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, xy_actuator_buzzer_play(&buzzer, &invalid, 1U));
    TEST_ASSERT_EQUAL_UINT(0U, g_write_calls);
}

static void test_failures_propagate_and_attempt_fail_safe_off(void)
{
    xy_actuator_buzzer_t buzzer = make_buzzer(true);
    xy_actuator_buzzer_step_t pattern[] = {{true, 10U}, {false, 10U}};

    g_init_result = XY_HAL_ERROR_IO;
    TEST_ASSERT_EQUAL(ACTUATOR_EIO, xy_actuator_buzzer_init(&buzzer));
    TEST_ASSERT_FALSE(buzzer.initialized);
    TEST_ASSERT_FALSE(buzzer.is_on);

    setUp();
    buzzer = make_buzzer(true);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, xy_actuator_buzzer_init(&buzzer));
    g_write_result_count = 5U;
    g_write_results[2] = XY_HAL_OK;
    g_write_results[3] = XY_HAL_ERROR_IO;
    g_write_results[4] = XY_HAL_OK;
    TEST_ASSERT_EQUAL(ACTUATOR_EIO, xy_actuator_buzzer_play(&buzzer, pattern, 2U));
    TEST_ASSERT_FALSE(buzzer.is_on);
    TEST_ASSERT_EQUAL_UINT8(0U, g_write_levels[4]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_forces_safe_off_before_and_after_gpio_configuration);
    RUN_TEST(test_on_off_and_active_low_mapping);
    RUN_TEST(test_pulse_and_pattern_finish_off);
    RUN_TEST(test_invalid_parameters_have_no_side_effects);
    RUN_TEST(test_failures_propagate_and_attempt_fail_safe_off);
    return UNITY_END();
}
