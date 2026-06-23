#include "xy_fuel_gauge.h"
#include "xy_fuel_gauge_safety.h"
#include "xy_fuel_gauge_security.h"
#include "xy_fuel_gauge_status.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint32_t fake_tick;
static int init_calls;
static int fetch_calls;
static xy_fuel_gauge_data_t fake_data;
static int foreach_calls;
static void *foreach_user_data;

uint32_t xy_os_tick_get(void)
{
    return fake_tick;
}

static int fake_init(xy_fuel_gauge_t *fg)
{
    init_calls++;
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

static int fake_fetch(xy_fuel_gauge_t *fg)
{
    fetch_calls++;
    fg->latest.voltage_mv++;
    return XY_FG_OK;
}

static int fake_channel_get(xy_fuel_gauge_t *fg, xy_fuel_gauge_data_type_t channel, int32_t *val)
{
    (void)fg;

    if (!val) {
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

static void count_device(xy_fuel_gauge_t *fg, void *user_data)
{
    assert(fg != NULL);
    foreach_calls++;
    foreach_user_data = user_data;
}

static void reset_fixture(void)
{
    fake_tick = 1234;
    init_calls = 0;
    fetch_calls = 0;
    memset(&fake_data, 0, sizeof(fake_data));
    fake_data.voltage_mv = 16800;
    fake_data.current_ma = -9000;
    fake_data.soc = 12;
    fake_data.soh = 76;
    fake_data.temperature_c = 565;
    fake_data.cycle_count = 9;
    fake_data.full_capacity_mah = 1900;
    fake_data.remain_capacity_mah = 760;
    foreach_calls = 0;
    foreach_user_data = NULL;
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

    assert(xy_fuel_gauge_device_register(NULL) == XY_FG_ERROR_INVALID_PARAM);
    assert(xy_fuel_gauge_device_register(&fg) == XY_FG_OK);
    assert(xy_fuel_gauge_device_register(&fg) == XY_FG_ERROR);
    assert(xy_fuel_gauge_device_get("fg-test") == &fg);
    assert(xy_fuel_gauge_device_get("missing") == NULL);
    assert(xy_fuel_gauge_device_count() == 1U);

    assert(xy_fuel_gauge_init(&fg) == XY_FG_OK);
    assert(init_calls == 1);
    assert(fg.initialized);

    assert(xy_fuel_gauge_get(&fg, XY_FG_DATA_VOLTAGE, &value) == XY_FG_OK);
    assert(value == 3701);
    assert(fetch_calls == 1);
    assert(fg.latest.timestamp == fake_tick);

    assert(xy_fuel_gauge_get(&fg, XY_FG_DATA_TIME_TO_EMPTY, &value) == XY_FG_ERROR_NOT_SUPPORTED);

    xy_fuel_gauge_device_foreach(count_device, &user_marker);
    assert(foreach_calls == 1);
    assert(foreach_user_data == &user_marker);

    assert(xy_fuel_gauge_deinit(&fg) == XY_FG_OK);
    assert(!fg.initialized);
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

    assert(xy_fuel_gauge_get_charge_current(NULL, NULL) == XY_FG_ERROR_INVALID_PARAM);
    assert(xy_fuel_gauge_get_charge_voltage(&fg, NULL) == XY_FG_ERROR_INVALID_PARAM);
    assert(xy_fuel_gauge_get_battery_health(&fg, &health) == XY_FG_OK);
    assert(health.full_charge_capacity == 1900U);
    assert(health.remaining_capacity == 760U);
    assert(health.cycle_count == 9U);
    assert(health.temperature == 56U);

    assert((xy_fuel_gauge_get_safety_status(&fg) & XY_FG_SAFETY_PACK_OTC) != 0);
    assert((xy_fuel_gauge_get_warning_status(&fg) & XY_FG_WARNING_SOC_LOW) != 0);
    assert(xy_fuel_gauge_config_safety_thresholds(NULL, &thresholds) == XY_FG_ERROR_INVALID_PARAM);
    assert(xy_fuel_gauge_get_safety_thresholds(&fg, &thresholds) == XY_FG_OK);
    assert(xy_fuel_gauge_get_safety_event(&fg, &event) == XY_FG_OK);
    assert(event.type == XY_FG_SAFETY_EVENT_NONE);
    assert(xy_fuel_gauge_get_safety_event_history(NULL, &event, 1) == XY_FG_ERROR_INVALID_PARAM);

    security_config.type = XY_FG_SECURITY_NONE;
    security_config.key = key;
    security_config.key_len = sizeof(key);
    security_config.challenge = NULL;
    security_config.challenge_len = 0;
    assert(xy_fuel_gauge_security_config(NULL, &security_config) == XY_FG_ERROR_INVALID_PARAM);
    assert(xy_fuel_gauge_security_config(&fg, &security_config) == XY_FG_OK);
    assert(xy_fuel_gauge_authenticate(&fg) == XY_FG_AUTH_OK);
    assert(xy_fuel_gauge_encrypt_data(&fg, plain, sizeof(plain), encrypted, &out_len) == XY_FG_OK);
    assert(out_len == sizeof(plain));
    assert(memcmp(encrypted, plain, sizeof(plain)) == 0);
    assert(xy_fuel_gauge_decrypt_data(&fg, encrypted, out_len, decrypted, &out_len) == XY_FG_OK);
    assert(out_len == sizeof(plain));
    assert(memcmp(decrypted, plain, sizeof(plain)) == 0);
    assert(xy_fuel_gauge_decrypt_data(&fg, NULL, 0, decrypted, &out_len) == XY_FG_ERROR_INVALID_PARAM);
}

int main(void)
{
    test_register_init_get_foreach();
    test_status_safety_security_helpers();
    puts("Fuel gauge core tests passed");
    return 0;
}
