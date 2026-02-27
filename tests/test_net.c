/**
 * @file test_net.c
 * @brief NET (Network) Component Unit Tests using Unity Framework
 * @version 1.0.0
 * @date 2026-02-28
 */

#include <stdint.h>
#include <string.h>
#include "unity.h"

/* NET headers - ISO7816 */
#include "xy_iso7816.h"

/* NET headers - Modbus */
#include "mb_slave.h"

/* ==================== Test Fixtures ==================== */

void setUp(void)
{
    /* Called before each test */
}

void tearDown(void)
{
    /* Called after each test */
}

/* ==================== ISO7816 Constants Tests ==================== */

void test_iso7816_constants(void)
{
    /* Test protocol constants */
    TEST_ASSERT_EQUAL(33, XY_ISO7816_ATR_MAX_LEN);
    TEST_ASSERT_EQUAL(261, XY_ISO7816_APDU_MAX_LEN);
    TEST_ASSERT_EQUAL(258, XY_ISO7816_RESPONSE_MAX_LEN);
    TEST_ASSERT_EQUAL(1000, XY_ISO7816_DEFAULT_TIMEOUT);
    TEST_ASSERT_EQUAL(20000, XY_ISO7816_ATR_TIMEOUT);
}

void test_iso7816_status_words(void)
{
    /* Test status words */
    TEST_ASSERT_EQUAL(0x9000, XY_ISO7816_SW_SUCCESS);
    TEST_ASSERT_EQUAL(0x61FF, XY_ISO7816_SW_MORE_DATA);
    TEST_ASSERT_EQUAL(0x6700, XY_ISO7816_SW_WRONG_LENGTH);
    TEST_ASSERT_EQUAL(0x6982, XY_ISO7816_SW_SECURITY_STATUS);
    TEST_ASSERT_EQUAL(0x6983, XY_ISO7816_SW_AUTH_BLOCKED);
    TEST_ASSERT_EQUAL(0x6A86, XY_ISO7816_SW_WRONG_PARAMS);
    TEST_ASSERT_EQUAL(0x6D00, XY_ISO7816_SW_INS_NOT_SUPPORTED);
    TEST_ASSERT_EQUAL(0x6E00, XY_ISO7816_SW_CLA_NOT_SUPPORTED);
}

void test_iso7816_apdu_instructions(void)
{
    /* Test APDU instruction bytes */
    TEST_ASSERT_EQUAL(0xA4, XY_ISO7816_INS_SELECT);
    TEST_ASSERT_EQUAL(0xB0, XY_ISO7816_INS_READ_BINARY);
    TEST_ASSERT_EQUAL(0xB2, XY_ISO7816_INS_READ_RECORD);
    TEST_ASSERT_EQUAL(0xD6, XY_ISO7816_INS_UPDATE_BINARY);
    TEST_ASSERT_EQUAL(0xDC, XY_ISO7816_INS_UPDATE_RECORD);
    TEST_ASSERT_EQUAL(0xC0, XY_ISO7816_INS_GET_RESPONSE);
    TEST_ASSERT_EQUAL(0x20, XY_ISO7816_INS_VERIFY_PIN);
    TEST_ASSERT_EQUAL(0x84, XY_ISO7816_INS_GET_CHALLENGE);
    TEST_ASSERT_EQUAL(0x88, XY_ISO7816_INS_AUTHENTICATE);
}

void test_iso7816_file_ids(void)
{
    /* Test common file IDs */
    TEST_ASSERT_EQUAL(0x3F00, XY_ISO7816_FID_MF);
    TEST_ASSERT_EQUAL(0x7F10, XY_ISO7816_FID_DF_TELECOM);
    TEST_ASSERT_EQUAL(0x7F20, XY_ISO7816_FID_DF_GSM);
    TEST_ASSERT_EQUAL(0x2FE2, XY_ISO7816_FID_EF_ICCID);
    TEST_ASSERT_EQUAL(0x6F07, XY_ISO7816_FID_EF_IMSI);
    TEST_ASSERT_EQUAL(0x6F7E, XY_ISO7816_FID_EF_LOCI);
    TEST_ASSERT_EQUAL(0x6F46, XY_ISO7816_FID_EF_SPN);
}

