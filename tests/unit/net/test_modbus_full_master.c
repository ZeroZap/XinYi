/**
 * @file test_modbus_full_master.c
 * @brief Host guard for the Modbus Full master: RTU and TCP request/response contracts.
 *
 * The master is exercised in a true loopback against the slave from the same
 * `mb_full.c` translation unit: the master's transport callbacks capture its
 * outgoing frame, feed it into `mb_full_slave_handle*()`, and hand the slave's
 * response back. This is the first time the master actually answers a frame,
 * and it exposes the structural defects the wire contract hides: the MBAP
 * request header was built by overlaying a struct on the buffer (byte-swapped
 * transaction id / length on little-endian hosts), TCP responses were parsed
 * with the RTU layout, RTU responses were never CRC-checked, and
 * `read_device_id` copied the trailing CRC into the caller's buffer.
 */

#include "mb_full.h"

#include <string.h>

#include "unity.h"

#define UNIT_ID 0x11U

static mb_full_slave_t g_slave;
static mb_full_master_t g_master;

/* 主站发出的请求帧（transport send 捕获）。 */
static uint8_t g_master_tx[MB_FULL_MAX_TCP_ADU_SIZE];
static uint16_t g_master_tx_len;

/* 从站发出的响应帧（slave send 捕获），供主站 recv 回调回传。 */
static uint8_t g_slave_tx[MB_FULL_MAX_TCP_ADU_SIZE];
static uint16_t g_slave_tx_len;

/* 故障注入：把从站响应的 CRC 高字节翻转，模拟线路损坏。 */
static bool g_corrupt_rsp_crc;

static int slave_send_capture(const uint8_t *data, uint16_t len)
{
    TEST_ASSERT_LESS_OR_EQUAL_UINT(sizeof(g_slave_tx), len);
    memcpy(g_slave_tx, data, len);
    g_slave_tx_len = len;
    return (int)len;
}

static int master_send_capture(const uint8_t *data, uint16_t len)
{
    TEST_ASSERT_LESS_OR_EQUAL_UINT(sizeof(g_master_tx), len);
    memcpy(g_master_tx, data, len);
    g_master_tx_len = len;
    return (int)len;
}

/* RTU recv：把主站刚发的帧交给从站，回传从站响应。 */
static int master_uart_recv(uint8_t *data, uint16_t len, uint32_t timeout_ms)
{
    (void)timeout_ms;
    g_slave_tx_len = 0;
    (void)mb_full_slave_handle(&g_slave, g_master_tx, g_master_tx_len);
    TEST_ASSERT_GREATER_THAN_UINT(0, g_slave_tx_len);
    TEST_ASSERT_LESS_OR_EQUAL_UINT(len, g_slave_tx_len);
    if (g_corrupt_rsp_crc) {
        g_slave_tx[g_slave_tx_len - 1U] ^= 0xFFU;
    }
    memcpy(data, g_slave_tx, g_slave_tx_len);
    return (int)g_slave_tx_len;
}

/* TCP recv：把主站刚发的 MBAP 帧交给从站，回传从站响应。 */
static int master_tcp_recv(uint8_t *data, uint16_t len, uint32_t timeout_ms)
{
    (void)timeout_ms;
    g_slave_tx_len = 0;
    (void)mb_full_slave_handle_tcp(&g_slave, g_master_tx, g_master_tx_len);
    TEST_ASSERT_GREATER_THAN_UINT(0, g_slave_tx_len);
    TEST_ASSERT_LESS_OR_EQUAL_UINT(len, g_slave_tx_len);
    memcpy(data, g_slave_tx, g_slave_tx_len);
    return (int)g_slave_tx_len;
}

