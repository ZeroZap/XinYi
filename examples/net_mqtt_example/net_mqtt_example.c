/**
 * @file net_mqtt_example.c
 * @brief MQTT Client Example
 *
 * This example demonstrates how to use the XY_MQTT client to:
 * - Initialize and configure an MQTT client
 * - Connect to an MQTT broker with username/password
 * - Subscribe to topics with wildcard support
 * - Publish messages to topics
 * - Handle incoming messages via callbacks
 * - Process keep-alive and disconnect gracefully
 *
 * Usage:
 *   ./net_mqtt_example <broker_host> <broker_port>
 *
 * Example:
 *   ./net_mqtt_example localhost 1883
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "xy_mqtt_client.h"
#include "xy_log.h"

/*============================================================================
 * Example Configuration
 *============================================================================*/

/** @brief Default MQTT broker host */
#define DEFAULT_BROKER_HOST "localhost"

/** @brief Default MQTT broker port */
#define DEFAULT_BROKER_PORT 1883

/** @brief Default client ID */
#define DEFAULT_CLIENT_ID "xy_mqtt_example"

/** @brief Default keep-alive interval in seconds */
#define DEFAULT_KEEPALIVE 60

/*============================================================================
 * Transport Layer Simulation (for PC testing)
 * Note: In real embedded systems, replace with actual TCP/TLS implementation
 *============================================================================*/

/**
 * @brief Simulated socket context for PC testing
 */
typedef struct {
    char *host;
    uint16_t port;
    int socket;  /* Placeholder for actual socket */
    bool connected;
} mqtt_socket_context_t;

/* Placeholder send function - replace with actual TCP send */
static int mqtt_sim_send(void *context, const uint8_t *data, size_t len)
{
    (void)context;
    (void)data;
    (void)len;

    /* In real implementation, this would send data over TCP socket */
    printf("[MQTT] Send: %zu bytes (simulated)\n", len);

    /* Print packet type for debugging */
    uint8_t packet_type = (data[0] >> 4) & 0x0F;
    switch (packet_type) {
        case XY_MQTT_TYPE_CONNECT:
            printf("         -> CONNECT\n");
            break;
        case XY_MQTT_TYPE_PUBLISH:
            printf("         -> PUBLISH\n");
            break;
        case XY_MQTT_TYPE_SUBSCRIBE:
            printf("         -> SUBSCRIBE\n");
            break;
        case XY_MQTT_TYPE_UNSUBSCRIBE:
            printf("         -> UNSUBSCRIBE\n");
            break;
        case XY_MQTT_TYPE_PINGREQ:
            printf("         -> PINGREQ\n");
            break;
        case XY_MQTT_TYPE_DISCONNECT:
            printf("         -> DISCONNECT\n");
            break;
        default:
            printf("         -> Packet type: %d\n", packet_type);
            break;
    }

    return (int)len;
}

/* Placeholder receive function - replace with actual TCP receive */
static int mqtt_sim_recv(void *context, uint8_t *data, size_t len, uint32_t timeout_ms)
{
    (void)context;
    (void)data;
    (void)len;
    (void)timeout_ms;

    /* In real implementation, this would receive data from TCP socket */
    /* For simulation, we return 0 (timeout) to indicate no data */
    return 0;
}

/*============================================================================
 * MQTT Callbacks
 *============================================================================*/

/**
 * @brief Connected callback
 */
static void on_connected(void *mqtt, uint8_t session_present, uint8_t return_code, void *user_data)
{
    (void)mqtt;
    (void)user_data;

    printf("\n[MQTT] Connected to broker!\n");
    printf("       Session present: %s\n", session_present ? "yes" : "no");
    printf("       Return code: %s (%d)\n", xy_mqtt_connack_rc_string(return_code), return_code);
}

/**
 * @brief Disconnected callback
 */
