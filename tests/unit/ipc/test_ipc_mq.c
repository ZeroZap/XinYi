#include "xy_mq.h"
#include "xy_os.h"

#include <stdint.h>
#include <string.h>

#include "unity.h"

static uint32_t fake_tick;
static uint32_t delay_call_count;

uint32_t xy_os_tick_get(void)
{
    return fake_tick;
}

xy_os_status_t xy_os_delay(uint32_t ticks)
{
    fake_tick += ticks;
    delay_call_count++;
    return XY_OS_OK;
}

void xy_log_char(char ch)
{
    (void)ch;
}

void setUp(void)
{
    fake_tick = 0;
    delay_call_count = 0;
}

void tearDown(void)
{
}

static xy_mq_msg_t make_msg(uint32_t id, xy_mq_priority_t priority, uint8_t *data,
                            uint16_t len)
{
    xy_mq_msg_t msg = {
        .id = id,
        .priority = priority,
        .timestamp = 0,
        .data = data,
        .len = len,
    };
    return msg;
}

static void test_mq_init_deinit_and_invalid_params(void)
{
    xy_mq_t mq;
    xy_mq_config_t config = {
        .msg_size = 4,
        .max_msgs = 2,
        .priority_enabled = false,
        .overwrite_old = false,
    };

    TEST_ASSERT_EQUAL(XY_MQ_INVALID_PARAM, xy_mq_init(NULL, &config));
    TEST_ASSERT_EQUAL(XY_MQ_INVALID_PARAM, xy_mq_init(&mq, NULL));

    config.msg_size = 0;
    TEST_ASSERT_EQUAL(XY_MQ_INVALID_PARAM, xy_mq_init(&mq, &config));
    config.msg_size = 4;
    config.max_msgs = 0;
    TEST_ASSERT_EQUAL(XY_MQ_INVALID_PARAM, xy_mq_init(&mq, &config));

    config.max_msgs = 2;
    TEST_ASSERT_EQUAL(XY_MQ_OK, xy_mq_init(&mq, &config));
    TEST_ASSERT_TRUE(mq.initialized);
    TEST_ASSERT_NOT_NULL(mq.buffer);
    TEST_ASSERT_EQUAL_UINT16(0U, xy_mq_get_count(&mq));
    TEST_ASSERT_EQUAL_UINT16(2U, xy_mq_get_free(&mq));

    TEST_ASSERT_EQUAL(XY_MQ_OK, xy_mq_deinit(&mq));
    TEST_ASSERT_FALSE(mq.initialized);
    TEST_ASSERT_NULL(mq.buffer);
    TEST_ASSERT_EQUAL(XY_MQ_INVALID_PARAM, xy_mq_deinit(NULL));
}

static void test_mq_send_receive_fifo_payloads(void)
{
    xy_mq_t mq;
    xy_mq_config_t config = {
        .msg_size = 4,
        .max_msgs = 3,
        .priority_enabled = false,
        .overwrite_old = false,
    };
    uint8_t first[] = {0x10, 0x11, 0x12, 0x13};
    uint8_t second[] = {0x20, 0x21, 0x22, 0x23};
    uint8_t out[4] = {0};
    xy_mq_msg_t msg = make_msg(1U, XY_MQ_PRIORITY_NORMAL, first, sizeof(first));
    xy_mq_msg_t recv = make_msg(0U, XY_MQ_PRIORITY_LOW, out, sizeof(out));

    TEST_ASSERT_EQUAL(XY_MQ_OK, xy_mq_init(&mq, &config));
    TEST_ASSERT_EQUAL(XY_MQ_OK, xy_mq_send(&mq, &msg, 0));
    msg = make_msg(2U, XY_MQ_PRIORITY_HIGH, second, sizeof(second));
    TEST_ASSERT_EQUAL(XY_MQ_OK, xy_mq_send(&mq, &msg, 0));
    TEST_ASSERT_EQUAL_UINT16(2U, xy_mq_get_count(&mq));
    TEST_ASSERT_EQUAL_UINT16(1U, xy_mq_get_free(&mq));

    TEST_ASSERT_EQUAL(XY_MQ_OK, xy_mq_recv(&mq, &recv, 0));
    TEST_ASSERT_EQUAL_MEMORY(first, out, sizeof(first));
    memset(out, 0, sizeof(out));
    TEST_ASSERT_EQUAL(XY_MQ_OK, xy_mq_recv(&mq, &recv, 0));
    TEST_ASSERT_EQUAL_MEMORY(second, out, sizeof(second));
    TEST_ASSERT_EQUAL_UINT16(0U, xy_mq_get_count(&mq));

    TEST_ASSERT_EQUAL(XY_MQ_OK, xy_mq_deinit(&mq));
}

