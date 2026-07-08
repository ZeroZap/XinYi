#include <stdint.h>
#include <string.h>

#include "unity.h"

#include "fff.h"
#include "xy_ats.h"

DEFINE_FFF_GLOBALS;

static char g_tx[1024];
static size_t g_tx_len;
static int g_led_state;
static at_server_t *g_server;

FAKE_VALUE_FUNC(int, mock_get_char, char *, uint32_t);
FAKE_VALUE_FUNC(size_t, mock_send, const char *, size_t);
FAKE_VALUE_FUNC(at_result_t, led_query);
FAKE_VALUE_FUNC(at_result_t, led_setup, const char *);
FAKE_VALUE_FUNC(at_result_t, led_exec);

void xy_log_char(char ch)
{
    (void)ch;
}

static void reset_tx(void)
{
    memset(g_tx, 0, sizeof(g_tx));
    g_tx_len = 0;
}

static int mock_get_char_impl(char *ch, uint32_t timeout)
{
    (void)ch;
    (void)timeout;
    return 0;
}

static size_t mock_send_impl(const char *data, size_t len)
{
    TEST_ASSERT_LESS_THAN_size_t(sizeof(g_tx), g_tx_len + len);
    memcpy(&g_tx[g_tx_len], data, len);
    g_tx_len += len;
    g_tx[g_tx_len] = '\0';
    return len;
}

static at_result_t led_query_impl(void);
static at_result_t led_setup_impl(const char *args);
static at_result_t led_exec_impl(void);

void setUp(void)
{
    RESET_FAKE(mock_get_char);
    RESET_FAKE(mock_send);
    RESET_FAKE(led_query);
    RESET_FAKE(led_setup);
    RESET_FAKE(led_exec);
    FFF_RESET_HISTORY();

    mock_get_char_fake.custom_fake = mock_get_char_impl;
    mock_send_fake.custom_fake = mock_send_impl;
    led_query_fake.custom_fake = led_query_impl;
    led_setup_fake.custom_fake = led_setup_impl;
    led_exec_fake.custom_fake = led_exec_impl;
}

void tearDown(void)
{
}

static at_result_t led_query_impl(void)
{
    at_server_printf(g_server, "+LED: %d", g_led_state);
    return ATS_RESULT_OK;
}

static at_result_t led_setup_impl(const char *args)
{
    if (!args || strcmp(args, "0") == 0) {
        g_led_state = 0;
        return ATS_RESULT_OK;
    }
    if (strcmp(args, "1") == 0) {
        g_led_state = 1;
        return ATS_RESULT_OK;
    }
    return ATS_RESULT_FAIL;
}

static at_result_t led_exec_impl(void)
{
    g_led_state = !g_led_state;
    return ATS_RESULT_OK;
}

