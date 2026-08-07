/**
 * @file test_lte_hal_uart_adapter.c
 * @brief Unit tests for the HAL UART-backed LTE transport adapter.
 */

#include "unity.h"

#include "xy_lte_hal_uart_adapter.h"

#include "xy_hal_uart.h"

#include <string.h>

#define HAL_UART_TX_MAX 32U
#define HAL_UART_RX_MAX 32U

typedef struct {
    uint8_t tx[HAL_UART_TX_MAX];
    size_t tx_len;
    uint8_t rx[HAL_UART_RX_MAX];
    size_t rx_len;
    uint32_t last_send_timeout;
    uint32_t last_recv_timeout;
    xy_hal_error_t send_result;
    xy_hal_error_t recv_result;
    xy_hal_error_t flush_result;
    unsigned send_calls;
    unsigned recv_calls;
    unsigned flush_calls;
} fake_hal_uart_t;

static fake_hal_uart_t g_uart;

xy_hal_error_t xy_hal_uart_init(void *uart, const xy_hal_uart_config_t *config)
{
    (void)uart;
    (void)config;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_deinit(void *uart)
{
    (void)uart;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_send(void *uart, const uint8_t *data, size_t len, uint32_t timeout)
{
    fake_hal_uart_t *fake = (fake_hal_uart_t *)uart;
    size_t copy_len = len;

    TEST_ASSERT_NOT_NULL(fake);
    TEST_ASSERT_NOT_NULL(data);
    if (copy_len > sizeof(fake->tx)) {
        copy_len = sizeof(fake->tx);
    }
    memcpy(fake->tx, data, copy_len);
    fake->tx_len = copy_len;
    fake->last_send_timeout = timeout;
    fake->send_calls++;
    return fake->send_result;
}

xy_hal_error_t xy_hal_uart_recv(void *uart, uint8_t *data, size_t len, uint32_t timeout)
{
    fake_hal_uart_t *fake = (fake_hal_uart_t *)uart;
    size_t copy_len;

    TEST_ASSERT_NOT_NULL(fake);
    TEST_ASSERT_NOT_NULL(data);
    copy_len = fake->rx_len;
    if (copy_len > len) {
        copy_len = len;
    }
    memcpy(data, fake->rx, copy_len);
    fake->last_recv_timeout = timeout;
    fake->recv_calls++;
    return fake->recv_result;
}

xy_hal_error_t xy_hal_uart_send_dma(void *uart, const uint8_t *data, size_t len)
{
    (void)uart;
    (void)data;
    (void)len;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_recv_dma(void *uart, uint8_t *data, size_t len)
{
    (void)uart;
    (void)data;
    (void)len;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_register_callback(void *uart, xy_hal_uart_callback_t callback,
                                             void *arg)
{
    (void)uart;
    (void)callback;
    (void)arg;
    return XY_HAL_OK;
}

int xy_hal_uart_available(void *uart)
{
    (void)uart;
    return 0;
}

xy_hal_error_t xy_hal_uart_flush(void *uart)
{
    fake_hal_uart_t *fake = (fake_hal_uart_t *)uart;

    TEST_ASSERT_NOT_NULL(fake);
    fake->flush_calls++;
    return fake->flush_result;
}

xy_hal_error_t xy_hal_uart_error(void *uart)
{
    (void)uart;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_set_error_cb(void *uart, xy_hal_uart_callback_t callback, void *arg)
{
    (void)uart;
    (void)callback;
    (void)arg;
    return XY_HAL_OK;
}

void setUp(void)
{
    memset(&g_uart, 0, sizeof(g_uart));
    g_uart.send_result = XY_HAL_OK;
    g_uart.recv_result = XY_HAL_OK;
    g_uart.flush_result = XY_HAL_OK;
}

void tearDown(void)
{
}

static void test_lte_hal_uart_adapter_init_rejects_missing_required_inputs(void)
{
    xy_lte_hal_uart_adapter_t adapter;
    uint8_t rx_buffer[8];
    int uart_token = 1;

    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM,
                      xy_lte_hal_uart_adapter_init(NULL, &uart_token, rx_buffer,
                                                   sizeof(rx_buffer), 1000));
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM,
                      xy_lte_hal_uart_adapter_init(&adapter, NULL, rx_buffer,
                                                   sizeof(rx_buffer), 1000));
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM,
                      xy_lte_hal_uart_adapter_init(&adapter, &uart_token, NULL,
                                                   sizeof(rx_buffer), 1000));
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM,
                      xy_lte_hal_uart_adapter_init(&adapter, &uart_token, rx_buffer, 0,
                                                   1000));
}

static void test_lte_hal_uart_adapter_exports_transport_context_and_callbacks(void)
{
    xy_lte_hal_uart_adapter_t adapter;
    xy_lte_transport_t transport;
    uint8_t rx_buffer[8];

    TEST_ASSERT_EQUAL(XY_LTE_OK,
                      xy_lte_hal_uart_adapter_init(&adapter, &g_uart, rx_buffer,
                                                   sizeof(rx_buffer), 2500));
    TEST_ASSERT_EQUAL_PTR(&g_uart, adapter.uart);
    TEST_ASSERT_EQUAL_PTR(rx_buffer, adapter.rx_buffer);
    TEST_ASSERT_EQUAL_UINT(sizeof(rx_buffer), adapter.rx_buffer_len);

    memset(&transport, 0, sizeof(transport));
    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_hal_uart_adapter_get_transport(&adapter, &transport));
    TEST_ASSERT_EQUAL_PTR(&adapter, transport.context);
    TEST_ASSERT_NOT_NULL(transport.write);
    TEST_ASSERT_NOT_NULL(transport.read);
    TEST_ASSERT_NOT_NULL(transport.flush);

    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM,
                      xy_lte_hal_uart_adapter_get_transport(NULL, &transport));
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM,
                      xy_lte_hal_uart_adapter_get_transport(&adapter, NULL));
}

