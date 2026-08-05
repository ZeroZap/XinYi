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

static void test_net_config_default_allocators_are_publicly_usable(void)
{
    uint8_t *buf = (uint8_t *)XY_NET_MALLOC(3U);

    TEST_ASSERT_NOT_NULL(buf);
    buf[0] = 0x11U;
    buf[1] = 0x22U;
    buf[2] = 0x33U;
    TEST_ASSERT_EQUAL_UINT8(0x11U, buf[0]);
    TEST_ASSERT_EQUAL_UINT8(0x22U, buf[1]);
    TEST_ASSERT_EQUAL_UINT8(0x33U, buf[2]);
    XY_NET_FREE(buf);
    XY_NET_FREE(NULL);
}

static void test_net_protocol_feature_flags_are_public(void)
{
    TEST_ASSERT_EQUAL_INT(1, XY_NET_ENABLE_MODBUS);
    TEST_ASSERT_EQUAL_INT(0, XY_NET_ENABLE_MQTT);
    TEST_ASSERT_EQUAL_INT(0, XY_NET_ENABLE_CAN);
    TEST_ASSERT_EQUAL_INT(0, XY_NET_ENABLE_LTE);
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

static void test_net_umbrella_exports_at_client_contract(void)
{
    xy_at_client_t client = {0};
    xy_at_response_t response = {0};
    xy_at_urc_t urc = {0};

    client.name = "modem";
    client.status = XY_AT_STATUS_IDLE;
    client.end_sign = '\r';
    response.timeout = XY_AT_DEFAULT_TIMEOUT;
    urc.prefix = "+CEREG:";

    TEST_ASSERT_EQUAL_STRING("modem", client.name);
    TEST_ASSERT_EQUAL(XY_AT_STATUS_IDLE, client.status);
    TEST_ASSERT_EQUAL_CHAR('\r', client.end_sign);
    TEST_ASSERT_EQUAL_UINT32(5000U, response.timeout);
    TEST_ASSERT_EQUAL_STRING("+CEREG:", urc.prefix);
    TEST_ASSERT_EQUAL(XY_AT_RESP_OK, 0);
    TEST_ASSERT_EQUAL(XY_AT_RESP_ERROR, -1);
    TEST_ASSERT_EQUAL(XY_AT_RESP_TIMEOUT, -2);
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
    RUN_TEST(test_net_config_default_allocators_are_publicly_usable);
    RUN_TEST(test_net_protocol_feature_flags_are_public);
    RUN_TEST(test_net_umbrella_exports_modbus_compat);
    RUN_TEST(test_net_umbrella_exports_at_client_contract);
    return UNITY_END();
}
