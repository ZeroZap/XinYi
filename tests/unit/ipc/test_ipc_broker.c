#include "xy_broker.h"
#include "xy_broker_isr_ingress.h"
#include "xy_os.h"

#include <stdint.h>
#include <string.h>

#include "unity.h"
#include "fff.h"

static uint32_t fake_tick;
static xy_broker_msg_t last_msg;
static unsigned int isr_wake_count;
static int isr_wake_result;
static xy_os_status_t delay_result;
static uint16_t unregister_server_on_delay;
static uint16_t reregister_server_on_delay;

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
    if (delay_result != XY_OS_OK) {
        return delay_result;
    }
    fake_tick += ticks;
    if (unregister_server_on_delay != 0U) {
        uint16_t server_id = unregister_server_on_delay;
        unregister_server_on_delay = 0U;
        TEST_ASSERT_EQUAL(XY_BROKER_OK, xy_broker_unregister_server(server_id));
        if (reregister_server_on_delay == server_id) {
            reregister_server_on_delay = 0U;
            TEST_ASSERT_EQUAL(XY_BROKER_OK,
                              xy_broker_register_server(server_id, direct_capture_handler, NULL));
        }
    }
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

static int wake_from_isr(void *context)
{
    TEST_ASSERT_EQUAL_PTR(&isr_wake_count, context);
    ++isr_wake_count;
    return isr_wake_result;
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
    delay_result = XY_OS_OK;
    unregister_server_on_delay = 0U;
    reregister_server_on_delay = 0U;
    isr_wake_count = 0U;
    isr_wake_result = XY_BROKER_OK;
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
    TEST_ASSERT_EQUAL(XY_BROKER_OK, xy_broker_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(XY_BROKER_MSG_QUEUE_SIZE + 1U,
                             stats.total_msg_dropped);
}

static void test_direct_queue_delivers_highest_priority_first(void)
{
    const uint32_t low_payload = 0x11111111U;
    const uint32_t critical_payload = 0xCCCCCCCCU;
    const uint32_t normal_payload = 0x22222222U;

    reset_broker();
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_SYSTEM,
                                                direct_capture_handler, NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_send_msg(XY_BROKER_SERVER_SENSOR,
                                         XY_BROKER_SERVER_SYSTEM,
                                         XY_BROKER_MSG_SENSOR_DATA, &low_payload,
                                         sizeof(low_payload), XY_BROKER_PRIORITY_LOW));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_send_msg(XY_BROKER_SERVER_SENSOR,
                                         XY_BROKER_SERVER_SYSTEM,
                                         XY_BROKER_MSG_SENSOR_ALARM, &critical_payload,
                                         sizeof(critical_payload), XY_BROKER_PRIORITY_CRITICAL));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_send_msg(XY_BROKER_SERVER_SENSOR,
                                         XY_BROKER_SERVER_SYSTEM,
                                         XY_BROKER_MSG_SENSOR_CONFIG, &normal_payload,
                                         sizeof(normal_payload), XY_BROKER_PRIORITY_NORMAL));

    TEST_ASSERT_EQUAL_INT(1, xy_broker_process_msgs(XY_BROKER_SERVER_SYSTEM, 1U));
    TEST_ASSERT_EQUAL(XY_BROKER_MSG_SENSOR_ALARM, last_msg.msg_id);
    TEST_ASSERT_EQUAL_MEMORY(&critical_payload, last_msg.payload, sizeof(critical_payload));
    TEST_ASSERT_EQUAL_INT(2, xy_broker_get_pending_count(XY_BROKER_SERVER_SYSTEM));

    TEST_ASSERT_EQUAL_INT(1, xy_broker_process_msgs(XY_BROKER_SERVER_SYSTEM, 1U));
    TEST_ASSERT_EQUAL(XY_BROKER_MSG_SENSOR_CONFIG, last_msg.msg_id);
    TEST_ASSERT_EQUAL_MEMORY(&normal_payload, last_msg.payload, sizeof(normal_payload));

    TEST_ASSERT_EQUAL_INT(1, xy_broker_process_msgs(XY_BROKER_SERVER_SYSTEM, 1U));
    TEST_ASSERT_EQUAL(XY_BROKER_MSG_SENSOR_DATA, last_msg.msg_id);
    TEST_ASSERT_EQUAL_MEMORY(&low_payload, last_msg.payload, sizeof(low_payload));
}