static void on_disconnected(void *mqtt, int reason, void *user_data)
{
    (void)mqtt;
    (void)user_data;

    printf("\n[MQTT] Disconnected from broker!\n");
    printf("       Reason: %d\n", reason);
}

/**
 * @brief Message callback - called when PUBLISH received
 */
static void on_message(void *mqtt, const char *topic, const uint8_t *payload,
                      size_t payload_len, uint8_t qos, uint8_t dup, void *user_data)
{
    (void)mqtt;
    (void)user_data;

    printf("\n[MQTT] Message received:\n");
    printf("       Topic: %s\n", topic);
    printf("       QoS: %d\n", qos);
    printf("       Dup: %s\n", dup ? "yes" : "no");
    printf("       Payload (%zu bytes): ", payload_len);

    /* Print payload (limit to 100 bytes) */
    size_t print_len = payload_len > 100 ? 100 : payload_len;
    for (size_t i = 0; i < print_len; i++) {
        if (payload[i] >= 32 && payload[i] <= 126) {
            printf("%c", payload[i]);
        } else {
            printf("\\x%02x", payload[i]);
        }
    }
    if (payload_len > 100) {
        printf("... (%zu more bytes)", payload_len - 100);
    }
    printf("\n");
}

/**
 * @brief Published callback - called when PUBLISH acknowledged (QoS 1)
 */
static void on_published(void *mqtt, uint16_t packet_id, void *user_data)
{
    (void)mqtt;
    (void)user_data;

    printf("\n[MQTT] Message published successfully!\n");
    printf("       Packet ID: %d\n", packet_id);
}

/**
 * @brief Subscribed callback - called when SUBSCRIBE acknowledged
 */
static void on_subscribed(void *mqtt, uint16_t packet_id, uint8_t qos, void *user_data)
{
    (void)mqtt;
    (void)user_data;

    printf("\n[MQTT] Topic subscribed successfully!\n");
    printf("       Packet ID: %d\n", packet_id);
    printf("       Granted QoS: %d\n", qos);
}

/**
 * @brief Unsubscribed callback - called when UNSUBSCRIBE acknowledged
 */
static void on_unsubscribed(void *mqtt, uint16_t packet_id, void *user_data)
{
    (void)mqtt;
    (void)user_data;

    printf("\n[MQTT] Topic unsubscribed successfully!\n");
    printf("       Packet ID: %d\n", packet_id);
}

/*============================================================================
 * Utility Functions
 *============================================================================*/

/**
 * @brief Print usage information
 */
static void print_usage(const char *program_name)
{
    printf("Usage: %s [options]\n", program_name);
    printf("\nOptions:\n");
    printf("  -h, --help          Show this help message\n");
    printf("  -H, --host <host>   MQTT broker host (default: %s)\n", DEFAULT_BROKER_HOST);
    printf("  -p, --port <port>   MQTT broker port (default: %d)\n", DEFAULT_BROKER_PORT);
    printf("  -i, --id <client>   Client ID (default: %s)\n", DEFAULT_CLIENT_ID);
    printf("  -u, --user <user>   Username (optional)\n");
    printf("  -P, --pass <pass>   Password (optional)\n");
    printf("  -k, --keepalive <s> Keep-alive interval (default: %d)\n", DEFAULT_KEEPALIVE);
    printf("\nExample:\n");
    printf("  %s -H localhost -p 1883 -u myuser -P mypass\n", program_name);
}

/**
 * @brief Parse command line arguments
 */
