#include "unity.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "xy_actuator_motor.h"

typedef struct {
    uint8_t pin;
    uint8_t level;
} write_t;

static xy_hal_error_t g_init_results[2];
static unsigned g_init_calls;
static xy_hal_error_t g_write_results[32];
static unsigned g_write_result_count;
static write_t g_writes[32];
static unsigned g_write_calls;
static uint32_t g_delays[16];
static unsigned g_delay_calls;

xy_hal_error_t xy_hal_gpio_init(xy_hal_gpio_port_t port, uint8_t pin,
                                const xy_hal_gpio_config_t *config)
{
    TEST_ASSERT_EQUAL_PTR((void *)0x1234, port);
    TEST_ASSERT_TRUE(pin == 0U || pin == 1U);
    TEST_ASSERT_NOT_NULL(config);
    TEST_ASSERT_EQUAL(XY_HAL_GPIO_MODE_OUTPUT, config->mode);
    TEST_ASSERT_EQUAL(XY_HAL_GPIO_PULL_DOWN, config->pull);
    return g_init_results[g_init_calls++];
}

xy_hal_error_t xy_hal_gpio_write(xy_hal_gpio_port_t port, uint8_t pin, uint8_t level)
{
    TEST_ASSERT_EQUAL_PTR((void *)0x1234, port);
    TEST_ASSERT_LESS_THAN_UINT(32U, g_write_calls);
    g_writes[g_write_calls] = (write_t){pin, level};
    return g_write_calls < g_write_result_count ? g_write_results[g_write_calls++]
                                                 : (g_write_calls++, XY_HAL_OK);
}

static void fake_delay(uint32_t milliseconds)
{
    TEST_ASSERT_LESS_THAN_UINT(16U, g_delay_calls);
    g_delays[g_delay_calls++] = milliseconds;
}

static xy_actuator_motor_t make_motor(void)
{
    xy_actuator_motor_t motor = {
        .ina_port = (xy_hal_gpio_port_t)0x1234,
        .ina_pin = 1U,
        .inb_port = (xy_hal_gpio_port_t)0x1234,
        .inb_pin = 0U,
        .break_before_make_ms = 2U,
        .delay_ms = fake_delay,
    };
    return motor;
}

void setUp(void)
{
    memset(g_init_results, 0, sizeof(g_init_results));
    g_init_calls = 0U;
    memset(g_write_results, 0, sizeof(g_write_results));
    g_write_result_count = 0U;
    memset(g_writes, 0, sizeof(g_writes));
    g_write_calls = 0U;
    memset(g_delays, 0, sizeof(g_delays));
    g_delay_calls = 0U;
}

void tearDown(void) {}

static void assert_last_levels(uint8_t ina, uint8_t inb)
{
    TEST_ASSERT_GREATER_OR_EQUAL_UINT(2U, g_write_calls);
    TEST_ASSERT_EQUAL_UINT8(1U, g_writes[g_write_calls - 2U].pin);
    TEST_ASSERT_EQUAL_UINT8(ina, g_writes[g_write_calls - 2U].level);
    TEST_ASSERT_EQUAL_UINT8(0U, g_writes[g_write_calls - 1U].pin);
    TEST_ASSERT_EQUAL_UINT8(inb, g_writes[g_write_calls - 1U].level);
}

static void test_init_forces_standby_and_maps_pandora_ina_inb_order(void)
{
    xy_actuator_motor_t motor = make_motor();

    TEST_ASSERT_EQUAL(ACTUATOR_EOK, xy_actuator_motor_init(&motor));
    TEST_ASSERT_TRUE(motor.initialized);
    TEST_ASSERT_EQUAL(XY_ACTUATOR_MOTOR_STANDBY, motor.mode);
    TEST_ASSERT_EQUAL_UINT(2U, g_init_calls);
    assert_last_levels(0U, 0U);
}

