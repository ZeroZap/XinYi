/**
 * @file test_modbus_full_slave.c
 * @brief Host guard for the Modbus Full slave: RTU and TCP frame contracts.
 *
 * The slave had no test at all, so these cases pin the wire contract that an
 * external master sees: RTU CRC framing, MBAP framing, exception responses,
 * slave-id filtering, and the packed bit layout of coil/discrete areas.
 */

#include "mb_full.h"

#include <string.h>

#include "unity.h"

#define UNIT_ID 0x11U

static uint8_t g_tx[MB_FULL_MAX_TCP_ADU_SIZE];
static uint16_t g_tx_len;
static uint32_t g_tx_count;

static int capture_send(const uint8_t *data, uint16_t len)
{
    TEST_ASSERT_LESS_OR_EQUAL_UINT(sizeof(g_tx), len);
    memcpy(g_tx, data, len);
    g_tx_len = len;
    g_tx_count++;
    return (int)len;
}

static void capture_reset(void)
{
    memset(g_tx, 0, sizeof(g_tx));
    g_tx_len = 0;
    g_tx_count = 0;
}

/* 在 pdu 之后追加 RTU CRC 并返回 ADU 长度。 */
static uint16_t rtu_frame(uint8_t *frame, const uint8_t *pdu, uint16_t pdu_len)
{
    uint16_t crc;

    memcpy(frame, pdu, pdu_len);
    crc = mb_full_crc16(pdu, pdu_len);
    frame[pdu_len] = (uint8_t)(crc & 0xFFU);
    frame[pdu_len + 1U] = (uint8_t)(crc >> 8);
    return (uint16_t)(pdu_len + 2U);
}

/* 组一帧带 RTU CRC 的请求并交给从站。 */
static int rtu_request(mb_full_slave_t *slave, const uint8_t *pdu, uint16_t pdu_len)
{
    uint8_t frame[MB_FULL_MAX_ADU_SIZE];
    uint16_t len = rtu_frame(frame, pdu, pdu_len);

    return mb_full_slave_handle(slave, frame, len);
}

/* 组一帧 MBAP + PDU 的请求并交给从站。 */
static int tcp_request(mb_full_slave_t *slave, uint16_t transaction_id,
                       uint8_t unit_id, const uint8_t *pdu, uint16_t pdu_len)
{
    uint8_t frame[MB_FULL_MAX_TCP_ADU_SIZE];
    uint16_t length = (uint16_t)(pdu_len + 1U); /* 单元 ID + PDU */

    frame[0] = (uint8_t)(transaction_id >> 8);
    frame[1] = (uint8_t)(transaction_id & 0xFFU);
    frame[2] = 0x00;
    frame[3] = 0x00;
    frame[4] = (uint8_t)(length >> 8);
    frame[5] = (uint8_t)(length & 0xFFU);
    frame[6] = unit_id;
    memcpy(&frame[7], pdu, pdu_len);

    return mb_full_slave_handle_tcp(slave, frame, (uint16_t)(pdu_len + 7U));
}

/* 断言捕获到的 RTU 响应帧 PDU 段逐字节相等，且尾部 CRC 正确。 */
static void assert_rtu_response(const uint8_t *expected, uint16_t expected_len)
{
    uint16_t crc;

    TEST_ASSERT_EQUAL_UINT(1U, g_tx_count);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT16(expected_len, g_tx_len);
    TEST_ASSERT_EQUAL_MEMORY(expected, g_tx, expected_len);
    crc = mb_full_crc16(g_tx, (uint16_t)(g_tx_len - 2U));
    TEST_ASSERT_EQUAL_UINT8((uint8_t)(crc & 0xFFU), g_tx[g_tx_len - 2U]);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)(crc >> 8), g_tx[g_tx_len - 1U]);
}

static void assert_no_response(void)
{
    TEST_ASSERT_EQUAL_UINT32(0U, g_tx_count);
}

static void assert_exception(uint8_t func, uint8_t code)
{
    uint8_t expected[3];

    expected[0] = UNIT_ID;
    expected[1] = (uint8_t)(func | 0x80U);
    expected[2] = code;
    assert_rtu_response(expected, 3);
}

