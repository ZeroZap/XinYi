/**
 * @file test_nano_modbus_full.c
 * @brief Host guard for the `NANO_MB_USE_TINY=0` (Full) branch of the unified API.
 *
 * `components/net/inc/nano_modbus.h` is the "unified API" umbrella: one set of
 * `nano_mb_*` names over either the Tiny (RTU only) or the Full (RTU + TCP +
 * ASCII) backend. The Full branch had never compiled: `mb_slave_config_t` was
 * used before it was defined, and `#define MB_FUNC_READ_COILS MB_FUNC_READ_COILS`
 * redefined a constant `mb_full.h` already provides.
 *
 * The target is built with `-DNANO_MB_USE_TINY=0`, so this file is the guard
 * that the Full branch of the umbrella stays compilable and behaves like the
 * Tiny branch it mirrors. The master is exercised in a true loopback against
 * the slave from the same `mb_full.c` translation unit.
 */

#include "nano_modbus.h"

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
    memcpy(data, g_slave_tx, g_slave_tx_len);
    return (int)g_slave_tx_len;
}

static void loopback_init(void)
{
    memset(&g_slave, 0, sizeof(g_slave));
    memset(&g_master, 0, sizeof(g_master));
    memset(g_master_tx, 0, sizeof(g_master_tx));
    memset(g_slave_tx, 0, sizeof(g_slave_tx));
    g_master_tx_len = 0;
    g_slave_tx_len = 0;

    TEST_ASSERT_EQUAL(MB_FULL_OK, mb_full_slave_init(&g_slave, UNIT_ID, MB_MODE_RTU));
    TEST_ASSERT_EQUAL(MB_FULL_OK, mb_full_master_init(&g_master, MB_MODE_RTU));
    mb_full_slave_set_uart(&g_slave, slave_send_capture);
    mb_full_master_set_uart(&g_master, master_send_capture, master_uart_recv);
}

static void test_compat_full_crc_and_error_string(void)
{
    const uint8_t frame[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x0A};

    /* 与 Tiny 分支同一条已知向量：两个后端必须给出同一个 CRC。 */
    TEST_ASSERT_EQUAL_UINT16(0xCDC5U, nano_mb_crc16(frame, sizeof(frame)));
    TEST_ASSERT_EQUAL_STRING("OK", nano_mb_error_string(NANO_MB_OK));
    TEST_ASSERT_EQUAL_STRING("Error", nano_mb_error_string(NANO_MB_ERROR));
    TEST_ASSERT_EQUAL_STRING("Invalid Parameter", nano_mb_error_string(NANO_MB_INVALID_PARAM));
    TEST_ASSERT_EQUAL_STRING("Timeout", nano_mb_error_string(NANO_MB_TIMEOUT));
    TEST_ASSERT_EQUAL_STRING("CRC Error", nano_mb_error_string(NANO_MB_CRC_ERROR));
}