static void test_lte_hal_uart_adapter_forwards_bytes_and_timeouts(void)
{
    xy_lte_hal_uart_adapter_t adapter;
    xy_lte_transport_t transport;
    uint8_t rx_storage[8];
    uint8_t read_buffer[8] = {0xCC, 0xCC, 0xCC, 0xCC};
    const uint8_t payload[] = {'A', 'T', '\r'};

    memcpy(g_uart.rx, "OK", 2U);
    g_uart.rx_len = 2U;

    TEST_ASSERT_EQUAL(XY_LTE_OK,
                      xy_lte_hal_uart_adapter_init(&adapter, &g_uart, rx_storage,
                                                   sizeof(rx_storage), 1234));
    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_hal_uart_adapter_get_transport(&adapter, &transport));

    TEST_ASSERT_EQUAL(XY_LTE_OK, transport.write(transport.context, payload, sizeof(payload), 99));
    TEST_ASSERT_EQUAL_UINT(sizeof(payload), g_uart.tx_len);
    TEST_ASSERT_EQUAL_MEMORY(payload, g_uart.tx, sizeof(payload));
    TEST_ASSERT_EQUAL_UINT32(99U, g_uart.last_send_timeout);

    TEST_ASSERT_EQUAL_INT(2, transport.read(transport.context, read_buffer, 2U, 77));
    TEST_ASSERT_EQUAL_UINT8('O', read_buffer[0]);
    TEST_ASSERT_EQUAL_UINT8('K', read_buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0xCC, read_buffer[2]);
    TEST_ASSERT_EQUAL_UINT32(77U, g_uart.last_recv_timeout);

    TEST_ASSERT_EQUAL(XY_LTE_OK, transport.flush(transport.context));
    TEST_ASSERT_EQUAL_UINT(1U, g_uart.flush_calls);
}

