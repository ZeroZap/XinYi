/**
 * @file test_lte.c
 * @brief Unit tests for LTE component state and callback registration behavior.
 */
#include "unity.h"
#include "fff.h"

#include "xy_lte.h"

#include <string.h>

#define LTE_TRANSPORT_MAX_CMDS 8U
#define LTE_TRANSPORT_MAX_RESP 8U
#define LTE_TRANSPORT_TEXT_MAX 64U

typedef struct {
    const char *responses[LTE_TRANSPORT_MAX_RESP];
    int write_results[LTE_TRANSPORT_MAX_CMDS];
    char commands[LTE_TRANSPORT_MAX_CMDS][LTE_TRANSPORT_TEXT_MAX];
    size_t command_count;
    size_t response_count;
    size_t response_index;
    size_t write_result_count;
    size_t write_result_index;
} fake_lte_transport_t;

DEFINE_FFF_GLOBALS;

FAKE_VOID_FUNC(on_urc, const char *)
FAKE_VOID_FUNC(on_recv, uint8_t *, size_t)

static fake_lte_transport_t g_transport;

static void fake_transport_reset(void)
{
    memset(&g_transport, 0, sizeof(g_transport));
}

static void fake_transport_push_response(const char *response)
{
    TEST_ASSERT_LESS_THAN_UINT(LTE_TRANSPORT_MAX_RESP, g_transport.response_count);
    g_transport.responses[g_transport.response_count++] = response;
}

static void fake_transport_push_write_result(int result)
{
    TEST_ASSERT_LESS_THAN_UINT(LTE_TRANSPORT_MAX_CMDS, g_transport.write_result_count);
    g_transport.write_results[g_transport.write_result_count++] = result;
}

static int fake_transport_write(void *context, const uint8_t *data, size_t len,
                                uint32_t timeout_ms)
{
    fake_lte_transport_t *transport = (fake_lte_transport_t *)context;
    size_t copy_len;

    (void)timeout_ms;
    TEST_ASSERT_NOT_NULL(transport);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_LESS_THAN_UINT(LTE_TRANSPORT_MAX_CMDS, transport->command_count);

    copy_len = len;
    if (copy_len >= LTE_TRANSPORT_TEXT_MAX) {
        copy_len = LTE_TRANSPORT_TEXT_MAX - 1U;
    }
    memcpy(transport->commands[transport->command_count], data, copy_len);
    transport->commands[transport->command_count][copy_len] = '\0';
    transport->command_count++;

    if (transport->write_result_index < transport->write_result_count) {
        return transport->write_results[transport->write_result_index++];
    }
    return XY_LTE_OK;
}

static int fake_transport_read(void *context, uint8_t *data, size_t len, uint32_t timeout_ms)
{
    fake_lte_transport_t *transport = (fake_lte_transport_t *)context;
    const char *response;
    size_t response_len;
    size_t copy_len;

    (void)timeout_ms;
    TEST_ASSERT_NOT_NULL(transport);
    TEST_ASSERT_NOT_NULL(data);

    if (transport->response_index >= transport->response_count) {
        return 0;
    }

    response = transport->responses[transport->response_index++];
    if (!response) {
        return XY_LTE_ERROR;
    }

    response_len = strlen(response);
    copy_len = response_len;
    if (copy_len > len) {
        copy_len = len;
    }
    memcpy(data, response, copy_len);
    return (int)copy_len;
}

static void bind_fake_transport(xy_lte_t *lte)
{
    xy_lte_transport_t transport = {
        .context = &g_transport,
        .write = fake_transport_write,
        .read = fake_transport_read,
        .flush = NULL,
    };

    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_bind_transport(lte, &transport));
}

void setUp(void)
{
    fake_transport_reset();
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
    TEST_ASSERT_EQUAL_UINT8(2U, lte.pdp.cid);
    TEST_ASSERT_EQUAL_STRING("iot.example", lte.pdp.apn);

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
    xy_lte_transport_t transport = {
        .context = &g_transport,
        .write = fake_transport_write,
        .read = fake_transport_read,
        .flush = NULL,
    };

    memset(&lte, 0, sizeof(lte));
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, xy_lte_bind_transport(&lte, &transport));
    TEST_ASSERT_NULL(lte.transport.write);

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