static void test_direct_queue_preserves_fifo_within_same_priority(void)
{
    const uint32_t first_payload = 0x11111111U;
    const uint32_t second_payload = 0x22222222U;
    const uint32_t third_payload = 0x33333333U;

    reset_broker();
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_SYSTEM,
                                                direct_capture_handler, NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_send_msg(XY_BROKER_SERVER_SENSOR,
                                         XY_BROKER_SERVER_SYSTEM,
                                         XY_BROKER_MSG_SENSOR_DATA, &first_payload,
                                         sizeof(first_payload), XY_BROKER_PRIORITY_HIGH));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_send_msg(XY_BROKER_SERVER_SENSOR,
                                         XY_BROKER_SERVER_SYSTEM,
                                         XY_BROKER_MSG_SENSOR_CONFIG, &second_payload,
                                         sizeof(second_payload), XY_BROKER_PRIORITY_HIGH));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_send_msg(XY_BROKER_SERVER_SENSOR,
                                         XY_BROKER_SERVER_SYSTEM,
                                         XY_BROKER_MSG_SENSOR_ALARM, &third_payload,
                                         sizeof(third_payload), XY_BROKER_PRIORITY_HIGH));

    TEST_ASSERT_EQUAL_INT(1, xy_broker_process_msgs(XY_BROKER_SERVER_SYSTEM, 1U));
    TEST_ASSERT_EQUAL(XY_BROKER_MSG_SENSOR_DATA, last_msg.msg_id);
    TEST_ASSERT_EQUAL_MEMORY(&first_payload, last_msg.payload, sizeof(first_payload));
    TEST_ASSERT_EQUAL_INT(1, xy_broker_process_msgs(XY_BROKER_SERVER_SYSTEM, 1U));
    TEST_ASSERT_EQUAL(XY_BROKER_MSG_SENSOR_CONFIG, last_msg.msg_id);
    TEST_ASSERT_EQUAL_MEMORY(&second_payload, last_msg.payload, sizeof(second_payload));
    TEST_ASSERT_EQUAL_INT(1, xy_broker_process_msgs(XY_BROKER_SERVER_SYSTEM, 1U));
    TEST_ASSERT_EQUAL(XY_BROKER_MSG_SENSOR_ALARM, last_msg.msg_id);
    TEST_ASSERT_EQUAL_MEMORY(&third_payload, last_msg.payload, sizeof(third_payload));
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

static void test_empty_topics_have_distinct_bounded_ownership(void)
{
    reset_broker();

    for (uint16_t i = 0; i < XY_BROKER_MAX_TOPICS; ++i) {
        TEST_ASSERT_EQUAL(XY_BROKER_OK,
                          xy_broker_create_topic((uint16_t)(XY_BROKER_TOPIC_USER_BASE + i)));
    }

    TEST_ASSERT_EQUAL(XY_BROKER_ALREADY_EXISTS,
                      xy_broker_create_topic(XY_BROKER_TOPIC_USER_BASE));
    TEST_ASSERT_EQUAL(XY_BROKER_NO_MEMORY,
                      xy_broker_create_topic(
                          (uint16_t)(XY_BROKER_TOPIC_USER_BASE + XY_BROKER_MAX_TOPICS)));
}