/* 线圈区按位打包：位 0 对应区域起始地址。 */
static void test_slave_init_and_param_guards(void)
{
    mb_full_slave_t slave;
    uint16_t holding[2];

    memset(&slave, 0xA5, sizeof(slave));
    TEST_ASSERT_EQUAL(MB_FULL_OK, mb_full_slave_init(&slave, UNIT_ID, MB_MODE_RTU));
    TEST_ASSERT_TRUE(slave.initialized);
    TEST_ASSERT_EQUAL_UINT8(UNIT_ID, slave.slave_id);
    TEST_ASSERT_EQUAL_INT(MB_MODE_RTU, slave.mode);
    TEST_ASSERT_EQUAL_UINT32(0U, slave.request_count);
    TEST_ASSERT_EQUAL_UINT32(0U, slave.error_count);
    TEST_ASSERT_EQUAL_UINT32(0U, slave.crc_error_count);

    TEST_ASSERT_EQUAL(MB_FULL_INVALID_PARAM, mb_full_slave_init(NULL, UNIT_ID, MB_MODE_RTU));
    TEST_ASSERT_EQUAL(MB_FULL_INVALID_PARAM,
                      mb_full_slave_config_holding(NULL, holding, 0, 2));
    TEST_ASSERT_EQUAL(MB_FULL_INVALID_PARAM,
                      mb_full_slave_config_holding(&slave, NULL, 0, 2));
    TEST_ASSERT_EQUAL(MB_FULL_INVALID_PARAM, mb_full_slave_config_coils(&slave, NULL, 0, 2));

    /* 没有注册发送回调时，处理帧只能是安全的空操作。 */
    capture_reset();
    TEST_ASSERT_EQUAL(MB_FULL_INVALID_PARAM, mb_full_slave_handle(&slave, NULL, 8));
}

static void test_crc16_known_vector(void)
{
    const uint8_t frame[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x0A};

    TEST_ASSERT_EQUAL_UINT16(0xCDC5U, mb_full_crc16(frame, sizeof(frame)));
}

static void test_lrc_known_vector(void)
{
    const uint8_t frame[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x0A};

    TEST_ASSERT_EQUAL_UINT8(0xF2U, mb_full_lrc(frame, sizeof(frame)));
}

static void test_ascii_encode_decode_roundtrip(void)
{
    const uint8_t payload[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x0A};
    const char *expected = ":01030000000AF2\r\n";
    uint8_t encoded[32];
    uint8_t decoded[16];
    int encoded_len;
    int decoded_len;

    encoded_len = mb_full_ascii_encode(payload, sizeof(payload), encoded);
    TEST_ASSERT_EQUAL_INT(17, encoded_len);
    TEST_ASSERT_EQUAL_MEMORY(expected, encoded, (size_t)encoded_len);

    /* 解码跳过起始符、停在 CRLF：数据段连同 LRC 一起还原，编码/解码互为逆运算。 */
    decoded_len = mb_full_ascii_decode((const uint8_t *)expected, 17, decoded);
    TEST_ASSERT_EQUAL_INT(7, decoded_len);
    TEST_ASSERT_EQUAL_MEMORY(payload, decoded, sizeof(payload));
    TEST_ASSERT_EQUAL_UINT8(mb_full_lrc(payload, sizeof(payload)), decoded[6]);
}

static void test_rtu_read_holding_returns_values(void)
{
    mb_full_slave_t slave;
    uint16_t holding[3] = {0x1234, 0xABCD, 0x0000};
    const uint8_t req[] = {UNIT_ID, MB_FUNC_READ_HOLDING_REGISTERS, 0x00, 0x01, 0x00, 0x02};
    const uint8_t expected[] = {UNIT_ID, MB_FUNC_READ_HOLDING_REGISTERS, 0x04,
                                0xAB, 0xCD, 0x00, 0x00};

    capture_reset();
    mb_full_slave_init(&slave, UNIT_ID, MB_MODE_RTU);
    mb_full_slave_config_holding(&slave, holding, 0, 3);
    mb_full_slave_set_uart(&slave, capture_send);

    TEST_ASSERT_EQUAL(MB_FULL_OK, rtu_request(&slave, req, sizeof(req)));
    TEST_ASSERT_EQUAL_UINT16(9U, g_tx_len);
    assert_rtu_response(expected, sizeof(expected));
    TEST_ASSERT_EQUAL_UINT32(1U, slave.request_count);
    TEST_ASSERT_EQUAL_UINT32(0U, slave.error_count);
}

