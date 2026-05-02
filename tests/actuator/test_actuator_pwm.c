/**
 * @file test_actuator_pwm.c
 * @brief Actuator PWM Tests - Set duty, frequency operations
 * @version 1.0.0
 * @date 2026-02-28
 */

#include <stdint.h>
#include <string.h>
#include "unity.h"
#include "xy_actuator.h"

/* ==================== Mock PWM Operations ==================== */
static uint16_t g_mock_pwm_duty = 0;
static uint32_t g_mock_pwm_frequency = 1000;
static uint8_t g_mock_init_called = 0;

static actuator_err_t mock_pwm_init(actuator_device_t *dev)
{
    (void)dev;
    g_mock_init_called++;
    return ACTUATOR_EOK;
}

static actuator_err_t mock_pwm_write(actuator_device_t *dev, const actuator_value_t *value)
{
    (void)dev;
    g_mock_pwm_duty = value->pwm.duty;
    return ACTUATOR_EOK;
}

static const actuator_ops_t mock_pwm_ops = {
    .init = mock_pwm_init,
    .write = mock_pwm_write,
    .read = NULL,
};

/* ==================== Test Device ==================== */
static actuator_device_t test_pwm = {
    .name = "test_pwm",
    .type = ACTUATOR_TYPE_PWM,
    .ops = &mock_pwm_ops,
    .status = ACTUATOR_STATUS_IDLE,
    .config.pwm_freq = 1000,
    .config.pwm_resolution = 16,
};

/* ==================== Setup/Teardown ==================== */
void setUp(void)
{
    g_mock_pwm_duty = 0;
    g_mock_pwm_frequency = 1000;
    g_mock_init_called = 0;
    actuator_unregister(&test_pwm);
    test_pwm.status = ACTUATOR_STATUS_IDLE;
}

void tearDown(void)
{
    actuator_unregister(&test_pwm);
}

/* ==================== PWM Init Tests ==================== */
void test_pwm_init(void)
{
    /* PWM doesn't have its own init, uses actuator_init */
    actuator_err_t ret = actuator_init(&test_pwm);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
    TEST_ASSERT_EQUAL(1, g_mock_init_called);
}

void test_pwm_init_null(void)
{
    actuator_err_t ret = actuator_init(NULL);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, ret);
}

/* ==================== PWM Set Duty Tests ==================== */
void test_pwm_set_duty(void)
{
    actuator_register(&test_pwm);

    /* Set 0% duty */
    actuator_err_t ret = pwm_set_duty(&test_pwm, 0);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);

    /* Set 50% duty (32768 for 16-bit) */
    ret = pwm_set_duty(&test_pwm, 32768);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
    TEST_ASSERT_EQUAL(32768, g_mock_pwm_duty);

    /* Set 100% duty */
    ret = pwm_set_duty(&test_pwm, 65535);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
    TEST_ASSERT_EQUAL(65535, g_mock_pwm_duty);
}

void test_pwm_set_duty_null(void)
{
    actuator_err_t ret = pwm_set_duty(NULL, 32768);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, ret);
}

void test_pwm_set_duty_min(void)
{
    actuator_register(&test_pwm);
    actuator_err_t ret = pwm_set_duty(&test_pwm, 0);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
    TEST_ASSERT_EQUAL(0, g_mock_pwm_duty);
}

void test_pwm_set_duty_max(void)
{
    actuator_register(&test_pwm);
    actuator_err_t ret = pwm_set_duty(&test_pwm, 65535);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
    TEST_ASSERT_EQUAL(65535, g_mock_pwm_duty);
}

/* ==================== PWM Set Frequency Tests ==================== */
void test_pwm_set_frequency(void)
{
    actuator_register(&test_pwm);

    /* Set 100 Hz */
    actuator_err_t ret = pwm_set_frequency(&test_pwm, 100);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);

    /* Set 1 kHz */
    ret = pwm_set_frequency(&test_pwm, 1000);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);

    /* Set 10 kHz */
    ret = pwm_set_frequency(&test_pwm, 10000);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);

    /* Set 1 MHz */
    ret = pwm_set_frequency(&test_pwm, 1000000);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
}

