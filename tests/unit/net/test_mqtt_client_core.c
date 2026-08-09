#include "unity.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "fff.h"
#include "xy_mqtt_client.h"

DEFINE_FFF_GLOBALS;

#define MOCK_CONTEXT ((void *)0x12345678u)

static uint8_t g_recv_payload[32];
static size_t g_recv_len;
static int g_recv_result;
static uint8_t g_last_sent[128];
static size_t g_last_sent_len;
static const char *g_message_topic;
static uint8_t g_message_payload[32];
static size_t g_message_payload_len;
static uint8_t g_message_qos;
static uint8_t g_message_dup;
static void *g_message_user_data;
static uint16_t g_published_packet_id;
static uint8_t g_published_called;
static uint16_t g_subscribed_packet_id;
static uint8_t g_subscribed_qos;
static uint8_t g_subscribed_called;
static uint16_t g_unsubscribed_packet_id;
static uint8_t g_unsubscribed_called;

static int mock_send_impl(void *context, const uint8_t *data, size_t len);
static int mock_recv_impl(void *context, uint8_t *data, size_t len, uint32_t timeout_ms);
static void message_cb_impl(void *mqtt, const char *topic, const uint8_t *payload,
                            size_t payload_len, uint8_t qos, uint8_t dup, void *user_data);
static void published_cb_impl(void *mqtt, uint16_t packet_id, void *user_data);
static void subscribed_cb_impl(void *mqtt, uint16_t packet_id, uint8_t qos, void *user_data);
static void unsubscribed_cb_impl(void *mqtt, uint16_t packet_id, void *user_data);

FAKE_VALUE_FUNC(int, mock_send, void *, const uint8_t *, size_t);
FAKE_VALUE_FUNC(int, mock_recv, void *, uint8_t *, size_t, uint32_t);

static int mock_send_impl(void *context, const uint8_t *data, size_t len)
{
    (void)context;
    TEST_ASSERT_LESS_OR_EQUAL_size_t(sizeof(g_last_sent), len);
    memcpy(g_last_sent, data, len);
    g_last_sent_len = len;
    return (int)len;
}

static int mock_recv_impl(void *context, uint8_t *data, size_t len, uint32_t timeout_ms)
{
    (void)context;
    (void)timeout_ms;

    if (g_recv_result != 0) {
        return g_recv_result;
    }

    if (len == 0U) {
        len = g_recv_len;
    }

    size_t copy_len = g_recv_len < len ? g_recv_len : len;
    if (copy_len > 0U) {
        memcpy(data, g_recv_payload, copy_len);
    }
    return (int)copy_len;
}

static void mock_recv_feed(const uint8_t *data, size_t len)
{
    TEST_ASSERT_LESS_OR_EQUAL_size_t(sizeof(g_recv_payload), len);
    memcpy(g_recv_payload, data, len);
    g_recv_len = len;
    g_recv_result = 0;
}

static void message_cb_impl(void *mqtt, const char *topic, const uint8_t *payload,
                            size_t payload_len, uint8_t qos, uint8_t dup, void *user_data)
{
    (void)mqtt;
    TEST_ASSERT_LESS_OR_EQUAL_size_t(sizeof(g_message_payload), payload_len);
    g_message_topic = topic;
    memcpy(g_message_payload, payload, payload_len);
    g_message_payload_len = payload_len;
    g_message_qos = qos;
    g_message_dup = dup;
    g_message_user_data = user_data;
}

static void published_cb_impl(void *mqtt, uint16_t packet_id, void *user_data)
{
    (void)mqtt;
    TEST_ASSERT_EQUAL_PTR(MOCK_CONTEXT, user_data);
    g_published_called++;
    g_published_packet_id = packet_id;
}

static void subscribed_cb_impl(void *mqtt, uint16_t packet_id, uint8_t qos, void *user_data)
{
    (void)mqtt;
    TEST_ASSERT_EQUAL_PTR(MOCK_CONTEXT, user_data);
    g_subscribed_called++;
    g_subscribed_packet_id = packet_id;
    g_subscribed_qos = qos;
}

