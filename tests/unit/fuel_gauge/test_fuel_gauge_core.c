#include "xy_fuel_gauge.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint32_t fake_tick;
static int init_calls;
static int fetch_calls;
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

int main(void)
{
    test_register_init_get_foreach();
    puts("Fuel gauge core tests passed");
    return 0;
}
