#include "xy_broker.h"
#include "xy_os.h"

#include <stdint.h>
#include <string.h>

#include "unity.h"
#include "fff.h"

static uint32_t fake_tick;
static xy_broker_msg_t last_msg;

typedef struct {
    int response_sent;
} test_context_t;

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(uint32_t, xy_os_tick_get)
FAKE_VALUE_FUNC(xy_os_status_t, xy_os_delay, uint32_t)
FAKE_VALUE_FUNC(int, direct_capture_handler, const xy_broker_msg_t *, void *)
FAKE_VALUE_FUNC(int, topic_capture_handler, const xy_broker_msg_t *, void *)
FAKE_VALUE_FUNC(int, rejecting_handler, const xy_broker_msg_t *, void *)

static uint32_t xy_os_tick_get_impl(void)
{
    return fake_tick;
}

static xy_os_status_t xy_os_delay_impl(uint32_t ticks)
{
    fake_tick += ticks;
    return XY_OS_OK;
}

static int capture_msg_impl(const xy_broker_msg_t *msg, void *user_data)
{
    (void)user_data;

    TEST_ASSERT_NOT_NULL(msg);
    memcpy(&last_msg, msg, sizeof(last_msg));
    return XY_BROKER_OK;
}

static int responder_handler(const xy_broker_msg_t *msg, void *user_data)
{
    test_context_t *ctx = (test_context_t *)user_data;
    const char response[] = "pong";

    TEST_ASSERT_NOT_NULL(msg);
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_EQUAL(XY_BROKER_MSG_COMM_SEND, msg->msg_id);
    TEST_ASSERT_EQUAL_UINT32(4U, msg->payload_len);
    TEST_ASSERT_EQUAL_MEMORY("ping", msg->payload, 4U);

    ctx->response_sent++;
    TEST_ASSERT_EQUAL(XY_BROKER_OK, xy_broker_respond(msg, response, sizeof(response) - 1U));
    return XY_BROKER_OK;
}

static void reset_fakes(void)
{
    RESET_FAKE(xy_os_tick_get);
    RESET_FAKE(xy_os_delay);
    RESET_FAKE(direct_capture_handler);
    RESET_FAKE(topic_capture_handler);
    RESET_FAKE(rejecting_handler);
    FFF_RESET_HISTORY();

    xy_os_tick_get_fake.custom_fake = xy_os_tick_get_impl;
    xy_os_delay_fake.custom_fake = xy_os_delay_impl;
    direct_capture_handler_fake.custom_fake = capture_msg_impl;
    topic_capture_handler_fake.custom_fake = capture_msg_impl;

    fake_tick = 0;
    memset(&last_msg, 0, sizeof(last_msg));
}

static void reset_broker(void)
{
    reset_fakes();
    (void)xy_broker_deinit();
    TEST_ASSERT_EQUAL(XY_BROKER_OK, xy_broker_init());
}