static void test_pubsub_handler_failure_is_propagated_and_counted(void)
{
    const uint32_t payload = 0xA5A55A5AU;
    xy_broker_stats_t stats;

    reset_broker();
    rejecting_handler_fake.return_val = XY_BROKER_ERROR;
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_subscribe(XY_BROKER_TOPIC_ALARM_EVENT,
                                          XY_BROKER_SERVER_SYSTEM,
                                          rejecting_handler, NULL));

    TEST_ASSERT_EQUAL(XY_BROKER_ERROR,
                      xy_broker_publish(XY_BROKER_SERVER_SENSOR,
                                        XY_BROKER_TOPIC_ALARM_EVENT,
                                        XY_BROKER_MSG_SENSOR_ALARM, &payload,
                                        sizeof(payload), XY_BROKER_PRIORITY_CRITICAL));
    TEST_ASSERT_EQUAL_UINT(1U, rejecting_handler_fake.call_count);
    TEST_ASSERT_EQUAL(XY_BROKER_OK, xy_broker_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(1U, stats.total_msg_sent);
    TEST_ASSERT_EQUAL_UINT32(0U, stats.total_msg_delivered);
    TEST_ASSERT_EQUAL_UINT32(1U, stats.total_msg_dropped);
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

static void test_request_skips_unrelated_source_queue_messages(void)
{
    const uint32_t unrelated_payload = 0xA5A55A5AU;
    const uint32_t response_payload = 0x12345678U;
    const uint32_t request_payload = 0xCAFEBABEU;
    xy_broker_msg_t pending_request;
    xy_broker_msg_t response;

    reset_broker();
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_SYSTEM,
                                                direct_capture_handler, NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_COMM,
                                                direct_capture_handler, NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_send_msg(XY_BROKER_SERVER_SENSOR, XY_BROKER_SERVER_SYSTEM,
                                         XY_BROKER_MSG_SYSTEM_STATUS, &unrelated_payload,
                                         sizeof(unrelated_payload), XY_BROKER_PRIORITY_LOW));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_send_msg(XY_BROKER_SERVER_SYSTEM, XY_BROKER_SERVER_COMM,
                                         XY_BROKER_MSG_SYSTEM_STATUS, NULL, 0U,
                                         XY_BROKER_PRIORITY_LOW));
    TEST_ASSERT_EQUAL_INT(1, xy_broker_process_msgs(XY_BROKER_SERVER_COMM, 1U));
    memcpy(&pending_request, &last_msg, sizeof(pending_request));
    pending_request.msg_id = XY_BROKER_MSG_COMM_SEND;
    pending_request.seq_num++;
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_respond(&pending_request, &response_payload,
                                        sizeof(response_payload)));

    memset(&response, 0, sizeof(response));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_request(XY_BROKER_SERVER_SYSTEM, XY_BROKER_SERVER_COMM,
                                        XY_BROKER_MSG_COMM_SEND, &request_payload,
                                        sizeof(request_payload), &response, 1U));
    TEST_ASSERT_EQUAL(XY_BROKER_SERVER_COMM, response.src_server);
    TEST_ASSERT_EQUAL(XY_BROKER_SERVER_SYSTEM, response.dst_server);
    TEST_ASSERT_EQUAL(XY_BROKER_MSG_COMM_SEND, response.msg_id);
    TEST_ASSERT_EQUAL_UINT16(sizeof(response_payload), response.payload_len);
    TEST_ASSERT_EQUAL_MEMORY(&response_payload, response.payload, sizeof(response_payload));
    TEST_ASSERT_EQUAL_INT(1, xy_broker_get_pending_count(XY_BROKER_SERVER_SYSTEM));

    TEST_ASSERT_EQUAL_INT(1, xy_broker_process_msgs(XY_BROKER_SERVER_SYSTEM, 1U));
    TEST_ASSERT_EQUAL(XY_BROKER_SERVER_SENSOR, last_msg.src_server);
    TEST_ASSERT_EQUAL(XY_BROKER_MSG_SYSTEM_STATUS, last_msg.msg_id);
    TEST_ASSERT_EQUAL_MEMORY(&unrelated_payload, last_msg.payload, sizeof(unrelated_payload));
}

static void test_request_zero_timeout_performs_nonblocking_response_poll(void)
{
    const uint32_t response_payload = 0x12345678U;
    const uint32_t request_payload = 0xCAFEBABEU;
    xy_broker_msg_t pending_request;
    xy_broker_msg_t response;

    reset_broker();
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_SYSTEM,
                                                direct_capture_handler, NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_COMM,
                                                direct_capture_handler, NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_send_msg(XY_BROKER_SERVER_SYSTEM, XY_BROKER_SERVER_COMM,
                                         XY_BROKER_MSG_SYSTEM_STATUS, NULL, 0U,
                                         XY_BROKER_PRIORITY_LOW));
    TEST_ASSERT_EQUAL_INT(1, xy_broker_process_msgs(XY_BROKER_SERVER_COMM, 1U));
    memcpy(&pending_request, &last_msg, sizeof(pending_request));
    pending_request.msg_id = XY_BROKER_MSG_COMM_SEND;
    pending_request.seq_num++;
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_respond(&pending_request, &response_payload,
                                        sizeof(response_payload)));

    memset(&response, 0, sizeof(response));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_request(XY_BROKER_SERVER_SYSTEM, XY_BROKER_SERVER_COMM,
                                        XY_BROKER_MSG_COMM_SEND, &request_payload,
                                        sizeof(request_payload), &response, 0U));
    TEST_ASSERT_EQUAL_MEMORY(&response_payload, response.payload, sizeof(response_payload));
    TEST_ASSERT_EQUAL_UINT(0U, xy_os_delay_fake.call_count);
}

