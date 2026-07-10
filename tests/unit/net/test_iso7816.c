/**
 * @file test_iso7816.c
 * @brief Unit tests for ISO7816 utility/lifecycle/APDU behavior.
 */
#include "unity.h"
#include "fff.h"

#include "xy_iso7816.h"
#include "xy_hal_error.h"

#include <stdbool.h>
#include <string.h>

static unsigned char g_uart;
static uint8_t g_tx[512];
static size_t g_tx_len;
static uint8_t g_rx[512];
static size_t g_rx_len;
static size_t g_rx_pos;

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(xy_hal_error_t, xy_hal_uart_send, void *, const uint8_t *, size_t, uint32_t)
FAKE_VALUE_FUNC(xy_hal_error_t, xy_hal_uart_recv, void *, uint8_t *, size_t, uint32_t)
FAKE_VALUE_FUNC(xy_hal_error_t, xy_hal_uart_flush, void *)

static xy_hal_error_t xy_hal_uart_send_impl(void *uart, const uint8_t *data, size_t len,
                                            uint32_t timeout)
{
    TEST_ASSERT_EQUAL_PTR(&g_uart, uart);
    TEST_ASSERT_LESS_OR_EQUAL_size_t(sizeof(g_tx), g_tx_len + len);
    memcpy(&g_tx[g_tx_len], data, len);
    g_tx_len += len;
    return XY_HAL_OK;
}

static xy_hal_error_t xy_hal_uart_recv_impl(void *uart, uint8_t *data, size_t len,
                                            uint32_t timeout)
{
    (void)timeout;
    TEST_ASSERT_EQUAL_PTR(&g_uart, uart);
    if (g_rx_pos + len > g_rx_len) {
        return XY_HAL_ERROR_TIMEOUT;
    }
    memcpy(data, &g_rx[g_rx_pos], len);
    g_rx_pos += len;
    return XY_HAL_OK;
}

static xy_hal_error_t xy_hal_uart_flush_impl(void *uart)
{
    TEST_ASSERT_EQUAL_PTR(&g_uart, uart);
    return XY_HAL_OK;
}

static void reset_uart_fixture(void)
{
    RESET_FAKE(xy_hal_uart_send);
    RESET_FAKE(xy_hal_uart_recv);
    RESET_FAKE(xy_hal_uart_flush);
    FFF_RESET_HISTORY();

    xy_hal_uart_send_fake.custom_fake = xy_hal_uart_send_impl;
    xy_hal_uart_recv_fake.custom_fake = xy_hal_uart_recv_impl;
    xy_hal_uart_flush_fake.custom_fake = xy_hal_uart_flush_impl;

    memset(g_tx, 0, sizeof(g_tx));
    memset(g_rx, 0, sizeof(g_rx));
    g_tx_len = 0;
    g_rx_len = 0;
    g_rx_pos = 0;
}

void setUp(void)
{
    reset_uart_fixture();
}

void tearDown(void)
{
}

static void push_rx(const uint8_t *data, size_t len)
{
    TEST_ASSERT_LESS_OR_EQUAL_size_t(sizeof(g_rx), g_rx_len + len);
    memcpy(&g_rx[g_rx_len], data, len);
    g_rx_len += len;
}

static void test_lifecycle_and_utilities(void)
{
    xy_iso7816_handle_t handle;
    xy_iso7816_apdu_resp_t ok = {.sw1 = 0x90, .sw2 = 0x00};
    xy_iso7816_apdu_resp_t fail = {.sw1 = 0x6A, .sw2 = 0x82};
    char ascii[16];
    const xy_u8 bcd[] = {0x21, 0x43, 0xF5};

    TEST_ASSERT_EQUAL(XY_ISO7816_ERROR_INVALID_PARAM, xy_iso7816_init(NULL, &g_uart));
    TEST_ASSERT_EQUAL(XY_ISO7816_ERROR_INVALID_PARAM, xy_iso7816_init(&handle, NULL));
    TEST_ASSERT_EQUAL(XY_ISO7816_OK, xy_iso7816_init(&handle, &g_uart));
    TEST_ASSERT_TRUE(handle.initialized);
    TEST_ASSERT_EQUAL_PTR(&g_uart, handle.uart);
    TEST_ASSERT_EQUAL_UINT32(XY_ISO7816_DEFAULT_TIMEOUT, handle.timeout);

    TEST_ASSERT_TRUE(xy_iso7816_is_success(&ok));
    TEST_ASSERT_FALSE(xy_iso7816_is_success(&fail));
    TEST_ASSERT_FALSE(xy_iso7816_is_success(NULL));
    TEST_ASSERT_EQUAL_HEX16(0x6A82U, xy_iso7816_get_sw(&fail));
    TEST_ASSERT_EQUAL_HEX16(0U, xy_iso7816_get_sw(NULL));

    TEST_ASSERT_EQUAL_UINT(5U, xy_iso7816_bcd_to_ascii(bcd, sizeof(bcd), ascii, sizeof(ascii)));
    TEST_ASSERT_EQUAL_STRING("12345", ascii);
    TEST_ASSERT_EQUAL_UINT(0U, xy_iso7816_bcd_to_ascii(NULL, 1, ascii, sizeof(ascii)));

    TEST_ASSERT_EQUAL(XY_ISO7816_OK, xy_iso7816_deinit(&handle));
    TEST_ASSERT_FALSE(handle.initialized);
}

