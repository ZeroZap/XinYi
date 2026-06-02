/**
 * @file test_nano_modbus.c
 * @brief Unit tests for nano_modbus compatibility API over mb_tiny.
 */

#include "nano_modbus.h"

#include <assert.h>
#include <string.h>

static uint8_t g_tx[MB_TINY_MAX_ADU_SIZE];
static uint16_t g_tx_len;
static uint8_t g_rx[MB_TINY_MAX_ADU_SIZE];
static uint16_t g_rx_len;
static uint32_t g_recv_timeout;

static void append_crc(uint8_t *frame, uint16_t payload_len)
{
    uint16_t crc = nano_mb_crc16(frame, payload_len);
    frame[payload_len] = (uint8_t)(crc & 0xFFU);
    frame[payload_len + 1U] = (uint8_t)(crc >> 8);
}

static int slave_send_capture(const uint8_t *data, uint16_t len)
{
    assert(len <= sizeof(g_tx));
    memcpy(g_tx, data, len);
    g_tx_len = len;
    return len;
}

static int master_send_capture(const uint8_t *data, uint16_t len)
{
    assert(len <= sizeof(g_tx));
    memcpy(g_tx, data, len);
    g_tx_len = len;
    return len;
}

static int master_recv_fixture(uint8_t *data, uint16_t len, uint32_t timeout_ms)
{
    assert(len >= g_rx_len);
    g_recv_timeout = timeout_ms;
    memcpy(data, g_rx, g_rx_len);
    return g_rx_len;
}

static void reset_capture(void)
{
    memset(g_tx, 0, sizeof(g_tx));
    memset(g_rx, 0, sizeof(g_rx));
    g_tx_len = 0;
    g_rx_len = 0;
    g_recv_timeout = 0;
}

static void test_compat_slave_holding_read(void)
{
    mb_slave_t slave;
    mb_slave_config_t cfg = {.slave_id = 3};
    uint16_t holding[2] = {0x1234, 0xABCD};
    uint8_t req[8] = {3, MB_FUNC_READ_HOLDING, 0, 0, 0, 2, 0, 0};

    reset_capture();
    append_crc(req, 6);

    assert(nano_mb_slave_init(&slave, &cfg) == NANO_MB_OK);
    assert(slave.initialized);
    assert(slave.slave_id == 3U);
    assert(mb_tiny_slave_config_holding(&slave, holding, 0, 2) == MB_TINY_OK);
    mb_tiny_slave_set_send(&slave, slave_send_capture);

    assert(nano_mb_slave_poll(&slave, req, sizeof(req)) == NANO_MB_OK);
    assert(g_tx_len == 9U);
    assert(g_tx[0] == 3U);
    assert(g_tx[1] == MB_FUNC_READ_HOLDING);
    assert(g_tx[2] == 4U);
    assert(g_tx[3] == 0x12U && g_tx[4] == 0x34U);
    assert(g_tx[5] == 0xABU && g_tx[6] == 0xCDU);
    assert(nano_mb_crc16(g_tx, 7) == ((uint16_t)g_tx[8] << 8 | g_tx[7]));
    assert(slave.request_count == 1U);
    assert(nano_mb_slave_deinit(&slave) == NANO_MB_OK);
}

static void test_compat_master_read_holding(void)
{
    mb_master_t master;
    uint16_t regs[2] = {0, 0};

    reset_capture();
    g_rx[0] = 7;
    g_rx[1] = MB_FUNC_READ_HOLDING;
    g_rx[2] = 4;
    g_rx[3] = 0xBE;
    g_rx[4] = 0xEF;
    g_rx[5] = 0xCA;
    g_rx[6] = 0xFE;
    append_crc(g_rx, 7);
    g_rx_len = 9;

    assert(nano_mb_master_init(&master) == NANO_MB_OK);
    mb_tiny_master_set_uart(&master, master_send_capture, master_recv_fixture);
    mb_tiny_master_set_timeout(&master, 321);

    assert(nano_mb_master_read_holding(&master, 7, 0x0010, 2, regs, 999) == NANO_MB_OK);
    assert(g_tx_len == 8U);
    assert(g_tx[0] == 7U);
    assert(g_tx[1] == MB_FUNC_READ_HOLDING);
    assert(g_tx[2] == 0x00U && g_tx[3] == 0x10U);
    assert(g_tx[4] == 0x00U && g_tx[5] == 0x02U);
    assert(regs[0] == 0xBEEFU);
    assert(regs[1] == 0xCAFEU);
    assert(g_recv_timeout == 321U);
    assert(master.request_count == 1U);
    assert(nano_mb_master_deinit(&master) == NANO_MB_OK);
}

static void test_compat_validation_and_crc(void)
{
    const uint8_t frame[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x0A};
    mb_master_t master;
    uint16_t data[1];

    assert(nano_mb_crc16(frame, sizeof(frame)) == 0xCDC5U);
    assert(strcmp(nano_mb_error_string(NANO_MB_CRC_ERROR), "CRC Error") == 0);
    assert(nano_mb_slave_init(NULL, NULL) == NANO_MB_INVALID_PARAM);
    assert(nano_mb_master_init(&master) == NANO_MB_OK);
    assert(nano_mb_master_read_holding(&master, 1, 0, 0, data, 0) == NANO_MB_INVALID_PARAM);
    assert(nano_mb_master_read_holding(&master, 1, 0, 126, data, 0) == NANO_MB_INVALID_PARAM);
    assert(nano_mb_master_read_holding(NULL, 1, 0, 1, data, 0) == NANO_MB_INVALID_PARAM);
}

int main(void)
{
    test_compat_slave_holding_read();
    test_compat_master_read_holding();
    test_compat_validation_and_crc();
    return 0;
}
