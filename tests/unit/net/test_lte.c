/**
 * @file test_lte.c
 * @brief Unit tests for LTE component state and callback registration behavior.
 */

#include "xy_lte.h"

#include <assert.h>
#include <string.h>

static uint32_t g_urc_count;
static uint32_t g_recv_count;
static size_t g_recv_len;

static void on_urc(const char *urc)
{
    assert(urc != NULL);
    g_urc_count++;
}

static void on_recv(uint8_t *data, size_t len)
{
    assert(data != NULL);
    g_recv_count++;
    g_recv_len = len;
}

static void test_lte_lifecycle_and_callbacks(void)
{
    xy_lte_t lte;
    uint8_t rx[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    xy_lte_pdp_context_t pdp;
    int uart_token = 1;

    memset(&lte, 0xA5, sizeof(lte));
    assert(xy_lte_init(&lte, &uart_token, 0) == XY_LTE_OK);
    assert(lte.initialized);
    assert(lte.uart_handle == &uart_token);
    assert(lte.baudrate == 115200U);
    assert(lte.pdp.cid == 1U);
    assert(strcmp(lte.pdp.apn, "cmnet") == 0);

    assert(xy_lte_register_urc(&lte, on_urc) == XY_LTE_OK);
    assert(lte.urc_callback == on_urc);
    lte.urc_callback("+READY");
    assert(g_urc_count == 1U);

    assert(xy_lte_register_recv(&lte, on_recv) == XY_LTE_OK);
    assert(lte.recv_callback == on_recv);
    lte.recv_callback(rx, sizeof(rx));
    assert(g_recv_count == 1U);
    assert(g_recv_len == sizeof(rx));

    memset(&pdp, 0, sizeof(pdp));
    pdp.cid = 2;
    strcpy(pdp.apn, "iot.example");
    strcpy(pdp.username, "user");
    strcpy(pdp.password, "pass");
    assert(xy_lte_set_pdp_context(&lte, &pdp) == XY_LTE_OK);

    assert(xy_lte_attach(&lte) == XY_LTE_OK);
    assert(xy_lte_is_attached(&lte) == 0); /* query path still uses stub AT response */
    assert(lte.attached);
    assert(xy_lte_detach(&lte) == XY_LTE_OK);
    assert(!lte.attached);

    assert(xy_lte_activate_pdp(&lte, pdp.cid) == XY_LTE_OK);
    assert(xy_lte_is_pdp_active(&lte, pdp.cid) == 1);
    assert(xy_lte_deactivate_pdp(&lte, pdp.cid) == XY_LTE_OK);
    assert(xy_lte_is_pdp_active(&lte, pdp.cid) == 0);

    assert(xy_lte_deinit(&lte) == XY_LTE_OK);
    assert(!lte.initialized);
}

static void test_lte_parameter_validation(void)
{
    xy_lte_t lte;
    uint8_t payload[2] = {1, 2};
    uint8_t rx[4] = {0x11, 0x22, 0x33, 0x44};
    int uart_token = 1;

    assert(xy_lte_init(NULL, &uart_token, 115200) == XY_LTE_INVALID_PARAM);
    assert(xy_lte_init(&lte, NULL, 115200) == XY_LTE_INVALID_PARAM);
    assert(xy_lte_init(&lte, &uart_token, 9600) == XY_LTE_OK);

    assert(xy_lte_register_urc(NULL, on_urc) == XY_LTE_INVALID_PARAM);
    assert(xy_lte_register_recv(NULL, on_recv) == XY_LTE_INVALID_PARAM);

    assert(xy_lte_connect(NULL, 0, "example.com", 80, true) == XY_LTE_INVALID_PARAM);
    assert(xy_lte_connect(&lte, 8, "example.com", 80, true) == XY_LTE_INVALID_PARAM);
    assert(xy_lte_connect(&lte, 0, NULL, 80, true) == XY_LTE_INVALID_PARAM);
    assert(xy_lte_connect(&lte, 0, "example.com", 80, true) == XY_LTE_OK);

    assert(xy_lte_send(&lte, 8, payload, sizeof(payload)) == XY_LTE_INVALID_PARAM);
    assert(xy_lte_send(&lte, 0, NULL, sizeof(payload)) == XY_LTE_INVALID_PARAM);
    assert(xy_lte_send(&lte, 0, payload, 0) == XY_LTE_INVALID_PARAM);
    assert(xy_lte_send(&lte, 0, payload, sizeof(payload)) == XY_LTE_OK);

    assert(xy_lte_recv(&lte, 8, rx, sizeof(rx), 0) == XY_LTE_INVALID_PARAM);
    assert(xy_lte_recv(&lte, 0, NULL, sizeof(rx), 0) == XY_LTE_INVALID_PARAM);
    assert(xy_lte_recv(&lte, 0, rx, 0, 0) == XY_LTE_INVALID_PARAM);
    assert(xy_lte_recv(&lte, 0, rx, sizeof(rx), 0) == 0);
    assert(rx[0] == 0U && rx[3] == 0U);

    assert(xy_lte_close(&lte, 8) == XY_LTE_INVALID_PARAM);
    assert(xy_lte_close(&lte, 0) == XY_LTE_OK);
    assert(xy_lte_deinit(&lte) == XY_LTE_OK);
}

int main(void)
{
    test_lte_lifecycle_and_callbacks();
    test_lte_parameter_validation();
    return 0;
}