void test_iso7816_error_codes(void)
{
    /* Test error codes */
    TEST_ASSERT_EQUAL(0, XY_ISO7816_OK);
    TEST_ASSERT_EQUAL(-1, XY_ISO7816_ERROR);
    TEST_ASSERT_EQUAL(-2, XY_ISO7816_ERROR_INVALID_PARAM);
    TEST_ASSERT_EQUAL(-3, XY_ISO7816_ERROR_TIMEOUT);
    TEST_ASSERT_EQUAL(-4, XY_ISO7816_ERROR_IO);
    TEST_ASSERT_EQUAL(-5, XY_ISO7816_ERROR_PROTOCOL);
    TEST_ASSERT_EQUAL(-6, XY_ISO7816_ERROR_ATR);
    TEST_ASSERT_EQUAL(-7, XY_ISO7816_ERROR_NOT_INIT);
    TEST_ASSERT_EQUAL(-8, XY_ISO7816_ERROR_CARD);
}

void test_iso7816_card_types(void)
{
    /* Test card type enum values */
    TEST_ASSERT_EQUAL(0, XY_ISO7816_CARD_UNKNOWN);
    TEST_ASSERT_EQUAL(1, XY_ISO7816_CARD_SIM);
    TEST_ASSERT_EQUAL(2, XY_ISO7816_CARD_USIM);
    TEST_ASSERT_EQUAL(3, XY_ISO7816_CARD_ISIM);
    TEST_ASSERT_EQUAL(4, XY_ISO7816_CARD_GENERIC);
}

/* ==================== ISO7816 Structure Tests ==================== */

void test_iso7816_atr_structure(void)
{
    xy_iso7816_atr_t atr;

    /* Test ATR structure size */
    TEST_ASSERT_TRUE(sizeof(atr.data) >= XY_ISO7816_ATR_MAX_LEN);
    TEST_ASSERT_TRUE(sizeof(atr.length) == 1);
}

void test_iso7816_apdu_cmd_structure(void)
{
    xy_iso7816_apdu_cmd_t cmd;

    /* Test APDU command structure */
    TEST_ASSERT_TRUE(sizeof(cmd.data) >= 256);
    TEST_ASSERT_EQUAL(0, cmd.cla);
    TEST_ASSERT_EQUAL(0, cmd.ins);
    TEST_ASSERT_EQUAL(0, cmd.p1);
    TEST_ASSERT_EQUAL(0, cmd.p2);
    TEST_ASSERT_EQUAL(0, cmd.lc);
    TEST_ASSERT_EQUAL(0, cmd.le);
}

void test_iso7816_apdu_resp_structure(void)
{
    xy_iso7816_apdu_resp_t resp;

    /* Test APDU response structure */
    TEST_ASSERT_TRUE(sizeof(resp.data) >= 256);
    TEST_ASSERT_EQUAL(0, resp.length);
    TEST_ASSERT_EQUAL(0, resp.sw1);
    TEST_ASSERT_EQUAL(0, resp.sw2);
}

void test_iso7816_handle_structure(void)
{
    xy_iso7816_handle_t handle;

    /* Test handle structure */
    TEST_ASSERT_EQUAL_PTR(NULL, handle.uart);
    TEST_ASSERT_EQUAL(0, handle.initialized);
    TEST_ASSERT_EQUAL(0, handle.timeout);
}

/* ==================== Modbus Constants Tests ==================== */

void test_modbus_function_codes(void)
{
    /* Test Modbus function codes */
    TEST_ASSERT_EQUAL(0x01, MB_FC_READ_COILS);
    TEST_ASSERT_EQUAL(0x02, MB_FC_READ_DISCRETE_INPUTS);
    TEST_ASSERT_EQUAL(0x03, MB_FC_READ_HOLDING_REGISTERS);
    TEST_ASSERT_EQUAL(0x04, MB_FC_READ_INPUT_REGISTERS);
    TEST_ASSERT_EQUAL(0x05, MB_FC_WRITE_SINGLE_COIL);
    TEST_ASSERT_EQUAL(0x06, MB_FC_WRITE_SINGLE_REGISTER);
    TEST_ASSERT_EQUAL(0x0F, MB_FC_WRITE_MULTIPLE_COILS);
    TEST_ASSERT_EQUAL(0x10, MB_FC_WRITE_MULTIPLE_REGISTERS);
}