static void test_response_rejects_unstamped_request_without_enqueue(void)
{
    const uint32_t response_payload = 0x12345678U;
    const uint32_t request_payload = 0xCAFEBABEU;
    xy_broker_msg_t unstamped_request;
    xy_broker_msg_t response;

    reset_broker();
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_SYSTEM,
                                                direct_capture_handler, NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_COMM,
                                                direct_capture_handler, NULL));
    memset(&unstamped_request, 0, sizeof(unstamped_request));
    unstamped_request.src_server = XY_BROKER_SERVER_SYSTEM;
    unstamped_request.dst_server = XY_BROKER_SERVER_COMM;
    unstamped_request.msg_id = XY_BROKER_MSG_COMM_SEND;
    TEST_ASSERT_EQUAL(XY_BROKER_INVALID_PARAM,
                      xy_broker_respond(&unstamped_request, &response_payload,
                                        sizeof(response_payload)));
    memset(&response, 0xA5, sizeof(response));

    TEST_ASSERT_EQUAL(XY_BROKER_TIMEOUT,
                      xy_broker_request(XY_BROKER_SERVER_SYSTEM, XY_BROKER_SERVER_COMM,
                                        XY_BROKER_MSG_COMM_SEND, &request_payload,
                                        sizeof(request_payload), &response, 0U));
    TEST_ASSERT_EQUAL_INT(0, xy_broker_get_pending_count(XY_BROKER_SERVER_SYSTEM));
    TEST_ASSERT_EQUAL_HEX8(0xA5, response.payload[0]);
}

static void test_request_rejects_stale_response_with_same_server_and_message_id(void)
{
    const uint32_t stale_payload = 0xDEADBEEFU;
    const uint32_t request_payload = 0xCAFEBABEU;
    xy_broker_msg_t response;

    reset_broker();
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_SYSTEM,
                                                direct_capture_handler, NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_COMM,
                                                direct_capture_handler, NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_send_msg(XY_BROKER_SERVER_COMM, XY_BROKER_SERVER_SYSTEM,
                                         XY_BROKER_MSG_COMM_SEND, &stale_payload,
                                         sizeof(stale_payload), XY_BROKER_PRIORITY_NORMAL));
    memset(&response, 0xA5, sizeof(response));

    TEST_ASSERT_EQUAL(XY_BROKER_TIMEOUT,
                      xy_broker_request(XY_BROKER_SERVER_SYSTEM, XY_BROKER_SERVER_COMM,
                                        XY_BROKER_MSG_COMM_SEND, &request_payload,
                                        sizeof(request_payload), &response, 0U));
    TEST_ASSERT_EQUAL_INT(1, xy_broker_get_pending_count(XY_BROKER_SERVER_SYSTEM));
    TEST_ASSERT_EQUAL_HEX8(0xA5, response.payload[0]);
}

static void test_request_rejects_stale_response_after_16bit_sequence_wrap(void)
{
    const uint32_t stale_payload = 0xDEADBEEFU;
    const uint32_t request_payload = 0xCAFEBABEU;
    xy_broker_msg_t pending_request;
    xy_broker_msg_t response;

    reset_broker();
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_SYSTEM,
                                                direct_capture_handler, NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_COMM,
                                                direct_capture_handler, NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_send_msg(XY_BROKER_SERVER_SYSTEM, XY_BROKER_SERVER_COMM,
                                         XY_BROKER_MSG_COMM_SEND, NULL, 0U,
                                         XY_BROKER_PRIORITY_LOW));
    TEST_ASSERT_EQUAL_INT(1, xy_broker_process_msgs(XY_BROKER_SERVER_COMM, 1U));
    memcpy(&pending_request, &last_msg, sizeof(pending_request));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_respond(&pending_request, &stale_payload,
                                        sizeof(stale_payload)));

    for (uint32_t i = 0U; i <= UINT16_MAX; ++i) {
        TEST_ASSERT_EQUAL(XY_BROKER_OK,
                          xy_broker_send_msg(XY_BROKER_SERVER_SYSTEM,
                                             XY_BROKER_SERVER_COMM,
                                             XY_BROKER_MSG_SYSTEM_STATUS, NULL, 0U,
                                             XY_BROKER_PRIORITY_LOW));
        TEST_ASSERT_EQUAL_INT(1, xy_broker_process_msgs(XY_BROKER_SERVER_COMM, 1U));
    }
    memset(&response, 0xA5, sizeof(response));

    TEST_ASSERT_EQUAL(XY_BROKER_TIMEOUT,
                      xy_broker_request(XY_BROKER_SERVER_SYSTEM, XY_BROKER_SERVER_COMM,
                                        XY_BROKER_MSG_COMM_SEND, &request_payload,
                                        sizeof(request_payload), &response, 0U));
    TEST_ASSERT_EQUAL_INT(1, xy_broker_get_pending_count(XY_BROKER_SERVER_SYSTEM));
    TEST_ASSERT_EQUAL_HEX8(0xA5, response.payload[0]);
}