static void test_lte_transport_drives_check_and_signal_contracts(void)
{
    xy_lte_t lte;
    xy_lte_signal_t signal = {
        .rssi = -99,
        .ber = 7,
        .rsrp = -140,
        .rsrq = -20,
        .sinr = -5,
    };
    int uart_token = 1;

    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_init(&lte, &uart_token, 115200));
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, xy_lte_bind_transport(NULL, NULL));
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, xy_lte_bind_transport(&lte, NULL));

    bind_fake_transport(&lte);
    fake_transport_push_response("\r\nOK\r\n");
    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_check(&lte));
    TEST_ASSERT_EQUAL_UINT(1U, g_transport.command_count);
    TEST_ASSERT_EQUAL_STRING("AT", g_transport.commands[0]);

    fake_transport_reset();
    bind_fake_transport(&lte);
    fake_transport_push_response("\r\nERROR\r\n");
    TEST_ASSERT_EQUAL(XY_LTE_ERROR, xy_lte_check(&lte));

    fake_transport_reset();
    bind_fake_transport(&lte);
    fake_transport_push_response("+CSQ: 17,3\r\nOK\r\n");
    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_get_signal(&lte, &signal));
    TEST_ASSERT_EQUAL_INT(17, signal.rssi);
    TEST_ASSERT_EQUAL_INT(3, signal.ber);
    TEST_ASSERT_EQUAL_INT(-140, signal.rsrp);

    fake_transport_reset();
    bind_fake_transport(&lte);
    fake_transport_push_response("ERROR");
    TEST_ASSERT_EQUAL(XY_LTE_ERROR, xy_lte_get_signal(&lte, &signal));
    TEST_ASSERT_EQUAL_INT(17, signal.rssi);
    TEST_ASSERT_EQUAL_INT(3, signal.ber);
}

static void test_lte_transport_failures_preserve_state(void)
{
    xy_lte_t lte;
    xy_lte_pdp_context_t previous;
    xy_lte_pdp_context_t next;
    int uart_token = 1;

    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_init(&lte, &uart_token, 115200));
    bind_fake_transport(&lte);

    lte.attached = false;
    fake_transport_push_write_result(XY_LTE_ERROR);
    TEST_ASSERT_EQUAL(XY_LTE_ERROR, xy_lte_attach(&lte));
    TEST_ASSERT_FALSE(lte.attached);
    TEST_ASSERT_EQUAL_UINT(1U, g_transport.command_count);
    TEST_ASSERT_EQUAL_STRING("AT+CNACT=2", g_transport.commands[0]);

    fake_transport_reset();
    bind_fake_transport(&lte);
    fake_transport_push_write_result(XY_LTE_OK);
    fake_transport_push_write_result(XY_LTE_TIMEOUT);
    TEST_ASSERT_EQUAL(XY_LTE_TIMEOUT, xy_lte_attach(&lte));
    TEST_ASSERT_FALSE(lte.attached);
    TEST_ASSERT_EQUAL_UINT(2U, g_transport.command_count);
    TEST_ASSERT_EQUAL_STRING("AT+CGATT=1", g_transport.commands[1]);

    lte.attached = true;
    fake_transport_reset();
    bind_fake_transport(&lte);
    fake_transport_push_write_result(XY_LTE_ERROR);
    TEST_ASSERT_EQUAL(XY_LTE_ERROR, xy_lte_detach(&lte));
    TEST_ASSERT_TRUE(lte.attached);

    previous = lte.pdp;
    memset(&next, 0, sizeof(next));
    next.cid = 3;
    strcpy(next.apn, "private.apn");
    fake_transport_reset();
    bind_fake_transport(&lte);
    fake_transport_push_write_result(XY_LTE_TIMEOUT);
    TEST_ASSERT_EQUAL(XY_LTE_TIMEOUT, xy_lte_set_pdp_context(&lte, &next));
    TEST_ASSERT_EQUAL_UINT8(previous.cid, lte.pdp.cid);
    TEST_ASSERT_EQUAL_STRING(previous.apn, lte.pdp.apn);
}

