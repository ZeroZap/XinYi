/**
 * @file test_modbus_legacy.c
 * @brief Host guard for the dormant legacy Modbus RTU slave implementation.
 */

#include "mb_slave.h"

#include <string.h>

#include "unity.h"

static uint8_t g_tx[MB_TX_BUFFER_SIZE];
static uint16_t g_tx_len;
static uint32_t g_now_ms;
static uint16_t g_coil_cb_addr;
static bool g_coil_cb_value;
static uint16_t g_reg_cb_addr;
static uint16_t g_reg_cb_value;

void mb_uart_send_byte(uint8_t data)
{
    (void)data;
}

void mb_uart_send_buffer(const uint8_t *buffer, uint16_t length)
{
    TEST_ASSERT_LESS_OR_EQUAL_size_t(sizeof(g_tx), length);
    memcpy(g_tx, buffer, length);
    g_tx_len = length;
}

void mb_uart_enable_rx(bool enable)
{
    (void)enable;
}

uint32_t mb_get_time_ms(void)
{
    return g_now_ms;
}

void setUp(void)
{
}

void tearDown(void)
{
}

static void reset_capture(void)
{
    memset(g_tx, 0, sizeof(g_tx));
    g_tx_len = 0;
    g_now_ms = 0;
    g_coil_cb_addr = 0xFFFFU;
    g_coil_cb_value = false;
    g_reg_cb_addr = 0xFFFFU;
    g_reg_cb_value = 0xFFFFU;
}

static void append_crc(uint8_t *frame, uint16_t payload_len)
{
    uint16_t crc = mb_crc16(frame, payload_len);
    frame[payload_len] = (uint8_t)(crc & 0xFFU);
    frame[payload_len + 1U] = (uint8_t)(crc >> 8);
}

static uint16_t captured_crc(void)
{
    TEST_ASSERT_GREATER_OR_EQUAL_UINT16(2U, g_tx_len);
    return (uint16_t)((uint16_t)g_tx[g_tx_len - 1U] << 8U) | g_tx[g_tx_len - 2U];
}

static void coil_written(uint16_t addr, bool value)
{
    g_coil_cb_addr = addr;
    g_coil_cb_value = value;
}

static void register_written(uint16_t addr, uint16_t value)
{
    g_reg_cb_addr = addr;
    g_reg_cb_value = value;
}

static void load_frame(mb_slave_t *slave, const uint8_t *frame, uint16_t len)
{
    TEST_ASSERT_LESS_OR_EQUAL_UINT16(MB_RX_BUFFER_SIZE, len);
    memcpy(slave->rx_buffer, frame, len);
    slave->rx_count = len;
}