static void test_truth_table_and_break_before_make(void)
{
    xy_actuator_motor_t motor = make_motor();

    TEST_ASSERT_EQUAL(ACTUATOR_EOK, xy_actuator_motor_init(&motor));
    TEST_ASSERT_EQUAL(ACTUATOR_EOK,
                      xy_actuator_motor_set_mode(&motor, XY_ACTUATOR_MOTOR_FORWARD));
    assert_last_levels(1U, 0U);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK,
                      xy_actuator_motor_set_mode(&motor, XY_ACTUATOR_MOTOR_REVERSE));
    TEST_ASSERT_EQUAL_UINT32(2U, g_delays[0]);
    TEST_ASSERT_EQUAL_UINT8(0U, g_writes[g_write_calls - 4U].level);
    TEST_ASSERT_EQUAL_UINT8(0U, g_writes[g_write_calls - 3U].level);
    assert_last_levels(0U, 1U);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK,
                      xy_actuator_motor_set_mode(&motor, XY_ACTUATOR_MOTOR_BRAKE));
    assert_last_levels(1U, 1U);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, xy_actuator_motor_standby(&motor));
    assert_last_levels(0U, 0U);
}

static void test_forward_only_pattern_is_bounded_and_finishes_standby(void)
{
    const xy_actuator_motor_step_t pattern[] = {
        {XY_ACTUATOR_MOTOR_FORWARD, 120U},
        {XY_ACTUATOR_MOTOR_STANDBY, 120U},
        {XY_ACTUATOR_MOTOR_FORWARD, 120U},
    };
    xy_actuator_motor_t motor = make_motor();

    TEST_ASSERT_EQUAL(ACTUATOR_EOK, xy_actuator_motor_init(&motor));
    TEST_ASSERT_EQUAL(ACTUATOR_EOK,
                      xy_actuator_motor_play(&motor, pattern,
                                             sizeof(pattern) / sizeof(pattern[0])));
    TEST_ASSERT_EQUAL(XY_ACTUATOR_MOTOR_STANDBY, motor.mode);
    TEST_ASSERT_EQUAL_UINT(3U, g_delay_calls);
    TEST_ASSERT_EQUAL_UINT32(120U, g_delays[2]);
    assert_last_levels(0U, 0U);

    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, xy_actuator_motor_forward_pulse(&motor, 5001U));
}

static void test_gpio_failures_propagate_and_attempt_standby(void)
{
    xy_actuator_motor_t motor = make_motor();

    g_init_results[1] = XY_HAL_ERROR_IO;
    TEST_ASSERT_EQUAL(ACTUATOR_EIO, xy_actuator_motor_init(&motor));
    TEST_ASSERT_FALSE(motor.initialized);
    assert_last_levels(0U, 0U);

    setUp();
    motor = make_motor();
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, xy_actuator_motor_init(&motor));
    g_write_result_count = g_write_calls + 2U;
    g_write_results[g_write_calls] = XY_HAL_OK;
    g_write_results[g_write_calls + 1U] = XY_HAL_ERROR_TIMEOUT;
    TEST_ASSERT_EQUAL(ACTUATOR_ETIMEOUT,
                      xy_actuator_motor_set_mode(&motor, XY_ACTUATOR_MOTOR_FORWARD));
    TEST_ASSERT_EQUAL(XY_ACTUATOR_MOTOR_STANDBY, motor.mode);
    assert_last_levels(0U, 0U);
}

static void test_invalid_parameters_have_no_gpio_side_effects(void)
{
    xy_actuator_motor_t motor = make_motor();
    xy_actuator_motor_step_t invalid = {XY_ACTUATOR_MOTOR_FORWARD, 0U};

    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, xy_actuator_motor_init(NULL));
    motor.ina_port = NULL;
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, xy_actuator_motor_init(&motor));
    TEST_ASSERT_EQUAL_UINT(0U, g_write_calls);
    motor = make_motor();
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL,
                      xy_actuator_motor_set_mode(&motor, XY_ACTUATOR_MOTOR_FORWARD));
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, xy_actuator_motor_play(&motor, &invalid, 1U));
    TEST_ASSERT_EQUAL_UINT(0U, g_write_calls);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_forces_standby_and_maps_pandora_ina_inb_order);
    RUN_TEST(test_truth_table_and_break_before_make);
    RUN_TEST(test_forward_only_pattern_is_bounded_and_finishes_standby);
    RUN_TEST(test_gpio_failures_propagate_and_attempt_standby);
    RUN_TEST(test_invalid_parameters_have_no_gpio_side_effects);
    return UNITY_END();
}