static void test_rtu_read_holding_rejects_bad_counts(void)
{
    mb_full_slave_t slave;
    uint16_t holding[4] = {1, 2, 3, 4};
    const uint8_t zero_count[] = {UNIT_ID, MB_FUNC_READ_HOLDING_REGISTERS, 0, 0, 0, 0};
    const uint8_t overrun[] = {UNIT_ID, MB_FUNC_READ_HOLDING_REGISTERS, 0, 1, 0, 4};
    const uint8_t above_area[] = {UNIT_ID, MB_FUNC_READ_HOLDING_REGISTERS, 0, 4, 0, 1};
    const uint8_t too_many[] = {UNIT_ID, MB_FUNC_READ_HOLDING_REGISTERS, 0, 0, 0, 126};

    capture_reset();
    mb_full_slave_init(&slave, UNIT_ID, MB_MODE_RTU);
    mb_full_slave_config_holding(&slave, holding, 0, 4);
    mb_full_slave_set_uart(&slave, capture_send);

    TEST_ASSERT_EQUAL(MB_FULL_OK, rtu_request(&slave, zero_count, sizeof(zero_count)));
    assert_exception(MB_FUNC_READ_HOLDING_REGISTERS, MB_ERR_ILLEGAL_DATA_VALUE);

    capture_reset();
    TEST_ASSERT_EQUAL(MB_FULL_OK, rtu_request(&slave, overrun, sizeof(overrun)));
    assert_exception(MB_FUNC_READ_HOLDING_REGISTERS, MB_ERR_ILLEGAL_DATA_ADDRESS);

    capture_reset();
    TEST_ASSERT_EQUAL(MB_FULL_OK, rtu_request(&slave, above_area, sizeof(above_area)));
    assert_exception(MB_FUNC_READ_HOLDING_REGISTERS, MB_ERR_ILLEGAL_DATA_ADDRESS);

    capture_reset();
    TEST_ASSERT_EQUAL(MB_FULL_OK, rtu_request(&slave, too_many, sizeof(too_many)));
    assert_exception(MB_FUNC_READ_HOLDING_REGISTERS, MB_ERR_ILLEGAL_DATA_VALUE);

    TEST_ASSERT_EQUAL_UINT32(4U, slave.error_count);
    TEST_ASSERT_EQUAL_UINT32(0U, slave.request_count);
}

static void test_rtu_write_single_register_echoes_and_stores(void)
{
    mb_full_slave_t slave;
    uint16_t holding[2] = {0x0000, 0x1111};
    const uint8_t req[] = {UNIT_ID, MB_FUNC_WRITE_SINGLE_REGISTER, 0x00, 0x01, 0x00, 0xC8};
    const uint8_t expected[] = {UNIT_ID, MB_FUNC_WRITE_SINGLE_REGISTER, 0x00, 0x01, 0x00, 0xC8};

    capture_reset();
    mb_full_slave_init(&slave, UNIT_ID, MB_MODE_RTU);
    mb_full_slave_config_holding(&slave, holding, 0, 2);
    mb_full_slave_set_uart(&slave, capture_send);

    TEST_ASSERT_EQUAL(MB_FULL_OK, rtu_request(&slave, req, sizeof(req)));
    assert_rtu_response(expected, sizeof(expected));
    TEST_ASSERT_EQUAL_UINT16(0x00C8U, holding[1]);
    TEST_ASSERT_EQUAL_UINT16(0x0000U, holding[0]);
}

static void test_rtu_write_single_register_rejects_foreign_address(void)
{
    mb_full_slave_t slave;
    uint16_t holding[2] = {0x0022, 0x0033};
    const uint8_t req[] = {UNIT_ID, MB_FUNC_WRITE_SINGLE_REGISTER, 0x00, 0x09, 0x00, 0xC8};

    capture_reset();
    mb_full_slave_init(&slave, UNIT_ID, MB_MODE_RTU);
    mb_full_slave_config_holding(&slave, holding, 0, 2);
    mb_full_slave_set_uart(&slave, capture_send);

    TEST_ASSERT_EQUAL(MB_FULL_OK, rtu_request(&slave, req, sizeof(req)));
    assert_exception(MB_FUNC_WRITE_SINGLE_REGISTER, MB_ERR_ILLEGAL_DATA_ADDRESS);
    TEST_ASSERT_EQUAL_UINT16(0x0022U, holding[0]);
    TEST_ASSERT_EQUAL_UINT16(0x0033U, holding[1]);
}