void test_modbus_exception_codes(void)
{
    /* Test Modbus exception codes */
    TEST_ASSERT_EQUAL(0x00, MB_EX_NONE);
    TEST_ASSERT_EQUAL(0x01, MB_EX_ILLEGAL_FUNCTION);
    TEST_ASSERT_EQUAL(0x02, MB_EX_ILLEGAL_DATA_ADDRESS);
    TEST_ASSERT_EQUAL(0x03, MB_EX_ILLEGAL_DATA_VALUE);
    TEST_ASSERT_EQUAL(0x04, MB_EX_SLAVE_DEVICE_FAILURE);
    TEST_ASSERT_EQUAL(0x05, MB_EX_ACKNOWLEDGE);
    TEST_ASSERT_EQUAL(0x06, MB_EX_SLAVE_BUSY);
    TEST_ASSERT_EQUAL(0x08, MB_EX_MEMORY_PARITY_ERROR);
}

void test_modbus_default_config(void)
{
    /* Test default configuration values */
    TEST_ASSERT_EQUAL(1, MB_SLAVE_ADDRESS);
    TEST_ASSERT_EQUAL(9600, MB_UART_BAUDRATE);
    TEST_ASSERT_EQUAL(64, MB_COIL_COUNT);
    TEST_ASSERT_EQUAL(64, MB_DISCRETE_COUNT);
    TEST_ASSERT_EQUAL(32, MB_INPUT_REG_COUNT);
    TEST_ASSERT_EQUAL(32, MB_HOLDING_REG_COUNT);
    TEST_ASSERT_EQUAL(256, MB_RX_BUFFER_SIZE);
    TEST_ASSERT_EQUAL(256, MB_TX_BUFFER_SIZE);
}

/* ==================== Modbus Structure Tests ==================== */

void test_modbus_slave_context_size(void)
{
    mb_slave_t slave;

    /* Test context structure sizes */
    TEST_ASSERT_TRUE(sizeof(slave.coils) >= (MB_COIL_COUNT / 8 + 1));
    TEST_ASSERT_TRUE(sizeof(slave.discrete) >= (MB_DISCRETE_COUNT / 8 + 1));
    TEST_ASSERT_TRUE(sizeof(slave.input_regs) >= (MB_INPUT_REG_COUNT * 2));
    TEST_ASSERT_TRUE(sizeof(slave.holding_regs) >= (MB_HOLDING_REG_COUNT * 2));
    TEST_ASSERT_TRUE(sizeof(slave.rx_buffer) >= MB_RX_BUFFER_SIZE);
    TEST_ASSERT_TRUE(sizeof(slave.tx_buffer) >= MB_TX_BUFFER_SIZE);
}

void test_modbus_slave_context_init(void)
{
    mb_slave_t slave;

    /* Initialize slave context */
    mb_slave_init(&slave);

    /* Verify initialization */
    TEST_ASSERT_EQUAL(MB_SLAVE_ADDRESS, slave.address);
    TEST_ASSERT_EQUAL(MB_UART_BAUDRATE, slave.baudrate);
    TEST_ASSERT_EQUAL_PTR(NULL, slave.uart_handle);
}

/* ==================== Modbus CRC Tests ==================== */

void test_modbus_crc_calculation(void)
{
    uint8_t data[] = { 0x01, 0x03, 0x00, 0x00, 0x00, 0x0A };
    uint16_t crc;

    /* Calculate CRC */
    crc = mb_crc16(data, sizeof(data));

    /* Verify CRC is non-zero */
    TEST_ASSERT_TRUE(crc != 0);
}

void test_modbus_crc_consistency(void)
{
    uint8_t data[] = "Test data for CRC";
    uint16_t crc1, crc2;

    /* Calculate CRC twice */
    crc1 = mb_crc16(data, sizeof(data));
    crc2 = mb_crc16(data, sizeof(data));

    /* CRC should be consistent */
    TEST_ASSERT_EQUAL_UINT16(crc1, crc2);
}