static void test_request_stops_when_delay_backend_fails(void)
{
    const uint32_t request_payload = 0xCAFEBABEU;
    xy_broker_msg_t response;

    reset_broker();
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_SYSTEM,
                                                direct_capture_handler, NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_COMM,
                                                direct_capture_handler, NULL));
    delay_result = XY_OS_ERROR;
    memset(&response, 0xA5, sizeof(response));

    TEST_ASSERT_EQUAL(XY_BROKER_ERROR,
                      xy_broker_request(XY_BROKER_SERVER_SYSTEM, XY_BROKER_SERVER_COMM,
                                        XY_BROKER_MSG_COMM_SEND, &request_payload,
                                        sizeof(request_payload), &response, 3U));
    TEST_ASSERT_EQUAL_UINT(1U, xy_os_delay_fake.call_count);
    TEST_ASSERT_EQUAL_UINT32(0U, fake_tick);
}

static void test_request_maps_delay_timeout_to_broker_timeout(void)
{
    const uint32_t request_payload = 0xCAFEBABEU;
    xy_broker_msg_t response;

    reset_broker();
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_SYSTEM,
                                                direct_capture_handler, NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_COMM,
                                                direct_capture_handler, NULL));
    delay_result = XY_OS_ERROR_TIMEOUT;
    memset(&response, 0xA5, sizeof(response));

    TEST_ASSERT_EQUAL(XY_BROKER_TIMEOUT,
                      xy_broker_request(XY_BROKER_SERVER_SYSTEM, XY_BROKER_SERVER_COMM,
                                        XY_BROKER_MSG_COMM_SEND, &request_payload,
                                        sizeof(request_payload), &response, 3U));
    TEST_ASSERT_EQUAL_UINT(1U, xy_os_delay_fake.call_count);
    TEST_ASSERT_EQUAL_UINT32(0U, fake_tick);
    TEST_ASSERT_EQUAL_HEX8(0xA5, response.payload[0]);
}

static void test_request_rejects_unregistered_source_without_enqueue(void)
{
    const uint32_t request_payload = 0xCAFEBABEU;
    xy_broker_msg_t response;

    reset_broker();
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_COMM,
                                                direct_capture_handler, NULL));
    memset(&response, 0xA5, sizeof(response));

    TEST_ASSERT_EQUAL(XY_BROKER_NOT_FOUND,
                      xy_broker_request(XY_BROKER_SERVER_SYSTEM, XY_BROKER_SERVER_COMM,
                                        XY_BROKER_MSG_COMM_SEND, &request_payload,
                                        sizeof(request_payload), &response, 3U));
    TEST_ASSERT_EQUAL_INT(0, xy_broker_get_pending_count(XY_BROKER_SERVER_COMM));
    TEST_ASSERT_EQUAL_UINT(0U, xy_os_delay_fake.call_count);
}

static void test_response_rejects_unregistered_responder_without_enqueue(void)
{
    const uint32_t response_payload = 0x12345678U;
    xy_broker_msg_t request;

    reset_broker();
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_SYSTEM,
                                                direct_capture_handler, NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_COMM,
                                                direct_capture_handler, NULL));
    memset(&request, 0, sizeof(request));
    request.src_server = XY_BROKER_SERVER_SYSTEM;
    request.dst_server = XY_BROKER_SERVER_COMM;
    request.msg_id = XY_BROKER_MSG_COMM_SEND;
    request.seq_num = 7U;
    request.priority = XY_BROKER_PRIORITY_NORMAL;

    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_unregister_server(XY_BROKER_SERVER_COMM));
    TEST_ASSERT_EQUAL(XY_BROKER_NOT_FOUND,
                      xy_broker_respond(&request, &response_payload,
                                        sizeof(response_payload)));
    TEST_ASSERT_EQUAL_INT(0, xy_broker_get_pending_count(XY_BROKER_SERVER_SYSTEM));
}

static void test_response_rejects_replacement_request_owners(void)
{
    const uint32_t request_payload = 0xCAFEBABEU;
    const uint32_t response_payload = 0x12345678U;
    xy_broker_msg_t stale_request;

    reset_broker();
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_SYSTEM,
                                                direct_capture_handler, NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_COMM,
                                                direct_capture_handler, NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_send_msg(XY_BROKER_SERVER_SYSTEM, XY_BROKER_SERVER_COMM,
                                         XY_BROKER_MSG_COMM_SEND, &request_payload,
                                         sizeof(request_payload), XY_BROKER_PRIORITY_NORMAL));
    TEST_ASSERT_EQUAL_INT(1, xy_broker_process_msgs(XY_BROKER_SERVER_COMM, 1U));
    memcpy(&stale_request, &last_msg, sizeof(stale_request));

    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_unregister_server(XY_BROKER_SERVER_SYSTEM));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_SYSTEM,
                                                direct_capture_handler, NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_NOT_FOUND,
                      xy_broker_respond(&stale_request, &response_payload,
                                        sizeof(response_payload)));
    TEST_ASSERT_EQUAL_INT(0, xy_broker_get_pending_count(XY_BROKER_SERVER_SYSTEM));

    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_unregister_server(XY_BROKER_SERVER_COMM));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_COMM,
                                                direct_capture_handler, NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_NOT_FOUND,
                      xy_broker_respond(&stale_request, &response_payload,
                                        sizeof(response_payload)));
    TEST_ASSERT_EQUAL_INT(0, xy_broker_get_pending_count(XY_BROKER_SERVER_SYSTEM));
}

