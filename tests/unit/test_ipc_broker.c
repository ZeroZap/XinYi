#include "xy_broker.h"
#include "xy_os.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

static uint32_t fake_tick;
static int direct_count;
static int topic_count;
static xy_broker_msg_t last_msg;

typedef struct {
    int response_sent;
    int delay_calls;
} test_context_t;

uint32_t xy_os_tick_get(void)
{
    return fake_tick;
}

xy_os_status_t xy_os_delay(uint32_t ticks)
{
    fake_tick += ticks;
    return XY_OS_OK;
}

static int capture_handler(const xy_broker_msg_t *msg, void *user_data)
{
    int *counter = (int *)user_data;

    assert(msg != NULL);
    if (counter != NULL) {
        (*counter)++;
    }
    memcpy(&last_msg, msg, sizeof(last_msg));
    return XY_BROKER_OK;
}

static int responder_handler(const xy_broker_msg_t *msg, void *user_data)
{
    test_context_t *ctx = (test_context_t *)user_data;
    const char response[] = "pong";

    assert(msg != NULL);
    assert(ctx != NULL);
    assert(msg->msg_id == XY_BROKER_MSG_COMM_SEND);
    assert(msg->payload_len == 4U);
    assert(memcmp(msg->payload, "ping", 4U) == 0);

    ctx->response_sent++;
    assert(xy_broker_respond(msg, response, sizeof(response) - 1U) == XY_BROKER_OK);
    return XY_BROKER_OK;
}

static void reset_broker(void)
{
    fake_tick = 0;
    direct_count = 0;
    topic_count = 0;
    memset(&last_msg, 0, sizeof(last_msg));
    (void)xy_broker_deinit();
    assert(xy_broker_init() == XY_BROKER_OK);
}

static void test_lifecycle_and_server_registration(void)
{
    xy_broker_stats_t stats;

    (void)xy_broker_deinit();
    assert(xy_broker_get_stats(&stats) == XY_BROKER_ERROR);
    assert(xy_broker_init() == XY_BROKER_OK);
    assert(xy_broker_init() == XY_BROKER_OK);
    assert(xy_broker_register_server(0, capture_handler, &direct_count) ==
           XY_BROKER_INVALID_PARAM);
    assert(xy_broker_register_server(XY_BROKER_SERVER_SYSTEM, capture_handler,
                                     &direct_count) == XY_BROKER_OK);
    assert(xy_broker_is_server_registered(XY_BROKER_SERVER_SYSTEM) == 1);
    assert(xy_broker_register_server(XY_BROKER_SERVER_SYSTEM, capture_handler,
                                     &direct_count) == XY_BROKER_ALREADY_EXISTS);
    assert(xy_broker_get_stats(&stats) == XY_BROKER_OK);
    assert(stats.active_servers == 1U);
    assert(xy_broker_unregister_server(XY_BROKER_SERVER_SYSTEM) == XY_BROKER_OK);
    assert(xy_broker_is_server_registered(XY_BROKER_SERVER_SYSTEM) == 0);
    assert(xy_broker_unregister_server(XY_BROKER_SERVER_SYSTEM) == XY_BROKER_NOT_FOUND);
}