static void test_lte_transport_preserves_pdp_and_send_state_on_failures(void)
{
    xy_lte_t lte;
    const uint8_t payload[3] = {0x11, 0x22, 0x33};
    int uart_token = 1;

    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_init(&lte, &uart_token, 115200));
    bind_fake_transport(&lte);

    lte.pdp_active = false;
    fake_transport_push_write_result(XY_LTE_TIMEOUT);
    TEST_ASSERT_EQUAL(XY_LTE_TIMEOUT, xy_lte_activate_pdp(&lte, lte.pdp.cid));
    TEST_ASSERT_FALSE(lte.pdp_active);
    TEST_ASSERT_EQUAL_UINT(1U, g_transport.command_count);
    TEST_ASSERT_EQUAL_STRING("AT+CIICR", g_transport.commands[0]);

    lte.pdp_active = true;
    fake_transport_reset();
    bind_fake_transport(&lte);
    fake_transport_push_write_result(XY_LTE_ERROR);
    TEST_ASSERT_EQUAL(XY_LTE_ERROR, xy_lte_deactivate_pdp(&lte, lte.pdp.cid));
    TEST_ASSERT_TRUE(lte.pdp_active);
    TEST_ASSERT_EQUAL_UINT(1U, g_transport.command_count);
    TEST_ASSERT_EQUAL_STRING("AT+CIPSHUT", g_transport.commands[0]);

    fake_transport_reset();
    bind_fake_transport(&lte);
    fake_transport_push_response("ERROR");
    TEST_ASSERT_EQUAL(XY_LTE_ERROR, xy_lte_send(&lte, 1, payload, sizeof(payload)));
    TEST_ASSERT_EQUAL_UINT(1U, g_transport.command_count);
    TEST_ASSERT_EQUAL_STRING("AT+CIPSEND=1,3", g_transport.commands[0]);

    fake_transport_reset();
    bind_fake_transport(&lte);
    fake_transport_push_response("> ");
    fake_transport_push_write_result(XY_LTE_OK);
    fake_transport_push_write_result(XY_LTE_TIMEOUT);
    TEST_ASSERT_EQUAL(XY_LTE_TIMEOUT, xy_lte_send(&lte, 1, payload, sizeof(payload)));
    TEST_ASSERT_EQUAL_UINT(2U, g_transport.command_count);
    TEST_ASSERT_EQUAL_STRING("AT+CIPSEND=1,3", g_transport.commands[0]);
    TEST_ASSERT_EQUAL_UINT8(payload[0], (uint8_t)g_transport.commands[1][0]);
    TEST_ASSERT_EQUAL_UINT8(payload[2], (uint8_t)g_transport.commands[1][2]);

    fake_transport_reset();
    bind_fake_transport(&lte);
    fake_transport_push_response("> ");
    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_send(&lte, 1, payload, sizeof(payload)));
    TEST_ASSERT_EQUAL_UINT(2U, g_transport.command_count);
    TEST_ASSERT_EQUAL_STRING("AT+CIPSEND=1,3", g_transport.commands[0]);
    TEST_ASSERT_EQUAL_UINT8(payload[0], (uint8_t)g_transport.commands[1][0]);
    TEST_ASSERT_EQUAL_UINT8(payload[1], (uint8_t)g_transport.commands[1][1]);
    TEST_ASSERT_EQUAL_UINT8(payload[2], (uint8_t)g_transport.commands[1][2]);
}