static void test_rtu_write_multiple_registers(void)
{
    mb_full_slave_t slave;
    uint16_t holding[4] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
    const uint8_t req[] = {UNIT_ID, MB_FUNC_WRITE_MULTIPLE_REGISTERS, 0x00, 0x01, 0x00, 0x02,
                           0x04, 0xAA, 0xAA, 0xBB, 0xBB};
    const uint8_t expected[] = {UNIT_ID, MB_FUNC_WRITE_MULTIPLE_REGISTERS, 0x00, 0x01, 0x00, 0x02};

    capture_reset();
    mb_full_slave_init(&slave, UNIT_ID, MB_MODE_RTU);
    mb_full_slave_config_holding(&slave, holding, 0, 4);
    mb_full_slave_set_uart(&slave, capture_send);

    TEST_ASSERT_EQUAL(MB_FULL_OK, rtu_request(&slave, req, sizeof(req)));
    assert_rtu_response(expected, sizeof(expected));
    TEST_ASSERT_EQUAL_UINT16(0xFFFFU, holding[0]);
    TEST_ASSERT_EQUAL_UINT16(0xAAAAU, holding[1]);
    TEST_ASSERT_EQUAL_UINT16(0xBBBBU, holding[2]);
}

static void test_rtu_write_multiple_registers_rejects_bad_frames(void)
{
    mb_full_slave_t slave;
    uint16_t holding[4] = {0, 0, 0, 0};
    /* byte count 说 4，实际只带了 2 个数据字节。 */
    const uint8_t truncated[] = {UNIT_ID, MB_FUNC_WRITE_MULTIPLE_REGISTERS, 0x00, 0x00, 0x00, 0x02,
                                 0x04, 0xAA, 0xAA};
    /* byte count 与寄存器数量不一致。 */
    const uint8_t mismatch[] = {UNIT_ID, MB_FUNC_WRITE_MULTIPLE_REGISTERS, 0x00, 0x00, 0x00, 0x02,
                                0x02, 0xAA, 0xAA};

    capture_reset();
    mb_full_slave_init(&slave, UNIT_ID, MB_MODE_RTU);
    mb_full_slave_config_holding(&slave, holding, 0, 4);
    mb_full_slave_set_uart(&slave, capture_send);

    TEST_ASSERT_EQUAL(MB_FULL_OK, rtu_request(&slave, truncated, sizeof(truncated)));
    assert_exception(MB_FUNC_WRITE_MULTIPLE_REGISTERS, MB_ERR_ILLEGAL_DATA_VALUE);

    capture_reset();
    TEST_ASSERT_EQUAL(MB_FULL_OK, rtu_request(&slave, mismatch, sizeof(mismatch)));
    assert_exception(MB_FUNC_WRITE_MULTIPLE_REGISTERS, MB_ERR_ILLEGAL_DATA_VALUE);

    TEST_ASSERT_EQUAL_UINT16(0U, holding[0]);
    TEST_ASSERT_EQUAL_UINT16(0U, holding[1]);
}

static void test_rtu_read_input_registers(void)
{
    mb_full_slave_t slave;
    uint16_t input[2] = {0x0102, 0x0304};
    const uint8_t req[] = {UNIT_ID, MB_FUNC_READ_INPUT_REGISTERS, 0x00, 0x00, 0x00, 0x02};
    const uint8_t expected[] = {UNIT_ID, MB_FUNC_READ_INPUT_REGISTERS, 0x04,
                                0x01, 0x02, 0x03, 0x04};

    capture_reset();
    mb_full_slave_init(&slave, UNIT_ID, MB_MODE_RTU);
    mb_full_slave_config_input(&slave, input, 0, 2);
    mb_full_slave_set_uart(&slave, capture_send);

    TEST_ASSERT_EQUAL(MB_FULL_OK, rtu_request(&slave, req, sizeof(req)));
    assert_rtu_response(expected, sizeof(expected));
}

static void test_rtu_read_coils_packs_bits(void)
{
    mb_full_slave_t slave;
    uint8_t coils[2] = {0x09, 0x02}; /* 线圈 0、3、9 为 ON */
    const uint8_t req[] = {UNIT_ID, MB_FUNC_READ_COILS, 0x00, 0x00, 0x00, 0x0A};
    const uint8_t expected[] = {UNIT_ID, MB_FUNC_READ_COILS, 0x02, 0x09, 0x02};

    capture_reset();
    mb_full_slave_init(&slave, UNIT_ID, MB_MODE_RTU);
    mb_full_slave_config_coils(&slave, coils, 0, 16);
    mb_full_slave_set_uart(&slave, capture_send);

    TEST_ASSERT_EQUAL(MB_FULL_OK, rtu_request(&slave, req, sizeof(req)));
    assert_rtu_response(expected, sizeof(expected));
}