static void test_unregister_discards_queued_messages_before_server_id_reuse(void)
{
    const uint32_t stale_payload = 0xDEADBEEFU;
    xy_broker_stats_t stats;

    reset_broker();
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_SYSTEM,
                                                direct_capture_handler, NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_COMM,
                                                direct_capture_handler, NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_send_msg(XY_BROKER_SERVER_COMM,
                                         XY_BROKER_SERVER_SYSTEM,
                                         XY_BROKER_MSG_SYSTEM_STATUS,
                                         &stale_payload, sizeof(stale_payload),
                                         XY_BROKER_PRIORITY_NORMAL));
    TEST_ASSERT_EQUAL_INT(1, xy_broker_get_pending_count(XY_BROKER_SERVER_SYSTEM));

    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_unregister_server(XY_BROKER_SERVER_SYSTEM));
    TEST_ASSERT_EQUAL(XY_BROKER_OK, xy_broker_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(1U, stats.total_msg_sent);
    TEST_ASSERT_EQUAL_UINT32(0U, stats.total_msg_delivered);
    TEST_ASSERT_EQUAL_UINT32(1U, stats.total_msg_dropped);
    TEST_ASSERT_EQUAL(XY_BROKER_NOT_FOUND,
                      xy_broker_get_pending_count(XY_BROKER_SERVER_SYSTEM));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_SYSTEM,
                                                direct_capture_handler, NULL));
    TEST_ASSERT_EQUAL_INT(0, xy_broker_get_pending_count(XY_BROKER_SERVER_SYSTEM));
    TEST_ASSERT_EQUAL_INT(0, xy_broker_process_msgs(XY_BROKER_SERVER_SYSTEM, 0));
    TEST_ASSERT_EQUAL_UINT(0U, direct_capture_handler_fake.call_count);
}

static void test_request_stops_when_source_owner_is_unregistered_while_waiting(void)
{
    const uint32_t request_payload = 0xCAFEBABEU;
    xy_broker_msg_t response;

    reset_broker();
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_SYSTEM,
                                                direct_capture_handler, NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_COMM,
                                                direct_capture_handler, NULL));
    unregister_server_on_delay = XY_BROKER_SERVER_SYSTEM;
    memset(&response, 0xA5, sizeof(response));

    TEST_ASSERT_EQUAL(XY_BROKER_NOT_FOUND,
                      xy_broker_request(XY_BROKER_SERVER_SYSTEM, XY_BROKER_SERVER_COMM,
                                        XY_BROKER_MSG_COMM_SEND, &request_payload,
                                        sizeof(request_payload), &response, 3U));
    TEST_ASSERT_EQUAL_UINT(1U, xy_os_delay_fake.call_count);
    TEST_ASSERT_EQUAL_INT(0, xy_broker_is_server_registered(XY_BROKER_SERVER_SYSTEM));
    TEST_ASSERT_EQUAL_HEX8(0xA5, response.payload[0]);
}

static void test_response_rejects_request_from_previous_broker_lifecycle(void)
{
    const uint32_t request_payload = 0xCAFEBABEU;
    const uint32_t response_payload = 0x12345678U;
    xy_broker_msg_t stale_request;

    reset_broker();
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_SYSTEM,
                                                direct_capture_handler, NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_COMM,
                                                direct_capture_handler, NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_send_msg(XY_BROKER_SERVER_SYSTEM, XY_BROKER_SERVER_COMM,
                                         XY_BROKER_MSG_COMM_SEND, &request_payload,
                                         sizeof(request_payload), XY_BROKER_PRIORITY_NORMAL));
    TEST_ASSERT_EQUAL_INT(1, xy_broker_process_msgs(XY_BROKER_SERVER_COMM, 1U));
    memcpy(&stale_request, &last_msg, sizeof(stale_request));

    TEST_ASSERT_EQUAL(XY_BROKER_OK, xy_broker_deinit());
    TEST_ASSERT_EQUAL(XY_BROKER_OK, xy_broker_init());
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_SYSTEM,
                                                direct_capture_handler, NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_COMM,
                                                direct_capture_handler, NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_NOT_FOUND,
                      xy_broker_respond(&stale_request, &response_payload,
                                        sizeof(response_payload)));
    TEST_ASSERT_EQUAL_INT(0, xy_broker_get_pending_count(XY_BROKER_SERVER_SYSTEM));
}