static void test_lte_read_style_helpers_preserve_outputs_on_transport_failures(void)
{
    xy_lte_t lte;
    xy_lte_network_info_t network;
    xy_lte_sim_info_t sim;
    char manufacturer[8] = "maker";
    char model[8] = "model";
    char revision[8] = "rev";
    char ip[16] = "1.2.3.4";
    char imei[16] = "123456789012345";
    int uart_token = 1;

    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_init(&lte, &uart_token, 115200));
    bind_fake_transport(&lte);

    memset(&network, 0xA5, sizeof(network));
    fake_transport_push_response(NULL);
    TEST_ASSERT_EQUAL(XY_LTE_ERROR, xy_lte_get_network_info(&lte, &network));
    TEST_ASSERT_EQUAL_HEX8(0xA5, network.mcc[0]);
    TEST_ASSERT_EQUAL_HEX8(0xA5, network.mnc[0]);

    memset(&sim, 0x5A, sizeof(sim));
    fake_transport_reset();
    bind_fake_transport(&lte);
    fake_transport_push_response(NULL);
    TEST_ASSERT_EQUAL(XY_LTE_ERROR, xy_lte_get_sim_info(&lte, &sim));
    TEST_ASSERT_EQUAL_HEX8(0x5A, sim.iccid[0]);
    TEST_ASSERT_EQUAL_HEX8(0x5A, sim.imsi[0]);

    fake_transport_reset();
    bind_fake_transport(&lte);
    fake_transport_push_response(NULL);
    TEST_ASSERT_EQUAL(XY_LTE_ERROR, xy_lte_get_module_info(&lte, manufacturer, model, revision));
    TEST_ASSERT_EQUAL_STRING("maker", manufacturer);
    TEST_ASSERT_EQUAL_STRING("model", model);
    TEST_ASSERT_EQUAL_STRING("rev", revision);

    fake_transport_reset();
    bind_fake_transport(&lte);
    fake_transport_push_response(NULL);
    TEST_ASSERT_EQUAL(XY_LTE_ERROR, xy_lte_get_ip(&lte, ip, sizeof(ip)));
    TEST_ASSERT_EQUAL_STRING("1.2.3.4", ip);

    fake_transport_reset();
    bind_fake_transport(&lte);
    fake_transport_push_response(NULL);
    TEST_ASSERT_EQUAL(XY_LTE_ERROR, xy_lte_get_imei(&lte, imei, sizeof(imei)));
    TEST_ASSERT_EQUAL_STRING("123456789012345", imei);

    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, xy_lte_get_network_info(NULL, &network));
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, xy_lte_get_sim_info(NULL, &sim));
    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM,
                      xy_lte_get_module_info(NULL, manufacturer, model, revision));
}

static void test_lte_recv_uses_bound_transport_and_preserves_output_on_failure(void)
{
    xy_lte_t lte;
    uint8_t rx[8] = {0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5};
    uint8_t before[sizeof(rx)];
    int uart_token = 1;

    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, xy_lte_recv(NULL, 0, rx, sizeof(rx), 10));

    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_init(&lte, &uart_token, 115200));
    bind_fake_transport(&lte);
    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_register_recv(&lte, on_recv));

    fake_transport_push_response("DATA");
    TEST_ASSERT_EQUAL_INT(4, xy_lte_recv(&lte, 0, rx, sizeof(rx), 25));
    TEST_ASSERT_EQUAL_MEMORY("DATA", rx, 4U);
    TEST_ASSERT_EQUAL_UINT(1U, on_recv_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(rx, on_recv_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT(4U, on_recv_fake.arg1_val);

    memcpy(before, rx, sizeof(rx));
    fake_transport_reset();
    bind_fake_transport(&lte);
    fake_transport_push_response(NULL);
    TEST_ASSERT_EQUAL(XY_LTE_ERROR, xy_lte_recv(&lte, 0, rx, sizeof(rx), 25));
    TEST_ASSERT_EQUAL_MEMORY(before, rx, sizeof(rx));
    TEST_ASSERT_EQUAL_UINT(1U, on_recv_fake.call_count);
}

static void test_lte_send_at_preserves_response_on_transport_failures(void)
{
    xy_lte_t lte;
    char response[16] = "sentinel";
    int uart_token = 1;

    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_init(&lte, &uart_token, 115200));
    bind_fake_transport(&lte);

    fake_transport_push_write_result(XY_LTE_TIMEOUT);
    TEST_ASSERT_EQUAL(XY_LTE_TIMEOUT,
                      xy_lte_send_at(&lte, "AT+FAIL", response, sizeof(response), 100));
    TEST_ASSERT_EQUAL_STRING("sentinel", response);
    TEST_ASSERT_EQUAL_UINT(1U, g_transport.command_count);
    TEST_ASSERT_EQUAL_STRING("AT+FAIL", g_transport.commands[0]);

    fake_transport_reset();
    bind_fake_transport(&lte);
    fake_transport_push_response(NULL);
    TEST_ASSERT_EQUAL(XY_LTE_ERROR,
                      xy_lte_send_at(&lte, "AT+READ", response, sizeof(response), 100));
    TEST_ASSERT_EQUAL_STRING("sentinel", response);

    fake_transport_reset();
    bind_fake_transport(&lte);
    fake_transport_push_response("OK");
    TEST_ASSERT_EQUAL(XY_LTE_OK,
                      xy_lte_send_at(&lte, "AT+OK", response, sizeof(response), 100));
    TEST_ASSERT_EQUAL_STRING("OK", response);
}

