/**
 * @file test_net_feature_gated_umbrella.c
 * @brief Opt-in feature-gate probe for xy_net umbrella protocol exports.
 */

#include "xy_net.h"

#include "unity.h"

static void test_net_umbrella_exports_can_when_feature_enabled(void)
{
    xy_can_config_t config = {
        .baudrate = 250000U,
        .rx_fifo_size = 3U,
        .tx_fifo_size = 4U,
    };
    xy_can_msg_t msg = {
        .id = 0x321U,
        .data = {0x12U, 0x34U, 0x56U},
        .len = 3U,
    };

    TEST_ASSERT_EQUAL_INT(1, XY_NET_ENABLE_CAN);
    TEST_ASSERT_EQUAL_UINT32(250000U, config.baudrate);
    TEST_ASSERT_EQUAL_UINT32(3U, config.rx_fifo_size);
    TEST_ASSERT_EQUAL_UINT32(4U, config.tx_fifo_size);
    TEST_ASSERT_EQUAL_UINT32(0x321U, msg.id);
    TEST_ASSERT_EQUAL_UINT8(3U, msg.len);
    TEST_ASSERT_EQUAL_HEX8(0x12U, msg.data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x34U, msg.data[1]);
    TEST_ASSERT_EQUAL_HEX8(0x56U, msg.data[2]);
    TEST_ASSERT_EQUAL_INT(XY_CAN_OK, 0);
    TEST_ASSERT_EQUAL_INT(XY_CAN_FIFO_FULL, -4);
}

static void test_net_umbrella_exports_lte_when_feature_enabled(void)
{
    xy_lte_t lte = {
        .baudrate = 9600U,
        .net_type = XY_LTE_NET_CATM1,
    };
    xy_lte_transport_t transport = {0};
    xy_lte_signal_t signal = {
        .rssi = -83,
        .ber = 2,
        .rsrp = -101,
        .rsrq = -12,
        .sinr = 8,
    };

    TEST_ASSERT_EQUAL_INT(1, XY_NET_ENABLE_LTE);
    TEST_ASSERT_EQUAL_UINT32(9600U, lte.baudrate);
    TEST_ASSERT_EQUAL_INT(XY_LTE_NET_CATM1, lte.net_type);
    TEST_ASSERT_NULL(transport.context);
    TEST_ASSERT_NULL(transport.write);
    TEST_ASSERT_NULL(transport.read);
    TEST_ASSERT_NULL(transport.flush);
    TEST_ASSERT_EQUAL_INT(-83, signal.rssi);
    TEST_ASSERT_EQUAL_INT(2, signal.ber);
    TEST_ASSERT_EQUAL_INT(-101, signal.rsrp);
    TEST_ASSERT_EQUAL_INT(-12, signal.rsrq);
    TEST_ASSERT_EQUAL_INT(8, signal.sinr);
    TEST_ASSERT_EQUAL_INT(XY_LTE_OK, 0);
    TEST_ASSERT_EQUAL_INT(XY_LTE_PDP_FAILED, -8);
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
    RUN_TEST(test_net_umbrella_exports_can_when_feature_enabled);
    RUN_TEST(test_net_umbrella_exports_lte_when_feature_enabled);
    return UNITY_END();
}