static void test_direct_message_queue_and_limits(void)
{
    const char payload[] = "hello";
    xy_broker_stats_t stats;

    reset_broker();
    assert(xy_broker_register_server(XY_BROKER_SERVER_SYSTEM, capture_handler,
                                     &direct_count) == XY_BROKER_OK);
    assert(xy_broker_register_server(XY_BROKER_SERVER_COMM, capture_handler,
                                     &direct_count) == XY_BROKER_OK);

    assert(xy_broker_send_msg(XY_BROKER_SERVER_SYSTEM, XY_BROKER_SERVER_COMM,
                              XY_BROKER_MSG_COMM_SEND, payload,
                              sizeof(payload) - 1U, XY_BROKER_PRIORITY_HIGH) ==
           XY_BROKER_OK);
    assert(xy_broker_get_pending_count(XY_BROKER_SERVER_COMM) == 1);
    assert(xy_broker_process_msgs(XY_BROKER_SERVER_COMM, 1) == 1);
    assert(direct_count == 1);
    assert(last_msg.src_server == XY_BROKER_SERVER_SYSTEM);
    assert(last_msg.dst_server == XY_BROKER_SERVER_COMM);
    assert(last_msg.msg_id == XY_BROKER_MSG_COMM_SEND);
    assert(last_msg.priority == XY_BROKER_PRIORITY_HIGH);
    assert(last_msg.payload_len == sizeof(payload) - 1U);
    assert(memcmp(last_msg.payload, payload, sizeof(payload) - 1U) == 0);
    assert(xy_broker_get_pending_count(XY_BROKER_SERVER_COMM) == 0);

    assert(xy_broker_send_msg(XY_BROKER_SERVER_SYSTEM, 0x9999,
                              XY_BROKER_MSG_COMM_SEND, payload,
                              sizeof(payload) - 1U, XY_BROKER_PRIORITY_NORMAL) ==
           XY_BROKER_NOT_FOUND);
    assert(xy_broker_send_msg(XY_BROKER_SERVER_SYSTEM, XY_BROKER_SERVER_COMM,
                              XY_BROKER_MSG_COMM_SEND, payload,
                              XY_BROKER_MAX_MSG_SIZE + 1U,
                              XY_BROKER_PRIORITY_NORMAL) == XY_BROKER_INVALID_PARAM);

    for (uint16_t i = 0; i < XY_BROKER_MSG_QUEUE_SIZE; i++) {
        assert(xy_broker_send_msg(XY_BROKER_SERVER_SYSTEM, XY_BROKER_SERVER_COMM,
                                  XY_BROKER_MSG_COMM_SEND, &i, sizeof(i),
                                  XY_BROKER_PRIORITY_NORMAL) == XY_BROKER_OK);
    }
    assert(xy_broker_send_msg(XY_BROKER_SERVER_SYSTEM, XY_BROKER_SERVER_COMM,
                              XY_BROKER_MSG_COMM_SEND, payload,
                              sizeof(payload) - 1U, XY_BROKER_PRIORITY_NORMAL) ==
           XY_BROKER_QUEUE_FULL);
    assert(xy_broker_get_stats(&stats) == XY_BROKER_OK);
    assert(stats.queue_overflow_count == 1U);
    assert(stats.total_msg_dropped == 1U);

    assert(xy_broker_clear_queue(XY_BROKER_SERVER_COMM) == XY_BROKER_OK);
    assert(xy_broker_get_pending_count(XY_BROKER_SERVER_COMM) == 0);
}

static void test_pubsub_create_publish_and_unsubscribe(void)
{
    const uint8_t payload[] = {0xAA, 0x55, 0x12};
    xy_broker_stats_t stats;

    reset_broker();
    assert(xy_broker_publish(XY_BROKER_SERVER_SYSTEM, XY_BROKER_TOPIC_SENSOR_DATA,
                             XY_BROKER_MSG_SENSOR_DATA, payload, sizeof(payload),
                             XY_BROKER_PRIORITY_LOW) == XY_BROKER_NOT_FOUND);

    assert(xy_broker_subscribe(XY_BROKER_TOPIC_SENSOR_DATA, XY_BROKER_SERVER_SENSOR,
                               capture_handler, &topic_count) == XY_BROKER_OK);
    assert(xy_broker_subscribe(XY_BROKER_TOPIC_SENSOR_DATA, XY_BROKER_SERVER_SENSOR,
                               capture_handler, &topic_count) ==
           XY_BROKER_ALREADY_EXISTS);

    assert(xy_broker_publish(XY_BROKER_SERVER_SYSTEM, XY_BROKER_TOPIC_SENSOR_DATA,
                             XY_BROKER_MSG_SENSOR_DATA, payload, sizeof(payload),
                             XY_BROKER_PRIORITY_CRITICAL) == XY_BROKER_OK);
    assert(topic_count == 1);
    assert(last_msg.topic_id == XY_BROKER_TOPIC_SENSOR_DATA);
    assert(last_msg.flags == XY_BROKER_FLAG_BROADCAST);
    assert(last_msg.priority == XY_BROKER_PRIORITY_CRITICAL);
    assert(last_msg.payload_len == sizeof(payload));
    assert(memcmp(last_msg.payload, payload, sizeof(payload)) == 0);

    assert(xy_broker_get_stats(&stats) == XY_BROKER_OK);
    assert(stats.active_topics == 1U);
    assert(stats.total_msg_sent == 1U);
    assert(stats.total_msg_delivered == 1U);

    assert(xy_broker_unsubscribe(XY_BROKER_TOPIC_SENSOR_DATA,
                                 XY_BROKER_SERVER_SENSOR) == XY_BROKER_OK);
    assert(xy_broker_get_stats(&stats) == XY_BROKER_OK);
    assert(stats.active_topics == 0U);
    assert(xy_broker_unsubscribe(XY_BROKER_TOPIC_SENSOR_DATA,
                                 XY_BROKER_SERVER_SENSOR) == XY_BROKER_NOT_FOUND);
}

