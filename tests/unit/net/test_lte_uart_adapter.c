/**
 * @file test_lte_uart_adapter.c
 * @brief Unit tests for the callback-backed LTE UART transport adapter.
 */

#include "unity.h"

#include "xy_lte_uart_adapter.h"

#include <limits.h>
#include <string.h>

#define UART_TX_MAX 32U
#define UART_RX_MAX 32U

typedef struct {
    uint8_t tx[UART_TX_MAX];
    size_t tx_len;
    uint8_t rx[UART_RX_MAX];
    size_t rx_len;
    uint32_t last_write_timeout;
    uint32_t last_read_timeout;
    int write_result;
    int read_result;
    int flush_result;
    unsigned write_calls;
    unsigned read_calls;
    unsigned flush_calls;
} fake_uart_t;

static fake_uart_t g_uart;

static int fake_uart_write(void *uart, const uint8_t *data, size_t len, uint32_t timeout_ms)
{
    fake_uart_t *fake = (fake_uart_t *)uart;
    size_t copy_len = len;

    TEST_ASSERT_NOT_NULL(fake);
    TEST_ASSERT_NOT_NULL(data);
    if (copy_len > sizeof(fake->tx)) {
        copy_len = sizeof(fake->tx);
    }
    memcpy(fake->tx, data, copy_len);
    fake->tx_len = copy_len;
    fake->last_write_timeout = timeout_ms;
    fake->write_calls++;
    return fake->write_result;
}

static int fake_uart_read(void *uart, uint8_t *data, size_t len, uint32_t timeout_ms)
{
    fake_uart_t *fake = (fake_uart_t *)uart;
    size_t copy_len = fake->rx_len;

    TEST_ASSERT_NOT_NULL(fake);
    TEST_ASSERT_NOT_NULL(data);
    if (copy_len > len) {
        copy_len = len;
    }
    memcpy(data, fake->rx, copy_len);
    fake->last_read_timeout = timeout_ms;
    fake->read_calls++;
    return fake->read_result == INT_MIN ? (int)copy_len : fake->read_result;
}

static int fake_uart_flush(void *uart)
{
    fake_uart_t *fake = (fake_uart_t *)uart;

    TEST_ASSERT_NOT_NULL(fake);
    fake->flush_calls++;
    return fake->flush_result;
}

void setUp(void)
{
    memset(&g_uart, 0, sizeof(g_uart));
    g_uart.write_result = XY_LTE_OK;
    g_uart.read_result = INT_MIN;
    g_uart.flush_result = XY_LTE_OK;
}

void tearDown(void)
{
}

static void test_lte_uart_adapter_init_rejects_missing_required_inputs(void)
{
    xy_lte_uart_adapter_t adapter;
    uint8_t rx_buffer[8];
    int uart_token = 1;

    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM,
                      xy_lte_uart_adapter_init(NULL, &uart_token, rx_buffer, sizeof(rx_buffer),
                                               1000, fake_uart_write, fake_uart_read, NULL));
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM,
                      xy_lte_uart_adapter_init(&adapter, NULL, rx_buffer, sizeof(rx_buffer), 1000,
                                               fake_uart_write, fake_uart_read, NULL));
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM,
                      xy_lte_uart_adapter_init(&adapter, &uart_token, NULL, sizeof(rx_buffer),
                                               1000, fake_uart_write, fake_uart_read, NULL));
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM,
                      xy_lte_uart_adapter_init(&adapter, &uart_token, rx_buffer, 0, 1000,
                                               fake_uart_write, fake_uart_read, NULL));
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM,
                      xy_lte_uart_adapter_init(&adapter, &uart_token, rx_buffer, sizeof(rx_buffer),
                                               1000, NULL, fake_uart_read, NULL));
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM,
                      xy_lte_uart_adapter_init(&adapter, &uart_token, rx_buffer, sizeof(rx_buffer),
                                               1000, fake_uart_write, NULL, NULL));
}

static void test_lte_uart_adapter_exports_transport_context_and_callbacks(void)
{
    xy_lte_uart_adapter_t adapter;
    xy_lte_transport_t transport;
    uint8_t rx_buffer[8];

    TEST_ASSERT_EQUAL(XY_LTE_OK,
                      xy_lte_uart_adapter_init(&adapter, &g_uart, rx_buffer, sizeof(rx_buffer),
                                               2500, fake_uart_write, fake_uart_read,
                                               fake_uart_flush));
    TEST_ASSERT_EQUAL_PTR(&g_uart, adapter.uart);
    TEST_ASSERT_EQUAL_PTR(rx_buffer, adapter.rx_buffer);
    TEST_ASSERT_EQUAL_UINT(sizeof(rx_buffer), adapter.rx_buffer_len);

    memset(&transport, 0, sizeof(transport));
    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_uart_adapter_get_transport(&adapter, &transport));
    TEST_ASSERT_EQUAL_PTR(&adapter, transport.context);
    TEST_ASSERT_NOT_NULL(transport.write);
    TEST_ASSERT_NOT_NULL(transport.read);
    TEST_ASSERT_NOT_NULL(transport.flush);

    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, xy_lte_uart_adapter_get_transport(NULL, &transport));
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, xy_lte_uart_adapter_get_transport(&adapter, NULL));
}

