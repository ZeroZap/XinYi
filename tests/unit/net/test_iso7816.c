/**
 * @file test_iso7816.c
 * @brief Unit tests for ISO7816 utility/lifecycle/APDU behavior.
 */

#include "xy_iso7816.h"
#include "xy_hal_error.h"

#include <assert.h>
#include <stdbool.h>
#include <string.h>

static unsigned char g_uart;
static uint8_t g_tx[512];
static size_t g_tx_len;
static uint8_t g_rx[512];
static size_t g_rx_len;
static size_t g_rx_pos;
static uint32_t g_last_send_timeout;
static uint32_t g_last_recv_timeout;
static unsigned g_flush_count;

static void reset_uart_fixture(void)
{
    memset(g_tx, 0, sizeof(g_tx));
    memset(g_rx, 0, sizeof(g_rx));
    g_tx_len = 0;
    g_rx_len = 0;
    g_rx_pos = 0;
    g_last_send_timeout = 0;
    g_last_recv_timeout = 0;
    g_flush_count = 0;
}

static void push_rx(const uint8_t *data, size_t len)
{
    assert(g_rx_len + len <= sizeof(g_rx));
    memcpy(&g_rx[g_rx_len], data, len);
    g_rx_len += len;
}

xy_hal_error_t xy_hal_uart_send(void *uart, const uint8_t *data, size_t len,
                                uint32_t timeout)
{
    assert(uart == &g_uart);
    assert(g_tx_len + len <= sizeof(g_tx));
    memcpy(&g_tx[g_tx_len], data, len);
    g_tx_len += len;
    g_last_send_timeout = timeout;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_recv(void *uart, uint8_t *data, size_t len,
                                uint32_t timeout)
{
    assert(uart == &g_uart);
    g_last_recv_timeout = timeout;
    if (g_rx_pos + len > g_rx_len) {
        return XY_HAL_ERROR_TIMEOUT;
    }
    memcpy(data, &g_rx[g_rx_pos], len);
    g_rx_pos += len;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_flush(void *uart)
{
    assert(uart == &g_uart);
    g_flush_count++;
    return XY_HAL_OK;
}

static void test_lifecycle_and_utilities(void)
{
    xy_iso7816_handle_t handle;
    xy_iso7816_apdu_resp_t ok = {.sw1 = 0x90, .sw2 = 0x00};
    xy_iso7816_apdu_resp_t fail = {.sw1 = 0x6A, .sw2 = 0x82};
    char ascii[16];
    const xy_u8 bcd[] = {0x21, 0x43, 0xF5};

    assert(xy_iso7816_init(NULL, &g_uart) == XY_ISO7816_ERROR_INVALID_PARAM);
    assert(xy_iso7816_init(&handle, NULL) == XY_ISO7816_ERROR_INVALID_PARAM);
    assert(xy_iso7816_init(&handle, &g_uart) == XY_ISO7816_OK);
    assert(handle.initialized);
    assert(handle.uart == &g_uart);
    assert(handle.timeout == XY_ISO7816_DEFAULT_TIMEOUT);

    assert(xy_iso7816_is_success(&ok));
    assert(!xy_iso7816_is_success(&fail));
    assert(!xy_iso7816_is_success(NULL));
    assert(xy_iso7816_get_sw(&fail) == 0x6A82U);
    assert(xy_iso7816_get_sw(NULL) == 0U);

    assert(xy_iso7816_bcd_to_ascii(bcd, sizeof(bcd), ascii, sizeof(ascii)) == 5U);
    assert(strcmp(ascii, "12345") == 0);
    assert(xy_iso7816_bcd_to_ascii(NULL, 1, ascii, sizeof(ascii)) == 0U);

    assert(xy_iso7816_deinit(&handle) == XY_ISO7816_OK);
    assert(!handle.initialized);
}

static void test_reset_parses_direct_atr(void)
{
    xy_iso7816_handle_t handle;
    xy_iso7816_atr_t atr;
    const uint8_t atr_bytes[] = {0x3B, 0x02, 0x11, 0x22};

    reset_uart_fixture();
    push_rx(atr_bytes, sizeof(atr_bytes));

    assert(xy_iso7816_init(&handle, &g_uart) == XY_ISO7816_OK);
    assert(xy_iso7816_reset(&handle, &atr) == XY_ISO7816_OK);
    assert(g_flush_count == 1U);
    assert(atr.valid);
    assert(atr.length == sizeof(atr_bytes));
    assert(memcmp(atr.data, atr_bytes, sizeof(atr_bytes)) == 0);
    assert(handle.atr.valid);
    assert(g_last_recv_timeout == XY_ISO7816_BYTE_TIMEOUT);
}

static void test_transceive_write_apdu_success(void)
{
    xy_iso7816_handle_t handle;
    xy_iso7816_apdu_cmd_t cmd;
    xy_iso7816_apdu_resp_t resp;
    const uint8_t body[] = {0xCA, 0xFE};
    const uint8_t rx[] = {XY_ISO7816_INS_UPDATE_BINARY, 0x90, 0x00};

    reset_uart_fixture();
    push_rx(rx, sizeof(rx));
    memset(&cmd, 0, sizeof(cmd));
    cmd.cla = XY_ISO7816_CLA_GSM;
    cmd.ins = XY_ISO7816_INS_UPDATE_BINARY;
    cmd.p1 = 0x12;
    cmd.p2 = 0x34;
    cmd.lc = sizeof(body);
    memcpy(cmd.data, body, sizeof(body));
    cmd.le = 0;

    assert(xy_iso7816_init(&handle, &g_uart) == XY_ISO7816_OK);
    assert(xy_iso7816_transceive(&handle, &cmd, &resp) == XY_ISO7816_OK);
    assert(g_tx_len == 7U);
    assert(g_tx[0] == XY_ISO7816_CLA_GSM);
    assert(g_tx[1] == XY_ISO7816_INS_UPDATE_BINARY);
    assert(g_tx[2] == 0x12U && g_tx[3] == 0x34U);
    assert(g_tx[4] == sizeof(body));
    assert(g_tx[5] == body[0] && g_tx[6] == body[1]);
    assert(g_last_send_timeout == XY_ISO7816_DEFAULT_TIMEOUT);
    assert(resp.length == 0U);
    assert(xy_iso7816_get_sw(&resp) == XY_ISO7816_SW_SUCCESS);
}

static void test_parse_atr_validation(void)
{
    xy_iso7816_atr_t atr = {0};

    assert(xy_iso7816_parse_atr(NULL) == XY_ISO7816_ERROR_INVALID_PARAM);
    assert(xy_iso7816_parse_atr(&atr) == XY_ISO7816_ERROR_INVALID_PARAM);
    atr.valid = true;
    assert(xy_iso7816_parse_atr(&atr) == XY_ISO7816_OK);
}

int main(void)
{
    test_lifecycle_and_utilities();
    test_reset_parses_direct_atr();
    test_transceive_write_apdu_success();
    test_parse_atr_validation();
    return 0;
}