static void test_rtu_write_single_coil_sets_and_clears(void)
{
    mb_full_slave_t slave;
    uint8_t coils[2] = {0x00, 0x00};
    const uint8_t set_bit3[] = {UNIT_ID, MB_FUNC_WRITE_SINGLE_COIL, 0x00, 0x03, 0xFF, 0x00};
    const uint8_t clr_bit3[] = {UNIT_ID, MB_FUNC_WRITE_SINGLE_COIL, 0x00, 0x03, 0x00, 0x00};
    const uint8_t set_bit9[] = {UNIT_ID, MB_FUNC_WRITE_SINGLE_COIL, 0x00, 0x09, 0xFF, 0x00};

    capture_reset();
    mb_full_slave_init(&slave, UNIT_ID, MB_MODE_RTU);
    mb_full_slave_config_coils(&slave, coils, 0, 16);
    mb_full_slave_set_uart(&slave, capture_send);

    TEST_ASSERT_EQUAL(MB_FULL_OK, rtu_request(&slave, set_bit3, sizeof(set_bit3)));
    TEST_ASSERT_EQUAL_UINT8(0x08U, coils[0]);
    TEST_ASSERT_EQUAL_UINT8(0x00U, coils[1]);

    capture_reset();
    TEST_ASSERT_EQUAL(MB_FULL_OK, rtu_request(&slave, clr_bit3, sizeof(clr_bit3)));
    TEST_ASSERT_EQUAL_UINT8(0x00U, coils[0]);

    capture_reset();
    TEST_ASSERT_EQUAL(MB_FULL_OK, rtu_request(&slave, set_bit9, sizeof(set_bit9)));
    TEST_ASSERT_EQUAL_UINT8(0x02U, coils[1]);
}

static void test_rtu_write_single_coil_rejects_invalid_value(void)
{
    mb_full_slave_t slave;
    uint8_t coils[1] = {0x00};
    const uint8_t req[] = {UNIT_ID, MB_FUNC_WRITE_SINGLE_COIL, 0x00, 0x00, 0x12, 0x34};

    capture_reset();
    mb_full_slave_init(&slave, UNIT_ID, MB_MODE_RTU);
    mb_full_slave_config_coils(&slave, coils, 0, 8);
    mb_full_slave_set_uart(&slave, capture_send);

    TEST_ASSERT_EQUAL(MB_FULL_OK, rtu_request(&slave, req, sizeof(req)));
    assert_exception(MB_FUNC_WRITE_SINGLE_COIL, MB_ERR_ILLEGAL_DATA_VALUE);
    TEST_ASSERT_EQUAL_UINT8(0x00U, coils[0]);
}

static void test_rtu_write_multiple_coils(void)
{
    mb_full_slave_t slave;
    uint8_t coils[2] = {0xFF, 0xFF};
    const uint8_t req[] = {UNIT_ID, MB_FUNC_WRITE_MULTIPLE_COILS, 0x00, 0x00, 0x00, 0x0A,
                           0x02, 0x05, 0x00};
    const uint8_t expected[] = {UNIT_ID, MB_FUNC_WRITE_MULTIPLE_COILS, 0x00, 0x00, 0x00, 0x0A};

    capture_reset();
    mb_full_slave_init(&slave, UNIT_ID, MB_MODE_RTU);
    mb_full_slave_config_coils(&slave, coils, 0, 16);
    mb_full_slave_set_uart(&slave, capture_send);

    TEST_ASSERT_EQUAL(MB_FULL_OK, rtu_request(&slave, req, sizeof(req)));
    assert_rtu_response(expected, sizeof(expected));
    TEST_ASSERT_EQUAL_UINT8(0x05U, coils[0]);
    /* 只请求了 10 个位，字节 1 的 bit2..7（线圈 10..15）不在请求范围内，保持原值。 */
    TEST_ASSERT_EQUAL_UINT8(0xFCU, coils[1]);
}

static void test_rtu_read_discrete_inputs(void)
{
    mb_full_slave_t slave;
    uint8_t discrete[1] = {0x81};
    const uint8_t req[] = {UNIT_ID, MB_FUNC_READ_DISCRETE_INPUTS, 0x00, 0x00, 0x00, 0x08};
    const uint8_t expected[] = {UNIT_ID, MB_FUNC_READ_DISCRETE_INPUTS, 0x01, 0x81};

    capture_reset();
    mb_full_slave_init(&slave, UNIT_ID, MB_MODE_RTU);
    mb_full_slave_config_discrete(&slave, discrete, 0, 8);
    mb_full_slave_set_uart(&slave, capture_send);

    TEST_ASSERT_EQUAL(MB_FULL_OK, rtu_request(&slave, req, sizeof(req)));
    assert_rtu_response(expected, sizeof(expected));
}