static void test_lte_hal_uart_adapter_uses_default_timeout_and_clamps_read_to_rx_buffer(void)
{
    xy_lte_hal_uart_adapter_t adapter;
    xy_lte_transport_t transport;
    uint8_t rx_storage[3];
    uint8_t read_buffer[8] = {0};
    uint8_t byte = 0x5A;

    memcpy(g_uart.rx, "ABCDE", 5U);
    g_uart.rx_len = 5U;

    TEST_ASSERT_EQUAL(XY_LTE_OK,
                      xy_lte_hal_uart_adapter_init(&adapter, &g_uart, rx_storage,
                                                   sizeof(rx_storage), 4321));
    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_hal_uart_adapter_get_transport(&adapter, &transport));

    TEST_ASSERT_EQUAL(XY_LTE_OK, transport.write(transport.context, &byte, 1U, 0));
    TEST_ASSERT_EQUAL_UINT32(4321U, g_uart.last_send_timeout);

    TEST_ASSERT_EQUAL_INT(3, transport.read(transport.context, read_buffer, sizeof(read_buffer), 0));
    TEST_ASSERT_EQUAL_MEMORY("ABC", read_buffer, 3U);
    TEST_ASSERT_EQUAL_UINT32(4321U, g_uart.last_recv_timeout);
}

static void test_lte_hal_uart_adapter_normalizes_hal_errors(void)
{
    xy_lte_hal_uart_adapter_t adapter;
    xy_lte_transport_t transport;
    uint8_t rx_storage[8];
    uint8_t byte = 0x5A;

    TEST_ASSERT_EQUAL(XY_LTE_OK,
                      xy_lte_hal_uart_adapter_init(&adapter, &g_uart, rx_storage,
                                                   sizeof(rx_storage), 1000));
    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_hal_uart_adapter_get_transport(&adapter, &transport));

    g_uart.send_result = XY_HAL_ERROR_TIMEOUT;
    TEST_ASSERT_EQUAL(XY_LTE_TIMEOUT, transport.write(transport.context, &byte, 1U, 10));

    g_uart.recv_result = XY_HAL_ERROR_INVALID_PARAM;
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, transport.read(transport.context, &byte, 1U, 10));

    g_uart.flush_result = XY_HAL_ERROR_BUSY;
    TEST_ASSERT_EQUAL(XY_LTE_ERROR, transport.flush(transport.context));
}

static void test_lte_hal_uart_adapter_rejects_bad_transport_arguments_without_hal_side_effects(void)
{
    xy_lte_hal_uart_adapter_t adapter;
    xy_lte_transport_t transport;
    uint8_t rx_storage[8];
    uint8_t byte = 0x5A;

    TEST_ASSERT_EQUAL(XY_LTE_OK,
                      xy_lte_hal_uart_adapter_init(&adapter, &g_uart, rx_storage,
                                                   sizeof(rx_storage), 1000));
    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_hal_uart_adapter_get_transport(&adapter, &transport));

    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, transport.write(NULL, &byte, 1U, 10));
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, transport.write(transport.context, NULL, 1U, 10));
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, transport.write(transport.context, &byte, 0U, 10));
    TEST_ASSERT_EQUAL_UINT(0U, g_uart.send_calls);

    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, transport.read(NULL, &byte, 1U, 10));
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, transport.read(transport.context, NULL, 1U, 10));
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, transport.read(transport.context, &byte, 0U, 10));
    TEST_ASSERT_EQUAL_UINT(0U, g_uart.recv_calls);
}

static void test_lte_hal_uart_adapter_get_transport_rejects_corrupted_adapter_state(void)
{
    xy_lte_hal_uart_adapter_t adapter;
    xy_lte_transport_t transport;
    uint8_t rx_storage[8];

    TEST_ASSERT_EQUAL(XY_LTE_OK,
                      xy_lte_hal_uart_adapter_init(&adapter, &g_uart, rx_storage,
                                                   sizeof(rx_storage), 1000));

    adapter.uart = NULL;
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM,
                      xy_lte_hal_uart_adapter_get_transport(&adapter, &transport));

    adapter.uart = &g_uart;
    adapter.rx_buffer = NULL;
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM,
                      xy_lte_hal_uart_adapter_get_transport(&adapter, &transport));

    adapter.rx_buffer = rx_storage;
    adapter.rx_buffer_len = 0U;
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM,
                      xy_lte_hal_uart_adapter_get_transport(&adapter, &transport));
}