static void test_request_rejects_replacement_source_owner_while_waiting(void)
{
    const uint32_t request_payload = 0xCAFEBABEU;
    xy_broker_msg_t response;

    reset_broker();
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_SYSTEM,
                                                direct_capture_handler, NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_COMM,
                                                direct_capture_handler, NULL));
    unregister_server_on_delay = XY_BROKER_SERVER_SYSTEM;
    reregister_server_on_delay = XY_BROKER_SERVER_SYSTEM;
    memset(&response, 0xA5, sizeof(response));

    TEST_ASSERT_EQUAL(XY_BROKER_NOT_FOUND,
                      xy_broker_request(XY_BROKER_SERVER_SYSTEM, XY_BROKER_SERVER_COMM,
                                        XY_BROKER_MSG_COMM_SEND, &request_payload,
                                        sizeof(request_payload), &response, 3U));
    TEST_ASSERT_EQUAL_UINT(1U, xy_os_delay_fake.call_count);
    TEST_ASSERT_EQUAL_INT(1, xy_broker_is_server_registered(XY_BROKER_SERVER_SYSTEM));
    TEST_ASSERT_EQUAL_HEX8(0xA5, response.payload[0]);
}

static void test_request_rejects_replacement_destination_owner_while_waiting(void)
{
    const uint32_t request_payload = 0xCAFEBABEU;
    xy_broker_msg_t response;

    reset_broker();
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_SYSTEM,
                                                direct_capture_handler, NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_COMM,
                                                direct_capture_handler, NULL));
    unregister_server_on_delay = XY_BROKER_SERVER_COMM;
    reregister_server_on_delay = XY_BROKER_SERVER_COMM;
    memset(&response, 0xA5, sizeof(response));

    TEST_ASSERT_EQUAL(XY_BROKER_NOT_FOUND,
                      xy_broker_request(XY_BROKER_SERVER_SYSTEM, XY_BROKER_SERVER_COMM,
                                        XY_BROKER_MSG_COMM_SEND, &request_payload,
                                        sizeof(request_payload), &response, 3U));
    TEST_ASSERT_EQUAL_UINT(1U, xy_os_delay_fake.call_count);
    TEST_ASSERT_EQUAL_INT(1, xy_broker_is_server_registered(XY_BROKER_SERVER_COMM));
    TEST_ASSERT_EQUAL_HEX8(0xA5, response.payload[0]);
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

static void test_isr_ingress_queue_full_and_broker_recovery(void)
{
    const uint32_t first = 0x12345678U;
    const uint32_t second = 0xA5A55A5AU;
    xy_broker_isr_msg_t storage[2];
    xy_broker_isr_ingress_t ingress;

    reset_broker();
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_SYSTEM,
                                                direct_capture_handler, NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_isr_ingress_init(&ingress, storage, 2U, wake_from_isr,
                                                 &isr_wake_count));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_isr_publish(&ingress, XY_BROKER_SERVER_TIMER,
                                            XY_BROKER_SERVER_SYSTEM, XY_BROKER_MSG_SYSTEM_STATUS,
                                            &first, sizeof(first), XY_BROKER_PRIORITY_HIGH));
    TEST_ASSERT_EQUAL_UINT(1U, isr_wake_count);
    TEST_ASSERT_EQUAL(XY_BROKER_QUEUE_FULL,
                      xy_broker_isr_publish(&ingress, XY_BROKER_SERVER_TIMER,
                                            XY_BROKER_SERVER_SYSTEM, XY_BROKER_MSG_SYSTEM_STATUS,
                                            &second, sizeof(second), XY_BROKER_PRIORITY_HIGH));
    TEST_ASSERT_EQUAL_UINT(1U, isr_wake_count);
    TEST_ASSERT_EQUAL(XY_BROKER_OK, xy_broker_isr_drain_one(&ingress));
    TEST_ASSERT_EQUAL_INT(1, xy_broker_process_msgs(XY_BROKER_SERVER_SYSTEM, 1U));
    TEST_ASSERT_EQUAL_MEMORY(&first, last_msg.payload, sizeof(first));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_isr_publish(&ingress, XY_BROKER_SERVER_TIMER,
                                            XY_BROKER_SERVER_SYSTEM, XY_BROKER_MSG_SYSTEM_STATUS,
                                            &second, sizeof(second), XY_BROKER_PRIORITY_HIGH));
    TEST_ASSERT_EQUAL(XY_BROKER_OK, xy_broker_isr_drain_one(&ingress));
    TEST_ASSERT_EQUAL_INT(1, xy_broker_process_msgs(XY_BROKER_SERVER_SYSTEM, 1U));
    TEST_ASSERT_EQUAL_MEMORY(&second, last_msg.payload, sizeof(second));
    TEST_ASSERT_EQUAL(XY_BROKER_NOT_FOUND, xy_broker_isr_drain_one(&ingress));
}