static void test_lte_uart_adapter_transport_forwards_bytes_and_timeouts(void)
{
    xy_lte_uart_adapter_t adapter;
    xy_lte_transport_t transport;
    uint8_t rx_buffer[8];
    uint8_t read_buffer[8] = {0};
    const uint8_t payload[] = {'A', 'T', '\r'};

    memcpy(g_uart.rx, "OK", 2U);
    g_uart.rx_len = 2U;

    TEST_ASSERT_EQUAL(XY_LTE_OK,
                      xy_lte_uart_adapter_init(&adapter, &g_uart, rx_buffer, sizeof(rx_buffer),
                                               1234, fake_uart_write, fake_uart_read,
                                               fake_uart_flush));
    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_uart_adapter_get_transport(&adapter, &transport));

    TEST_ASSERT_EQUAL(XY_LTE_OK, transport.write(transport.context, payload, sizeof(payload), 99));
    TEST_ASSERT_EQUAL_UINT(sizeof(payload), g_uart.tx_len);
    TEST_ASSERT_EQUAL_MEMORY(payload, g_uart.tx, sizeof(payload));
    TEST_ASSERT_EQUAL_UINT32(99U, g_uart.last_write_timeout);

    TEST_ASSERT_EQUAL_INT(2, transport.read(transport.context, read_buffer, sizeof(read_buffer), 77));
    TEST_ASSERT_EQUAL_UINT8('O', read_buffer[0]);
    TEST_ASSERT_EQUAL_UINT8('K', read_buffer[1]);
    TEST_ASSERT_EQUAL_UINT32(77U, g_uart.last_read_timeout);

    TEST_ASSERT_EQUAL(XY_LTE_OK, transport.flush(transport.context));
    TEST_ASSERT_EQUAL_UINT(1U, g_uart.flush_calls);
}

static void test_lte_uart_adapter_uses_default_timeout_for_zero_timeout(void)
{
    xy_lte_uart_adapter_t adapter;
    xy_lte_transport_t transport;
    uint8_t rx_buffer[8];
    uint8_t byte = 0x5A;

    g_uart.rx[0] = 0xA5;
    g_uart.rx_len = 1U;

    TEST_ASSERT_EQUAL(XY_LTE_OK,
                      xy_lte_uart_adapter_init(&adapter, &g_uart, rx_buffer, sizeof(rx_buffer),
                                               4321, fake_uart_write, fake_uart_read, NULL));
    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_uart_adapter_get_transport(&adapter, &transport));

    TEST_ASSERT_EQUAL(XY_LTE_OK, transport.write(transport.context, &byte, 1U, 0));
    TEST_ASSERT_EQUAL_UINT32(4321U, g_uart.last_write_timeout);
    TEST_ASSERT_EQUAL_INT(1, transport.read(transport.context, &byte, 1U, 0));
    TEST_ASSERT_EQUAL_UINT32(4321U, g_uart.last_read_timeout);
    TEST_ASSERT_EQUAL_UINT8(0xA5, byte);
}

static void test_lte_uart_adapter_propagates_backend_errors(void)
{
    xy_lte_uart_adapter_t adapter;
    xy_lte_transport_t transport;
    uint8_t rx_buffer[8];
    uint8_t byte = 0x5A;

    TEST_ASSERT_EQUAL(XY_LTE_OK,
                      xy_lte_uart_adapter_init(&adapter, &g_uart, rx_buffer, sizeof(rx_buffer),
                                               1000, fake_uart_write, fake_uart_read,
                                               fake_uart_flush));
    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_uart_adapter_get_transport(&adapter, &transport));

    g_uart.write_result = XY_LTE_TIMEOUT;
    TEST_ASSERT_EQUAL(XY_LTE_TIMEOUT, transport.write(transport.context, &byte, 1U, 10));

    g_uart.read_result = XY_LTE_ERROR;
    TEST_ASSERT_EQUAL(XY_LTE_ERROR, transport.read(transport.context, &byte, 1U, 10));

    g_uart.flush_result = XY_LTE_ERROR;
    TEST_ASSERT_EQUAL(XY_LTE_ERROR, transport.flush(transport.context));
}

