#include "unity.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "xy_mqtt_client.h"

static int mock_send(void *context, const uint8_t *data, size_t len)
{
    (void)context;
    (void)data;
    return (int)len;
}

static int mock_recv(void *context, uint8_t *data, size_t len, uint32_t timeout_ms)
{
    (void)context;
    (void)data;
    (void)len;
    (void)timeout_ms;
    return 0;
}

void setUp(void)
{
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
    RUN_TEST(test_connack_strings);
    return UNITY_END();
}