static void test_isr_ingress_wake_failure_does_not_publish_message(void)
{
    const uint32_t rejected = 0xA5A55A5AU;
    const uint32_t accepted = 0x12345678U;
    xy_broker_isr_msg_t storage[2];
    xy_broker_isr_ingress_t ingress;

    reset_broker();
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_register_server(XY_BROKER_SERVER_SYSTEM,
                                                direct_capture_handler, NULL));
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_isr_ingress_init(&ingress, storage, 2U, wake_from_isr,
                                                 &isr_wake_count));

    isr_wake_result = XY_BROKER_TIMEOUT;
    TEST_ASSERT_EQUAL(XY_BROKER_TIMEOUT,
                      xy_broker_isr_publish(&ingress, XY_BROKER_SERVER_TIMER,
                                            XY_BROKER_SERVER_SYSTEM, XY_BROKER_MSG_SYSTEM_STATUS,
                                            &rejected, sizeof(rejected), XY_BROKER_PRIORITY_HIGH));
    TEST_ASSERT_EQUAL_UINT(1U, isr_wake_count);
    TEST_ASSERT_EQUAL(XY_BROKER_NOT_FOUND, xy_broker_isr_drain_one(&ingress));

    isr_wake_result = XY_BROKER_OK;
    TEST_ASSERT_EQUAL(XY_BROKER_OK,
                      xy_broker_isr_publish(&ingress, XY_BROKER_SERVER_TIMER,
                                            XY_BROKER_SERVER_SYSTEM, XY_BROKER_MSG_SYSTEM_STATUS,
                                            &accepted, sizeof(accepted), XY_BROKER_PRIORITY_HIGH));
    TEST_ASSERT_EQUAL(XY_BROKER_OK, xy_broker_isr_drain_one(&ingress));
    TEST_ASSERT_EQUAL_INT(1, xy_broker_process_msgs(XY_BROKER_SERVER_SYSTEM, 1U));
    TEST_ASSERT_EQUAL_MEMORY(&accepted, last_msg.payload, sizeof(accepted));
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
    RUN_TEST(test_direct_queue_delivers_highest_priority_first);
    RUN_TEST(test_direct_queue_preserves_fifo_within_same_priority);
    RUN_TEST(test_pubsub_create_publish_and_unsubscribe);
    RUN_TEST(test_empty_topics_have_distinct_bounded_ownership);
    RUN_TEST(test_pubsub_handler_failure_is_propagated_and_counted);
    RUN_TEST(test_request_response_and_timeout);
    RUN_TEST(test_request_skips_unrelated_source_queue_messages);
    RUN_TEST(test_request_zero_timeout_performs_nonblocking_response_poll);
    RUN_TEST(test_response_rejects_unstamped_request_without_enqueue);
    RUN_TEST(test_request_rejects_stale_response_with_same_server_and_message_id);
    RUN_TEST(test_request_rejects_stale_response_after_16bit_sequence_wrap);
    RUN_TEST(test_request_stops_when_delay_backend_fails);
    RUN_TEST(test_request_maps_delay_timeout_to_broker_timeout);
    RUN_TEST(test_request_rejects_unregistered_source_without_enqueue);
    RUN_TEST(test_request_stops_when_source_owner_is_unregistered_while_waiting);
    RUN_TEST(test_request_rejects_replacement_source_owner_while_waiting);
    RUN_TEST(test_request_rejects_replacement_destination_owner_while_waiting);
    RUN_TEST(test_response_rejects_unregistered_responder_without_enqueue);
    RUN_TEST(test_response_rejects_replacement_request_owners);
    RUN_TEST(test_unregister_discards_queued_messages_before_server_id_reuse);
    RUN_TEST(test_response_rejects_request_from_previous_broker_lifecycle);
    RUN_TEST(test_handler_failure_is_propagated_and_queue_recovers);
    RUN_TEST(test_isr_ingress_queue_full_and_broker_recovery);
    RUN_TEST(test_isr_ingress_wake_failure_does_not_publish_message);
    RUN_TEST(test_debug_name_helpers);
    TEST_ASSERT_EQUAL(XY_BROKER_OK, xy_broker_deinit());
    return UNITY_END();
}