static void rtu_loopback_init(void)
{
    memset(&g_slave, 0, sizeof(g_slave));
    memset(&g_master, 0, sizeof(g_master));
    memset(g_master_tx, 0, sizeof(g_master_tx));
    memset(g_slave_tx, 0, sizeof(g_slave_tx));
    g_master_tx_len = 0;
    g_slave_tx_len = 0;
    g_corrupt_rsp_crc = false;

    TEST_ASSERT_EQUAL(MB_FULL_OK, mb_full_slave_init(&g_slave, UNIT_ID, MB_MODE_RTU));
    TEST_ASSERT_EQUAL(MB_FULL_OK, mb_full_master_init(&g_master, MB_MODE_RTU));
    mb_full_slave_set_uart(&g_slave, slave_send_capture);
    mb_full_master_set_uart(&g_master, master_send_capture, master_uart_recv);
}

static void tcp_loopback_init(void)
{
    memset(&g_slave, 0, sizeof(g_slave));
    memset(&g_master, 0, sizeof(g_master));
    memset(g_master_tx, 0, sizeof(g_master_tx));
    memset(g_slave_tx, 0, sizeof(g_slave_tx));
    g_master_tx_len = 0;
    g_slave_tx_len = 0;
    g_corrupt_rsp_crc = false;

    TEST_ASSERT_EQUAL(MB_FULL_OK, mb_full_slave_init(&g_slave, UNIT_ID, MB_MODE_TCP));
    TEST_ASSERT_EQUAL(MB_FULL_OK, mb_full_master_init(&g_master, MB_MODE_TCP));
    mb_full_slave_set_tcp(&g_slave, slave_send_capture);
    mb_full_master_set_tcp(&g_master, NULL, master_send_capture, master_tcp_recv, NULL);
}

static void test_master_init_and_param_guards(void)
{
    mb_full_master_t master;

    memset(&master, 0xA5, sizeof(master));
    TEST_ASSERT_EQUAL(MB_FULL_OK, mb_full_master_init(&master, MB_MODE_RTU));
    TEST_ASSERT_TRUE(master.initialized);
    TEST_ASSERT_EQUAL_INT(MB_MODE_RTU, master.mode);
    TEST_ASSERT_EQUAL_UINT(MB_FULL_TIMEOUT_MS, master.timeout_ms);

    TEST_ASSERT_EQUAL(MB_FULL_INVALID_PARAM, mb_full_master_init(NULL, MB_MODE_RTU));
    TEST_ASSERT_EQUAL(MB_FULL_INVALID_PARAM,
                      mb_full_master_read_holding(NULL, UNIT_ID, 0, 1, (uint16_t *)&(uint16_t){0}));
    TEST_ASSERT_EQUAL(MB_FULL_INVALID_PARAM,
                      mb_full_master_write_register(NULL, UNIT_ID, 0, 0));
    TEST_ASSERT_EQUAL(MB_FULL_INVALID_PARAM,
                      mb_full_master_read_device_id(NULL, UNIT_ID, MB_OBJ_BASIC, (uint8_t *)&(uint8_t){0}));

    mb_full_master_set_timeout(&master, 500);
    TEST_ASSERT_EQUAL_UINT(500, master.timeout_ms);
    mb_full_master_set_timeout(NULL, 100); /* 必须是安全空操作 */
}

static void test_rtu_read_holding_loopback(void)
{
    uint16_t holding[3] = {0x1234, 0xABCD, 0x0000};
    uint16_t data[2] = {0, 0};
    uint16_t crc;

    rtu_loopback_init();
    mb_full_slave_config_holding(&g_slave, holding, 0, 3);

    TEST_ASSERT_EQUAL(MB_FULL_OK, mb_full_master_read_holding(&g_master, UNIT_ID, 1, 2, data));
    TEST_ASSERT_EQUAL_UINT16(0xABCD, data[0]);
    TEST_ASSERT_EQUAL_UINT16(0x0000, data[1]);

    /* 主站发出的 RTU 帧 = 单元 ID + 功能码 + 地址 + 数量 + CRC，共 8 字节，CRC 正确。 */
    TEST_ASSERT_EQUAL_UINT16(8U, g_master_tx_len);
    TEST_ASSERT_EQUAL_UINT8(UNIT_ID, g_master_tx[0]);
    TEST_ASSERT_EQUAL_UINT8(MB_FUNC_READ_HOLDING_REGISTERS, g_master_tx[1]);
    crc = mb_full_crc16(g_master_tx, 6);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)(crc & 0xFFU), g_master_tx[6]);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)(crc >> 8), g_master_tx[7]);

    TEST_ASSERT_EQUAL_UINT32(1U, g_master.request_count);
    TEST_ASSERT_EQUAL_UINT32(0U, g_master.error_count);
}

