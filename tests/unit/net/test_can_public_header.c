/**
 * @file test_can_public_header.c
 * @brief Public include-root smoke test for the CAN component API.
 */
#include "unity.h"

#include "xy_can.h"

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_can_public_types_are_self_contained(void)
{
    xy_can_config_t config = {
        .baudrate = 500000U,
        .rx_fifo_size = 4U,
        .tx_fifo_size = 4U,
        .flags = 0U,
    };
    xy_can_msg_t msg = {
        .id = 0x123U,
        .data = {0x11U, 0x22U},
        .len = 2U,
        .rtr = 0U,
    };
    xy_can_t can = {0};

    can.config = config;
    can.hw_handle = (void *)0x1234;
    can.initialized = true;
    can.started = true;

    TEST_ASSERT_EQUAL_UINT32(500000U, config.baudrate);
    TEST_ASSERT_EQUAL_UINT32(0x123U, msg.id);
    TEST_ASSERT_EQUAL_UINT8(2U, msg.len);
    TEST_ASSERT_EQUAL_UINT8(0x22U, msg.data[1]);
    TEST_ASSERT_EQUAL_UINT32(4U, can.config.rx_fifo_size);
    TEST_ASSERT_TRUE(can.initialized);
    TEST_ASSERT_TRUE(can.started);
    TEST_ASSERT_EQUAL_PTR((void *)0x1234, can.hw_handle);
}

static void test_can_public_status_values_remain_stable(void)
{
    TEST_ASSERT_EQUAL_INT(0, XY_CAN_OK);
    TEST_ASSERT_EQUAL_INT(-1, XY_CAN_ERROR);
    TEST_ASSERT_EQUAL_INT(-2, XY_CAN_INVALID_PARAM);
    TEST_ASSERT_EQUAL_INT(-3, XY_CAN_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(-4, XY_CAN_FIFO_FULL);
    TEST_ASSERT_EQUAL_INT(-5, XY_CAN_FIFO_EMPTY);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_can_public_types_are_self_contained);
    RUN_TEST(test_can_public_status_values_remain_stable);
    return UNITY_END();
}
