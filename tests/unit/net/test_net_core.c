/**
 * @file test_net_core.c
 * @brief Unit tests for network component umbrella header and platform helpers.
 */

#include "xy_net.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

static void test_net_lifecycle(void)
{
    assert(xy_net_init() == 0);
    assert(xy_net_deinit() == 0);
}

static void test_net_platform_helpers(void)
{
    uint32_t tick0;
    uint32_t tick1;
    uint8_t *buf;

    tick0 = xy_net_get_tick();
    xy_net_delay_ms(1);
    tick1 = xy_net_get_tick();
    assert(tick1 >= tick0);

    buf = (uint8_t *)xy_net_malloc(4);
    assert(buf != NULL);
    memset(buf, 0xA5, 4);
    assert(buf[0] == 0xA5 && buf[3] == 0xA5);
    xy_net_free(buf);
    xy_net_free(NULL);
}

static void test_net_umbrella_exports_modbus_compat(void)
{
    mb_slave_t slave;
    mb_slave_config_t cfg = {.slave_id = 9};

    assert(XY_NET_ENABLE_MODBUS == 1);
    assert(nano_mb_slave_init(&slave, &cfg) == NANO_MB_OK);
    assert(slave.initialized);
    assert(slave.slave_id == 9U);
    assert(nano_mb_slave_deinit(&slave) == NANO_MB_OK);
}

int main(void)
{
    test_net_lifecycle();
    test_net_platform_helpers();
    test_net_umbrella_exports_modbus_compat();
    return 0;
}