static void test_compat_full_slave_init_and_poll(void)
{
    mb_slave_config_t cfg = {.slave_id = 7};
    uint16_t holding[2] = {0x1234, 0xABCD};
    uint8_t req[8] = {7, MB_FUNC_READ_HOLDING, 0, 0, 0, 2, 0, 0};
    uint16_t crc;

    loopback_init();

    /* nano_mb_slave_init() 走统一 API：从配置里取从站 ID 并把从站置为已初始化。 */
    TEST_ASSERT_EQUAL(NANO_MB_OK, nano_mb_slave_init(&g_slave, &cfg));
    TEST_ASSERT_TRUE(g_slave.initialized);
    TEST_ASSERT_EQUAL_UINT8(7U, g_slave.slave_id);
    TEST_ASSERT_EQUAL_INT(MB_MODE_RTU, g_slave.mode);

    /* 统一 API 不携带数据缓冲指针（mb_slave_config_t 里没有），数据区仍由后端
     * 自己的配置入口挂上——与 Tiny 分支的形状一致。 */
    TEST_ASSERT_EQUAL(MB_FULL_OK,
                      mb_full_slave_config_holding(&g_slave, holding, 0, 2));

    crc = nano_mb_crc16(req, 6);
    req[6] = (uint8_t)(crc & 0xFFU);
    req[7] = (uint8_t)(crc >> 8);

    TEST_ASSERT_EQUAL(NANO_MB_OK, nano_mb_slave_poll(&g_slave, req, sizeof(req)));
    TEST_ASSERT_EQUAL_UINT16(9U, g_slave_tx_len);
    TEST_ASSERT_EQUAL_UINT8(7U, g_slave_tx[0]);
    TEST_ASSERT_EQUAL_UINT8(MB_FUNC_READ_HOLDING, g_slave_tx[1]);
    TEST_ASSERT_EQUAL_UINT8(4U, g_slave_tx[2]);
    TEST_ASSERT_EQUAL_UINT8(0x12U, g_slave_tx[3]);
    TEST_ASSERT_EQUAL_UINT8(0x34U, g_slave_tx[4]);
    TEST_ASSERT_EQUAL_UINT8(0xABU, g_slave_tx[5]);
    TEST_ASSERT_EQUAL_UINT8(0xCDU, g_slave_tx[6]);
    TEST_ASSERT_EQUAL_UINT16(nano_mb_crc16(g_slave_tx, 7),
                             ((uint16_t)g_slave_tx[8] << 8 | g_slave_tx[7]));

    TEST_ASSERT_EQUAL(NANO_MB_OK, nano_mb_slave_deinit(&g_slave));
    TEST_ASSERT_FALSE(g_slave.initialized);
}

static void test_compat_full_master_read_holding_loopback(void)
{
    uint16_t holding[3] = {0x1234, 0xABCD, 0x0000};
    uint16_t data[2] = {0, 0};

    loopback_init();
    TEST_ASSERT_EQUAL(NANO_MB_OK, nano_mb_master_init(&g_master));
    TEST_ASSERT_TRUE(g_master.initialized);
    TEST_ASSERT_EQUAL(MB_FULL_OK, mb_full_slave_config_holding(&g_slave, holding, 0, 3));

    TEST_ASSERT_EQUAL(NANO_MB_OK,
                      nano_mb_master_read_holding(&g_master, UNIT_ID, 1, 2, data, 999));
    TEST_ASSERT_EQUAL_UINT16(0xABCD, data[0]);
    TEST_ASSERT_EQUAL_UINT16(0x0000, data[1]);

    /* 主站发出的 RTU 帧 = 单元 ID + 功能码 + 地址 + 数量 + CRC，共 8 字节。 */
    TEST_ASSERT_EQUAL_UINT16(8U, g_master_tx_len);
    TEST_ASSERT_EQUAL_UINT8(UNIT_ID, g_master_tx[0]);
    TEST_ASSERT_EQUAL_UINT8(MB_FUNC_READ_HOLDING, g_master_tx[1]);

    TEST_ASSERT_EQUAL_UINT32(1U, g_master.request_count);
    TEST_ASSERT_EQUAL_UINT32(0U, g_master.error_count);
}

static void test_compat_full_master_write_reg_loopback(void)
{
    uint16_t holding[2] = {0x0000, 0x1111};

    loopback_init();
    TEST_ASSERT_EQUAL(NANO_MB_OK, nano_mb_master_init(&g_master));
    TEST_ASSERT_EQUAL(MB_FULL_OK, mb_full_slave_config_holding(&g_slave, holding, 0, 2));

    TEST_ASSERT_EQUAL(NANO_MB_OK,
                      nano_mb_master_write_reg(&g_master, UNIT_ID, 1, 0x00C8, 999));
    TEST_ASSERT_EQUAL_UINT16(0x00C8, holding[1]);
    TEST_ASSERT_EQUAL_UINT16(0x0000, holding[0]);
}