static void test_mq_full_returns_immediately_without_overwrite(void)
{
    xy_mq_t mq;
    xy_mq_config_t config = {
        .msg_size = 2,
        .max_msgs = 1,
        .priority_enabled = false,
        .overwrite_old = false,
    };
    uint8_t first[] = {0xAA, 0xBB};
    uint8_t second[] = {0xCC, 0xDD};
    xy_mq_msg_t msg = make_msg(1U, XY_MQ_PRIORITY_NORMAL, first, sizeof(first));

    TEST_ASSERT_EQUAL(XY_MQ_OK, xy_mq_init(&mq, &config));
    TEST_ASSERT_EQUAL(XY_MQ_OK, xy_mq_send(&mq, &msg, 0));
    msg = make_msg(2U, XY_MQ_PRIORITY_NORMAL, second, sizeof(second));
    TEST_ASSERT_EQUAL(XY_MQ_FULL, xy_mq_try_send(&mq, &msg));
    TEST_ASSERT_EQUAL_UINT16(1U, xy_mq_get_count(&mq));
    TEST_ASSERT_EQUAL_UINT32(0U, delay_call_count);
    TEST_ASSERT_EQUAL_UINT32(0U, mq.drop_count);

    TEST_ASSERT_EQUAL(XY_MQ_OK, xy_mq_deinit(&mq));
}

static void test_mq_overwrite_old_drops_oldest(void)
{
    xy_mq_t mq;
    xy_mq_config_t config = {
        .msg_size = 2,
        .max_msgs = 1,
        .priority_enabled = false,
        .overwrite_old = true,
    };
    uint8_t first[] = {0x01, 0x02};
    uint8_t second[] = {0x03, 0x04};
    uint8_t out[2] = {0};
    xy_mq_msg_t msg = make_msg(1U, XY_MQ_PRIORITY_NORMAL, first, sizeof(first));
    xy_mq_msg_t recv = make_msg(0U, XY_MQ_PRIORITY_LOW, out, sizeof(out));

    TEST_ASSERT_EQUAL(XY_MQ_OK, xy_mq_init(&mq, &config));
    TEST_ASSERT_EQUAL(XY_MQ_OK, xy_mq_send(&mq, &msg, 0));
    msg = make_msg(2U, XY_MQ_PRIORITY_NORMAL, second, sizeof(second));
    TEST_ASSERT_EQUAL(XY_MQ_OK, xy_mq_send(&mq, &msg, 0));
    TEST_ASSERT_EQUAL_UINT16(1U, xy_mq_get_count(&mq));
    TEST_ASSERT_EQUAL_UINT32(1U, mq.drop_count);

    TEST_ASSERT_EQUAL(XY_MQ_OK, xy_mq_recv(&mq, &recv, 0));
    TEST_ASSERT_EQUAL_MEMORY(second, out, sizeof(second));

    TEST_ASSERT_EQUAL(XY_MQ_OK, xy_mq_deinit(&mq));
}

static void test_mq_empty_timeout_and_clear_stats(void)
{
    xy_mq_t mq;
    xy_mq_config_t config = {
        .msg_size = 2,
        .max_msgs = 2,
        .priority_enabled = false,
        .overwrite_old = false,
    };
    uint8_t payload[] = {0x55, 0x66};
    uint8_t out[2] = {0};
    xy_mq_msg_t msg = make_msg(1U, XY_MQ_PRIORITY_NORMAL, payload, sizeof(payload));
    xy_mq_msg_t recv = make_msg(0U, XY_MQ_PRIORITY_LOW, out, sizeof(out));
    uint32_t send = 0;
    uint32_t received = 0;
    uint32_t dropped = 0;

    TEST_ASSERT_EQUAL(XY_MQ_OK, xy_mq_init(&mq, &config));
    TEST_ASSERT_EQUAL(XY_MQ_TIMEOUT, xy_mq_recv(&mq, &recv, 3U));
    TEST_ASSERT_EQUAL_UINT32(4U, fake_tick);
    TEST_ASSERT_EQUAL_UINT32(4U, delay_call_count);

    TEST_ASSERT_EQUAL(XY_MQ_OK, xy_mq_send(&mq, &msg, 0));
    TEST_ASSERT_EQUAL_UINT16(1U, xy_mq_get_count(&mq));
    TEST_ASSERT_EQUAL(XY_MQ_OK, xy_mq_clear(&mq));
    TEST_ASSERT_EQUAL_UINT16(0U, xy_mq_get_count(&mq));

    TEST_ASSERT_EQUAL(XY_MQ_OK, xy_mq_get_stats(&mq, &send, &received, &dropped));
    TEST_ASSERT_EQUAL_UINT32(1U, send);
    TEST_ASSERT_EQUAL_UINT32(0U, received);
    TEST_ASSERT_EQUAL_UINT32(0U, dropped);

    TEST_ASSERT_EQUAL(XY_MQ_INVALID_PARAM, xy_mq_clear(NULL));
    TEST_ASSERT_EQUAL(XY_MQ_INVALID_PARAM, xy_mq_get_stats(NULL, &send, &received, &dropped));

    TEST_ASSERT_EQUAL(XY_MQ_OK, xy_mq_deinit(&mq));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_mq_init_deinit_and_invalid_params);
    RUN_TEST(test_mq_send_receive_fifo_payloads);
    RUN_TEST(test_mq_full_returns_immediately_without_overwrite);
    RUN_TEST(test_mq_overwrite_old_drops_oldest);
    RUN_TEST(test_mq_empty_timeout_and_clear_stats);
    return UNITY_END();
}
