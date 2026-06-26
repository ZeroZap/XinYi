/**
 * @file xy_iso7816_test.c
 * @brief ISO7816 Unit Tests
 * @version 1.0
 * @date 2025-11-01
 */

#include "xy_iso7816.h"
#include "unity.h"

#include <string.h>

/* Mock UART handle for testing */
static void *mock_uart = (void *)0x12345678;

/* Test macros */
#define ISO_TEST_ASSERT(cond, msg) TEST_ASSERT_TRUE_MESSAGE((cond), (msg))
#define ISO_TEST_PASS(msg)         (void)(msg)

xy_hal_error_t xy_hal_uart_send(void *uart, const uint8_t *data, size_t len, uint32_t timeout)
{
    (void)uart;
    (void)data;
    (void)len;
    (void)timeout;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_recv(void *uart, uint8_t *data, size_t len, uint32_t timeout)
{
    (void)uart;
    (void)timeout;
    if (data && len > 0U) {
        memset(data, 0, len);
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_flush(void *uart)
{
    (void)uart;
    return XY_HAL_OK;
}

/* ========================================================================== */
/* Test: Initialization and Deinitialization */
/* ========================================================================== */

void test_init_deinit(void)
{
    xy_iso7816_handle_t handle;
    xy_iso7816_error_t ret;

    /* Test normal initialization */
    ret = xy_iso7816_init(&handle, mock_uart);
    ISO_TEST_ASSERT(ret == XY_ISO7816_OK, "Init should succeed");
    ISO_TEST_ASSERT(handle.uart == mock_uart, "UART handle should be set");
    ISO_TEST_ASSERT(handle.initialized == true, "Initialized flag should be true");

    /* Test deinitialization */
    ret = xy_iso7816_deinit(&handle);
    ISO_TEST_ASSERT(ret == XY_ISO7816_OK, "Deinit should succeed");
    ISO_TEST_ASSERT(
        handle.initialized == false, "Initialized flag should be false");

    /* Test NULL parameter */
    ret = xy_iso7816_init(NULL, mock_uart);
    ISO_TEST_ASSERT(ret == XY_ISO7816_ERROR_INVALID_PARAM,
                "Init with NULL handle should fail");

    ret = xy_iso7816_init(&handle, NULL);
    ISO_TEST_ASSERT(ret == XY_ISO7816_ERROR_INVALID_PARAM,
                "Init with NULL uart should fail");

    ISO_TEST_PASS("Initialization and deinitialization");
}

/* ========================================================================== */
/* Test: Status Word Helpers */
/* ========================================================================== */

void test_status_word_helpers(void)
{
    xy_iso7816_apdu_resp_t resp;

    /* Test success detection */
    resp.sw1 = 0x90;
    resp.sw2 = 0x00;
    ISO_TEST_ASSERT(
        xy_iso7816_is_success(&resp) == true, "0x9000 should be success");

    resp.sw1 = 0x61;
    resp.sw2 = 0x10;
    ISO_TEST_ASSERT(
        xy_iso7816_is_success(&resp) == false, "0x6110 should not be success");

    /* Test get SW */
    resp.sw1  = 0x63;
    resp.sw2  = 0xC3;
    xy_u16 sw = xy_iso7816_get_sw(&resp);
    ISO_TEST_ASSERT(sw == 0x63C3, "SW should be 0x63C3");

    /* Test NULL parameter */
    ISO_TEST_ASSERT(
        xy_iso7816_is_success(NULL) == false, "NULL should return false");
    ISO_TEST_ASSERT(xy_iso7816_get_sw(NULL) == 0, "NULL should return 0");

    ISO_TEST_PASS("Status word helpers");
}

/* ========================================================================== */
/* Test: BCD to ASCII Conversion */
/* ========================================================================== */

void test_bcd_to_ascii(void)
{
    char ascii[21];
    xy_u8 len;

    /* Test ICCID conversion */
    xy_u8 iccid_bcd[] = { 0x89, 0x86, 0x04, 0x20, 0x00,
                          0x12, 0x34, 0x56, 0x78, 0x90 };
    len = xy_iso7816_bcd_to_ascii(iccid_bcd, 10, ascii, sizeof(ascii));

    ISO_TEST_ASSERT(len == 20, "Should convert to 20 digits");
    ISO_TEST_ASSERT(strlen(ascii) == 20, "String length should be 20");

    /* Test with smaller buffer */
    char small_buf[5];
    len = xy_iso7816_bcd_to_ascii(iccid_bcd, 10, small_buf, sizeof(small_buf));
    ISO_TEST_ASSERT(len == 4, "Should be limited by buffer size");
    ISO_TEST_ASSERT(small_buf[4] == '\0', "Should be null-terminated");

    /* Test NULL parameters */
    len = xy_iso7816_bcd_to_ascii(NULL, 10, ascii, sizeof(ascii));
    ISO_TEST_ASSERT(len == 0, "NULL BCD should return 0");

    len = xy_iso7816_bcd_to_ascii(iccid_bcd, 10, NULL, 0);
    ISO_TEST_ASSERT(len == 0, "NULL ASCII should return 0");

    ISO_TEST_PASS("BCD to ASCII conversion");
}

/* ========================================================================== */
/* Test: APDU Structure Validation */
/* ========================================================================== */

void test_apdu_structures(void)
{
    xy_iso7816_apdu_cmd_t cmd;
    xy_iso7816_apdu_resp_t resp;

    /* Test command structure size */
    memset(&cmd, 0, sizeof(cmd));
    cmd.cla     = XY_ISO7816_CLA_GSM;
    cmd.ins     = XY_ISO7816_INS_SELECT;
    cmd.p1      = 0x00;
    cmd.p2      = 0x04;
    cmd.lc      = 2;
    cmd.data[0] = 0x3F;
    cmd.data[1] = 0x00;

    ISO_TEST_ASSERT(cmd.cla == 0xA0, "CLA should be set");
    ISO_TEST_ASSERT(cmd.ins == 0xA4, "INS should be set");

    /* Test response structure */
    memset(&resp, 0, sizeof(resp));
    resp.data[0] = 0x12;
    resp.data[1] = 0x34;
    resp.length  = 2;
    resp.sw1     = 0x90;
    resp.sw2     = 0x00;

    ISO_TEST_ASSERT(resp.length == 2, "Response length should be 2");
    ISO_TEST_ASSERT(resp.sw1 == 0x90, "SW1 should be 0x90");

    ISO_TEST_PASS("APDU structure validation");
}

/* ========================================================================== */
/* Test: ATR Structure */
/* ========================================================================== */

void test_atr_structure(void)
{
    xy_iso7816_atr_t atr;

    memset(&atr, 0, sizeof(atr));

    /* Simulate a minimal ATR */
    atr.data[0]  = 0x3B; /* TS - Direct convention */
    atr.data[1]  = 0x00; /* T0 - No interface bytes, no historical bytes */
    atr.length   = 2;
    atr.protocol = 0; /* T=0 */
    atr.valid    = true;

    ISO_TEST_ASSERT(atr.data[0] == 0x3B, "TS should be 0x3B");
    ISO_TEST_ASSERT(atr.length == 2, "Length should be 2");
    ISO_TEST_ASSERT(atr.valid == true, "Should be valid");

    /* Test ATR parsing */
    xy_iso7816_error_t ret = xy_iso7816_parse_atr(&atr);
    ISO_TEST_ASSERT(ret == XY_ISO7816_OK, "Parse should succeed");

    /* Test invalid ATR */
    atr.valid = false;
    ret       = xy_iso7816_parse_atr(&atr);
    ISO_TEST_ASSERT(
        ret == XY_ISO7816_ERROR_INVALID_PARAM, "Invalid ATR should fail");

    ISO_TEST_PASS("ATR structure");
}

/* ========================================================================== */
/* Test: File ID Constants */
/* ========================================================================== */

void test_file_id_constants(void)
{
    ISO_TEST_ASSERT(XY_ISO7816_FID_MF == 0x3F00, "MF FID should be 0x3F00");
    ISO_TEST_ASSERT(
        XY_ISO7816_FID_DF_TELECOM == 0x7F10, "DF_TELECOM FID should be 0x7F10");
    ISO_TEST_ASSERT(XY_ISO7816_FID_DF_GSM == 0x7F20, "DF_GSM FID should be 0x7F20");
    ISO_TEST_ASSERT(
        XY_ISO7816_FID_EF_ICCID == 0x2FE2, "EF_ICCID FID should be 0x2FE2");
    ISO_TEST_ASSERT(
        XY_ISO7816_FID_EF_IMSI == 0x6F07, "EF_IMSI FID should be 0x6F07");

    ISO_TEST_PASS("File ID constants");
}

/* ========================================================================== */
/* Test: Status Word Constants */
/* ========================================================================== */

void test_status_word_constants(void)
{
    ISO_TEST_ASSERT(XY_ISO7816_SW_SUCCESS == 0x9000, "Success SW should be 0x9000");
    ISO_TEST_ASSERT(XY_ISO7816_SW_WRONG_LENGTH == 0x6700,
                "Wrong length SW should be 0x6700");
    ISO_TEST_ASSERT(XY_ISO7816_SW_FILE_NOT_FOUND == 0x6A82,
                "File not found SW should be 0x6A82");
    ISO_TEST_ASSERT(XY_ISO7816_SW_INS_NOT_SUPPORTED == 0x6D00,
                "INS not supported SW should be 0x6D00");

    ISO_TEST_PASS("Status word constants");
}

/* ========================================================================== */
/* Test: SIM Info Structure */
/* ========================================================================== */

void test_sim_info_structure(void)
{
    xy_iso7816_sim_info_t info;

    memset(&info, 0, sizeof(info));

    info.card_type = XY_ISO7816_CARD_USIM;
    info.iccid_len = 10;
    info.imsi_len  = 9;

    ISO_TEST_ASSERT(
        info.card_type == XY_ISO7816_CARD_USIM, "Card type should be USIM");
    ISO_TEST_ASSERT(info.iccid_len == 10, "ICCID length should be 10");
    ISO_TEST_ASSERT(info.imsi_len == 9, "IMSI length should be 9");

    /* Test card type enum values */
    ISO_TEST_ASSERT(XY_ISO7816_CARD_UNKNOWN == 0, "Unknown card type should be 0");
    ISO_TEST_ASSERT(XY_ISO7816_CARD_SIM == 1, "SIM card type should be 1");
    ISO_TEST_ASSERT(XY_ISO7816_CARD_USIM == 2, "USIM card type should be 2");

    ISO_TEST_PASS("SIM info structure");
}

/* ========================================================================== */
/* Test: Error Code Definitions */
/* ========================================================================== */

void test_error_codes(void)
{
    ISO_TEST_ASSERT(XY_ISO7816_OK == 0, "OK should be 0");
    ISO_TEST_ASSERT(XY_ISO7816_ERROR == -1, "ERROR should be -1");
    ISO_TEST_ASSERT(
        XY_ISO7816_ERROR_INVALID_PARAM == -2, "INVALID_PARAM should be -2");
    ISO_TEST_ASSERT(XY_ISO7816_ERROR_TIMEOUT == -3, "TIMEOUT should be -3");
    ISO_TEST_ASSERT(XY_ISO7816_ERROR_IO == -4, "IO should be -4");
    ISO_TEST_ASSERT(XY_ISO7816_ERROR_PROTOCOL == -5, "PROTOCOL should be -5");
    ISO_TEST_ASSERT(XY_ISO7816_ERROR_ATR == -6, "ATR should be -6");
    ISO_TEST_ASSERT(XY_ISO7816_ERROR_NOT_INIT == -7, "NOT_INIT should be -7");
    ISO_TEST_ASSERT(XY_ISO7816_ERROR_CARD == -8, "CARD should be -8");

    ISO_TEST_PASS("Error code definitions");
}

/* ========================================================================== */
/* Test Runner */
/* ========================================================================== */

void setUp(void)
{
}

void tearDown(void)
{
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_deinit);
    RUN_TEST(test_status_word_helpers);
    RUN_TEST(test_bcd_to_ascii);
    RUN_TEST(test_apdu_structures);
    RUN_TEST(test_atr_structure);
    RUN_TEST(test_file_id_constants);
    RUN_TEST(test_status_word_constants);
    RUN_TEST(test_sim_info_structure);
    RUN_TEST(test_error_codes);
    return UNITY_END();
}