static int parse_args(int argc, char *argv[],
                      char *host, uint16_t *port,
                      char *client_id, char *username, char *password,
                      uint16_t *keepalive)
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return -1;
        } else if (strcmp(argv[i], "-H") == 0 || strcmp(argv[i], "--host") == 0) {
            if (i + 1 < argc) {
                strncpy(host, argv[++i], 255);
            }
        } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) {
            if (i + 1 < argc) {
                *port = (uint16_t)atoi(argv[++i]);
            }
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--id") == 0) {
            if (i + 1 < argc) {
                strncpy(client_id, argv[++i], 23);
            }
        } else if (strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--user") == 0) {
            if (i + 1 < argc) {
                strncpy(username, argv[++i], 64);
            }
        } else if (strcmp(argv[i], "-P") == 0 || strcmp(argv[i], "--pass") == 0) {
            if (i + 1 < argc) {
                strncpy(password, argv[++i], 64);
            }
        } else if (strcmp(argv[i], "-k") == 0 || strcmp(argv[i], "--keepalive") == 0) {
            if (i + 1 < argc) {
                *keepalive = (uint16_t)atoi(argv[++i]);
            }
        } else {
            printf("Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return -1;
        }
    }

    return 0;
}

/*============================================================================
 * Topic Matching Test
 *============================================================================*/

static void test_topic_matching(void)
{
    printf("\n========================================\n");
    printf("Topic Matching Tests\n");
    printf("========================================\n\n");

    struct {
        const char *filter;
        const char *topic;
        bool expected;
    } tests[] = {
        /* Exact match */
        {"home/sensors/temperature", "home/sensors/temperature", true},

        /* Single-level wildcard */
        {"home/sensors/+", "home/sensors/temperature", true},
        {"home/sensors/+", "home/sensors/humidity", true},
        {"home/+/temperature", "home/sensors/temperature", true},
        {"+/sensors/temperature", "home/sensors/temperature", true},

        /* Multi-level wildcard */
        {"home/#", "home/sensors", true},
        {"home/#", "home/sensors/temperature", true},
        {"home/#", "home/sensors/temperature/bedroom", true},

        /* Combined wildcards */
        {"home/+/temperature/#", "home/sensors/temperature", true},
        {"home/+/temperature/#", "home/sensors/temperature/bedroom", true},

        /* Non-matches */
        {"home/sensors/+", "home/sensors/temperature/humidity", false},
        {"home/#", "home", false},
        {"home+/sensors", "homes/sensors", false},
    };

    int passed = 0;
    int failed = 0;

    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        bool result = xy_mqtt_topic_match(tests[i].filter, tests[i].topic);
        bool match = (result == tests[i].expected);

        printf("Filter: \"%-30s\" Topic: \"%-35s\" -> %s (%s)\n",
               tests[i].filter, tests[i].topic,
               result ? "MATCH" : "NO MATCH",
               match ? "PASS" : "FAIL");

        if (match) {
            passed++;
        } else {
            failed++;
        }
    }

    printf("\n----------------------------------------\n");
    printf("Results: %d passed, %d failed\n", passed, failed);
    printf("========================================\n\n");
}

/*============================================================================
 * Remaining Length Encoding Test
 *============================================================================*/

static void test_remaining_length_encoding(void)
{
    printf("\n========================================\n");
    printf("Remaining Length Encoding Tests\n");
    printf("========================================\n\n");

    struct {
        uint32_t value;
        uint8_t encoded[4];
        int enc_len;
    } tests[] = {
        {0, {0}, 1},
        {127, {127}, 1},
        {128, {128, 1}, 2},
        {16383, {255, 127}, 2},
        {16384, {128, 128, 1}, 3},
        {2097152, {128, 128, 128, 1}, 4},
        {268435455, {255, 255, 255, 127}, 4},  /* Max MQTT remaining length */
    };

    int passed = 0;
    int failed = 0;

    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        uint8_t buf[4];
        int enc_len = xy_mqtt_encode_remaining_length(buf, tests[i].value);

        printf("Value: %10u -> Encoded: [", tests[i].value);
        for (int j = 0; j < enc_len; j++) {
            printf(" 0x%02x", buf[j]);
        }
        printf(" ] Expected: [");
        for (int j = 0; j < tests[i].enc_len; j++) {
            printf(" 0x%02x", tests[i].encoded[j]);
        }
        printf(" ]");

        bool match = (enc_len == tests[i].enc_len);
        if (match) {
            for (int j = 0; j < enc_len; j++) {
                if (buf[j] != tests[i].encoded[j]) {
                    match = false;
                    break;
                }
            }
        }

        /* Verify decode */
        uint32_t decoded;
        uint8_t consumed;
        xy_mqtt_err_t err = xy_mqtt_decode_remaining_length(buf, &decoded, &consumed);

        if (err == XY_MQTT_OK && decoded == tests[i].value && consumed == (uint8_t)enc_len) {
            printf(" -> DECODE OK -> %u: PASS", decoded);
        } else {
            printf(" -> DECODE FAIL: FAIL");
            match = false;
        }

        printf("\n");

        if (match) {
            passed++;
        } else {
            failed++;
        }
    }

    printf("\n----------------------------------------\n");
    printf("Results: %d passed, %d failed\n", passed, failed);
    printf("========================================\n\n");
}