static void test_lte_hal_uart_adapter_write_and_flush_do_not_require_rx_storage(void)
{
    xy_lte_hal_uart_adapter_t adapter;
    xy_lte_transport_t transport;
    uint8_t rx_storage[8];
    uint8_t byte = 0xA5;

    TEST_ASSERT_EQUAL(XY_LTE_OK,
                      xy_lte_hal_uart_adapter_init(&adapter, &g_uart, rx_storage,
                                                   sizeof(rx_storage), 1000));
    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_hal_uart_adapter_get_transport(&adapter, &transport));

    adapter.rx_buffer = NULL;
    adapter.rx_buffer_len = 0U;

    TEST_ASSERT_EQUAL(XY_LTE_OK, transport.write(transport.context, &byte, 1U, 10));
    TEST_ASSERT_EQUAL_UINT(1U, g_uart.send_calls);
    TEST_ASSERT_EQUAL_UINT8(byte, g_uart.tx[0]);

    TEST_ASSERT_EQUAL(XY_LTE_OK, transport.flush(transport.context));
    TEST_ASSERT_EQUAL_UINT(1U, g_uart.flush_calls);

    byte = 0xCC;
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, transport.read(transport.context, &byte, 1U, 10));
    TEST_ASSERT_EQUAL_UINT8(0xCC, byte);
    TEST_ASSERT_EQUAL_UINT(0U, g_uart.recv_calls);
}

static void test_lte_core_can_bind_hal_uart_adapter_for_at_commands(void)
{
    xy_lte_hal_uart_adapter_t adapter;
    xy_lte_transport_t transport;
    xy_lte_t lte;
    uint8_t rx_storage[16];
    int uart_token = 1;

    memcpy(g_uart.rx, "OK", 2U);
    g_uart.rx_len = 2U;

    TEST_ASSERT_EQUAL(XY_LTE_OK,
                      xy_lte_hal_uart_adapter_init(&adapter, &g_uart, rx_storage,
                                                   sizeof(rx_storage), 2468));
    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_hal_uart_adapter_get_transport(&adapter, &transport));

    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_init(&lte, &uart_token, 115200));
    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_bind_transport(&lte, &transport));

    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_check(&lte));
    TEST_ASSERT_EQUAL_UINT(1U, g_uart.send_calls);
    TEST_ASSERT_EQUAL_UINT(1U, g_uart.recv_calls);
    TEST_ASSERT_EQUAL_UINT(2U, g_uart.tx_len);
    TEST_ASSERT_EQUAL_MEMORY("AT", g_uart.tx, 2U);
    TEST_ASSERT_EQUAL_UINT32(1000U, g_uart.last_send_timeout);
    TEST_ASSERT_EQUAL_UINT32(1000U, g_uart.last_recv_timeout);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_lte_hal_uart_adapter_init_rejects_missing_required_inputs);
    RUN_TEST(test_lte_hal_uart_adapter_exports_transport_context_and_callbacks);
    RUN_TEST(test_lte_hal_uart_adapter_forwards_bytes_and_timeouts);
    RUN_TEST(test_lte_hal_uart_adapter_uses_default_timeout_and_clamps_read_to_rx_buffer);
    RUN_TEST(test_lte_hal_uart_adapter_normalizes_hal_errors);
    RUN_TEST(test_lte_hal_uart_adapter_rejects_bad_transport_arguments_without_hal_side_effects);
    RUN_TEST(test_lte_hal_uart_adapter_get_transport_rejects_corrupted_adapter_state);
    RUN_TEST(test_lte_hal_uart_adapter_write_and_flush_do_not_require_rx_storage);
    RUN_TEST(test_lte_core_can_bind_hal_uart_adapter_for_at_commands);
    return UNITY_END();
}
