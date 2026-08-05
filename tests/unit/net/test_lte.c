/**
 * @file test_lte.c
 * @brief Unit tests for LTE component state and callback registration behavior.
 */
#include "unity.h"
#include "fff.h"

#include "xy_lte.h"

#include <string.h>

DEFINE_FFF_GLOBALS;

FAKE_VOID_FUNC(on_urc, const char *)
FAKE_VOID_FUNC(on_recv, uint8_t *, size_t)

void setUp(void)
{
    RESET_FAKE(on_urc);
    RESET_FAKE(on_recv);
    FFF_RESET_HISTORY();
}

void tearDown(void)
{
}

static void test_lte_lifecycle_and_callbacks(void)
{
    xy_lte_t lte;
    uint8_t rx[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    xy_lte_pdp_context_t pdp;
    int uart_token = 1;

    memset(&lte, 0xA5, sizeof(lte));
    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_init(&lte, &uart_token, 0));
    TEST_ASSERT_TRUE(lte.initialized);
    TEST_ASSERT_EQUAL_PTR(&uart_token, lte.uart_handle);
    TEST_ASSERT_EQUAL_UINT32(115200U, lte.baudrate);
    TEST_ASSERT_EQUAL_UINT8(1U, lte.pdp.cid);
    TEST_ASSERT_EQUAL_STRING("cmnet", lte.pdp.apn);

    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_register_urc(&lte, on_urc));
    TEST_ASSERT_EQUAL_PTR(on_urc, lte.urc_callback);
    lte.urc_callback("+READY");
    TEST_ASSERT_EQUAL_UINT(1U, on_urc_fake.call_count);
    TEST_ASSERT_EQUAL_STRING("+READY", on_urc_fake.arg0_val);

    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_register_recv(&lte, on_recv));
    TEST_ASSERT_EQUAL_PTR(on_recv, lte.recv_callback);
    lte.recv_callback(rx, sizeof(rx));
    TEST_ASSERT_EQUAL_UINT(1U, on_recv_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(rx, on_recv_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT(sizeof(rx), on_recv_fake.arg1_val);

    memset(&pdp, 0, sizeof(pdp));
    pdp.cid = 2;
    strcpy(pdp.apn, "iot.example");
    strcpy(pdp.username, "user");
    strcpy(pdp.password, "pass");
    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_set_pdp_context(&lte, &pdp));

    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_attach(&lte));
    TEST_ASSERT_EQUAL_INT(0, xy_lte_is_attached(&lte));
    TEST_ASSERT_TRUE(lte.attached);
    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_detach(&lte));
    TEST_ASSERT_FALSE(lte.attached);

    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_activate_pdp(&lte, pdp.cid));
    TEST_ASSERT_EQUAL_INT(1, xy_lte_is_pdp_active(&lte, pdp.cid));
    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_deactivate_pdp(&lte, pdp.cid));
    TEST_ASSERT_EQUAL_INT(0, xy_lte_is_pdp_active(&lte, pdp.cid));

    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_deinit(&lte));
    TEST_ASSERT_FALSE(lte.initialized);
}

static void test_lte_parameter_validation(void)
{
    xy_lte_t lte;
    uint8_t payload[2] = {1, 2};
    uint8_t rx[4] = {0x11, 0x22, 0x33, 0x44};
    char response[8];
    char ip[16];
    char imei[16];
    int uart_token = 1;

    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, xy_lte_init(NULL, &uart_token, 115200));
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, xy_lte_init(&lte, NULL, 115200));
    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_init(&lte, &uart_token, 9600));

    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, xy_lte_register_urc(NULL, on_urc));
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, xy_lte_register_recv(NULL, on_recv));

    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, xy_lte_connect(NULL, 0, "example.com", 80, true));
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, xy_lte_connect(&lte, 8, "example.com", 80, true));
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, xy_lte_connect(&lte, 0, NULL, 80, true));
    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_connect(&lte, 0, "example.com", 80, true));

    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, xy_lte_send(&lte, 8, payload, sizeof(payload)));
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, xy_lte_send(&lte, 0, NULL, sizeof(payload)));
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, xy_lte_send(&lte, 0, payload, 0));
    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_send(&lte, 0, payload, sizeof(payload)));

    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, xy_lte_recv(&lte, 8, rx, sizeof(rx), 0));
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, xy_lte_recv(&lte, 0, NULL, sizeof(rx), 0));
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, xy_lte_recv(&lte, 0, rx, 0, 0));
    TEST_ASSERT_EQUAL_INT(0, xy_lte_recv(&lte, 0, rx, sizeof(rx), 0));
    TEST_ASSERT_EQUAL_UINT8(0U, rx[0]);
    TEST_ASSERT_EQUAL_UINT8(0U, rx[3]);

    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM,
                      xy_lte_send_at(NULL, "AT", response, sizeof(response), 1000));
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM,
                      xy_lte_send_at(&lte, NULL, response, sizeof(response), 1000));
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, xy_lte_get_ip(&lte, ip, 0));
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, xy_lte_get_imei(&lte, imei, 0));

    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, xy_lte_close(&lte, 8));
    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_close(&lte, 0));
    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_deinit(&lte));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_lte_lifecycle_and_callbacks);
    RUN_TEST(test_lte_parameter_validation);
    return UNITY_END();
}