static void test_server_lifecycle_hal_and_echo(void)
{
    at_server_t *server = at_server_create("ats");
    TEST_ASSERT_NOT_NULL(server);
    TEST_ASSERT_EQUAL(ATS_SERVER_STATUS_INITIALIZED, server->status);
    TEST_ASSERT_EQUAL_INT(-1, at_server_set_hal(NULL, mock_get_char, mock_send));
    TEST_ASSERT_EQUAL_INT(0, at_server_set_hal(server, mock_get_char, mock_send));
    TEST_ASSERT_EQUAL_PTR(mock_get_char, server->get_char);
    TEST_ASSERT_EQUAL_PTR(mock_send, server->send);
    TEST_ASSERT_EQUAL_UINT(0, mock_get_char_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(0, mock_send_fake.call_count);

    TEST_ASSERT_EQUAL_INT(-1, at_server_start(NULL));
    TEST_ASSERT_EQUAL_INT(0, at_server_start(server));
    TEST_ASSERT_EQUAL(ATS_SERVER_STATUS_RUNNING, server->status);
    TEST_ASSERT_EQUAL_INT(0, at_server_start(server));

    at_server_set_echo(server, false);
    TEST_ASSERT_FALSE(at_server_get_echo(server));
    at_server_set_echo(server, true);
    TEST_ASSERT_TRUE(at_server_get_echo(server));

    TEST_ASSERT_EQUAL_INT(0, at_server_stop(server));
    TEST_ASSERT_EQUAL(ATS_SERVER_STATUS_INITIALIZED, server->status);
}

static void test_register_find_process_and_stats(void)
{
    at_server_t *server = at_server_create("ats");
    TEST_ASSERT_NOT_NULL(server);
    TEST_ASSERT_EQUAL_INT(0, at_server_set_hal(server, mock_get_char, mock_send));
    g_server = server;

    at_cmd_t led = {0};
    strncpy(led.name, "LED", sizeof(led.name) - 1);
    led.query = led_query;
    led.setup = led_setup;
    led.exec = led_exec;

    TEST_ASSERT_EQUAL_INT(-1, at_server_register_cmd(NULL, &led));
    TEST_ASSERT_EQUAL_INT(-1, at_server_register_cmd(server, NULL));
    TEST_ASSERT_EQUAL_INT(0, at_server_register_cmd(server, &led));
    TEST_ASSERT_EQUAL_PTR(&led, at_server_find_cmd(server, "LED"));
    TEST_ASSERT_NULL(at_server_find_cmd(server, "UNKNOWN"));

    reset_tx();
    g_led_state = 1;
    TEST_ASSERT_EQUAL_INT(0, at_server_process_command(server, "AT+LED?"));
    TEST_ASSERT_EQUAL_UINT(1, led_query_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(2, mock_send_fake.call_count);
    TEST_ASSERT_NOT_NULL(strstr(g_tx, "+LED: 1"));
    TEST_ASSERT_NOT_NULL(strstr(g_tx, "OK\r\n"));

    reset_tx();
    TEST_ASSERT_EQUAL_INT(0, at_server_process_command(server, "AT+LED=0"));
    TEST_ASSERT_EQUAL_UINT(1, led_setup_fake.call_count);
    TEST_ASSERT_EQUAL_STRING("0", led_setup_fake.arg0_val);
    TEST_ASSERT_EQUAL_UINT(3, mock_send_fake.call_count);
    TEST_ASSERT_EQUAL_INT(0, g_led_state);
    TEST_ASSERT_EQUAL_STRING("OK\r\n", g_tx);

    reset_tx();
    TEST_ASSERT_EQUAL_INT(0, at_server_process_command(server, "AT+LED"));
    TEST_ASSERT_EQUAL_UINT(1, led_exec_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(4, mock_send_fake.call_count);
    TEST_ASSERT_EQUAL_INT(1, g_led_state);
    TEST_ASSERT_EQUAL_STRING("OK\r\n", g_tx);

    reset_tx();
    TEST_ASSERT_EQUAL_INT(-1, at_server_process_command(server, "AT+NOPE"));
    TEST_ASSERT_EQUAL_UINT(5, mock_send_fake.call_count);
    TEST_ASSERT_EQUAL_STRING("ERROR\r\n", g_tx);

    uint32_t processed = 0, ok = 0, err = 0;
    at_server_get_stats(server, &processed, &ok, &err);
    TEST_ASSERT_EQUAL_UINT32(3, processed);
    TEST_ASSERT_EQUAL_UINT32(3, ok);
    TEST_ASSERT_EQUAL_UINT32(1, err);

    at_server_reset_stats(server);
    at_server_get_stats(server, &processed, &ok, &err);
    TEST_ASSERT_EQUAL_UINT32(0, processed);
    TEST_ASSERT_EQUAL_UINT32(0, ok);
    TEST_ASSERT_EQUAL_UINT32(0, err);
}

static void test_response_helpers_and_parsers(void)
{
    at_server_t *server = at_server_create("ats");
    TEST_ASSERT_NOT_NULL(server);
    TEST_ASSERT_EQUAL_INT(0, at_server_set_hal(server, mock_get_char, mock_send));

    reset_tx();
    TEST_ASSERT_GREATER_THAN_INT(0, at_server_printf(server, "A=%d B=%s C=%x", -7, "ok", 0x2Au));
    TEST_ASSERT_EQUAL_UINT(1, mock_send_fake.call_count);
    TEST_ASSERT_EQUAL_STRING("A=-7 B=ok C=2A", g_tx);

    reset_tx();
    TEST_ASSERT_GREATER_THAN_INT(0, at_server_printfln(server, "line %d", 3));
    TEST_ASSERT_EQUAL_UINT(2, mock_send_fake.call_count);
    TEST_ASSERT_EQUAL_STRING("line 3\r\n", g_tx);

    reset_tx();
    TEST_ASSERT_EQUAL_INT(0, at_server_print_result(server, ATS_RESULT_NULL));
    TEST_ASSERT_EQUAL_UINT(2, mock_send_fake.call_count);
    TEST_ASSERT_EQUAL_size_t(0, g_tx_len);
    TEST_ASSERT_EQUAL_INT(-1, at_server_print_result(NULL, ATS_RESULT_OK));

    int value = 0;
    uint32_t hex = 0;
    char text[16];
    TEST_ASSERT_EQUAL_INT(0, at_parse_int("42", &value));
    TEST_ASSERT_EQUAL_INT(42, value);
    TEST_ASSERT_EQUAL_INT(0, at_parse_hex("2A", &hex));
    TEST_ASSERT_EQUAL_HEX32(0x2A, hex);
    TEST_ASSERT_EQUAL_INT(0, at_parse_string("hello", text, sizeof(text)));
    TEST_ASSERT_EQUAL_STRING("hello", text);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_server_lifecycle_hal_and_echo);
    RUN_TEST(test_register_find_process_and_stats);
    RUN_TEST(test_response_helpers_and_parsers);
    return UNITY_END();
}