static void test_lte_status_helpers_return_safe_inactive_on_transport_failures(void)
{
    xy_lte_t lte;
    int uart_token = 1;

    TEST_ASSERT_EQUAL(XY_LTE_INVALID_PARAM, xy_lte_check_sim(NULL));
    TEST_ASSERT_EQUAL(0, xy_lte_is_attached(NULL));

    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_init(&lte, &uart_token, 115200));
    bind_fake_transport(&lte);

    fake_transport_push_response("+CPIN: READY\r\nOK\r\n");
    TEST_ASSERT_EQUAL(1, xy_lte_check_sim(&lte));
    TEST_ASSERT_EQUAL_UINT(1U, g_transport.command_count);
    TEST_ASSERT_EQUAL_STRING("AT+CPIN?", g_transport.commands[0]);

    fake_transport_reset();
    bind_fake_transport(&lte);
    fake_transport_push_response("+CPIN: SIM PIN\r\nOK\r\n");
    TEST_ASSERT_EQUAL(2, xy_lte_check_sim(&lte));

    fake_transport_reset();
    bind_fake_transport(&lte);
    fake_transport_push_response(NULL);
    TEST_ASSERT_EQUAL(0, xy_lte_check_sim(&lte));

    fake_transport_reset();
    bind_fake_transport(&lte);
    fake_transport_push_response("+CGATT: 1\r\nOK\r\n");
    TEST_ASSERT_EQUAL(1, xy_lte_is_attached(&lte));
    TEST_ASSERT_EQUAL_UINT(1U, g_transport.command_count);
    TEST_ASSERT_EQUAL_STRING("AT+CGATT?", g_transport.commands[0]);

    fake_transport_reset();
    bind_fake_transport(&lte);
    fake_transport_push_response("+CGATT: 0\r\nOK\r\n");
    TEST_ASSERT_EQUAL(0, xy_lte_is_attached(&lte));

    fake_transport_reset();
    bind_fake_transport(&lte);
    fake_transport_push_response(NULL);
    TEST_ASSERT_EQUAL(0, xy_lte_is_attached(&lte));
}

static void test_lte_deinit_propagates_detach_failure_and_preserves_state(void)
{
    xy_lte_t lte;
    int uart_token = 1;

    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_init(&lte, &uart_token, 115200));
    bind_fake_transport(&lte);

    lte.attached = true;
    fake_transport_push_write_result(XY_LTE_TIMEOUT);
    TEST_ASSERT_EQUAL(XY_LTE_TIMEOUT, xy_lte_deinit(&lte));
    TEST_ASSERT_TRUE(lte.initialized);
    TEST_ASSERT_TRUE(lte.attached);
    TEST_ASSERT_EQUAL_UINT(1U, g_transport.command_count);
    TEST_ASSERT_EQUAL_STRING("AT+CGATT=0", g_transport.commands[0]);

    fake_transport_reset();
    bind_fake_transport(&lte);
    fake_transport_push_write_result(XY_LTE_OK);
    TEST_ASSERT_EQUAL(XY_LTE_OK, xy_lte_deinit(&lte));
    TEST_ASSERT_FALSE(lte.initialized);
    TEST_ASSERT_FALSE(lte.attached);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_lte_lifecycle_and_callbacks);
    RUN_TEST(test_lte_parameter_validation);
    RUN_TEST(test_lte_transport_drives_check_and_signal_contracts);
    RUN_TEST(test_lte_transport_failures_preserve_state);
    RUN_TEST(test_lte_transport_preserves_pdp_and_send_state_on_failures);
    RUN_TEST(test_lte_read_style_helpers_preserve_outputs_on_transport_failures);
    RUN_TEST(test_lte_recv_uses_bound_transport_and_preserves_output_on_failure);
    RUN_TEST(test_lte_send_at_preserves_response_on_transport_failures);
    RUN_TEST(test_lte_status_helpers_return_safe_inactive_on_transport_failures);
    RUN_TEST(test_lte_deinit_propagates_detach_failure_and_preserves_state);
    return UNITY_END();
}