static void test_lte_uart_adapter_rejects_bad_transport_arguments_and_allows_missing_flush(void)
{
    xy_lte_uart_adapter_t adapter;
    xy_lte_transport_t transport;
    uint8_t rx_buffer[8];
    uint8_t byte = 0x5A;

    TEST_ASSERT_EQUAL(XY_LTE_OK,
                      xy_lte_uart_adapter_init(&adapter, &g_uart, rx_buffer, sizeof(rx_buffer),
                                               1000, fake_uart_write, fake_uart_read, NULL));
    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_uart_adapter_get_transport(&adapter, &transport));

    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, transport.write(NULL, &byte, 1U, 10));
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, transport.write(transport.context, NULL, 1U, 10));
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, transport.write(transport.context, &byte, 0U, 10));
    TEST_ASSERT_EQUAL_UINT(0U, g_uart.write_calls);

    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, transport.read(NULL, &byte, 1U, 10));
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, transport.read(transport.context, NULL, 1U, 10));
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, transport.read(transport.context, &byte, 0U, 10));
    TEST_ASSERT_EQUAL_UINT(0U, g_uart.read_calls);

    TEST_ASSERT_EQUAL(XY_LTE_OK, transport.flush(transport.context));
    TEST_ASSERT_EQUAL_UINT(0U, g_uart.flush_calls);
}

static void test_lte_uart_adapter_get_transport_rejects_corrupted_adapter_state(void)
{
    xy_lte_uart_adapter_t adapter;
    xy_lte_transport_t transport;
    uint8_t rx_buffer[8];

    TEST_ASSERT_EQUAL(XY_LTE_OK,
                      xy_lte_uart_adapter_init(&adapter, &g_uart, rx_buffer, sizeof(rx_buffer),
                                               1000, fake_uart_write, fake_uart_read, NULL));

    adapter.uart = NULL;
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM,
                      xy_lte_uart_adapter_get_transport(&adapter, &transport));

    adapter.uart = &g_uart;
    adapter.write = NULL;
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM,
                      xy_lte_uart_adapter_get_transport(&adapter, &transport));

    adapter.write = fake_uart_write;
    adapter.read = NULL;
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM,
                      xy_lte_uart_adapter_get_transport(&adapter, &transport));
}

static void test_lte_core_can_bind_uart_adapter_for_at_commands(void)
{
    xy_lte_uart_adapter_t adapter;
    xy_lte_transport_t transport;
    xy_lte_t lte;
    uint8_t rx_buffer[16];
    char response[16] = "sentinel";
    int uart_token = 1;

    memcpy(g_uart.rx, "OK", 2U);
    g_uart.rx_len = 2U;

    TEST_ASSERT_EQUAL(XY_LTE_OK,
                      xy_lte_uart_adapter_init(&adapter, &g_uart, rx_buffer, sizeof(rx_buffer),
                                               2468, fake_uart_write, fake_uart_read,
                                               fake_uart_flush));
    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_uart_adapter_get_transport(&adapter, &transport));

    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_init(&lte, &uart_token, 115200));
    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_bind_transport(&lte, &transport));

    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_check(&lte));
    TEST_ASSERT_EQUAL_UINT(1U, g_uart.write_calls);
    TEST_ASSERT_EQUAL_UINT(1U, g_uart.read_calls);
    TEST_ASSERT_EQUAL_UINT(2U, g_uart.tx_len);
    TEST_ASSERT_EQUAL_MEMORY("AT", g_uart.tx, 2U);
    TEST_ASSERT_EQUAL_UINT32(1000U, g_uart.last_write_timeout);
    TEST_ASSERT_EQUAL_UINT32(1000U, g_uart.last_read_timeout);

    g_uart.write_calls = 0U;
    g_uart.read_calls = 0U;
    memcpy(g_uart.rx, "READY", 5U);
    g_uart.rx_len = 5U;

    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_send_at(&lte, "AT+PING", response, sizeof(response), 0));
    TEST_ASSERT_EQUAL_STRING("AT+PING", (const char *)g_uart.tx);
    TEST_ASSERT_EQUAL_STRING("READY", response);
    TEST_ASSERT_EQUAL_UINT32(2468U, g_uart.last_write_timeout);
    TEST_ASSERT_EQUAL_UINT32(2468U, g_uart.last_read_timeout);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_lte_uart_adapter_init_rejects_missing_required_inputs);
    RUN_TEST(test_lte_uart_adapter_exports_transport_context_and_callbacks);
    RUN_TEST(test_lte_uart_adapter_transport_forwards_bytes_and_timeouts);
    RUN_TEST(test_lte_uart_adapter_uses_default_timeout_for_zero_timeout);
    RUN_TEST(test_lte_uart_adapter_propagates_backend_errors);
    RUN_TEST(test_lte_uart_adapter_rejects_bad_transport_arguments_and_allows_missing_flush);
    RUN_TEST(test_lte_uart_adapter_get_transport_rejects_corrupted_adapter_state);
    RUN_TEST(test_lte_core_can_bind_uart_adapter_for_at_commands);
    return UNITY_END();
}