static void test_reset_parses_direct_atr(void)
{
    xy_iso7816_handle_t handle;
    xy_iso7816_atr_t atr;
    const uint8_t atr_bytes[] = {0x3B, 0x02, 0x11, 0x22};

    push_rx(atr_bytes, sizeof(atr_bytes));

    TEST_ASSERT_EQUAL(XY_ISO7816_OK, xy_iso7816_init(&handle, &g_uart));
    TEST_ASSERT_EQUAL(XY_ISO7816_OK, xy_iso7816_reset(&handle, &atr));
    TEST_ASSERT_EQUAL_UINT(1U, xy_hal_uart_flush_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&g_uart, xy_hal_uart_flush_fake.arg0_val);
    TEST_ASSERT_TRUE(atr.valid);
    TEST_ASSERT_EQUAL_UINT(sizeof(atr_bytes), atr.length);
    TEST_ASSERT_EQUAL_MEMORY(atr_bytes, atr.data, sizeof(atr_bytes));
    TEST_ASSERT_TRUE(handle.atr.valid);
    TEST_ASSERT_EQUAL_UINT(4U, xy_hal_uart_recv_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&g_uart, xy_hal_uart_recv_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT(1U, xy_hal_uart_recv_fake.arg2_val);
    TEST_ASSERT_EQUAL_UINT32(XY_ISO7816_BYTE_TIMEOUT, xy_hal_uart_recv_fake.arg3_val);
}

static void test_transceive_write_apdu_success(void)
{
    xy_iso7816_handle_t handle;
    xy_iso7816_apdu_cmd_t cmd;
    xy_iso7816_apdu_resp_t resp;
    const uint8_t body[] = {0xCA, 0xFE};
    const uint8_t rx[] = {XY_ISO7816_INS_UPDATE_BINARY, 0x90, 0x00};

    push_rx(rx, sizeof(rx));
    memset(&cmd, 0, sizeof(cmd));
    cmd.cla = XY_ISO7816_CLA_GSM;
    cmd.ins = XY_ISO7816_INS_UPDATE_BINARY;
    cmd.p1 = 0x12;
    cmd.p2 = 0x34;
    cmd.lc = sizeof(body);
    memcpy(cmd.data, body, sizeof(body));
    cmd.le = 0;

    TEST_ASSERT_EQUAL(XY_ISO7816_OK, xy_iso7816_init(&handle, &g_uart));
    TEST_ASSERT_EQUAL(XY_ISO7816_OK, xy_iso7816_transceive(&handle, &cmd, &resp));
    TEST_ASSERT_EQUAL_UINT(2U, xy_hal_uart_send_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&g_uart, xy_hal_uart_send_fake.arg0_history[0]);
    TEST_ASSERT_EQUAL_PTR(&g_uart, xy_hal_uart_send_fake.arg0_history[1]);
    TEST_ASSERT_EQUAL_UINT(5U, xy_hal_uart_send_fake.arg2_history[0]);
    TEST_ASSERT_EQUAL_UINT(sizeof(body), xy_hal_uart_send_fake.arg2_history[1]);
    TEST_ASSERT_EQUAL_UINT32(XY_ISO7816_DEFAULT_TIMEOUT, xy_hal_uart_send_fake.arg3_val);
    TEST_ASSERT_EQUAL_UINT(7U, g_tx_len);
    TEST_ASSERT_EQUAL_HEX8(XY_ISO7816_CLA_GSM, g_tx[0]);
    TEST_ASSERT_EQUAL_HEX8(XY_ISO7816_INS_UPDATE_BINARY, g_tx[1]);
    TEST_ASSERT_EQUAL_HEX8(0x12U, g_tx[2]);
    TEST_ASSERT_EQUAL_HEX8(0x34U, g_tx[3]);
    TEST_ASSERT_EQUAL_UINT8(sizeof(body), g_tx[4]);
    TEST_ASSERT_EQUAL_HEX8(body[0], g_tx[5]);
    TEST_ASSERT_EQUAL_HEX8(body[1], g_tx[6]);
    TEST_ASSERT_EQUAL_UINT(3U, xy_hal_uart_recv_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(1U, xy_hal_uart_recv_fake.arg2_val);
    TEST_ASSERT_EQUAL_UINT32(XY_ISO7816_BYTE_TIMEOUT, xy_hal_uart_recv_fake.arg3_val);
    TEST_ASSERT_EQUAL_UINT(0U, resp.length);
    TEST_ASSERT_EQUAL_HEX16(XY_ISO7816_SW_SUCCESS, xy_iso7816_get_sw(&resp));
}

static void test_parse_atr_validation(void)
{
    xy_iso7816_atr_t atr = {0};

    TEST_ASSERT_EQUAL(XY_ISO7816_ERROR_INVALID_PARAM, xy_iso7816_parse_atr(NULL));
    TEST_ASSERT_EQUAL(XY_ISO7816_ERROR_INVALID_PARAM, xy_iso7816_parse_atr(&atr));
    atr.valid = true;
    TEST_ASSERT_EQUAL(XY_ISO7816_OK, xy_iso7816_parse_atr(&atr));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_lifecycle_and_utilities);
    RUN_TEST(test_reset_parses_direct_atr);
    RUN_TEST(test_transceive_write_apdu_success);
    RUN_TEST(test_parse_atr_validation);
    return UNITY_END();
}