static void test_rtu_exception_for_unknown_function(void)
{
    mb_full_slave_t slave;
    const uint8_t req[] = {UNIT_ID, 0x42, 0x00, 0x00};

    capture_reset();
    mb_full_slave_init(&slave, UNIT_ID, MB_MODE_RTU);
    mb_full_slave_set_uart(&slave, capture_send);

    TEST_ASSERT_EQUAL(MB_FULL_OK, rtu_request(&slave, req, sizeof(req)));
    assert_exception(0x42, MB_ERR_ILLEGAL_FUNCTION);
    TEST_ASSERT_EQUAL_UINT32(1U, slave.error_count);
}

static void test_rtu_crc_error_is_silent(void)
{
    mb_full_slave_t slave;
    uint8_t req[] = {UNIT_ID, MB_FUNC_READ_HOLDING_REGISTERS, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00};

    capture_reset();
    mb_full_slave_init(&slave, UNIT_ID, MB_MODE_RTU);
    mb_full_slave_set_uart(&slave, capture_send);

    TEST_ASSERT_EQUAL(MB_FULL_CRC_ERROR, mb_full_slave_handle(&slave, req, sizeof(req)));
    assert_no_response();
    TEST_ASSERT_EQUAL_UINT32(1U, slave.crc_error_count);
    TEST_ASSERT_EQUAL_UINT32(1U, slave.error_count);
    TEST_ASSERT_EQUAL_UINT32(0U, slave.request_count);
}

static void test_rtu_foreign_slave_id_is_silent(void)
{
    mb_full_slave_t slave;
    uint16_t holding[1] = {0x1234};
    const uint8_t req[] = {0x22, MB_FUNC_READ_HOLDING_REGISTERS, 0x00, 0x00, 0x00, 0x01};

    capture_reset();
    mb_full_slave_init(&slave, UNIT_ID, MB_MODE_RTU);
    mb_full_slave_config_holding(&slave, holding, 0, 1);
    mb_full_slave_set_uart(&slave, capture_send);

    TEST_ASSERT_EQUAL(MB_FULL_OK, rtu_request(&slave, req, sizeof(req)));
    assert_no_response();
    TEST_ASSERT_EQUAL_UINT32(0U, slave.request_count);
    TEST_ASSERT_EQUAL_UINT32(0U, slave.error_count);
}

static void test_rtu_short_frame_rejected(void)
{
    mb_full_slave_t slave;
    const uint8_t req[] = {UNIT_ID, MB_FUNC_READ_COILS, 0x00};

    capture_reset();
    mb_full_slave_init(&slave, UNIT_ID, MB_MODE_RTU);
    mb_full_slave_set_uart(&slave, capture_send);

    TEST_ASSERT_EQUAL(MB_FULL_INVALID_PARAM, mb_full_slave_handle(&slave, req, sizeof(req)));
    assert_no_response();
}

static void test_rtu_read_write_multiple_applies_write_before_read(void)
{
    mb_full_slave_t slave;
    uint16_t holding[3] = {0x0001, 0x0002, 0x0003};
    const uint8_t req[] = {UNIT_ID, MB_FUNC_READ_WRITE_MULTIPLE,
                           0x00, 0x00, 0x00, 0x03, /* 读 3 个寄存器 @0 */
                           0x00, 0x01, 0x00, 0x02, /* 写 2 个寄存器 @1 */
                           0x04, 0xAA, 0xAA, 0xBB, 0xBB};
    const uint8_t expected[] = {UNIT_ID, MB_FUNC_READ_WRITE_MULTIPLE, 0x06,
                                0x00, 0x01, 0xAA, 0xAA, 0xBB, 0xBB};

    capture_reset();
    mb_full_slave_init(&slave, UNIT_ID, MB_MODE_RTU);
    mb_full_slave_config_holding(&slave, holding, 0, 3);
    mb_full_slave_set_uart(&slave, capture_send);

    TEST_ASSERT_EQUAL(MB_FULL_OK, rtu_request(&slave, req, sizeof(req)));
    assert_rtu_response(expected, sizeof(expected));
    TEST_ASSERT_EQUAL_UINT16(0xAAAAU, holding[1]);
    TEST_ASSERT_EQUAL_UINT16(0xBBBBU, holding[2]);
}