void test_pwm_set_frequency_null(void)
{
    actuator_err_t ret = pwm_set_frequency(NULL, 1000);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, ret);
}

void test_pwm_set_frequency_low(void)
{
    actuator_register(&test_pwm);
    /* Some PWM hardware may have minimum frequency limits */
    actuator_err_t ret = pwm_set_frequency(&test_pwm, 1);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
}

void test_pwm_set_frequency_high(void)
{
    actuator_register(&test_pwm);
    /* Very high frequency - 10 MHz */
    actuator_err_t ret = pwm_set_frequency(&test_pwm, 10000000);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
}

/* ==================== PWM Value Structure Tests ==================== */
void test_pwm_value_struct(void)
{
    actuator_value_t value;

    /* Test PWM value union fields */
    value.pwm.duty = 12345;
    TEST_ASSERT_EQUAL(12345, value.pwm.duty);

    value.pwm.period = 1000000;  /* 1ms period in ns */
    TEST_ASSERT_EQUAL(1000000, value.pwm.period);
}

void test_pwm_write_using_actuator_write(void)
{
    actuator_register(&test_pwm);

    actuator_value_t value;
    value.pwm.duty = 5000;
    value.pwm.period = 1000000;

    actuator_err_t ret = actuator_write(&test_pwm, &value);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
    TEST_ASSERT_EQUAL(5000, g_mock_pwm_duty);
}

/* ==================== PWM Configuration Tests ==================== */
void test_pwm_config_initial(void)
{
    TEST_ASSERT_EQUAL(1000, test_pwm.config.pwm_freq);
    TEST_ASSERT_EQUAL(16, test_pwm.config.pwm_resolution);
}

void test_pwm_config_modify(void)
{
    test_pwm.config.pwm_freq = 2000;
    test_pwm.config.pwm_resolution = 8;

    TEST_ASSERT_EQUAL(2000, test_pwm.config.pwm_freq);
    TEST_ASSERT_EQUAL(8, test_pwm.config.pwm_resolution);
}

/* ==================== PWM Device Type Tests ==================== */
void test_pwm_type(void)
{
    TEST_ASSERT_EQUAL(ACTUATOR_TYPE_PWM, test_pwm.type);
}

void test_pwm_type_string(void)
{
    const char *str = actuator_type_str(ACTUATOR_TYPE_PWM);
    TEST_ASSERT_EQUAL_STRING("PWM", str);
}

/* ==================== Main ==================== */
int main(void)
{
    UNITY_BEGIN();

    /* Init Tests */
    RUN_TEST(test_pwm_init);
    RUN_TEST(test_pwm_init_null);

    /* Set Duty Tests */
    RUN_TEST(test_pwm_set_duty);
    RUN_TEST(test_pwm_set_duty_null);
    RUN_TEST(test_pwm_set_duty_min);
    RUN_TEST(test_pwm_set_duty_max);

    /* Set Frequency Tests */
    RUN_TEST(test_pwm_set_frequency);
    RUN_TEST(test_pwm_set_frequency_null);
    RUN_TEST(test_pwm_set_frequency_low);
    RUN_TEST(test_pwm_set_frequency_high);

    /* Value Structure Tests */
    RUN_TEST(test_pwm_value_struct);
    RUN_TEST(test_pwm_write_using_actuator_write);

    /* Configuration Tests */
    RUN_TEST(test_pwm_config_initial);
    RUN_TEST(test_pwm_config_modify);

    /* Device Type Tests */
    RUN_TEST(test_pwm_type);
    RUN_TEST(test_pwm_type_string);

    return UNITY_END();
}