static void unsubscribed_cb_impl(void *mqtt, uint16_t packet_id, void *user_data)
{
    (void)mqtt;
    TEST_ASSERT_EQUAL_PTR(MOCK_CONTEXT, user_data);
    g_unsubscribed_called++;
    g_unsubscribed_packet_id = packet_id;
}

void setUp(void)
{
    memset(g_recv_payload, 0, sizeof(g_recv_payload));
    g_recv_len = 0;
    g_recv_result = 0;
    memset(g_last_sent, 0, sizeof(g_last_sent));
    g_last_sent_len = 0;
    g_message_topic = NULL;
    memset(g_message_payload, 0, sizeof(g_message_payload));
    g_message_payload_len = 0;
    g_message_qos = 0xFFU;
    g_message_dup = 0xFFU;
    g_message_user_data = NULL;
    g_published_packet_id = 0U;
    g_published_called = 0U;
    g_subscribed_packet_id = 0U;
    g_subscribed_qos = 0xFFU;
    g_subscribed_called = 0U;
    g_unsubscribed_packet_id = 0U;
    g_unsubscribed_called = 0U;

    RESET_FAKE(mock_send);
    RESET_FAKE(mock_recv);
    FFF_RESET_HISTORY();

    mock_send_fake.custom_fake = mock_send_impl;
    mock_recv_fake.custom_fake = mock_recv_impl;
}

void tearDown(void)
{
}

static void test_remaining_length_vectors(void)
{
    struct vector {
        uint32_t value;
        uint8_t encoded[4];
        uint8_t encoded_len;
    } vectors[] = {
        {0, {0x00, 0x00, 0x00, 0x00}, 1},
        {127, {0x7F, 0x00, 0x00, 0x00}, 1},
        {128, {0x80, 0x01, 0x00, 0x00}, 2},
        {321, {0xC1, 0x02, 0x00, 0x00}, 2},
        {16384, {0x80, 0x80, 0x01, 0x00}, 3},
        {2097151, {0xFF, 0xFF, 0x7F, 0x00}, 3},
        {268435455, {0xFF, 0xFF, 0xFF, 0x7F}, 4},
    };

    for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); i++) {
        uint8_t out[4] = {0};
        uint32_t decoded = 0;
        uint8_t consumed = 0;

        TEST_ASSERT_EQUAL_UINT8(vectors[i].encoded_len, xy_mqtt_encode_remaining_length(out, vectors[i].value));
        TEST_ASSERT_EQUAL_MEMORY(vectors[i].encoded, out, vectors[i].encoded_len);
        TEST_ASSERT_EQUAL(XY_MQTT_OK, xy_mqtt_decode_remaining_length(out, &decoded, &consumed));
        TEST_ASSERT_EQUAL_UINT32(vectors[i].value, decoded);
        TEST_ASSERT_EQUAL_UINT8(vectors[i].encoded_len, consumed);
    }
}

static void test_remaining_length_validation(void)
{
    uint8_t out[4] = {0};
    uint8_t malformed[] = {0x80, 0x80, 0x80, 0x80, 0x00};
    uint32_t decoded = 0;
    uint8_t consumed = 0;

    TEST_ASSERT_EQUAL_INT(-1, xy_mqtt_encode_remaining_length(out, 268435456U));
    TEST_ASSERT_EQUAL(XY_MQTT_ERR_INVALID_REMAINING_LEN,
                      xy_mqtt_decode_remaining_length(malformed, &decoded, &consumed));
}

static void test_topic_match_exact_and_wildcards(void)
{
    TEST_ASSERT_TRUE(xy_mqtt_topic_match("sensor/temp", "sensor/temp"));
    TEST_ASSERT_FALSE(xy_mqtt_topic_match("sensor/temp", "sensor/humidity"));

    TEST_ASSERT_TRUE(xy_mqtt_topic_match("sensor/+", "sensor/temp"));
    TEST_ASSERT_FALSE(xy_mqtt_topic_match("sensor/+", "sensor/room/temp"));

    TEST_ASSERT_TRUE(xy_mqtt_topic_match("sensor/#", "sensor/room/temp"));
    TEST_ASSERT_TRUE(xy_mqtt_topic_match("#", "sensor/room/temp"));
    TEST_ASSERT_FALSE(xy_mqtt_topic_match("sensor/+/temp", "sensor/room/humidity"));
}

