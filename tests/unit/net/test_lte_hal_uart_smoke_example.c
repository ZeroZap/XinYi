/**
 * @file test_lte_hal_uart_smoke_example.c
 * @brief Build-guarded smoke skeleton for binding LTE core to the HAL UART adapter.
 */

#include "unity.h"

#include "xy_lte.h"
#include "xy_lte_hal_uart_adapter.h"
#include "xy_hal_uart.h"

#include <string.h>

#define SMOKE_RX_MAX 64U
#define SMOKE_TX_MAX 64U

typedef struct {
    uint8_t tx[SMOKE_TX_MAX];
    size_t tx_len;
    uint8_t rx[SMOKE_RX_MAX];
    size_t rx_len;
    xy_hal_error_t send_result;
    xy_hal_error_t recv_result;
    xy_hal_error_t flush_result;
    uint32_t send_timeout;
    uint32_t recv_timeout;
    unsigned send_calls;
    unsigned recv_calls;
} smoke_uart_t;

static smoke_uart_t g_uart;

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
    smoke_uart_t *fake = (smoke_uart_t *)uart;
    size_t copy_len = len;

    TEST_ASSERT_NOT_NULL(fake);
    TEST_ASSERT_NOT_NULL(data);
    if (copy_len > sizeof(fake->tx)) {
        copy_len = sizeof(fake->tx);
    }
    memcpy(fake->tx, data, copy_len);
    fake->tx_len = copy_len;
    fake->send_timeout = timeout;
    fake->send_calls++;
    return fake->send_result;
}

xy_hal_error_t xy_hal_uart_recv(void *uart, uint8_t *data, size_t len, uint32_t timeout)
{
    smoke_uart_t *fake = (smoke_uart_t *)uart;
    size_t copy_len;

    TEST_ASSERT_NOT_NULL(fake);
    TEST_ASSERT_NOT_NULL(data);
    copy_len = fake->rx_len;
    if (copy_len > len) {
        copy_len = len;
    }
    memcpy(data, fake->rx, copy_len);
    fake->recv_timeout = timeout;
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
    (void)uart;
    return g_uart.flush_result;
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

static void test_smoke_binds_hal_uart_adapter_and_checks_at_response(void)
{
    xy_lte_t lte;
    xy_lte_hal_uart_adapter_t adapter;
    xy_lte_transport_t transport;
    uint8_t rx_storage[32];
    int uart_token = 1;

    memcpy(g_uart.rx, "\r\nOK\r\n", 6U);
    g_uart.rx_len = 6U;

    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_init(&lte, &uart_token, 115200));
    TEST_ASSERT_EQUAL(XY_LTE_OK,
                      xy_lte_hal_uart_adapter_init(&adapter, &g_uart, rx_storage,
                                                   sizeof(rx_storage), 2000));
    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_hal_uart_adapter_get_transport(&adapter, &transport));
    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_bind_transport(&lte, &transport));

    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_check(&lte));
    TEST_ASSERT_EQUAL_UINT(1U, g_uart.send_calls);
    TEST_ASSERT_EQUAL_MEMORY("AT", g_uart.tx, 2U);
    TEST_ASSERT_EQUAL_UINT(2U, g_uart.tx_len);
    TEST_ASSERT_EQUAL_UINT32(1000U, g_uart.send_timeout);
    TEST_ASSERT_EQUAL_UINT(1U, g_uart.recv_calls);
    TEST_ASSERT_EQUAL_UINT32(1000U, g_uart.recv_timeout);
}

static void test_smoke_records_modem_absent_timeout_as_blocking_result(void)
{
    xy_lte_t lte;
    xy_lte_hal_uart_adapter_t adapter;
    xy_lte_transport_t transport;
    uint8_t rx_storage[32];
    int uart_token = 1;

    g_uart.recv_result = XY_HAL_ERROR_TIMEOUT;

    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_init(&lte, &uart_token, 115200));
    TEST_ASSERT_EQUAL(XY_LTE_OK,
                      xy_lte_hal_uart_adapter_init(&adapter, &g_uart, rx_storage,
                                                   sizeof(rx_storage), 2000));
    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_hal_uart_adapter_get_transport(&adapter, &transport));
    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_bind_transport(&lte, &transport));

    TEST_ASSERT_EQUAL(XY_LTE_TIMEOUT, xy_lte_check(&lte));
    TEST_ASSERT_EQUAL_UINT(1U, g_uart.send_calls);
    TEST_ASSERT_EQUAL_MEMORY("AT", g_uart.tx, 2U);
    TEST_ASSERT_EQUAL_UINT(1U, g_uart.recv_calls);
}

static void test_smoke_keeps_lte_default_off_policy_documented(void)
{
#ifdef XY_NET_ENABLE_LTE
    TEST_ASSERT_EQUAL_UINT(0U, XY_NET_ENABLE_LTE);
#else
    TEST_PASS_MESSAGE("XY_NET_ENABLE_LTE is not defined by default; LTE remains direct-opt-in.");
#endif
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_smoke_binds_hal_uart_adapter_and_checks_at_response);
    RUN_TEST(test_smoke_records_modem_absent_timeout_as_blocking_result);
    RUN_TEST(test_smoke_keeps_lte_default_off_policy_documented);
    return UNITY_END();
}