static void test_request_response_and_timeout(void)
{
    const char request[] = "ping";
    xy_broker_msg_t response;
    test_context_t ctx = {0};

    reset_broker();
    assert(xy_broker_register_server(XY_BROKER_SERVER_SYSTEM, capture_handler,
                                     &direct_count) == XY_BROKER_OK);
    assert(xy_broker_register_server(XY_BROKER_SERVER_COMM, responder_handler,
                                     &ctx) == XY_BROKER_OK);

    assert(xy_broker_send_msg(XY_BROKER_SERVER_SYSTEM, XY_BROKER_SERVER_COMM,
                              XY_BROKER_MSG_COMM_SEND, request,
                              sizeof(request) - 1U, XY_BROKER_PRIORITY_NORMAL) ==
           XY_BROKER_OK);
    assert(xy_broker_process_msgs(XY_BROKER_SERVER_COMM, 0) == 1);
    assert(ctx.response_sent == 1);
    assert(xy_broker_get_pending_count(XY_BROKER_SERVER_SYSTEM) == 1);
    assert(xy_broker_process_msgs(XY_BROKER_SERVER_SYSTEM, 0) == 1);
    assert(last_msg.src_server == XY_BROKER_SERVER_COMM);
    assert(last_msg.dst_server == XY_BROKER_SERVER_SYSTEM);
    assert(last_msg.payload_len == 4U);
    assert(memcmp(last_msg.payload, "pong", 4U) == 0);

    memset(&response, 0, sizeof(response));
    fake_tick = 0;
    assert(xy_broker_request(XY_BROKER_SERVER_SYSTEM, XY_BROKER_SERVER_COMM,
                             XY_BROKER_MSG_COMM_SEND, request,
                             sizeof(request) - 1U, &response, 3U) ==
           XY_BROKER_TIMEOUT);
    assert(fake_tick == 3U);
}

static void test_debug_name_helpers(void)
{
    assert(strcmp(xy_broker_get_server_name(XY_BROKER_SERVER_SYSTEM), "SYSTEM") == 0);
    assert(strcmp(xy_broker_get_server_name(0xEEEE), "UNKNOWN") == 0);
    assert(strcmp(xy_broker_get_msg_name(XY_BROKER_MSG_SENSOR_DATA), "SENSOR_DATA") == 0);
    assert(strcmp(xy_broker_get_msg_name(0xEEEE), "UNKNOWN") == 0);
    assert(strcmp(xy_broker_get_topic_name(XY_BROKER_TOPIC_LOG_EVENT), "LOG_EVENT") == 0);
    assert(strcmp(xy_broker_get_topic_name(0xEEEE), "UNKNOWN") == 0);
}

int main(void)
{
    test_lifecycle_and_server_registration();
    test_direct_message_queue_and_limits();
    test_pubsub_create_publish_and_unsubscribe();
    test_request_response_and_timeout();
    test_debug_name_helpers();
    assert(xy_broker_deinit() == XY_BROKER_OK);
    return 0;
}