static void test_rtu_master_detects_corrupt_response_crc(void)
{
    uint16_t holding[2] = {0x1111, 0x2222};
    uint16_t data[1] = {0};

    rtu_loopback_init();
    mb_full_slave_config_holding(&g_slave, holding, 0, 2);

    g_corrupt_rsp_crc = true;
    TEST_ASSERT_EQUAL(MB_FULL_CRC_ERROR,
                      mb_full_master_read_holding(&g_master, UNIT_ID, 0, 1, data));
    TEST_ASSERT_EQUAL_UINT32(0U, g_master.request_count);
    TEST_ASSERT_EQUAL_UINT32(1U, g_master.error_count);
}

static void test_rtu_write_register_loopback(void)
{
    uint16_t holding[2] = {0x0000, 0x1111};

    rtu_loopback_init();
    mb_full_slave_config_holding(&g_slave, holding, 0, 2);

    TEST_ASSERT_EQUAL(MB_FULL_OK, mb_full_master_write_register(&g_master, UNIT_ID, 1, 0x00C8));
    TEST_ASSERT_EQUAL_UINT16(0x00C8, holding[1]);
    TEST_ASSERT_EQUAL_UINT16(0x0000, holding[0]);
}

static void test_rtu_read_write_multiple_loopback(void)
{
    uint16_t holding[3] = {0x0001, 0x0002, 0x0003};
    uint16_t write_data[2] = {0xAAAA, 0xBBBB};
    uint16_t read_data[3] = {0, 0, 0};

    rtu_loopback_init();
    mb_full_slave_config_holding(&g_slave, holding, 0, 3);

    TEST_ASSERT_EQUAL(MB_FULL_OK,
                      mb_full_master_read_write_registers(&g_master, UNIT_ID,
                                                          0, 3, read_data,
                                                          1, 2, write_data));
    /* 规范：先写后读，读回的 3 个寄存器里能看到刚写的两个。 */
    TEST_ASSERT_EQUAL_UINT16(0x0001, read_data[0]);
    TEST_ASSERT_EQUAL_UINT16(0xAAAA, read_data[1]);
    TEST_ASSERT_EQUAL_UINT16(0xBBBB, read_data[2]);
}

static void test_rtu_read_device_id_no_crc_leak(void)
{
    uint8_t data[16];

    rtu_loopback_init();
    g_slave.device_id.device_id = 0x64;

    memset(data, 0xCC, sizeof(data));
    TEST_ASSERT_EQUAL(MB_FULL_OK, mb_full_master_read_device_id(&g_master, UNIT_ID,
                                                                MB_OBJ_BASIC, data));
    /* 响应体 = MEI + 读取码 + 一致性 + 后续 + 下一个 + 对象数 + 对象ID + 长度 + 值。 */
    TEST_ASSERT_EQUAL_UINT8(0x0E, data[0]);
    TEST_ASSERT_EQUAL_UINT8(0x01, data[1]);
    TEST_ASSERT_EQUAL_UINT8(0x01, data[2]);
    TEST_ASSERT_EQUAL_UINT8(0x64, data[8]);
    /* 第 10 个字节（下标 9）不再被 CRC 污染。 */
    TEST_ASSERT_EQUAL_UINT8(0xCC, data[9]);
}