static void test_lifecycle_and_server_registration(void)
{
    xy_broker_stats_t stats;

    (void)xy_broker_deinit();
    TEST_ASSERT_EQUAL(XY_BROKER_ERROR, xy_broker_get_stats(&stats));
    TEST_ASSERT_EQUAL(XY_BROKER_OK, xy_broker_init());
    TEST_ASSERT_EQUAL(XY_BROKER_OK, xy_broker_init());
    TEST_ASSERT_EQUAL(XY_BROKER_INVALID_PARAM,
                      xy_broker_register_server(0, direct_capture_handler,
                                                NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_SYSTEM,
                                                direct_capture_handler, NULL));
    TEST_ASSERT_EQUAL_INT(1, xy_broker_is_server_registered(XY_BROKER_SERVER_SYSTEM));
    TEST_ASSERT_EQUAL(XY_BROKER_ALREADY_EXISTS,
                      xy_broker_register_server(XY_BROKER_SERVER_SYSTEM,
                                                direct_capture_handler, NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_OK, xy_broker_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(1U, stats.active_servers);
    TEST_ASSERT_EQUAL(XY_BROKER_OK, xy_broker_unregister_server(XY_BROKER_SERVER_SYSTEM));
    TEST_ASSERT_EQUAL_INT(0, xy_broker_is_server_registered(XY_BROKER_SERVER_SYSTEM));
    TEST_ASSERT_EQUAL(XY_BROKER_NOT_FOUND,
                      xy_broker_unregister_server(XY_BROKER_SERVER_SYSTEM));
}

static void test_direct_message_queue_and_limits(void)
{
    const char payload[] = "hello";
    xy_broker_stats_t stats;

    reset_broker();
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_SYSTEM,
                                                direct_capture_handler,
                                                NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_COMM,
                                                direct_capture_handler,
                                                NULL));

    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_send_msg(XY_BROKER_SERVER_SYSTEM, XY_BROKER_SERVER_COMM,
                                         XY_BROKER_MSG_COMM_SEND, payload,
                                         sizeof(payload) - 1U, XY_BROKER_PRIORITY_HIGH));
    TEST_ASSERT_EQUAL_INT(1, xy_broker_get_pending_count(XY_BROKER_SERVER_COMM));
    TEST_ASSERT_EQUAL_INT(1, xy_broker_process_msgs(XY_BROKER_SERVER_COMM, 1));
    TEST_ASSERT_EQUAL_UINT(1U, direct_capture_handler_fake.call_count);
    TEST_ASSERT_NULL(direct_capture_handler_fake.arg1_val);
    TEST_ASSERT_EQUAL(XY_BROKER_SERVER_SYSTEM, last_msg.src_server);
    TEST_ASSERT_EQUAL(XY_BROKER_SERVER_COMM, last_msg.dst_server);
    TEST_ASSERT_EQUAL(XY_BROKER_MSG_COMM_SEND, last_msg.msg_id);
    TEST_ASSERT_EQUAL(XY_BROKER_PRIORITY_HIGH, last_msg.priority);
    TEST_ASSERT_EQUAL_UINT32(sizeof(payload) - 1U, last_msg.payload_len);
    TEST_ASSERT_EQUAL_MEMORY(payload, last_msg.payload, sizeof(payload) - 1U);
    TEST_ASSERT_EQUAL_INT(0, xy_broker_get_pending_count(XY_BROKER_SERVER_COMM));

    TEST_ASSERT_EQUAL(XY_BROKER_NOT_FOUND,
                      xy_broker_send_msg(XY_BROKER_SERVER_SYSTEM, 0x9999,
                                         XY_BROKER_MSG_COMM_SEND, payload,
                                         sizeof(payload) - 1U, XY_BROKER_PRIORITY_NORMAL));
    TEST_ASSERT_EQUAL(XY_BROKER_INVALID_PARAM,
                      xy_broker_send_msg(XY_BROKER_SERVER_SYSTEM, XY_BROKER_SERVER_COMM,
                                         XY_BROKER_MSG_COMM_SEND, payload,
                                         XY_BROKER_MAX_MSG_SIZE + 1U,
                                         XY_BROKER_PRIORITY_NORMAL));
    TEST_ASSERT_EQUAL(XY_BROKER_INVALID_PARAM,
                      xy_broker_send_msg(XY_BROKER_SERVER_SYSTEM, XY_BROKER_SERVER_COMM,
                                         XY_BROKER_MSG_COMM_SEND, NULL, 1U,
                                         XY_BROKER_PRIORITY_NORMAL));
    TEST_ASSERT_EQUAL_INT(0, xy_broker_get_pending_count(XY_BROKER_SERVER_COMM));
    TEST_ASSERT_EQUAL(XY_BROKER_OK, xy_broker_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(1U, stats.total_msg_sent);
    TEST_ASSERT_EQUAL_UINT32(1U, stats.total_msg_delivered);

    for (uint16_t i = 0; i < XY_BROKER_MSG_QUEUE_SIZE; i++) {
        TEST_ASSERT_EQUAL(XY_BROKER_OK,
                          xy_broker_send_msg(XY_BROKER_SERVER_SYSTEM,
                                             XY_BROKER_SERVER_COMM,
                                             XY_BROKER_MSG_COMM_SEND, &i, sizeof(i),
                                             XY_BROKER_PRIORITY_NORMAL));
    }
    TEST_ASSERT_EQUAL(XY_BROKER_QUEUE_FULL,
                      xy_broker_send_msg(XY_BROKER_SERVER_SYSTEM, XY_BROKER_SERVER_COMM,
                                         XY_BROKER_MSG_COMM_SEND, payload,
                                         sizeof(payload) - 1U, XY_BROKER_PRIORITY_NORMAL));
    TEST_ASSERT_EQUAL(XY_BROKER_OK, xy_broker_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(1U, stats.queue_overflow_count);
    TEST_ASSERT_EQUAL_UINT32(1U, stats.total_msg_dropped);

    TEST_ASSERT_EQUAL(XY_BROKER_OK, xy_broker_clear_queue(XY_BROKER_SERVER_COMM));
    TEST_ASSERT_EQUAL_INT(0, xy_broker_get_pending_count(XY_BROKER_SERVER_COMM));
}

static void test_pubsub_create_publish_and_unsubscribe(void)
{
    const uint8_t payload[] = {0xAA, 0x55, 0x12};
    xy_broker_stats_t stats;

    reset_broker();
    TEST_ASSERT_EQUAL(XY_BROKER_NOT_FOUND,
                      xy_broker_publish(XY_BROKER_SERVER_SYSTEM, XY_BROKER_TOPIC_SENSOR_DATA,
                                        XY_BROKER_MSG_SENSOR_DATA, payload, sizeof(payload),
                                        XY_BROKER_PRIORITY_LOW));

    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_subscribe(XY_BROKER_TOPIC_SENSOR_DATA,
                                          XY_BROKER_SERVER_SENSOR,
                                          topic_capture_handler, NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_ALREADY_EXISTS,
                      xy_broker_subscribe(XY_BROKER_TOPIC_SENSOR_DATA,
                                          XY_BROKER_SERVER_SENSOR,
                                          topic_capture_handler, NULL));

    TEST_ASSERT_EQUAL(XY_BROKER_INVALID_PARAM,
                      xy_broker_publish(XY_BROKER_SERVER_SYSTEM,
                                        XY_BROKER_TOPIC_SENSOR_DATA,
                                        XY_BROKER_MSG_SENSOR_DATA, NULL, 1U,
                                        XY_BROKER_PRIORITY_LOW));
    TEST_ASSERT_EQUAL_UINT(0U, topic_capture_handler_fake.call_count);
    TEST_ASSERT_EQUAL(XY_BROKER_OK, xy_broker_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(1U, stats.active_topics);
    TEST_ASSERT_EQUAL_UINT32(0U, stats.total_msg_sent);
    TEST_ASSERT_EQUAL_UINT32(0U, stats.total_msg_delivered);

    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_publish(XY_BROKER_SERVER_SYSTEM, XY_BROKER_TOPIC_SENSOR_DATA,
                                        XY_BROKER_MSG_SENSOR_DATA, payload, sizeof(payload),
                                        XY_BROKER_PRIORITY_CRITICAL));
    TEST_ASSERT_EQUAL_UINT(1U, topic_capture_handler_fake.call_count);
    TEST_ASSERT_NULL(topic_capture_handler_fake.arg1_val);
    TEST_ASSERT_EQUAL(XY_BROKER_TOPIC_SENSOR_DATA, last_msg.topic_id);
    TEST_ASSERT_EQUAL(XY_BROKER_FLAG_BROADCAST, last_msg.flags);
    TEST_ASSERT_EQUAL(XY_BROKER_PRIORITY_CRITICAL, last_msg.priority);
    TEST_ASSERT_EQUAL_UINT32(sizeof(payload), last_msg.payload_len);
    TEST_ASSERT_EQUAL_MEMORY(payload, last_msg.payload, sizeof(payload));

    TEST_ASSERT_EQUAL(XY_BROKER_OK, xy_broker_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(1U, stats.active_topics);
    TEST_ASSERT_EQUAL_UINT32(1U, stats.total_msg_sent);
    TEST_ASSERT_EQUAL_UINT32(1U, stats.total_msg_delivered);

    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_unsubscribe(XY_BROKER_TOPIC_SENSOR_DATA,
                                            XY_BROKER_SERVER_SENSOR));
    TEST_ASSERT_EQUAL(XY_BROKER_OK, xy_broker_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(0U, stats.active_topics);
    TEST_ASSERT_EQUAL(XY_BROKER_NOT_FOUND,
                      xy_broker_unsubscribe(XY_BROKER_TOPIC_SENSOR_DATA,
                                            XY_BROKER_SERVER_SENSOR));
}

static void test_request_response_and_timeout(void)
{
    const char request[] = "ping";
    xy_broker_msg_t response;
    test_context_t ctx = {0};

    reset_broker();
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_SYSTEM,
                                                direct_capture_handler,
                                                NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_COMM, responder_handler, &ctx));

    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_send_msg(XY_BROKER_SERVER_SYSTEM, XY_BROKER_SERVER_COMM,
                                         XY_BROKER_MSG_COMM_SEND, request,
                                         sizeof(request) - 1U, XY_BROKER_PRIORITY_NORMAL));
    TEST_ASSERT_EQUAL_INT(1, xy_broker_process_msgs(XY_BROKER_SERVER_COMM, 0));
    TEST_ASSERT_EQUAL_INT(1, ctx.response_sent);
    TEST_ASSERT_EQUAL_INT(1, xy_broker_get_pending_count(XY_BROKER_SERVER_SYSTEM));
    TEST_ASSERT_EQUAL_INT(1, xy_broker_process_msgs(XY_BROKER_SERVER_SYSTEM, 0));
    TEST_ASSERT_EQUAL_UINT(1U, direct_capture_handler_fake.call_count);
    TEST_ASSERT_NULL(direct_capture_handler_fake.arg1_val);
    TEST_ASSERT_EQUAL(XY_BROKER_SERVER_COMM, last_msg.src_server);
    TEST_ASSERT_EQUAL(XY_BROKER_SERVER_SYSTEM, last_msg.dst_server);
    TEST_ASSERT_EQUAL_UINT32(4U, last_msg.payload_len);
    TEST_ASSERT_EQUAL_MEMORY("pong", last_msg.payload, 4U);

    memset(&response, 0, sizeof(response));
    fake_tick = 0;
    TEST_ASSERT_EQUAL(XY_BROKER_TIMEOUT,
                      xy_broker_request(XY_BROKER_SERVER_SYSTEM, XY_BROKER_SERVER_COMM,
                                        XY_BROKER_MSG_COMM_SEND, request,
                                        sizeof(request) - 1U, &response, 3U));
    TEST_ASSERT_EQUAL_UINT32(3U, fake_tick);
    TEST_ASSERT_EQUAL_UINT(3U, xy_os_delay_fake.call_count);
    TEST_ASSERT_EQUAL_UINT32(1U, xy_os_delay_fake.arg0_val);
}

