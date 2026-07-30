#include "xy_fuel_gauge.h"
#include "xy_fuel_gauge_safety.h"
#include "xy_fuel_gauge_security.h"
#include "xy_fuel_gauge_status.h"

#include "unity.h"
#include "fff.h"
#include <stdint.h>
#include <string.h>

static xy_fuel_gauge_data_t fake_data;

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(uint32_t, xy_os_tick_get)
FAKE_VALUE_FUNC(int, fake_init, xy_fuel_gauge_t *)
FAKE_VALUE_FUNC(int, fake_fetch, xy_fuel_gauge_t *)
FAKE_VALUE_FUNC(int, fake_channel_get, xy_fuel_gauge_t *, xy_fuel_gauge_data_type_t, int32_t *)
FAKE_VALUE_FUNC(int, fake_alert_set, xy_fuel_gauge_t *, const xy_fuel_gauge_alert_t *)
FAKE_VALUE_FUNC(int, fake_alert_get, xy_fuel_gauge_t *, xy_fuel_gauge_alert_t *)
FAKE_VOID_FUNC(count_device, xy_fuel_gauge_t *, void *)

void setUp(void)
{
}

void tearDown(void)
{
}

static int fake_init_impl(xy_fuel_gauge_t *fg)
{
    fg->latest.voltage_mv = 3700;
    fg->latest.current_ma = -120;
    fg->latest.soc = 66;
    fg->latest.soh = 95;
    fg->latest.temperature_c = 245;
    fg->latest.cycle_count = 7;
    fg->latest.full_capacity_mah = 2000;
    fg->latest.remain_capacity_mah = 1320;
    return 0;
}

static int fake_init_error_impl(xy_fuel_gauge_t *fg)
{
    fg->latest.voltage_mv = 0xEEEE;
    return XY_FG_ERROR_INVALID_PARAM;
}

static int fake_fetch_impl(xy_fuel_gauge_t *fg)
{
    fg->latest.voltage_mv++;
    return XY_FG_OK;
}

static int fake_channel_get_impl(xy_fuel_gauge_t *fg, xy_fuel_gauge_data_type_t channel, int32_t *val)
{
    if (!fg || !val) {
        return XY_FG_ERROR_INVALID_PARAM;
    }

    switch (channel) {
    case XY_FG_DATA_VOLTAGE:
        *val = fake_data.voltage_mv;
        break;
    case XY_FG_DATA_CURRENT:
        *val = fake_data.current_ma;
        break;
    case XY_FG_DATA_SOC:
        *val = fake_data.soc;
        break;
    case XY_FG_DATA_SOH:
        *val = fake_data.soh;
        break;
    case XY_FG_DATA_TEMPERATURE:
        *val = fake_data.temperature_c;
        break;
    case XY_FG_DATA_CYCLE_COUNT:
        *val = fake_data.cycle_count;
        break;
    case XY_FG_DATA_FULL_CAPACITY:
        *val = fake_data.full_capacity_mah;
        break;
    case XY_FG_DATA_REMAIN_CAPACITY:
        *val = fake_data.remain_capacity_mah;
        break;
    default:
        return XY_FG_ERROR_NOT_SUPPORTED;
    }

    return XY_FG_OK;
}

static void reset_fixture(void)
{
    RESET_FAKE(xy_os_tick_get);
    RESET_FAKE(fake_init);
    RESET_FAKE(fake_fetch);
    RESET_FAKE(fake_channel_get);
    RESET_FAKE(fake_alert_set);
    RESET_FAKE(fake_alert_get);
    RESET_FAKE(count_device);
    FFF_RESET_HISTORY();

    xy_os_tick_get_fake.return_val = 1234;
    fake_init_fake.custom_fake = fake_init_impl;
    fake_fetch_fake.custom_fake = fake_fetch_impl;
    fake_channel_get_fake.custom_fake = fake_channel_get_impl;
    fake_alert_set_fake.return_val = XY_FG_OK;
    fake_alert_get_fake.return_val = XY_FG_OK;

    memset(&fake_data, 0, sizeof(fake_data));
    fake_data.voltage_mv = 16800;
    fake_data.current_ma = -9000;
    fake_data.soc = 12;
    fake_data.soh = 76;
    fake_data.temperature_c = 565;
    fake_data.cycle_count = 9;
    fake_data.full_capacity_mah = 1900;
    fake_data.remain_capacity_mah = 760;
}