static void test_rtu_read_device_id_basic(void)
{
    mb_full_slave_t slave;
    const uint8_t req[] = {UNIT_ID, MB_FUNC_READ_DEVICE_ID, 0x0E, 0x01, 0x00};
    const uint8_t expected[] = {UNIT_ID, MB_FUNC_READ_DEVICE_ID, 0x0E, 0x01,
                                0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x64};

    capture_reset();
    mb_full_slave_init(&slave, UNIT_ID, MB_MODE_RTU);
    slave.device_id.device_id = 0x64;
    mb_full_slave_set_uart(&slave, capture_send);

    TEST_ASSERT_EQUAL(MB_FULL_OK, rtu_request(&slave, req, sizeof(req)));
    assert_rtu_response(expected, sizeof(expected));
}

static void test_rtu_read_device_id_rejects_foreign_mei(void)
{
    mb_full_slave_t slave;
    const uint8_t req[] = {UNIT_ID, MB_FUNC_READ_DEVICE_ID, 0x0D, 0x01, 0x00};

    capture_reset();
    mb_full_slave_init(&slave, UNIT_ID, MB_MODE_RTU);
    mb_full_slave_set_uart(&slave, capture_send);

    TEST_ASSERT_EQUAL(MB_FULL_OK, rtu_request(&slave, req, sizeof(req)));
    assert_exception(MB_FUNC_READ_DEVICE_ID, MB_ERR_ILLEGAL_DATA_VALUE);
}

static void test_tcp_read_holding_builds_mbap(void)
{
    mb_full_slave_t slave;
    uint16_t holding[2] = {0x1234, 0x5678};
    const uint8_t req[] = {MB_FUNC_READ_HOLDING_REGISTERS, 0x00, 0x00, 0x00, 0x02};
    const uint8_t expected[] = {0x00, 0x2A, 0x00, 0x00, 0x00, 0x07, UNIT_ID,
                                MB_FUNC_READ_HOLDING_REGISTERS, 0x04,
                                0x12, 0x34, 0x56, 0x78};

    capture_reset();
    mb_full_slave_init(&slave, UNIT_ID, MB_MODE_TCP);
    mb_full_slave_config_holding(&slave, holding, 0, 2);
    mb_full_slave_set_tcp(&slave, capture_send);

    TEST_ASSERT_EQUAL(MB_FULL_OK, tcp_request(&slave, 0x002A, UNIT_ID, req, sizeof(req)));
    TEST_ASSERT_EQUAL_UINT(1U, g_tx_count);
    TEST_ASSERT_EQUAL_UINT16(sizeof(expected), g_tx_len);
    TEST_ASSERT_EQUAL_MEMORY(expected, g_tx, sizeof(expected));
    TEST_ASSERT_EQUAL_UINT32(1U, slave.request_count);
}

static void test_tcp_exception_frame(void)
{
    mb_full_slave_t slave;
    uint16_t holding[1] = {0x0000};
    const uint8_t req[] = {MB_FUNC_READ_HOLDING_REGISTERS, 0x00, 0x08, 0x00, 0x01};
    const uint8_t expected[] = {0x00, 0x07, 0x00, 0x00, 0x00, 0x03, UNIT_ID,
                                (uint8_t)(MB_FUNC_READ_HOLDING_REGISTERS | 0x80),
                                MB_ERR_ILLEGAL_DATA_ADDRESS};

    capture_reset();
    mb_full_slave_init(&slave, UNIT_ID, MB_MODE_TCP);
    mb_full_slave_config_holding(&slave, holding, 0, 1);
    mb_full_slave_set_tcp(&slave, capture_send);

    TEST_ASSERT_EQUAL(MB_FULL_OK, tcp_request(&slave, 0x0007, UNIT_ID, req, sizeof(req)));
    TEST_ASSERT_EQUAL_UINT(1U, g_tx_count);
    TEST_ASSERT_EQUAL_UINT16(sizeof(expected), g_tx_len);
    TEST_ASSERT_EQUAL_MEMORY(expected, g_tx, sizeof(expected));
    TEST_ASSERT_EQUAL_UINT32(1U, slave.error_count);
}