static void test_handler_failure_is_propagated_and_queue_recovers(void)
{
    const uint32_t payload = 0xA5A55A5AU;
    xy_broker_stats_t stats;

    reset_broker();
    rejecting_handler_fake.return_val = XY_BROKER_ERROR;
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_SYSTEM, rejecting_handler,
                                                NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_send_msg(XY_BROKER_SERVER_SENSOR, XY_BROKER_SERVER_SYSTEM,
                                         XY_BROKER_MSG_SYSTEM_STATUS, &payload, sizeof(payload),
                                         XY_BROKER_PRIORITY_NORMAL));

    TEST_ASSERT_EQUAL(XY_BROKER_ERROR,
                      xy_broker_process_msgs(XY_BROKER_SERVER_SYSTEM, 1U));
    TEST_ASSERT_EQUAL_UINT(1U, rejecting_handler_fake.call_count);
    TEST_ASSERT_EQUAL_INT(0, xy_broker_get_pending_count(XY_BROKER_SERVER_SYSTEM));
    TEST_ASSERT_EQUAL(XY_BROKER_OK, xy_broker_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(0U, stats.total_msg_delivered);
    TEST_ASSERT_EQUAL_UINT32(1U, stats.total_msg_dropped);

    rejecting_handler_fake.return_val = XY_BROKER_OK;
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_send_msg(XY_BROKER_SERVER_SENSOR, XY_BROKER_SERVER_SYSTEM,
                                         XY_BROKER_MSG_SYSTEM_STATUS, &payload, sizeof(payload),
                                         XY_BROKER_PRIORITY_NORMAL));
    TEST_ASSERT_EQUAL_INT(1, xy_broker_process_msgs(XY_BROKER_SERVER_SYSTEM, 1U));
    TEST_ASSERT_EQUAL_UINT(2U, rejecting_handler_fake.call_count);
    TEST_ASSERT_EQUAL(XY_BROKER_OK, xy_broker_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(1U, stats.total_msg_delivered);
    TEST_ASSERT_EQUAL_UINT32(1U, stats.total_msg_dropped);
}