static void test_client_lifecycle_and_validation(void)
{
    xy_mqtt_config_t config = {0};

    TEST_ASSERT_NULL(xy_mqtt_client_new(NULL));
    TEST_ASSERT_NULL(xy_mqtt_client_new(&config));

    config.transport_context = MOCK_CONTEXT;
    config.send = mock_send;
    config.recv = mock_recv;
    config.keepalive = 0;
    config.tx_buffer_size = 128;
    config.rx_buffer_size = 128;

    xy_mqtt_client_t *client = xy_mqtt_client_new(&config);
    TEST_ASSERT_NOT_NULL(client);
    TEST_ASSERT_FALSE(xy_mqtt_is_connected(client));
    TEST_ASSERT_EQUAL(XY_MQTT_STATE_DISCONNECTED, xy_mqtt_get_state(client));

    TEST_ASSERT_EQUAL(XY_MQTT_ERR_INVALID_PARAM,
                      xy_mqtt_publish(NULL, "topic", (const uint8_t *)"x", 1, XY_MQTT_QOS_0, 0, NULL));
    TEST_ASSERT_EQUAL(XY_MQTT_ERR_INVALID_PARAM,
                      xy_mqtt_publish(client, NULL, (const uint8_t *)"x", 1, XY_MQTT_QOS_0, 0, NULL));
    TEST_ASSERT_EQUAL(XY_MQTT_ERR_INVALID_PARAM,
                      xy_mqtt_publish(client, "topic", NULL, 1, XY_MQTT_QOS_0, 0, NULL));
    TEST_ASSERT_EQUAL(XY_MQTT_ERR_NOT_CONNECTED,
                      xy_mqtt_publish(client, "topic", (const uint8_t *)"x", 1, XY_MQTT_QOS_0, 0, NULL));

    TEST_ASSERT_EQUAL(XY_MQTT_ERR_INVALID_PARAM, xy_mqtt_subscribe(NULL, "topic", XY_MQTT_QOS_0, NULL));
    TEST_ASSERT_EQUAL(XY_MQTT_ERR_INVALID_PARAM, xy_mqtt_subscribe(client, NULL, XY_MQTT_QOS_0, NULL));
    TEST_ASSERT_EQUAL(XY_MQTT_ERR_NOT_CONNECTED, xy_mqtt_subscribe(client, "topic", XY_MQTT_QOS_0, NULL));

    TEST_ASSERT_EQUAL_UINT(0U, mock_send_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(0U, mock_recv_fake.call_count);

    xy_mqtt_client_delete(client);
}

static void test_connect_process_and_transport_callbacks(void)
{
    xy_mqtt_config_t config = {0};
    config.transport_context = MOCK_CONTEXT;
    config.send = mock_send;
    config.recv = mock_recv;
    config.keepalive = 30;
    config.tx_buffer_size = 128;
    config.rx_buffer_size = 128;

    xy_mqtt_client_t *client = xy_mqtt_client_new(&config);
    TEST_ASSERT_NOT_NULL(client);

    TEST_ASSERT_EQUAL(XY_MQTT_OK, xy_mqtt_connect(client, "client1", NULL, NULL));
    TEST_ASSERT_EQUAL(XY_MQTT_STATE_MQTT_CONNECTING, xy_mqtt_get_state(client));
    TEST_ASSERT_EQUAL_UINT(1U, mock_send_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(MOCK_CONTEXT, mock_send_fake.arg0_val);
    TEST_ASSERT_NOT_NULL(mock_send_fake.arg1_val);
    TEST_ASSERT_GREATER_THAN_UINT(0U, mock_send_fake.arg2_val);
    TEST_ASSERT_EQUAL_UINT8((XY_MQTT_TYPE_CONNECT << 4), mock_send_fake.arg1_val[0]);

    const uint8_t connack[] = { (uint8_t)(XY_MQTT_TYPE_CONNACK << 4), 0x02, 0x00, 0x00 };
    mock_recv_feed(connack, sizeof(connack));

    TEST_ASSERT_EQUAL(XY_MQTT_OK, xy_mqtt_process(client, 250));
    TEST_ASSERT_TRUE(xy_mqtt_is_connected(client));
    TEST_ASSERT_EQUAL_UINT(1U, mock_recv_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(MOCK_CONTEXT, mock_recv_fake.arg0_val);
    TEST_ASSERT_NOT_NULL(mock_recv_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT(0U, mock_recv_fake.arg2_val);
    TEST_ASSERT_EQUAL_UINT32(250U, mock_recv_fake.arg3_val);

    xy_mqtt_client_delete(client);
}

static xy_mqtt_client_t *connected_client_with_callbacks(void)
{
    xy_mqtt_config_t config = {0};
    config.transport_context = MOCK_CONTEXT;
    config.send = mock_send;
    config.recv = mock_recv;
    config.keepalive = 30;
    config.tx_buffer_size = 128;
    config.rx_buffer_size = 128;
    config.message_cb = message_cb_impl;
    config.published_cb = published_cb_impl;
    config.subscribed_cb = subscribed_cb_impl;
    config.unsubscribed_cb = unsubscribed_cb_impl;
    config.user_data = MOCK_CONTEXT;

    xy_mqtt_client_t *client = xy_mqtt_client_new(&config);
    TEST_ASSERT_NOT_NULL(client);
    TEST_ASSERT_EQUAL(XY_MQTT_OK, xy_mqtt_connect(client, "client1", NULL, NULL));

    const uint8_t connack[] = { (uint8_t)(XY_MQTT_TYPE_CONNACK << 4), 0x02, 0x00, 0x00 };
    mock_recv_feed(connack, sizeof(connack));
    TEST_ASSERT_EQUAL(XY_MQTT_OK, xy_mqtt_process(client, 250));
    TEST_ASSERT_TRUE(xy_mqtt_is_connected(client));

    RESET_FAKE(mock_send);
    mock_send_fake.custom_fake = mock_send_impl;
    g_last_sent_len = 0;
    memset(g_last_sent, 0, sizeof(g_last_sent));
    return client;
}

static void test_publish_qos1_ack_and_inbound_subscription_callback_flow(void)
{
    xy_mqtt_client_t *client = connected_client_with_callbacks();
    uint16_t packet_id = 0U;
    const uint8_t payload[] = { 'o', 'n' };

    TEST_ASSERT_EQUAL(XY_MQTT_OK,
                      xy_mqtt_publish(client, "cmd/led", payload, sizeof(payload), XY_MQTT_QOS_1,
                                      1U, &packet_id));
    TEST_ASSERT_EQUAL_UINT16(2U, packet_id);
    TEST_ASSERT_EQUAL_UINT(1U, mock_send_fake.call_count);
    TEST_ASSERT_EQUAL_UINT8((XY_MQTT_TYPE_PUBLISH << 4) | 0x03U, g_last_sent[0]);
    TEST_ASSERT_EQUAL_UINT8(0x0DU, g_last_sent[1]);
    TEST_ASSERT_EQUAL_UINT8(0x00U, g_last_sent[2]);
    TEST_ASSERT_EQUAL_UINT8(0x07U, g_last_sent[3]);
    TEST_ASSERT_EQUAL_MEMORY("cmd/led", &g_last_sent[4], 7U);
    TEST_ASSERT_EQUAL_UINT8(0x00U, g_last_sent[11]);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)packet_id, g_last_sent[12]);
    TEST_ASSERT_EQUAL_MEMORY(payload, &g_last_sent[13], sizeof(payload));
    TEST_ASSERT_EQUAL_UINT(15U, g_last_sent_len);

    const uint8_t puback[] = { (uint8_t)(XY_MQTT_TYPE_PUBACK << 4), 0x02, 0x00, 0x02 };
    mock_recv_feed(puback, sizeof(puback));
    TEST_ASSERT_EQUAL(XY_MQTT_OK, xy_mqtt_process(client, 250));
    TEST_ASSERT_EQUAL_UINT8(1U, g_published_called);
    TEST_ASSERT_EQUAL_UINT16(2U, g_published_packet_id);

    TEST_ASSERT_EQUAL(XY_MQTT_OK, xy_mqtt_subscribe(client, "sensor/+", XY_MQTT_QOS_0, NULL));
    const uint8_t publish[] = { (uint8_t)(XY_MQTT_TYPE_PUBLISH << 4), 0x0FU, 0x00U, 0x0BU,
                                's', 'e', 'n', 's', 'o', 'r', '/', 't', 'e', 'm', 'p',
                                '4', '2' };
    mock_recv_feed(publish, sizeof(publish));
    TEST_ASSERT_EQUAL(XY_MQTT_OK, xy_mqtt_process(client, 250));
    TEST_ASSERT_EQUAL_STRING("sensor/temp", g_message_topic);
    TEST_ASSERT_EQUAL_MEMORY("42", g_message_payload, 2U);
    TEST_ASSERT_EQUAL_UINT(2U, g_message_payload_len);
    TEST_ASSERT_EQUAL_UINT8(XY_MQTT_QOS_0, g_message_qos);
    TEST_ASSERT_EQUAL_UINT8(0U, g_message_dup);
    TEST_ASSERT_EQUAL_PTR(NULL, g_message_user_data);

    xy_mqtt_client_delete(client);
}

static void test_subscribe_and_unsubscribe_ack_callbacks(void)
{
    xy_mqtt_client_t *client = connected_client_with_callbacks();
    uint16_t subscribe_id = 0U;
    uint16_t unsubscribe_id = 0U;

    TEST_ASSERT_EQUAL(XY_MQTT_OK, xy_mqtt_subscribe(client, "sensor/+", XY_MQTT_QOS_1,
                                                   &subscribe_id));
    TEST_ASSERT_EQUAL_UINT16(2U, subscribe_id);
    TEST_ASSERT_EQUAL_UINT8((XY_MQTT_TYPE_SUBSCRIBE << 4) | 0x02U, g_last_sent[0]);
    TEST_ASSERT_EQUAL_UINT8(0x0DU, g_last_sent[1]);
    TEST_ASSERT_EQUAL_UINT8(0x00U, g_last_sent[2]);
    TEST_ASSERT_EQUAL_UINT8(0x02U, g_last_sent[3]);
    TEST_ASSERT_EQUAL_MEMORY("sensor/+", &g_last_sent[6], 8U);
    TEST_ASSERT_EQUAL_UINT8(XY_MQTT_QOS_1, g_last_sent[14]);

    const uint8_t suback[] = { (uint8_t)(XY_MQTT_TYPE_SUBACK << 4), 0x03, 0x00, 0x02,
                               XY_MQTT_QOS_1 };
    mock_recv_feed(suback, sizeof(suback));
    TEST_ASSERT_EQUAL(XY_MQTT_OK, xy_mqtt_process(client, 250));
    TEST_ASSERT_EQUAL_UINT8(1U, g_subscribed_called);
    TEST_ASSERT_EQUAL_UINT16(subscribe_id, g_subscribed_packet_id);
    TEST_ASSERT_EQUAL_UINT8(XY_MQTT_QOS_1, g_subscribed_qos);

    TEST_ASSERT_EQUAL(XY_MQTT_OK, xy_mqtt_unsubscribe(client, "sensor/+", &unsubscribe_id));
    TEST_ASSERT_EQUAL_UINT16(3U, unsubscribe_id);
    TEST_ASSERT_EQUAL_UINT8((XY_MQTT_TYPE_UNSUBSCRIBE << 4) | 0x02U, g_last_sent[0]);
    TEST_ASSERT_EQUAL_UINT8(0x0CU, g_last_sent[1]);
    TEST_ASSERT_EQUAL_UINT8(0x00U, g_last_sent[2]);
    TEST_ASSERT_EQUAL_UINT8(0x03U, g_last_sent[3]);
    TEST_ASSERT_EQUAL_MEMORY("sensor/+", &g_last_sent[6], 8U);

    const uint8_t unsuback[] = { (uint8_t)(XY_MQTT_TYPE_UNSUBACK << 4), 0x02, 0x00,
                                 0x03 };
    mock_recv_feed(unsuback, sizeof(unsuback));
    TEST_ASSERT_EQUAL(XY_MQTT_OK, xy_mqtt_process(client, 250));
    TEST_ASSERT_EQUAL_UINT8(1U, g_unsubscribed_called);
    TEST_ASSERT_EQUAL_UINT16(unsubscribe_id, g_unsubscribed_packet_id);

    const uint8_t publish[] = { (uint8_t)(XY_MQTT_TYPE_PUBLISH << 4), 0x0FU, 0x00U, 0x0BU,
                                's', 'e', 'n', 's', 'o', 'r', '/', 't', 'e', 'm', 'p',
                                '4', '2' };
    mock_recv_feed(publish, sizeof(publish));
    TEST_ASSERT_EQUAL(XY_MQTT_OK, xy_mqtt_process(client, 250));
    TEST_ASSERT_NULL(g_message_topic);

    xy_mqtt_client_delete(client);
}

static void test_suback_failure_sets_error_without_claiming_success_callback(void)
{
    xy_mqtt_client_t *client = connected_client_with_callbacks();
    uint16_t subscribe_id = 0U;

    TEST_ASSERT_EQUAL(XY_MQTT_OK, xy_mqtt_subscribe(client, "alarm/+", XY_MQTT_QOS_1,
                                                   &subscribe_id));

    const uint8_t suback_failed[] = { (uint8_t)(XY_MQTT_TYPE_SUBACK << 4), 0x03U, 0x00U,
                                      (uint8_t)subscribe_id, 0x80U };
    mock_recv_feed(suback_failed, sizeof(suback_failed));

    TEST_ASSERT_EQUAL(XY_MQTT_ERR_SUBSCRIPTION_FAILED, xy_mqtt_process(client, 250));
    TEST_ASSERT_EQUAL(XY_MQTT_ERR_SUBSCRIPTION_FAILED, xy_mqtt_get_error(client));
    TEST_ASSERT_EQUAL_UINT8(0U, g_subscribed_called);

    xy_mqtt_client_delete(client);
}

static void test_disconnect_sends_mqtt_disconnect_before_clearing_state(void)
{
    xy_mqtt_client_t *client = connected_client_with_callbacks();

    TEST_ASSERT_EQUAL(XY_MQTT_OK, xy_mqtt_disconnect(client));
    TEST_ASSERT_EQUAL(XY_MQTT_STATE_DISCONNECTED, xy_mqtt_get_state(client));
    TEST_ASSERT_EQUAL_UINT(1U, mock_send_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(2U, g_last_sent_len);
    TEST_ASSERT_EQUAL_UINT8((XY_MQTT_TYPE_DISCONNECT << 4), g_last_sent[0]);
    TEST_ASSERT_EQUAL_UINT8(0U, g_last_sent[1]);

    xy_mqtt_client_delete(client);
}

static void test_connack_strings(void)
{
    TEST_ASSERT_EQUAL_STRING("Connection accepted", xy_mqtt_connack_rc_string(XY_MQTT_CONNACK_RC_ACCEPTED));
    TEST_ASSERT_EQUAL_STRING("Not authorized", xy_mqtt_connack_rc_string(XY_MQTT_CONNACK_RC_NOT_AUTHORIZED));
    TEST_ASSERT_EQUAL_STRING("Unknown error", xy_mqtt_connack_rc_string(99));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_remaining_length_vectors);
    RUN_TEST(test_remaining_length_validation);
    RUN_TEST(test_topic_match_exact_and_wildcards);
    RUN_TEST(test_client_lifecycle_and_validation);
    RUN_TEST(test_connect_process_and_transport_callbacks);
    RUN_TEST(test_publish_qos1_ack_and_inbound_subscription_callback_flow);
    RUN_TEST(test_subscribe_and_unsubscribe_ack_callbacks);
    RUN_TEST(test_suback_failure_sets_error_without_claiming_success_callback);
    RUN_TEST(test_disconnect_sends_mqtt_disconnect_before_clearing_state);
    RUN_TEST(test_connack_strings);
    return UNITY_END();
}