static void test_tcp_rejects_bad_protocol_and_length(void)
{
    mb_full_slave_t slave;
    uint16_t holding[1] = {0x1234};
    uint8_t frame[16];
    const uint8_t pdu[] = {MB_FUNC_READ_HOLDING_REGISTERS, 0x00, 0x00, 0x00, 0x01};

    capture_reset();
    mb_full_slave_init(&slave, UNIT_ID, MB_MODE_TCP);
    mb_full_slave_config_holding(&slave, holding, 0, 1);
    mb_full_slave_set_tcp(&slave, capture_send);

    /* 协议 ID 非 0：不是 Modbus。 */
    memset(frame, 0, sizeof(frame));
    frame[0] = 0x00;
    frame[1] = 0x01;
    frame[2] = 0x00;
    frame[3] = 0x01;
    frame[4] = 0x00;
    frame[5] = 0x06;
    frame[6] = UNIT_ID;
    memcpy(&frame[7], pdu, sizeof(pdu));
    TEST_ASSERT_EQUAL(MB_FULL_INVALID_PARAM,
                      mb_full_slave_handle_tcp(&slave, frame, (uint16_t)(7U + sizeof(pdu))));
    assert_no_response();

    /* MBAP 长度与实到字节不一致。 */
    memset(frame, 0, sizeof(frame));
    frame[1] = 0x01;
    frame[4] = 0x00;
    frame[5] = 0x09;
    frame[6] = UNIT_ID;
    memcpy(&frame[7], pdu, sizeof(pdu));
    TEST_ASSERT_EQUAL(MB_FULL_INVALID_PARAM,
                      mb_full_slave_handle_tcp(&slave, frame, (uint16_t)(7U + sizeof(pdu))));
    assert_no_response();
}

static void test_tcp_foreign_unit_id_is_silent(void)
{
    mb_full_slave_t slave;
    uint16_t holding[1] = {0x1234};
    const uint8_t req[] = {MB_FUNC_READ_HOLDING_REGISTERS, 0x00, 0x00, 0x00, 0x01};

    capture_reset();
    mb_full_slave_init(&slave, UNIT_ID, MB_MODE_TCP);
    mb_full_slave_config_holding(&slave, holding, 0, 1);
    mb_full_slave_set_tcp(&slave, capture_send);

    TEST_ASSERT_EQUAL(MB_FULL_OK, tcp_request(&slave, 0x0001, 0x22, req, sizeof(req)));
    assert_no_response();
    TEST_ASSERT_EQUAL_UINT32(0U, slave.request_count);
}

static void test_error_string_covers_public_codes(void)
{
    TEST_ASSERT_EQUAL_STRING("OK", mb_full_error_string(MB_FULL_OK));
    TEST_ASSERT_EQUAL_STRING("CRC Error", mb_full_error_string(MB_FULL_CRC_ERROR));
    TEST_ASSERT_EQUAL_STRING("Unknown", mb_full_error_string(12345));
    TEST_ASSERT_EQUAL_STRING("1.0.0", mb_full_version());
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
    RUN_TEST(test_slave_init_and_param_guards);
    RUN_TEST(test_crc16_known_vector);
    RUN_TEST(test_lrc_known_vector);
    RUN_TEST(test_ascii_encode_decode_roundtrip);
    RUN_TEST(test_rtu_read_holding_returns_values);
    RUN_TEST(test_rtu_read_holding_rejects_bad_counts);
    RUN_TEST(test_rtu_write_single_register_echoes_and_stores);
    RUN_TEST(test_rtu_write_single_register_rejects_foreign_address);
    RUN_TEST(test_rtu_write_multiple_registers);
    RUN_TEST(test_rtu_write_multiple_registers_rejects_bad_frames);
    RUN_TEST(test_rtu_read_input_registers);
    RUN_TEST(test_rtu_read_coils_packs_bits);
    RUN_TEST(test_rtu_write_single_coil_sets_and_clears);
    RUN_TEST(test_rtu_write_single_coil_rejects_invalid_value);
    RUN_TEST(test_rtu_write_multiple_coils);
    RUN_TEST(test_rtu_read_discrete_inputs);
    RUN_TEST(test_rtu_exception_for_unknown_function);
    RUN_TEST(test_rtu_crc_error_is_silent);
    RUN_TEST(test_rtu_foreign_slave_id_is_silent);
    RUN_TEST(test_rtu_short_frame_rejected);
    RUN_TEST(test_rtu_read_write_multiple_applies_write_before_read);
    RUN_TEST(test_rtu_read_device_id_basic);
    RUN_TEST(test_rtu_read_device_id_rejects_foreign_mei);
    RUN_TEST(test_tcp_read_holding_builds_mbap);
    RUN_TEST(test_tcp_exception_frame);
    RUN_TEST(test_tcp_rejects_bad_protocol_and_length);
    RUN_TEST(test_tcp_foreign_unit_id_is_silent);
    RUN_TEST(test_error_string_covers_public_codes);
    return UNITY_END();
}