static void test_debug_name_helpers(void)
{
    TEST_ASSERT_EQUAL_STRING("SYSTEM", xy_broker_get_server_name(XY_BROKER_SERVER_SYSTEM));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN", xy_broker_get_server_name(0xEEEE));
    TEST_ASSERT_EQUAL_STRING("SENSOR_DATA", xy_broker_get_msg_name(XY_BROKER_MSG_SENSOR_DATA));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN", xy_broker_get_msg_name(0xEEEE));
    TEST_ASSERT_EQUAL_STRING("LOG_EVENT", xy_broker_get_topic_name(XY_BROKER_TOPIC_LOG_EVENT));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN", xy_broker_get_topic_name(0xEEEE));
}

void setUp(void)
{
    reset_fakes();
}

void tearDown(void)
{
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_lifecycle_and_server_registration);
    RUN_TEST(test_direct_message_queue_and_limits);
    RUN_TEST(test_pubsub_create_publish_and_unsubscribe);
    RUN_TEST(test_request_response_and_timeout);
    RUN_TEST(test_handler_failure_is_propagated_and_queue_recovers);
    RUN_TEST(test_debug_name_helpers);
    TEST_ASSERT_EQUAL(XY_BROKER_OK, xy_broker_deinit());
    return UNITY_END();
}