void test_modbus_crc_different_data(void)
{
    uint8_t data1[] = "Hello";
    uint8_t data2[] = "World";
    uint16_t crc1, crc2;

    /* Calculate CRC for different data */
    crc1 = mb_crc16(data1, sizeof(data1));
    crc2 = mb_crc16(data2, sizeof(data2));

    /* CRC should be different for different data */
    TEST_ASSERT_NOT_EQUAL_UINT16(crc1, crc2);
}

/* ==================== Modbus Address Validation Tests ==================== */

void test_modbus_valid_address(void)
{
    /* Test valid address range (1-247) */
    TEST_ASSERT_TRUE(mb_is_valid_address(1));
    TEST_ASSERT_TRUE(mb_is_valid_address(127));
    TEST_ASSERT_TRUE(mb_is_valid_address(247));
}

void test_modbus_invalid_address(void)
{
    /* Test invalid address range */
    TEST_ASSERT_FALSE(mb_is_valid_address(0));
    TEST_ASSERT_FALSE(mb_is_valid_address(248));
    TEST_ASSERT_FALSE(mb_is_valid_address(255));
}

/* ==================== Modbus Register Access Tests ==================== */

void test_modbus_read_coil(void)
{
    mb_slave_t slave;
    mb_bool coil_value;

    mb_slave_init(&slave);

    /* Write a coil */
    mb_slave_write_coil(&slave, 0, MB_TRUE);
    coil_value = mb_slave_read_coil(&slave, 0);
    TEST_ASSERT_EQUAL(MB_TRUE, coil_value);

    /* Write another coil */
    mb_slave_write_coil(&slave, 5, MB_FALSE);
    coil_value = mb_slave_read_coil(&slave, 5);
    TEST_ASSERT_EQUAL(MB_FALSE, coil_value);
}

void test_modbus_read_holding_register(void)
{
    mb_slave_t slave;
    uint16_t reg_value;

    mb_slave_init(&slave);

    /* Write a holding register */
    mb_slave_write_holding_register(&slave, 0, 0x1234);
    reg_value = mb_slave_read_holding_register(&slave, 0);
    TEST_ASSERT_EQUAL_UINT16(0x1234, reg_value);

    /* Write another register */
    mb_slave_write_holding_register(&slave, 10, 0xABCD);
    reg_value = mb_slave_read_holding_register(&slave, 10);
    TEST_ASSERT_EQUAL_UINT16(0xABCD, reg_value);
}

/* ==================== Main ==================== */

int main(void)
{
    UNITY_BEGIN();

    /* ISO7816 Constants Tests */
    RUN_TEST(test_iso7816_constants);
    RUN_TEST(test_iso7816_status_words);
    RUN_TEST(test_iso7816_apdu_instructions);
    RUN_TEST(test_iso7816_file_ids);
    RUN_TEST(test_iso7816_error_codes);
    RUN_TEST(test_iso7816_card_types);

    /* ISO7816 Structure Tests */
    RUN_TEST(test_iso7816_atr_structure);
    RUN_TEST(test_iso7816_apdu_cmd_structure);
    RUN_TEST(test_iso7816_apdu_resp_structure);
    RUN_TEST(test_iso7816_handle_structure);

    /* Modbus Constants Tests */
    RUN_TEST(test_modbus_function_codes);
    RUN_TEST(test_modbus_exception_codes);
    RUN_TEST(test_modbus_default_config);

    /* Modbus Structure Tests */
    RUN_TEST(test_modbus_slave_context_size);
    RUN_TEST(test_modbus_slave_context_init);

    /* Modbus CRC Tests */
    RUN_TEST(test_modbus_crc_calculation);
    RUN_TEST(test_modbus_crc_consistency);
    RUN_TEST(test_modbus_crc_different_data);

    /* Modbus Address Validation Tests */
    RUN_TEST(test_modbus_valid_address);
    RUN_TEST(test_modbus_invalid_address);

    /* Modbus Register Access Tests */
    RUN_TEST(test_modbus_read_coil);
    RUN_TEST(test_modbus_read_holding_register);

    return UNITY_END();
}
