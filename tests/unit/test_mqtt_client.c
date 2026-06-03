#include <assert.h>
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

        assert(xy_mqtt_encode_remaining_length(out, vectors[i].value) == vectors[i].encoded_len);
        assert(memcmp(out, vectors[i].encoded, vectors[i].encoded_len) == 0);
        assert(xy_mqtt_decode_remaining_length(out, &decoded, &consumed) == XY_MQTT_OK);
        assert(decoded == vectors[i].value);
        assert(consumed == vectors[i].encoded_len);
    }
}

static void test_remaining_length_validation(void)
{
    uint8_t out[4] = {0};
    uint8_t malformed[] = {0x80, 0x80, 0x80, 0x80, 0x00};
    uint32_t decoded = 0;
    uint8_t consumed = 0;

    assert(xy_mqtt_encode_remaining_length(out, 268435456U) == -1);
    assert(xy_mqtt_decode_remaining_length(malformed, &decoded, &consumed) == XY_MQTT_ERR_INVALID_REMAINING_LEN);
}

static void test_topic_match_exact_and_wildcards(void)
{
    assert(xy_mqtt_topic_match("sensor/temp", "sensor/temp"));
    assert(!xy_mqtt_topic_match("sensor/temp", "sensor/humidity"));

    assert(xy_mqtt_topic_match("sensor/+", "sensor/temp"));
    assert(!xy_mqtt_topic_match("sensor/+", "sensor/room/temp"));

    assert(xy_mqtt_topic_match("sensor/#", "sensor/room/temp"));
    assert(xy_mqtt_topic_match("#", "sensor/room/temp"));
    assert(!xy_mqtt_topic_match("sensor/+/temp", "sensor/room/humidity"));
}

static void test_client_lifecycle_and_validation(void)
{
    xy_mqtt_config_t config = {0};

    assert(xy_mqtt_client_new(NULL) == NULL);
    assert(xy_mqtt_client_new(&config) == NULL);

    config.send = mock_send;
    config.recv = mock_recv;
    config.keepalive = 0;
    config.tx_buffer_size = 128;
    config.rx_buffer_size = 128;

    xy_mqtt_client_t *client = xy_mqtt_client_new(&config);
    assert(client != NULL);
    assert(!xy_mqtt_is_connected(client));
    assert(xy_mqtt_get_state(client) == XY_MQTT_STATE_DISCONNECTED);

    assert(xy_mqtt_publish(NULL, "topic", (const uint8_t *)"x", 1, XY_MQTT_QOS_0, 0, NULL) == XY_MQTT_ERR_INVALID_PARAM);
    assert(xy_mqtt_publish(client, NULL, (const uint8_t *)"x", 1, XY_MQTT_QOS_0, 0, NULL) == XY_MQTT_ERR_INVALID_PARAM);
    assert(xy_mqtt_publish(client, "topic", NULL, 1, XY_MQTT_QOS_0, 0, NULL) == XY_MQTT_ERR_INVALID_PARAM);
    assert(xy_mqtt_publish(client, "topic", (const uint8_t *)"x", 1, XY_MQTT_QOS_0, 0, NULL) == XY_MQTT_ERR_NOT_CONNECTED);

    assert(xy_mqtt_subscribe(NULL, "topic", XY_MQTT_QOS_0, NULL) == XY_MQTT_ERR_INVALID_PARAM);
    assert(xy_mqtt_subscribe(client, NULL, XY_MQTT_QOS_0, NULL) == XY_MQTT_ERR_INVALID_PARAM);
    assert(xy_mqtt_subscribe(client, "topic", XY_MQTT_QOS_0, NULL) == XY_MQTT_ERR_NOT_CONNECTED);

    xy_mqtt_client_delete(client);
}

static void test_connack_strings(void)
{
    assert(strcmp(xy_mqtt_connack_rc_string(XY_MQTT_CONNACK_RC_ACCEPTED), "Connection accepted") == 0);
    assert(strcmp(xy_mqtt_connack_rc_string(XY_MQTT_CONNACK_RC_NOT_AUTHORIZED), "Not authorized") == 0);
    assert(strcmp(xy_mqtt_connack_rc_string(99), "Unknown error") == 0);
}

int main(void)
{
    test_remaining_length_vectors();
    test_remaining_length_validation();
    test_topic_match_exact_and_wildcards();
    test_client_lifecycle_and_validation();
    test_connack_strings();
    return 0;
}