static void test_compat_full_master_write_multi_regs_loopback(void)
{
    uint16_t holding[3] = {0x0000, 0x0000, 0x0000};
    uint16_t write_data[2] = {0xAAAA, 0xBBBB};

    loopback_init();
    TEST_ASSERT_EQUAL(NANO_MB_OK, nano_mb_master_init(&g_master));
    TEST_ASSERT_EQUAL(MB_FULL_OK, mb_full_slave_config_holding(&g_slave, holding, 0, 3));

    TEST_ASSERT_EQUAL(NANO_MB_OK,
                      nano_mb_master_write_multi_regs(&g_master, UNIT_ID, 1, 2,
                                                      write_data, 999));
    TEST_ASSERT_EQUAL_UINT16(0xAAAA, holding[1]);
    TEST_ASSERT_EQUAL_UINT16(0xBBBB, holding[2]);
    TEST_ASSERT_EQUAL_UINT16(0x0000, holding[0]);
}

static void test_compat_full_master_read_coils_loopback(void)
{
    uint8_t coils[1] = {0x05};
    uint8_t data[1] = {0};

    loopback_init();
    TEST_ASSERT_EQUAL(NANO_MB_OK, nano_mb_master_init(&g_master));
    TEST_ASSERT_EQUAL(MB_FULL_OK, mb_full_slave_config_coils(&g_slave, coils, 0, 8));

    TEST_ASSERT_EQUAL(NANO_MB_OK,
                      nano_mb_master_read_coils(&g_master, UNIT_ID, 0, 8, data, 999));
    TEST_ASSERT_EQUAL_UINT8(0x05, data[0]);
}

static void test_compat_full_param_guards(void)
{
    mb_slave_t slave;
    mb_master_t master;
    uint16_t data[1] = {0};

    TEST_ASSERT_EQUAL(NANO_MB_INVALID_PARAM, nano_mb_slave_init(NULL, NULL));
    TEST_ASSERT_EQUAL(NANO_MB_INVALID_PARAM, nano_mb_slave_deinit(NULL));
    TEST_ASSERT_EQUAL(NANO_MB_INVALID_PARAM, nano_mb_master_init(NULL));
    TEST_ASSERT_EQUAL(NANO_MB_INVALID_PARAM, nano_mb_master_deinit(NULL));

    TEST_ASSERT_EQUAL(NANO_MB_OK, nano_mb_master_init(&master));
    TEST_ASSERT_EQUAL(NANO_MB_INVALID_PARAM,
                      nano_mb_master_read_holding(NULL, UNIT_ID, 0, 1, data, 0));
    TEST_ASSERT_EQUAL(NANO_MB_INVALID_PARAM,
                      nano_mb_master_read_holding(&master, UNIT_ID, 0, 0, data, 0));
    TEST_ASSERT_EQUAL(NANO_MB_INVALID_PARAM,
                      nano_mb_master_read_holding(&master, UNIT_ID, 0, 126, data, 0));
    TEST_ASSERT_EQUAL(NANO_MB_INVALID_PARAM,
                      nano_mb_master_read_holding(&master, UNIT_ID, 0, 1, NULL, 0));
    TEST_ASSERT_EQUAL(NANO_MB_INVALID_PARAM,
                      nano_mb_master_write_reg(NULL, UNIT_ID, 0, 0, 0));

    /* NULL 配置退回到从站 ID 1，与 Tiny 分支同一形状。 */
    memset(&slave, 0, sizeof(slave));
    TEST_ASSERT_EQUAL(NANO_MB_OK, nano_mb_slave_init(&slave, NULL));
    TEST_ASSERT_EQUAL_UINT8(1U, slave.slave_id);
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
    RUN_TEST(test_compat_full_crc_and_error_string);
    RUN_TEST(test_compat_full_slave_init_and_poll);
    RUN_TEST(test_compat_full_master_read_holding_loopback);
    RUN_TEST(test_compat_full_master_write_reg_loopback);
    RUN_TEST(test_compat_full_master_write_multi_regs_loopback);
    RUN_TEST(test_compat_full_master_read_coils_loopback);
    RUN_TEST(test_compat_full_param_guards);
    return UNITY_END();
}
