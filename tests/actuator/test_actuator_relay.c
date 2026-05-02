/**
 * @file test_actuator_relay.c
 * @brief Actuator Relay Tests - On/off, toggle operations
 * @version 1.0.0
 * @date 2026-02-28
 */

#include <stdint.h>
#include <string.h>
#include "unity.h"
#include "xy_actuator.h"

/* ==================== Mock Relay Operations ==================== */
static uint8_t g_mock_relay_state = RELAY_STATE_OFF;
static uint8_t g_mock_init_called = 0;
static uint8_t g_mock_deinit_called = 0;

static actuator_err_t mock_relay_init(actuator_device_t *dev)
{
    (void)dev;
    g_mock_init_called++;
    g_mock_relay_state = RELAY_STATE_OFF;
    return ACTUATOR_EOK;
}

static actuator_err_t mock_relay_deinit(actuator_device_t *dev)
{
    (void)dev;
    g_mock_deinit_called++;
    return ACTUATOR_EOK;
}

static actuator_err_t mock_relay_set(actuator_device_t *dev, uint8_t state)
{
    (void)dev;
    g_mock_relay_state = state;
    return ACTUATOR_EOK;
}

static actuator_err_t mock_relay_get(actuator_device_t *dev, uint8_t *state)
{
    (void)dev;
    *state = g_mock_relay_state;
    return ACTUATOR_EOK;
}

static actuator_err_t mock_relay_toggle(actuator_device_t *dev)
{
    (void)dev;
    g_mock_relay_state = (g_mock_relay_state == RELAY_STATE_OFF) ? RELAY_STATE_ON : RELAY_STATE_OFF;
    return ACTUATOR_EOK;
}

static const relay_ops_t mock_relay_ops = {
    .init = mock_relay_init,
    .set = mock_relay_set,
    .get = mock_relay_get,
    .toggle = mock_relay_toggle,
};

static const actuator_ops_t mock_actuator_ops = {
    .init = (actuator_err_t (*)(actuator_device_t *))mock_relay_init,
    .deinit = (actuator_err_t (*)(actuator_device_t *))mock_relay_deinit,
    .write = NULL,  /* Will be set per device */
    .read = NULL,
};

/* ==================== Test Device ==================== */
static actuator_device_t test_relay = {
    .name = "test_relay",
    .type = ACTUATOR_TYPE_RELAY,
    .ops = &mock_actuator_ops,
    .status = ACTUATOR_STATUS_IDLE,
};

/* ==================== Setup/Teardown ==================== */
void setUp(void)
{
    g_mock_relay_state = RELAY_STATE_OFF;
    g_mock_init_called = 0;
    g_mock_deinit_called = 0;
    actuator_unregister(&test_relay);
    test_relay.status = ACTUATOR_STATUS_IDLE;
}

void tearDown(void)
{
    actuator_unregister(&test_relay);
}

/* ==================== Relay Init Tests ==================== */
void test_relay_init(void)
{
    actuator_err_t ret = relay_init(&test_relay);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
    TEST_ASSERT_EQUAL(1, g_mock_init_called);
}

void test_relay_init_null(void)
{
    actuator_err_t ret = relay_init(NULL);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, ret);
}

/* ==================== Relay On/Off Tests ==================== */
void test_relay_on(void)
{
    actuator_register(&test_relay);
    actuator_err_t ret = relay_on(&test_relay);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
    TEST_ASSERT_EQUAL(RELAY_STATE_ON, g_mock_relay_state);
}

void test_relay_on_null(void)
{
    actuator_err_t ret = relay_on(NULL);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, ret);
}

void test_relay_off(void)
{
    actuator_register(&test_relay);
    g_mock_relay_state = RELAY_STATE_ON;

    actuator_err_t ret = relay_off(&test_relay);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, g_mock_relay_state);
}

void test_relay_off_null(void)
{
    actuator_err_t ret = relay_off(NULL);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, ret);
}

/* ==================== Relay Set Tests ==================== */
void test_relay_set(void)
{
    actuator_register(&test_relay);

    actuator_err_t ret = relay_set(&test_relay, RELAY_STATE_ON);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
    TEST_ASSERT_EQUAL(RELAY_STATE_ON, g_mock_relay_state);

    ret = relay_set(&test_relay, RELAY_STATE_OFF);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, g_mock_relay_state);
}

void test_relay_set_null(void)
{
    actuator_err_t ret = relay_set(NULL, RELAY_STATE_ON);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, ret);
}

/* ==================== Relay Get Tests ==================== */
void test_relay_get(void)
{
    uint8_t state;

    actuator_register(&test_relay);
    g_mock_relay_state = RELAY_STATE_ON;

    actuator_err_t ret = relay_get(&test_relay, &state);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
    TEST_ASSERT_EQUAL(RELAY_STATE_ON, state);

    g_mock_relay_state = RELAY_STATE_OFF;
    ret = relay_get(&test_relay, &state);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, state);
}

