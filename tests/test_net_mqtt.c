/**
 * @file test_net_mqtt.c
 * @brief NET MQTT Protocol Unit Tests
 * @version 1.0.0
 * @date 2026-03-01
 */

#include <stdint.h>
#include <string.h>
#include "unity.h"

/* MQTT header */
#include "xy_mqtt.h"

/* ==================== Test Fixtures ==================== */

static xy_mqtt_client_t client;
static uint8_t buffer[256];

void setUp(void)
{
    memset(&client, 0, sizeof(client));
    memset(buffer, 0, sizeof(buffer));
}

void tearDown(void)
{
}

/* ==================== MQTT Structure Tests ==================== */

void test_mqtt_client_structure_size(void)
{
    TEST_ASSERT_TRUE(sizeof(xy_mqtt_client_t) >= 16);
}

void test_mqtt_message_structure_size(void)
{
    TEST_ASSERT_TRUE(sizeof(xy_mqtt_message_t) >= 12);
}

/* ==================== MQTT Connect Tests ==================== */

void test_mqtt_connect_null_param(void)
{
    int ret = xy_mqtt_connect(NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(XY_DEVICE_INVALID_PARAM, ret);
}

/* ==================== MQTT Publish Tests ==================== */

void test_mqtt_publish_null_param(void)
{
    int ret = xy_mqtt_publish(NULL, NULL, NULL, 0);
    TEST_ASSERT_EQUAL(XY_DEVICE_INVALID_PARAM, ret);
}

void test_mqtt_publish_null_topic(void)
{
    int ret = xy_mqtt_publish(&client, NULL, "data", 4);
    TEST_ASSERT_EQUAL(XY_DEVICE_INVALID_PARAM, ret);
}

/* ==================== MQTT Subscribe Tests ==================== */

void test_mqtt_subscribe_null_param(void)
{
    int ret = xy_mqtt_subscribe(NULL, NULL, 0);
    TEST_ASSERT_EQUAL(XY_DEVICE_INVALID_PARAM, ret);
}

/* ==================== MQTT Packet Tests ==================== */

void test_mqtt_packet_encode_null_param(void)
{
    int ret = xy_mqtt_packet_encode(NULL, NULL, 0);
    TEST_ASSERT_EQUAL(XY_DEVICE_INVALID_PARAM, ret);
}

void test_mqtt_packet_decode_null_param(void)
{
    int ret = xy_mqtt_packet_decode(NULL, NULL, 0);
    TEST_ASSERT_EQUAL(XY_DEVICE_INVALID_PARAM, ret);
}

/* ==================== Main ==================== */

int main(void)
{
    UNITY_BEGIN();

    /* Structure Tests */
    RUN_TEST(test_mqtt_client_structure_size);
    RUN_TEST(test_mqtt_message_structure_size);

    /* Connect Tests */
    RUN_TEST(test_mqtt_connect_null_param);

    /* Publish Tests */
    RUN_TEST(test_mqtt_publish_null_param);
    RUN_TEST(test_mqtt_publish_null_topic);

    /* Subscribe Tests */
    RUN_TEST(test_mqtt_subscribe_null_param);

    /* Packet Tests */
    RUN_TEST(test_mqtt_packet_encode_null_param);
    RUN_TEST(test_mqtt_packet_decode_null_param);

    return UNITY_END();
}