static void test_register_init_get_foreach(void)
{
    static const xy_fuel_gauge_api_t api = {
        .init = fake_init,
        .fetch = fake_fetch,
    };
    xy_fuel_gauge_t fg;
    int32_t value = 0;
    int user_marker = 42;

    reset_fixture();
    memset(&fg, 0, sizeof(fg));
    fg.name = "fg-test";
    fg.api = &api;

    TEST_ASSERT_EQUAL_INT(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_device_register(NULL));
    TEST_ASSERT_EQUAL_INT(XY_FG_OK, xy_fuel_gauge_device_register(&fg));
    TEST_ASSERT_EQUAL_INT(XY_FG_ERROR, xy_fuel_gauge_device_register(&fg));
    TEST_ASSERT_EQUAL_PTR(&fg, xy_fuel_gauge_device_get("fg-test"));
    TEST_ASSERT_NULL(xy_fuel_gauge_device_get("missing"));
    TEST_ASSERT_EQUAL_UINT(1U, xy_fuel_gauge_device_count());

    TEST_ASSERT_EQUAL_INT(XY_FG_OK, xy_fuel_gauge_init(&fg));
    TEST_ASSERT_EQUAL_UINT(1U, fake_init_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&fg, fake_init_fake.arg0_val);
    TEST_ASSERT_TRUE(fg.initialized);

    TEST_ASSERT_EQUAL_INT(XY_FG_OK, xy_fuel_gauge_get(&fg, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(3701, value);
    TEST_ASSERT_EQUAL_UINT(1U, fake_fetch_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&fg, fake_fetch_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT(1U, xy_os_tick_get_fake.call_count);
    TEST_ASSERT_EQUAL_UINT32(xy_os_tick_get_fake.return_val, fg.latest.timestamp);

    TEST_ASSERT_EQUAL_INT(XY_FG_ERROR_NOT_SUPPORTED, xy_fuel_gauge_get(&fg, XY_FG_DATA_TIME_TO_EMPTY, &value));

    xy_fuel_gauge_device_foreach(count_device, &user_marker);
    TEST_ASSERT_EQUAL_UINT(1U, count_device_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&fg, count_device_fake.arg0_val);
    TEST_ASSERT_EQUAL_PTR(&user_marker, count_device_fake.arg1_val);

    TEST_ASSERT_EQUAL_INT(XY_FG_OK, xy_fuel_gauge_deinit(&fg));
    TEST_ASSERT_FALSE(fg.initialized);
}

static void test_core_init_failure_preserves_status_and_return_code(void)
{
    static const xy_fuel_gauge_api_t api = {
        .init = fake_init,
        .fetch = fake_fetch,
    };
    xy_fuel_gauge_t fg;

    reset_fixture();
    memset(&fg, 0, sizeof(fg));
    fg.name = "fg-init-error";
    fg.api = &api;
    fg.latest.voltage_mv = 0x1234;
    fake_init_fake.custom_fake = fake_init_error_impl;

    TEST_ASSERT_EQUAL_INT(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_init(&fg));
    TEST_ASSERT_FALSE(fg.initialized);
    TEST_ASSERT_EQUAL_UINT16(0xEEEE, fg.latest.voltage_mv);
    TEST_ASSERT_EQUAL_UINT(1U, fake_init_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&fg, fake_init_fake.arg0_val);
}

static void test_core_public_calls_reject_initialized_device_without_api(void)
{
    xy_fuel_gauge_t fg;
    xy_fuel_gauge_alert_t alert;
    int32_t value = 0x12345678;
    uint16_t voltage = 0x1234;
    int16_t current = -1234;
    uint8_t soc = 12;
    uint8_t soh = 34;
    int16_t temp = -567;

    reset_fixture();
    memset(&fg, 0, sizeof(fg));
    memset(&alert, 0, sizeof(alert));
    fg.name = "fg-no-api";
    fg.initialized = true;

    TEST_ASSERT_EQUAL_INT(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_fetch(&fg));
    TEST_ASSERT_EQUAL_INT(XY_FG_ERROR_INVALID_PARAM,
                          xy_fuel_gauge_get(&fg, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(0x12345678, value);
    TEST_ASSERT_EQUAL_INT(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_voltage(&fg, &voltage));
    TEST_ASSERT_EQUAL_UINT16(0x1234, voltage);
    TEST_ASSERT_EQUAL_INT(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_voltage(&fg, NULL));
    TEST_ASSERT_EQUAL_INT(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_current(&fg, &current));
    TEST_ASSERT_EQUAL_INT16(-1234, current);
    TEST_ASSERT_EQUAL_INT(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_current(&fg, NULL));
    TEST_ASSERT_EQUAL_INT(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_soc(&fg, &soc));
    TEST_ASSERT_EQUAL_UINT8(12, soc);
    TEST_ASSERT_EQUAL_INT(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_soc(&fg, NULL));
    TEST_ASSERT_EQUAL_INT(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_soh(&fg, &soh));
    TEST_ASSERT_EQUAL_UINT8(34, soh);
    TEST_ASSERT_EQUAL_INT(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_soh(&fg, NULL));
    TEST_ASSERT_EQUAL_INT(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_temperature(&fg, &temp));
    TEST_ASSERT_EQUAL_INT16(-567, temp);
    TEST_ASSERT_EQUAL_INT(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_temperature(&fg, NULL));
    TEST_ASSERT_EQUAL_INT(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_set_alert(&fg, &alert));
    TEST_ASSERT_EQUAL_INT(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_alert(&fg, &alert));
    TEST_ASSERT_EQUAL_UINT(0U, fake_fetch_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(0U, xy_os_tick_get_fake.call_count);
}

static void test_core_public_calls_reject_missing_callbacks_without_side_effects(void)
{
    static const xy_fuel_gauge_api_t no_callbacks = {0};
    static const xy_fuel_gauge_api_t no_channel = {
        .init = fake_init,
        .fetch = fake_fetch,
    };
    static const xy_fuel_gauge_api_t no_alerts = {
        .init = fake_init,
        .fetch = fake_fetch,
        .channel_get = fake_channel_get,
    };
    xy_fuel_gauge_t fg;
    xy_fuel_gauge_alert_t alert;
    int32_t value = 0x12345678;

    reset_fixture();
    memset(&fg, 0, sizeof(fg));
    memset(&alert, 0, sizeof(alert));
    fg.name = "fg-no-callbacks";
    fg.api = &no_callbacks;
    fg.initialized = true;

    TEST_ASSERT_EQUAL_INT(XY_FG_ERROR_NOT_SUPPORTED, xy_fuel_gauge_fetch(&fg));
    TEST_ASSERT_EQUAL_UINT(0U, fake_fetch_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(0U, xy_os_tick_get_fake.call_count);

    fg.api = &no_channel;
    TEST_ASSERT_EQUAL_INT(XY_FG_ERROR_NOT_SUPPORTED,
                          xy_fuel_gauge_get(&fg, XY_FG_DATA_TIME_TO_EMPTY, &value));
    TEST_ASSERT_EQUAL_INT32(0x12345678, value);
    TEST_ASSERT_EQUAL_UINT(1U, fake_fetch_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(1U, xy_os_tick_get_fake.call_count);

    fg.api = &no_alerts;
    TEST_ASSERT_EQUAL_INT(XY_FG_ERROR_NOT_SUPPORTED, xy_fuel_gauge_set_alert(&fg, &alert));
    TEST_ASSERT_EQUAL_INT(XY_FG_ERROR_NOT_SUPPORTED, xy_fuel_gauge_get_alert(&fg, &alert));
    TEST_ASSERT_EQUAL_UINT(0U, fake_alert_set_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(0U, fake_alert_get_fake.call_count);
}

static void test_core_cached_data_fallback_covers_supported_channels(void)
{
    static const xy_fuel_gauge_api_t no_channel = {
        .init = fake_init,
        .fetch = fake_fetch,
    };
    xy_fuel_gauge_t fg;
    int32_t value = 0;

    reset_fixture();
    memset(&fg, 0, sizeof(fg));
    fg.name = "fg-cache-fallback";
    fg.api = &no_channel;
    fg.initialized = true;
    fg.latest.voltage_mv = 3600;
    fg.latest.current_ma = -250;
    fg.latest.soc = 44;
    fg.latest.soh = 91;
    fg.latest.temperature_c = 312;
    fg.latest.cycle_count = 1234;
    fg.latest.full_capacity_mah = 2800;
    fg.latest.remain_capacity_mah = 1350;

    TEST_ASSERT_EQUAL_INT(XY_FG_OK, xy_fuel_gauge_get(&fg, XY_FG_DATA_VOLTAGE, &value));
    TEST_ASSERT_EQUAL_INT32(3601, value);
    TEST_ASSERT_EQUAL_INT(XY_FG_OK, xy_fuel_gauge_get(&fg, XY_FG_DATA_CURRENT, &value));
    TEST_ASSERT_EQUAL_INT32(-250, value);
    TEST_ASSERT_EQUAL_INT(XY_FG_OK, xy_fuel_gauge_get(&fg, XY_FG_DATA_SOC, &value));
    TEST_ASSERT_EQUAL_INT32(44, value);
    TEST_ASSERT_EQUAL_INT(XY_FG_OK, xy_fuel_gauge_get(&fg, XY_FG_DATA_SOH, &value));
    TEST_ASSERT_EQUAL_INT32(91, value);
    TEST_ASSERT_EQUAL_INT(XY_FG_OK, xy_fuel_gauge_get(&fg, XY_FG_DATA_TEMPERATURE, &value));
    TEST_ASSERT_EQUAL_INT32(312, value);
    TEST_ASSERT_EQUAL_INT(XY_FG_OK, xy_fuel_gauge_get(&fg, XY_FG_DATA_CYCLE_COUNT, &value));
    TEST_ASSERT_EQUAL_INT32(1234, value);
    TEST_ASSERT_EQUAL_INT(XY_FG_OK, xy_fuel_gauge_get(&fg, XY_FG_DATA_FULL_CAPACITY, &value));
    TEST_ASSERT_EQUAL_INT32(2800, value);
    TEST_ASSERT_EQUAL_INT(XY_FG_OK, xy_fuel_gauge_get(&fg, XY_FG_DATA_REMAIN_CAPACITY, &value));
    TEST_ASSERT_EQUAL_INT32(1350, value);
    TEST_ASSERT_EQUAL_UINT(8U, fake_fetch_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(8U, xy_os_tick_get_fake.call_count);
}

static void test_status_queries_preserve_outputs_on_invalid_and_failed_reads(void)
{
    static const xy_fuel_gauge_api_t api = {
        .init = fake_init,
        .fetch = fake_fetch,
        .channel_get = fake_channel_get,
    };
    xy_fuel_gauge_t fg;
    xy_fg_battery_health_t health;
    uint16_t charge_current = 0xCAFE;
    uint16_t charge_voltage = 0xBEEF;

    reset_fixture();
    memset(&fg, 0, sizeof(fg));
    fg.name = "fg-status-guards";
    fg.api = &api;
    fg.initialized = true;

    memset(&health, 0xA5, sizeof(health));
    TEST_ASSERT_EQUAL_INT(XY_FG_ERROR_INVALID_PARAM,
                          xy_fuel_gauge_get_battery_health(NULL, &health));
    TEST_ASSERT_EQUAL_HEX8(0xA5, health.soh_percent);
    TEST_ASSERT_EQUAL_INT(XY_FG_ERROR_INVALID_PARAM,
                          xy_fuel_gauge_get_battery_health(&fg, NULL));

    TEST_ASSERT_EQUAL_INT(XY_FG_ERROR_INVALID_PARAM,
                          xy_fuel_gauge_get_charge_current(NULL, &charge_current));
    TEST_ASSERT_EQUAL_UINT16(0xCAFE, charge_current);
    TEST_ASSERT_EQUAL_INT(XY_FG_ERROR_INVALID_PARAM,
                          xy_fuel_gauge_get_charge_voltage(NULL, &charge_voltage));
    TEST_ASSERT_EQUAL_UINT16(0xBEEF, charge_voltage);

    fake_channel_get_fake.return_val = XY_FG_ERROR_NO_DATA;
    fake_channel_get_fake.custom_fake = NULL;
    memset(&health, 0x5A, sizeof(health));
    TEST_ASSERT_EQUAL_INT(XY_FG_ERROR_NO_DATA, xy_fuel_gauge_get_battery_health(&fg, &health));
    TEST_ASSERT_EQUAL_HEX8(0x5A, health.soh_percent);
    TEST_ASSERT_EQUAL_HEX16(0x5A5A, health.full_charge_capacity);
}

static void test_status_safety_security_helpers(void)
{
    static const xy_fuel_gauge_api_t api = {
        .init = fake_init,
        .fetch = fake_fetch,
        .channel_get = fake_channel_get,
    };
    xy_fuel_gauge_t fg;
    xy_fg_battery_health_t health;
    xy_fg_safety_thresholds_t thresholds;
    xy_fg_safety_event_t event;
    xy_fg_security_config_t security_config;
    uint8_t key[16] = {0};
    uint8_t plain[3] = {1, 2, 3};
    uint8_t encrypted[3] = {0};
    uint8_t decrypted[3] = {0};
    uint16_t out_len = 0;

    reset_fixture();
    memset(&fg, 0, sizeof(fg));
    fg.name = "fg-helpers";
    fg.api = &api;
    fg.data = &(struct {
        int security_type;
        uint8_t auth_key[32];
        uint16_t key_len;
        bool authenticated;
    }){0};
    fg.initialized = true;

    TEST_ASSERT_EQUAL_INT(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_charge_current(NULL, NULL));
    TEST_ASSERT_EQUAL_INT(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_charge_voltage(&fg, NULL));
    TEST_ASSERT_EQUAL_INT(XY_FG_OK, xy_fuel_gauge_get_battery_health(&fg, &health));
    TEST_ASSERT_EQUAL_UINT(4U, fake_channel_get_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&fg, fake_channel_get_fake.arg0_history[0]);
    TEST_ASSERT_EQUAL(XY_FG_DATA_FULL_CAPACITY, fake_channel_get_fake.arg1_history[0]);
    TEST_ASSERT_EQUAL(XY_FG_DATA_REMAIN_CAPACITY, fake_channel_get_fake.arg1_history[1]);
    TEST_ASSERT_EQUAL(XY_FG_DATA_CYCLE_COUNT, fake_channel_get_fake.arg1_history[2]);
    TEST_ASSERT_EQUAL(XY_FG_DATA_TEMPERATURE, fake_channel_get_fake.arg1_history[3]);
    TEST_ASSERT_EQUAL_UINT32(1900U, health.full_charge_capacity);
    TEST_ASSERT_EQUAL_UINT32(760U, health.remaining_capacity);
    TEST_ASSERT_EQUAL_UINT16(9U, health.cycle_count);
    TEST_ASSERT_EQUAL_INT16(56, health.temperature);

    TEST_ASSERT_BITS_HIGH(XY_FG_SAFETY_PACK_OTC, xy_fuel_gauge_get_safety_status(&fg));
    TEST_ASSERT_BITS_HIGH(XY_FG_WARNING_SOC_LOW, xy_fuel_gauge_get_warning_status(&fg));
    TEST_ASSERT_EQUAL_INT(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_config_safety_thresholds(NULL, &thresholds));
    TEST_ASSERT_EQUAL_INT(XY_FG_OK, xy_fuel_gauge_get_safety_thresholds(&fg, &thresholds));
    TEST_ASSERT_EQUAL_INT(XY_FG_OK, xy_fuel_gauge_get_safety_event(&fg, &event));
    TEST_ASSERT_EQUAL_INT(XY_FG_SAFETY_EVENT_NONE, event.type);
    TEST_ASSERT_EQUAL_INT(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_get_safety_event_history(NULL, &event, 1));

    security_config.type = XY_FG_SECURITY_NONE;
    security_config.key = key;
    security_config.key_len = sizeof(key);
    security_config.challenge = NULL;
    security_config.challenge_len = 0;
    TEST_ASSERT_EQUAL_INT(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_security_config(NULL, &security_config));
    TEST_ASSERT_EQUAL_INT(XY_FG_OK, xy_fuel_gauge_security_config(&fg, &security_config));
    TEST_ASSERT_EQUAL_INT(XY_FG_AUTH_OK, xy_fuel_gauge_authenticate(&fg));
    TEST_ASSERT_EQUAL_INT(XY_FG_OK, xy_fuel_gauge_encrypt_data(&fg, plain, sizeof(plain), encrypted, &out_len));
    TEST_ASSERT_EQUAL_UINT16(sizeof(plain), out_len);
    TEST_ASSERT_EQUAL_MEMORY(plain, encrypted, sizeof(plain));
    TEST_ASSERT_EQUAL_INT(XY_FG_OK, xy_fuel_gauge_decrypt_data(&fg, encrypted, out_len, decrypted, &out_len));
    TEST_ASSERT_EQUAL_UINT16(sizeof(plain), out_len);
    TEST_ASSERT_EQUAL_MEMORY(plain, decrypted, sizeof(plain));
    TEST_ASSERT_EQUAL_INT(XY_FG_ERROR_INVALID_PARAM, xy_fuel_gauge_decrypt_data(&fg, NULL, 0, decrypted, &out_len));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_register_init_get_foreach);
    RUN_TEST(test_core_init_failure_preserves_status_and_return_code);
    RUN_TEST(test_core_public_calls_reject_initialized_device_without_api);
    RUN_TEST(test_core_public_calls_reject_missing_callbacks_without_side_effects);
    RUN_TEST(test_core_cached_data_fallback_covers_supported_channels);
    RUN_TEST(test_status_queries_preserve_outputs_on_invalid_and_failed_reads);
    RUN_TEST(test_status_safety_security_helpers);
    return UNITY_END();
}
