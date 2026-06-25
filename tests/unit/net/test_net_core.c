/**
 * @file test_net_core.c
 * @brief Unit tests for network component umbrella header and platform helpers.
 */

#include "xy_net.h"

#include "unity.h"

#include <stdint.h>
#include <string.h>

static void test_net_lifecycle(void)
{
    TEST_ASSERT_EQUAL(0, xy_net_init());
    TEST_ASSERT_EQUAL(0, xy_net_deinit());
}

static void test_net_platform_helpers(void)
{
    uint32_t tick0;
    uint32_t tick1;
    uint8_t *buf;

    tick0 = xy_net_get_tick();
    xy_net_delay_ms(1);
    tick1 = xy_net_get_tick();
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(tick0, tick1);

    buf = (uint8_t *)xy_net_malloc(4);
    TEST_ASSERT_NOT_NULL(buf);
    memset(buf, 0xA5, 4);
    TEST_ASSERT_EQUAL_UINT8(0xA5, buf[0]);
    TEST_ASSERT_EQUAL_UINT8(0xA5, buf[3]);
    xy_net_free(buf);
    xy_net_free(NULL);
}

static void test_net_umbrella_exports_modbus_compat(void)
{
    mb_slave_t slave;
    mb_slave_config_t cfg = {.slave_id = 9};

    TEST_ASSERT_EQUAL(1, XY_NET_ENABLE_MODBUS);
    TEST_ASSERT_EQUAL(NANO_MB_OK, nano_mb_slave_init(&slave, &cfg));
    TEST_ASSERT_TRUE(slave.initialized);
    TEST_ASSERT_EQUAL_UINT8(9U, slave.slave_id);
    TEST_ASSERT_EQUAL(NANO_MB_OK, nano_mb_slave_deinit(&slave));
}

void setUp(void)
{
}

void tearDown(void)
{
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_net_lifecycle);
    RUN_TEST(test_net_platform_helpers);
    RUN_TEST(test_net_umbrella_exports_modbus_compat);
    return UNITY_END();
}