static void test_init_validation_and_accessors(void)
{
    mb_slave_t slave;
    uint32_t requests = 123U;
    uint32_t exceptions = 456U;
    uint32_t crc_errors = 789U;

    reset_capture();
    TEST_ASSERT_EQUAL_INT(-1, mb_slave_init(NULL, 1, 9600));
    TEST_ASSERT_EQUAL_INT(-1, mb_slave_init(&slave, 248, 9600));
    TEST_ASSERT_EQUAL_INT(-1, mb_slave_init(&slave, 1, 0));
    TEST_ASSERT_EQUAL_INT(0, mb_slave_init(&slave, 1, 9600));
    TEST_ASSERT_EQUAL_UINT8(1U, slave.address);
    TEST_ASSERT_EQUAL_UINT32(9600U, slave.baudrate);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT16(2U, slave.frame_timeout);

    mb_slave_set_coil_callback(&slave, coil_written);
    mb_slave_set_register_callback(&slave, register_written);

    TEST_ASSERT_EQUAL_INT(0, mb_slave_set_coil(&slave, 3, true));
    TEST_ASSERT_TRUE(mb_slave_get_coil(&slave, 3));
    TEST_ASSERT_EQUAL_UINT16(3U, g_coil_cb_addr);
    TEST_ASSERT_TRUE(g_coil_cb_value);
    TEST_ASSERT_EQUAL_INT(-1, mb_slave_set_coil(&slave, MB_COIL_COUNT, true));
    TEST_ASSERT_FALSE(mb_slave_get_coil(&slave, MB_COIL_COUNT));

    TEST_ASSERT_EQUAL_INT(0, mb_slave_set_discrete(&slave, 4, true));
    TEST_ASSERT_TRUE(mb_slave_get_discrete(&slave, 4));
    TEST_ASSERT_EQUAL_INT(-1, mb_slave_set_discrete(&slave, MB_DISCRETE_COUNT, true));

    TEST_ASSERT_EQUAL_INT(0, mb_slave_set_input_register(&slave, 2, 0x1234U));
    TEST_ASSERT_EQUAL_HEX16(0x1234U, mb_slave_get_input_register(&slave, 2));
    TEST_ASSERT_EQUAL_INT(0, mb_slave_set_holding_register(&slave, 1, 0xABCDU));
    TEST_ASSERT_EQUAL_HEX16(0xABCDU, mb_slave_get_holding_register(&slave, 1));
    TEST_ASSERT_EQUAL_UINT16(1U, g_reg_cb_addr);
    TEST_ASSERT_EQUAL_HEX16(0xABCDU, g_reg_cb_value);

    mb_slave_get_stats(&slave, &requests, &exceptions, &crc_errors);
    TEST_ASSERT_EQUAL_UINT32(0U, requests);
    TEST_ASSERT_EQUAL_UINT32(0U, exceptions);
    TEST_ASSERT_EQUAL_UINT32(0U, crc_errors);
    slave.request_count = 7U;
    slave.exception_count = 8U;
    slave.crc_error_count = 9U;
    mb_slave_reset_stats(&slave);
    mb_slave_get_stats(&slave, &requests, &exceptions, &crc_errors);
    TEST_ASSERT_EQUAL_UINT32(0U, requests);
    TEST_ASSERT_EQUAL_UINT32(0U, exceptions);
    TEST_ASSERT_EQUAL_UINT32(0U, crc_errors);

    TEST_ASSERT_EQUAL_INT(-1, mb_slave_set_coil(NULL, 0, true));
    TEST_ASSERT_FALSE(mb_slave_get_coil(NULL, 0));
    TEST_ASSERT_EQUAL_INT(-1, mb_slave_set_holding_register(NULL, 0, 1));
    TEST_ASSERT_EQUAL_HEX16(0U, mb_slave_get_holding_register(NULL, 0));
}

static void test_crc_and_read_holding_response(void)
{
    mb_slave_t slave;
    uint8_t req[8] = {5, MB_FC_READ_HOLDING_REGISTERS, 0, 0, 0, 2, 0, 0};

    reset_capture();
    TEST_ASSERT_EQUAL_HEX16(0x4B37U, mb_crc16((const uint8_t *)"123456789", 9));
    TEST_ASSERT_EQUAL_INT(0, mb_slave_init(&slave, 5, 19200));
    TEST_ASSERT_EQUAL_INT(0, mb_slave_set_holding_register(&slave, 0, 0x1234U));
    TEST_ASSERT_EQUAL_INT(0, mb_slave_set_holding_register(&slave, 1, 0xABCDU));
    append_crc(req, 6);

    load_frame(&slave, req, sizeof(req));
    mb_slave_process_frame(&slave);

    TEST_ASSERT_EQUAL_UINT16(9U, g_tx_len);
    TEST_ASSERT_EQUAL_HEX8(5U, g_tx[0]);
    TEST_ASSERT_EQUAL_HEX8(MB_FC_READ_HOLDING_REGISTERS, g_tx[1]);
    TEST_ASSERT_EQUAL_HEX8(4U, g_tx[2]);
    TEST_ASSERT_EQUAL_HEX8(0x12U, g_tx[3]);
    TEST_ASSERT_EQUAL_HEX8(0x34U, g_tx[4]);
    TEST_ASSERT_EQUAL_HEX8(0xABU, g_tx[5]);
    TEST_ASSERT_EQUAL_HEX8(0xCDU, g_tx[6]);
    TEST_ASSERT_EQUAL_HEX16(mb_crc16(g_tx, g_tx_len - 2U), captured_crc());
    TEST_ASSERT_EQUAL_UINT32(1U, slave.request_count);
}

