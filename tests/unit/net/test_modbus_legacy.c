/**
 * @file test_modbus_legacy.c
 * @brief Host guard for the dormant legacy Modbus RTU slave implementation.
 */

#include "mb_slave.h"

#include <assert.h>
#include <string.h>

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
    assert(length <= sizeof(g_tx));
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
    assert(g_tx_len >= 2U);
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
    assert(len <= MB_RX_BUFFER_SIZE);
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
    assert(mb_slave_init(NULL, 1, 9600) == -1);
    assert(mb_slave_init(&slave, 248, 9600) == -1);
    assert(mb_slave_init(&slave, 1, 0) == -1);
    assert(mb_slave_init(&slave, 1, 9600) == 0);
    assert(slave.address == 1U);
    assert(slave.baudrate == 9600U);
    assert(slave.frame_timeout >= 2U);

    mb_slave_set_coil_callback(&slave, coil_written);
    mb_slave_set_register_callback(&slave, register_written);

    assert(mb_slave_set_coil(&slave, 3, true) == 0);
    assert(mb_slave_get_coil(&slave, 3));
    assert(g_coil_cb_addr == 3U && g_coil_cb_value);
    assert(mb_slave_set_coil(&slave, MB_COIL_COUNT, true) == -1);
    assert(!mb_slave_get_coil(&slave, MB_COIL_COUNT));

    assert(mb_slave_set_discrete(&slave, 4, true) == 0);
    assert(mb_slave_get_discrete(&slave, 4));
    assert(mb_slave_set_discrete(&slave, MB_DISCRETE_COUNT, true) == -1);

    assert(mb_slave_set_input_register(&slave, 2, 0x1234U) == 0);
    assert(mb_slave_get_input_register(&slave, 2) == 0x1234U);
    assert(mb_slave_set_holding_register(&slave, 1, 0xABCDU) == 0);
    assert(mb_slave_get_holding_register(&slave, 1) == 0xABCDU);
    assert(g_reg_cb_addr == 1U && g_reg_cb_value == 0xABCDU);

    mb_slave_get_stats(&slave, &requests, &exceptions, &crc_errors);
    assert(requests == 0U && exceptions == 0U && crc_errors == 0U);
    slave.request_count = 7U;
    slave.exception_count = 8U;
    slave.crc_error_count = 9U;
    mb_slave_reset_stats(&slave);
    mb_slave_get_stats(&slave, &requests, &exceptions, &crc_errors);
    assert(requests == 0U && exceptions == 0U && crc_errors == 0U);

    assert(mb_slave_set_coil(NULL, 0, true) == -1);
    assert(!mb_slave_get_coil(NULL, 0));
    assert(mb_slave_set_holding_register(NULL, 0, 1) == -1);
    assert(mb_slave_get_holding_register(NULL, 0) == 0U);
}

static void test_crc_and_read_holding_response(void)
{
    mb_slave_t slave;
    uint8_t req[8] = {5, MB_FC_READ_HOLDING_REGISTERS, 0, 0, 0, 2, 0, 0};

    reset_capture();
    assert(mb_crc16((const uint8_t *)"123456789", 9) == 0x4B37U);
    assert(mb_slave_init(&slave, 5, 19200) == 0);
    assert(mb_slave_set_holding_register(&slave, 0, 0x1234U) == 0);
    assert(mb_slave_set_holding_register(&slave, 1, 0xABCDU) == 0);
    append_crc(req, 6);

    load_frame(&slave, req, sizeof(req));
    mb_slave_process_frame(&slave);

    assert(g_tx_len == 9U);
    assert(g_tx[0] == 5U);
    assert(g_tx[1] == MB_FC_READ_HOLDING_REGISTERS);
    assert(g_tx[2] == 4U);
    assert(g_tx[3] == 0x12U && g_tx[4] == 0x34U);
    assert(g_tx[5] == 0xABU && g_tx[6] == 0xCDU);
    assert(mb_crc16(g_tx, g_tx_len - 2U) == captured_crc());
    assert(slave.request_count == 1U);
}

static void test_write_single_coil_exception_and_crc_stats(void)
{
    mb_slave_t slave;
    uint8_t write_req[8] = {2, MB_FC_WRITE_SINGLE_COIL, 0, 3, 0xFF, 0, 0, 0};
    uint8_t invalid_addr_req[8] = {2, MB_FC_READ_INPUT_REGISTERS, 0, MB_INPUT_REG_COUNT, 0, 1, 0, 0};
    uint8_t bad_crc_req[8] = {2, MB_FC_READ_COILS, 0, 0, 0, 1, 0x12, 0x34};

    reset_capture();
    assert(mb_slave_init(&slave, 2, 9600) == 0);
    append_crc(write_req, 6);
    load_frame(&slave, write_req, sizeof(write_req));
    mb_slave_process_frame(&slave);
    assert(mb_slave_get_coil(&slave, 3));
    assert(g_tx_len == 8U);
    assert(memcmp(g_tx, write_req, 6) == 0);
    assert(mb_crc16(g_tx, g_tx_len - 2U) == captured_crc());

    reset_capture();
    append_crc(invalid_addr_req, 6);
    load_frame(&slave, invalid_addr_req, sizeof(invalid_addr_req));
    mb_slave_process_frame(&slave);
    assert(g_tx_len == 5U);
    assert(g_tx[0] == 2U);
    assert(g_tx[1] == (MB_FC_READ_INPUT_REGISTERS | 0x80U));
    assert(g_tx[2] == MB_EX_ILLEGAL_DATA_ADDRESS);
    assert(slave.exception_count == 1U);

    reset_capture();
    load_frame(&slave, bad_crc_req, sizeof(bad_crc_req));
    mb_slave_process_frame(&slave);
    assert(g_tx_len == 0U);
    assert(slave.crc_error_count == 1U);
}

static void test_receive_poll_timeout(void)
{
    mb_slave_t slave;
    uint8_t req[8] = {4, MB_FC_READ_COILS, 0, 0, 0, 1, 0, 0};

    reset_capture();
    assert(mb_slave_init(&slave, 4, 9600) == 0);
    assert(mb_slave_set_coil(&slave, 0, true) == 0);
    append_crc(req, 6);

    for (uint16_t i = 0; i < sizeof(req); ++i) {
        g_now_ms = i;
        mb_slave_receive_byte(&slave, req[i]);
    }
    assert(slave.rx_count == sizeof(req));

    mb_slave_poll(&slave, g_now_ms + slave.frame_timeout);
    assert(slave.rx_count == 0U);
    assert(g_tx_len == 6U);
    assert(g_tx[0] == 4U);
    assert(g_tx[1] == MB_FC_READ_COILS);
    assert(g_tx[2] == 1U);
    assert(g_tx[3] == 1U);
}

int main(void)
{
    test_init_validation_and_accessors();
    test_crc_and_read_holding_response();
    test_write_single_coil_exception_and_crc_stats();
    test_receive_poll_timeout();
    return 0;
}
