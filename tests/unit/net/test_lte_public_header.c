/**
 * @file test_lte_public_header.c
 * @brief Public include-root smoke test for the LTE component API.
 */
#include "unity.h"

#include "xy_lte.h"

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_lte_public_types_are_self_contained(void)
{
    xy_lte_transport_t transport = {
        .context = (void *)0x1234,
        .write = 0,
        .read = 0,
        .flush = 0,
    };
    xy_lte_pdp_context_t pdp = {
        .cid = 1U,
        .apn = "cmnet",
        .username = "user",
        .password = "pass",
        .auth_type = 0U,
    };
    xy_lte_signal_t signal = {
        .rssi = -75,
        .ber = 3,
        .rsrp = -100,
        .rsrq = -10,
        .sinr = 12,
    };
    xy_lte_network_info_t network = {
        .net_type = XY_LTE_NET_4G,
        .mcc = "460",
        .mnc = "00",
        .lac = 0x1234U,
        .cell_id = 0x10203040U,
        .earfcn = 38400,
    };
    xy_lte_sim_info_t sim = {
        .iccid = "89860000000000000000",
        .imsi = "460001234567890",
        .msisdn = "13800138000",
    };
    xy_lte_t lte = {
        .uart_handle = (void *)0x5678,
        .baudrate = 115200U,
        .initialized = true,
        .attached = true,
        .pdp_active = true,
        .net_type = XY_LTE_NET_4G,
        .signal = signal,
        .pdp = pdp,
        .transport = transport,
    };

    TEST_ASSERT_EQUAL_PTR((void *)0x1234, transport.context);
    TEST_ASSERT_EQUAL_UINT8(1U, pdp.cid);
    TEST_ASSERT_EQUAL_STRING("cmnet", pdp.apn);
    TEST_ASSERT_EQUAL_INT(-75, signal.rssi);
    TEST_ASSERT_EQUAL_INT(XY_LTE_NET_4G, network.net_type);
    TEST_ASSERT_EQUAL_STRING("460", network.mcc);
    TEST_ASSERT_EQUAL_STRING("89860000000000000000", sim.iccid);
    TEST_ASSERT_EQUAL_PTR((void *)0x5678, lte.uart_handle);
    TEST_ASSERT_TRUE(lte.initialized);
    TEST_ASSERT_TRUE(lte.attached);
    TEST_ASSERT_TRUE(lte.pdp_active);
    TEST_ASSERT_EQUAL_INT(XY_LTE_NET_4G, lte.net_type);
}

static void test_lte_public_status_values_remain_stable(void)
{
    TEST_ASSERT_EQUAL_INT(0, XY_LTE_OK);
    TEST_ASSERT_EQUAL_INT(-1, XY_LTE_ERROR);
    TEST_ASSERT_EQUAL_INT(-2, XY_LTE_INVALID_PARAM);
    TEST_ASSERT_EQUAL_INT(-3, XY_LTE_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(-4, XY_LTE_NO_NETWORK);
    TEST_ASSERT_EQUAL_INT(-5, XY_LTE_NO_SIM);
    TEST_ASSERT_EQUAL_INT(-6, XY_LTE_PIN_REQUIRED);
    TEST_ASSERT_EQUAL_INT(-7, XY_LTE_ATTACH_FAILED);
    TEST_ASSERT_EQUAL_INT(-8, XY_LTE_PDP_FAILED);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_lte_public_types_are_self_contained);
    RUN_TEST(test_lte_public_status_values_remain_stable);
    return UNITY_END();
}