static void test_write_single_coil_exception_and_crc_stats(void)
{
    mb_slave_t slave;
    uint8_t write_req[8] = {2, MB_FC_WRITE_SINGLE_COIL, 0, 3, 0xFF, 0, 0, 0};
    uint8_t invalid_addr_req[8] = {2, MB_FC_READ_INPUT_REGISTERS, 0, MB_INPUT_REG_COUNT, 0, 1, 0, 0};
    uint8_t bad_crc_req[8] = {2, MB_FC_READ_COILS, 0, 0, 0, 1, 0x12, 0x34};

    reset_capture();
    TEST_ASSERT_EQUAL_INT(0, mb_slave_init(&slave, 2, 9600));
    append_crc(write_req, 6);
    load_frame(&slave, write_req, sizeof(write_req));
    mb_slave_process_frame(&slave);
    TEST_ASSERT_TRUE(mb_slave_get_coil(&slave, 3));
    TEST_ASSERT_EQUAL_UINT16(8U, g_tx_len);
    TEST_ASSERT_EQUAL_MEMORY(write_req, g_tx, 6);
    TEST_ASSERT_EQUAL_HEX16(mb_crc16(g_tx, g_tx_len - 2U), captured_crc());

    reset_capture();
    append_crc(invalid_addr_req, 6);
    load_frame(&slave, invalid_addr_req, sizeof(invalid_addr_req));
    mb_slave_process_frame(&slave);
    TEST_ASSERT_EQUAL_UINT16(5U, g_tx_len);
    TEST_ASSERT_EQUAL_HEX8(2U, g_tx[0]);
    TEST_ASSERT_EQUAL_HEX8((MB_FC_READ_INPUT_REGISTERS | 0x80U), g_tx[1]);
    TEST_ASSERT_EQUAL_HEX8(MB_EX_ILLEGAL_DATA_ADDRESS, g_tx[2]);
    TEST_ASSERT_EQUAL_UINT32(1U, slave.exception_count);

    reset_capture();
    load_frame(&slave, bad_crc_req, sizeof(bad_crc_req));
    mb_slave_process_frame(&slave);
    TEST_ASSERT_EQUAL_UINT16(0U, g_tx_len);
    TEST_ASSERT_EQUAL_UINT32(1U, slave.crc_error_count);
}

static void test_receive_poll_timeout(void)
{
    mb_slave_t slave;
    uint8_t req[8] = {4, MB_FC_READ_COILS, 0, 0, 0, 1, 0, 0};

    reset_capture();
    TEST_ASSERT_EQUAL_INT(0, mb_slave_init(&slave, 4, 9600));
    TEST_ASSERT_EQUAL_INT(0, mb_slave_set_coil(&slave, 0, true));
    append_crc(req, 6);

    for (uint16_t i = 0; i < sizeof(req); ++i) {
        g_now_ms = i;
        mb_slave_receive_byte(&slave, req[i]);
    }
    TEST_ASSERT_EQUAL_UINT16(sizeof(req), slave.rx_count);

    mb_slave_poll(&slave, g_now_ms + slave.frame_timeout);
    TEST_ASSERT_EQUAL_UINT16(0U, slave.rx_count);
    TEST_ASSERT_EQUAL_UINT16(6U, g_tx_len);
    TEST_ASSERT_EQUAL_HEX8(4U, g_tx[0]);
    TEST_ASSERT_EQUAL_HEX8(MB_FC_READ_COILS, g_tx[1]);
    TEST_ASSERT_EQUAL_HEX8(1U, g_tx[2]);
    TEST_ASSERT_EQUAL_HEX8(1U, g_tx[3]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_validation_and_accessors);
    RUN_TEST(test_crc_and_read_holding_response);
    RUN_TEST(test_write_single_coil_exception_and_crc_stats);
    RUN_TEST(test_receive_poll_timeout);
    return UNITY_END();
}