void test_relay_get_null(void)
{
    uint8_t state;
    actuator_err_t ret = relay_get(NULL, &state);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, ret);
}

void test_relay_get_null_state(void)
{
    actuator_err_t ret = relay_get(&test_relay, NULL);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, ret);
}

/* ==================== Relay Toggle Tests ==================== */
void test_relay_toggle(void)
{
    actuator_register(&test_relay);

    /* Initial state is OFF */
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, g_mock_relay_state);

    /* Toggle to ON */
    actuator_err_t ret = relay_toggle(&test_relay);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
    TEST_ASSERT_EQUAL(RELAY_STATE_ON, g_mock_relay_state);

    /* Toggle back to OFF */
    ret = relay_toggle(&test_relay);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, g_mock_relay_state);
}

void test_relay_toggle_null(void)
{
    actuator_err_t ret = relay_toggle(NULL);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, ret);
}

void test_relay_toggle_multiple(void)
{
    actuator_register(&test_relay);

    for (int i = 0; i < 10; i++) {
        relay_toggle(&test_relay);
        if (i % 2 == 0) {
            TEST_ASSERT_EQUAL(RELAY_STATE_ON, g_mock_relay_state);
        } else {
            TEST_ASSERT_EQUAL(RELAY_STATE_OFF, g_mock_relay_state);
        }
    }
}

/* ==================== Relay Pulse Tests ==================== */
void test_relay_pulse(void)
{
    actuator_register(&test_relay);
    g_mock_relay_state = RELAY_STATE_OFF;

    /* Pulse with 100ms width - just verify API accepts the call */
    actuator_err_t ret = relay_pulse(&test_relay, 100);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);

    /* Note: Actual pulse timing would need hardware/threads to test */
}

void test_relay_pulse_null(void)
{
    actuator_err_t ret = relay_pulse(NULL, 100);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, ret);
}

void test_relay_pulse_zero_width(void)
{
    actuator_register(&test_relay);
    actuator_err_t ret = relay_pulse(&test_relay, 0);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
}

void test_relay_pulse_large_width(void)
{
    actuator_register(&test_relay);
    actuator_err_t ret = relay_pulse(&test_relay, 60000);  /* 60 seconds */
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
}

/* ==================== Relay State Enum Tests ==================== */
void test_relay_state_enum_values(void)
{
    TEST_ASSERT_EQUAL(0, RELAY_STATE_OFF);
    TEST_ASSERT_EQUAL(1, RELAY_STATE_ON);
    TEST_ASSERT_EQUAL(2, RELAY_STATE_TOGGLE);
}

/* ==================== Relay Deinit Tests ==================== */
void test_relay_deinit(void)
{
    actuator_register(&test_relay);
    actuator_err_t ret = relay_deinit(&test_relay);
    TEST_ASSERT_EQUAL(ACTUATOR_EOK, ret);
    TEST_ASSERT_EQUAL(1, g_mock_deinit_called);
}

void test_relay_deinit_null(void)
{
    actuator_err_t ret = relay_deinit(NULL);
    TEST_ASSERT_EQUAL(ACTUATOR_EINVAL, ret);
}

/* ==================== Main ==================== */
int main(void)
{
    UNITY_BEGIN();

    /* Init Tests */
    RUN_TEST(test_relay_init);
    RUN_TEST(test_relay_init_null);

    /* On/Off Tests */
    RUN_TEST(test_relay_on);
    RUN_TEST(test_relay_on_null);
    RUN_TEST(test_relay_off);
    RUN_TEST(test_relay_off_null);

    /* Set Tests */
    RUN_TEST(test_relay_set);
    RUN_TEST(test_relay_set_null);

    /* Get Tests */
    RUN_TEST(test_relay_get);
    RUN_TEST(test_relay_get_null);
    RUN_TEST(test_relay_get_null_state);

    /* Toggle Tests */
    RUN_TEST(test_relay_toggle);
    RUN_TEST(test_relay_toggle_null);
    RUN_TEST(test_relay_toggle_multiple);

    /* Pulse Tests */
    RUN_TEST(test_relay_pulse);
    RUN_TEST(test_relay_pulse_null);
    RUN_TEST(test_relay_pulse_zero_width);
    RUN_TEST(test_relay_pulse_large_width);

    /* Enum Tests */
    RUN_TEST(test_relay_state_enum_values);

    /* Deinit Tests */
    RUN_TEST(test_relay_deinit);
    RUN_TEST(test_relay_deinit_null);

    return UNITY_END();
}