/*============================================================================
 * Main Function
 *============================================================================*/

int main(int argc, char *argv[])
{
    printf("\n");
    printf("========================================\n");
    printf("XY MQTT Client Example\n");
    printf("========================================\n\n");

    /* Default configuration */
    char broker_host[256] = DEFAULT_BROKER_HOST;
    uint16_t broker_port = DEFAULT_BROKER_PORT;
    char client_id[24] = DEFAULT_CLIENT_ID;
    char username[65] = {0};
    char password[65] = {0};
    uint16_t keepalive = DEFAULT_KEEPALIVE;

    /* Parse command line arguments */
    if (parse_args(argc, argv, broker_host, &broker_port,
                   client_id, username, password, &keepalive) != 0) {
        return 1;
    }

    /* Print configuration */
    printf("Configuration:\n");
    printf("  Broker: %s:%d\n", broker_host, broker_port);
    printf("  Client ID: %s\n", client_id);
    if (username[0]) {
        printf("  Username: %s\n", username);
    }
    printf("  Keep-alive: %d seconds\n", keepalive);
    printf("\n");

    /* Run unit tests */
    test_remaining_length_encoding();
    test_topic_matching();

    /* Create socket context */
    mqtt_socket_context_t socket_ctx = {
        .host = broker_host,
        .port = broker_port,
        .socket = -1,
        .connected = false
    };

    /* Configure MQTT client */
    xy_mqtt_config_t config = {
        .keepalive = keepalive,
        .clean_session = 1,
        .qos_default = XY_MQTT_QOS_0,
        .tx_buffer_size = 1024,
        .rx_buffer_size = 1024,
        .transport_context = &socket_ctx,
        .send = mqtt_sim_send,
        .recv = mqtt_sim_recv,
        .connected_cb = on_connected,
        .disconnected_cb = on_disconnected,
        .message_cb = on_message,
        .published_cb = on_published,
        .subscribed_cb = on_subscribed,
        .unsubscribed_cb = on_unsubscribed,
        .user_data = NULL
    };

    /* Create MQTT client */
    printf("Creating MQTT client...\n");
    xy_mqtt_client_t *mqtt = xy_mqtt_client_new(&config);
    if (!mqtt) {
        printf("ERROR: Failed to create MQTT client\n");
        return 1;
    }
    printf("MQTT client created successfully\n\n");

    /* Connect to broker */
    printf("Connecting to MQTT broker...\n");
    xy_mqtt_err_t err;

    if (username[0] && password[0]) {
        err = xy_mqtt_connect(mqtt, client_id, username, password);
    } else {
        err = xy_mqtt_connect(mqtt, client_id, NULL, NULL);
    }

    if (err != XY_MQTT_OK) {
        printf("ERROR: Failed to send CONNECT packet (error %d)\n", err);
        xy_mqtt_client_delete(mqtt);
        return 1;
    }

    /* Note: In real implementation, we would wait for CONNACK here */
    /* For simulation, we just demonstrate the API */

    printf("\n");
    printf("========================================\n");
    printf("Simulated MQTT Session\n");
    printf("========================================\n\n");

    /* Simulate connected state for demonstration */
    printf("Simulating CONNACK received...\n");
    printf("       Session present: 0\n");
    printf("       Return code: Connection accepted (0)\n\n");

    /* Subscribe to topics */
    printf("Subscribing to topics...\n");

    uint16_t packet_id;
    err = xy_mqtt_subscribe(mqtt, "home/sensors/#", XY_MQTT_QOS_1, &packet_id);
    if (err == XY_MQTT_OK) {
        printf("  Subscribed to: home/sensors/# (QoS 1, packet_id: %d)\n", packet_id);
    } else {
        printf("  Failed to subscribe: error %d\n", err);
    }

    err = xy_mqtt_subscribe(mqtt, "devices/+/status", XY_MQTT_QOS_0, &packet_id);
    if (err == XY_MQTT_OK) {
        printf("  Subscribed to: devices/+/status (QoS 0, packet_id: %d)\n", packet_id);
    } else {
        printf("  Failed to subscribe: error %d\n", err);
    }

    err = xy_mqtt_subscribe(mqtt, "commands/#", XY_MQTT_QOS_1, &packet_id);
    if (err == XY_MQTT_OK) {
        printf("  Subscribed to: commands/# (QoS 1, packet_id: %d)\n", packet_id);
    } else {
        printf("  Failed to subscribe: error %d\n", err);
    }

    printf("\n");

    /* Publish messages */
    printf("Publishing messages...\n");

    const char *test_payload = "Hello, MQTT!";
    err = xy_mqtt_publish(mqtt, "home/sensors/temperature",
                          (const uint8_t *)test_payload, strlen(test_payload),
                          XY_MQTT_QOS_0, 0, NULL);
    if (err == XY_MQTT_OK) {
        printf("  Published to: home/sensors/temperature (QoS 0)\n");
    } else {
        printf("  Failed to publish: error %d\n", err);
    }

    const char *json_payload = "{\"temp\": 25.5, \"humidity\": 60}";
    err = xy_mqtt_publish(mqtt, "home/sensors/temperature",
                          (const uint8_t *)json_payload, strlen(json_payload),
                          XY_MQTT_QOS_1, 0, &packet_id);
    if (err == XY_MQTT_OK) {
        printf("  Published to: home/sensors/temperature (QoS 1, packet_id: %d)\n", packet_id);
    } else {
        printf("  Failed to publish: error %d\n", err);
    }

    printf("\n");

    /* Unsubscribe from a topic */
    printf("Unsubscribing from topic...\n");
    err = xy_mqtt_unsubscribe(mqtt, "devices/+/status", &packet_id);
    if (err == XY_MQTT_OK) {
        printf("  Unsubscribed from: devices/+/status (packet_id: %d)\n", packet_id);
    } else {
        printf("  Failed to unsubscribe: error %d\n", err);
    }

    printf("\n");

    /* Keep-alive check demonstration */
    printf("Keep-alive demonstration...\n");
    err = xy_mqtt_keepalive_check(mqtt);
    if (err == XY_MQTT_OK) {
        printf("  Keep-alive check: OK (no PING needed yet)\n");
    } else {
        printf("  Keep-alive check failed: error %d\n", err);
    }

    printf("\n");

    /* Disconnect */
    printf("Disconnecting...\n");
    err = xy_mqtt_disconnect(mqtt);
    if (err == XY_MQTT_OK) {
        printf("  Disconnected successfully\n");
    } else {
        printf("  Disconnect failed: error %d\n", err);
    }

    /* Clean up */
    printf("\nCleaning up...\n");
    xy_mqtt_client_delete(mqtt);
    printf("MQTT client deleted\n");

    printf("\n");
    printf("========================================\n");
    printf("Example completed successfully!\n");
    printf("========================================\n\n");

    return 0;
}