static void test_tcp_read_holding_loopback_big_endian_mbap(void)
{
    uint16_t holding[2] = {0x1234, 0x5678};
    uint16_t data[2] = {0, 0};

    tcp_loopback_init();
    mb_full_slave_config_holding(&g_slave, holding, 0, 2);

    TEST_ASSERT_EQUAL(MB_FULL_OK, mb_full_master_read_holding(&g_master, UNIT_ID, 0, 2, data));
    TEST_ASSERT_EQUAL_UINT16(0x1234, data[0]);
    TEST_ASSERT_EQUAL_UINT16(0x5678, data[1]);

    /* 请求 MBAP 逐字节大端：事务 ID 0x0000、协议 0、长度 6（单元 + 5 字节 PDU）。 */
    TEST_ASSERT_EQUAL_UINT16(12U, g_master_tx_len);
    TEST_ASSERT_EQUAL_UINT8(0x00, g_master_tx[0]);
    TEST_ASSERT_EQUAL_UINT8(0x00, g_master_tx[1]);
    TEST_ASSERT_EQUAL_UINT8(0x00, g_master_tx[2]);
    TEST_ASSERT_EQUAL_UINT8(0x00, g_master_tx[3]);
    TEST_ASSERT_EQUAL_UINT8(0x00, g_master_tx[4]);
    TEST_ASSERT_EQUAL_UINT8(0x06, g_master_tx[5]);
    TEST_ASSERT_EQUAL_UINT8(UNIT_ID, g_master_tx[6]);
    TEST_ASSERT_EQUAL_UINT8(MB_FUNC_READ_HOLDING_REGISTERS, g_master_tx[7]);
}

static void test_tcp_request_transaction_id_increments_big_endian(void)
{
    uint16_t holding[1] = {0x0000};
    uint16_t data[1] = {0};

    tcp_loopback_init();
    mb_full_slave_config_holding(&g_slave, holding, 0, 1);

    TEST_ASSERT_EQUAL(MB_FULL_OK, mb_full_master_read_holding(&g_master, UNIT_ID, 0, 1, data));
    TEST_ASSERT_EQUAL_UINT8(0x00, g_master_tx[0]);
    TEST_ASSERT_EQUAL_UINT8(0x00, g_master_tx[1]);

    TEST_ASSERT_EQUAL(MB_FULL_OK, mb_full_master_read_holding(&g_master, UNIT_ID, 0, 1, data));
    /* 第二个事务 ID = 0x0001，仍是大端。 */
    TEST_ASSERT_EQUAL_UINT8(0x00, g_master_tx[0]);
    TEST_ASSERT_EQUAL_UINT8(0x01, g_master_tx[1]);
}

static void test_tcp_write_register_loopback(void)
{
    uint16_t holding[2] = {0x0000, 0x1111};

    tcp_loopback_init();
    mb_full_slave_config_holding(&g_slave, holding, 0, 2);

    TEST_ASSERT_EQUAL(MB_FULL_OK, mb_full_master_write_register(&g_master, UNIT_ID, 1, 0x00C8));
    TEST_ASSERT_EQUAL_UINT16(0x00C8, holding[1]);
}

static void test_tcp_exception_is_error(void)
{
    uint16_t holding[1] = {0x0000};
    uint16_t data[1] = {0};

    tcp_loopback_init();
    mb_full_slave_config_holding(&g_slave, holding, 0, 1);

    /* 地址越界 → 从站异常响应；主站把归一化后的功能码按异常处理。
     * 往返已完成（request_count=1），但响应是异常（error_count=1）。 */
    TEST_ASSERT_EQUAL(MB_FULL_ERROR, mb_full_master_read_holding(&g_master, UNIT_ID, 8, 1, data));
    TEST_ASSERT_EQUAL_UINT32(1U, g_master.error_count);
    TEST_ASSERT_EQUAL_UINT32(1U, g_master.request_count);
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
    RUN_TEST(test_master_init_and_param_guards);
    RUN_TEST(test_rtu_read_holding_loopback);
    RUN_TEST(test_rtu_master_detects_corrupt_response_crc);
    RUN_TEST(test_rtu_write_register_loopback);
    RUN_TEST(test_rtu_read_write_multiple_loopback);
    RUN_TEST(test_rtu_read_device_id_no_crc_leak);
    RUN_TEST(test_tcp_read_holding_loopback_big_endian_mbap);
    RUN_TEST(test_tcp_request_transaction_id_increments_big_endian);
    RUN_TEST(test_tcp_write_register_loopback);
    RUN_TEST(test_tcp_exception_is_error);
    return UNITY_END();
}
