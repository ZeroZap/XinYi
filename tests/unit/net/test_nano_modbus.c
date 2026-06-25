/**
 * @file test_nano_modbus.c
 * @brief Unit tests for nano_modbus compatibility API over mb_tiny.
 */

#include "nano_modbus.h"

#include "unity.h"

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
    TEST_ASSERT_LESS_OR_EQUAL_UINT(sizeof(g_tx), len);
    memcpy(g_tx, data, len);
    g_tx_len = len;
    return len;
}

static int master_send_capture(const uint8_t *data, uint16_t len)
{
    TEST_ASSERT_LESS_OR_EQUAL_UINT(sizeof(g_tx), len);
    memcpy(g_tx, data, len);
    g_tx_len = len;
    return len;
}

static int master_recv_fixture(uint8_t *data, uint16_t len, uint32_t timeout_ms)
{
    TEST_ASSERT_GREATER_OR_EQUAL_UINT(g_rx_len, len);
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

    TEST_ASSERT_EQUAL(NANO_MB_OK, nano_mb_slave_init(&slave, &cfg));
    TEST_ASSERT_TRUE(slave.initialized);
    TEST_ASSERT_EQUAL_UINT8(3U, slave.slave_id);
    TEST_ASSERT_EQUAL(MB_TINY_OK, mb_tiny_slave_config_holding(&slave, holding, 0, 2));
    mb_tiny_slave_set_send(&slave, slave_send_capture);

    TEST_ASSERT_EQUAL(NANO_MB_OK, nano_mb_slave_poll(&slave, req, sizeof(req)));
    TEST_ASSERT_EQUAL_UINT16(9U, g_tx_len);
    TEST_ASSERT_EQUAL_UINT8(3U, g_tx[0]);
    TEST_ASSERT_EQUAL_UINT8(MB_FUNC_READ_HOLDING, g_tx[1]);
    TEST_ASSERT_EQUAL_UINT8(4U, g_tx[2]);
    TEST_ASSERT_EQUAL_UINT8(0x12U, g_tx[3]);
    TEST_ASSERT_EQUAL_UINT8(0x34U, g_tx[4]);
    TEST_ASSERT_EQUAL_UINT8(0xABU, g_tx[5]);
    TEST_ASSERT_EQUAL_UINT8(0xCDU, g_tx[6]);
    TEST_ASSERT_EQUAL_UINT16(nano_mb_crc16(g_tx, 7), ((uint16_t)g_tx[8] << 8 | g_tx[7]));
    TEST_ASSERT_EQUAL_UINT32(1U, slave.request_count);
    TEST_ASSERT_EQUAL(NANO_MB_OK, nano_mb_slave_deinit(&slave));
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

    TEST_ASSERT_EQUAL(NANO_MB_OK, nano_mb_master_init(&master));
    mb_tiny_master_set_uart(&master, master_send_capture, master_recv_fixture);
    mb_tiny_master_set_timeout(&master, 321);

    TEST_ASSERT_EQUAL(NANO_MB_OK, nano_mb_master_read_holding(&master, 7, 0x0010, 2, regs, 999));
    TEST_ASSERT_EQUAL_UINT16(8U, g_tx_len);
    TEST_ASSERT_EQUAL_UINT8(7U, g_tx[0]);
    TEST_ASSERT_EQUAL_UINT8(MB_FUNC_READ_HOLDING, g_tx[1]);
    TEST_ASSERT_EQUAL_UINT8(0x00U, g_tx[2]);
    TEST_ASSERT_EQUAL_UINT8(0x10U, g_tx[3]);
    TEST_ASSERT_EQUAL_UINT8(0x00U, g_tx[4]);
    TEST_ASSERT_EQUAL_UINT8(0x02U, g_tx[5]);
    TEST_ASSERT_EQUAL_UINT16(0xBEEFU, regs[0]);
    TEST_ASSERT_EQUAL_UINT16(0xCAFEU, regs[1]);
    TEST_ASSERT_EQUAL_UINT32(321U, g_recv_timeout);
    TEST_ASSERT_EQUAL_UINT32(1U, master.request_count);
    TEST_ASSERT_EQUAL(NANO_MB_OK, nano_mb_master_deinit(&master));
}

static void test_compat_validation_and_crc(void)
{
    const uint8_t frame[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x0A};
    mb_master_t master;
    uint16_t data[1];

    TEST_ASSERT_EQUAL_UINT16(0xCDC5U, nano_mb_crc16(frame, sizeof(frame)));
    TEST_ASSERT_EQUAL_STRING("CRC Error", nano_mb_error_string(NANO_MB_CRC_ERROR));
    TEST_ASSERT_EQUAL(NANO_MB_INVALID_PARAM, nano_mb_slave_init(NULL, NULL));
    TEST_ASSERT_EQUAL(NANO_MB_OK, nano_mb_master_init(&master));
    TEST_ASSERT_EQUAL(NANO_MB_INVALID_PARAM, nano_mb_master_read_holding(&master, 1, 0, 0, data, 0));
    TEST_ASSERT_EQUAL(NANO_MB_INVALID_PARAM, nano_mb_master_read_holding(&master, 1, 0, 126, data, 0));
    TEST_ASSERT_EQUAL(NANO_MB_INVALID_PARAM, nano_mb_master_read_holding(NULL, 1, 0, 1, data, 0));
}

void setUp(void)
{
}

void tearDown(void)
{
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_compat_slave_holding_read);
    RUN_TEST(test_compat_master_read_holding);
    RUN_TEST(test_compat_validation_and_crc);
    return UNITY_END();
}
