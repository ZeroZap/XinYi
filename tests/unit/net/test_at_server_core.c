#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "xy_ats.h"

static char g_tx[1024];
static size_t g_tx_len;
static int g_led_state;
static at_server_t *g_server;

void xy_log_char(char ch)
{
    (void)ch;
}

static void reset_tx(void)
{
    memset(g_tx, 0, sizeof(g_tx));
    g_tx_len = 0;
}

static int mock_get_char(char *ch, uint32_t timeout)
{
    (void)ch;
    (void)timeout;
    return 0;
}

static size_t mock_send(const char *data, size_t len)
{
    assert(g_tx_len + len < sizeof(g_tx));
    memcpy(&g_tx[g_tx_len], data, len);
    g_tx_len += len;
    g_tx[g_tx_len] = '\0';
    return len;
}

static at_result_t led_query(void)
{
    at_server_printf(g_server, "+LED: %d", g_led_state);
    return ATS_RESULT_OK;
}

static at_result_t led_setup(const char *args)
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

static at_result_t led_exec(void)
{
    g_led_state = !g_led_state;
    return ATS_RESULT_OK;
}

static void test_server_lifecycle_hal_and_echo(void)
{
    at_server_t *server = at_server_create("ats");
    assert(server != NULL);
    assert(server->status == ATS_SERVER_STATUS_INITIALIZED);
    assert(at_server_set_hal(NULL, mock_get_char, mock_send) == -1);
    assert(at_server_set_hal(server, mock_get_char, mock_send) == 0);

    assert(at_server_start(NULL) == -1);
    assert(at_server_start(server) == 0);
    assert(server->status == ATS_SERVER_STATUS_RUNNING);
    assert(at_server_start(server) == 0);

    at_server_set_echo(server, false);
    assert(!at_server_get_echo(server));
    at_server_set_echo(server, true);
    assert(at_server_get_echo(server));

    assert(at_server_stop(server) == 0);
    assert(server->status == ATS_SERVER_STATUS_INITIALIZED);
}

static void test_register_find_process_and_stats(void)
{
    at_server_t *server = at_server_create("ats");
    assert(server != NULL);
    assert(at_server_set_hal(server, mock_get_char, mock_send) == 0);
    g_server = server;

    at_cmd_t led = {0};
    strncpy(led.name, "LED", sizeof(led.name) - 1);
    led.query = led_query;
    led.setup = led_setup;
    led.exec = led_exec;

    assert(at_server_register_cmd(NULL, &led) == -1);
    assert(at_server_register_cmd(server, NULL) == -1);
    assert(at_server_register_cmd(server, &led) == 0);
    assert(at_server_find_cmd(server, "LED") == &led);
    assert(at_server_find_cmd(server, "UNKNOWN") == NULL);

    reset_tx();
    g_led_state = 1;
    assert(at_server_process_command(server, "AT+LED?") == 0);
    assert(strstr(g_tx, "+LED: 1") != NULL);
    assert(strstr(g_tx, "OK\r\n") != NULL);

    reset_tx();
    assert(at_server_process_command(server, "AT+LED=0") == 0);
    assert(g_led_state == 0);
    assert(strcmp(g_tx, "OK\r\n") == 0);

    reset_tx();
    assert(at_server_process_command(server, "AT+LED") == 0);
    assert(g_led_state == 1);
    assert(strcmp(g_tx, "OK\r\n") == 0);

    reset_tx();
    assert(at_server_process_command(server, "AT+NOPE") == -1);
    assert(strcmp(g_tx, "ERROR\r\n") == 0);

    uint32_t processed = 0, ok = 0, err = 0;
    at_server_get_stats(server, &processed, &ok, &err);
    assert(processed == 3);
    assert(ok == 3);
    assert(err == 1);

    at_server_reset_stats(server);
    at_server_get_stats(server, &processed, &ok, &err);
    assert(processed == 0 && ok == 0 && err == 0);
}

static void test_response_helpers_and_parsers(void)
{
    at_server_t *server = at_server_create("ats");
    assert(server != NULL);
    assert(at_server_set_hal(server, mock_get_char, mock_send) == 0);

    reset_tx();
    assert(at_server_printf(server, "A=%d B=%s C=%x", -7, "ok", 0x2Au) > 0);
    assert(strcmp(g_tx, "A=-7 B=ok C=2A") == 0);

    reset_tx();
    assert(at_server_printfln(server, "line %d", 3) > 0);
    assert(strcmp(g_tx, "line 3\r\n") == 0);

    reset_tx();
    assert(at_server_print_result(server, ATS_RESULT_NULL) == 0);
    assert(g_tx_len == 0);
    assert(at_server_print_result(NULL, ATS_RESULT_OK) == -1);

    int value = 0;
    uint32_t hex = 0;
    char text[16];
    assert(at_parse_int("42", &value) == 0 && value == 42);
    assert(at_parse_hex("2A", &hex) == 0 && hex == 0x2A);
    assert(at_parse_string("hello", text, sizeof(text)) == 0 && strcmp(text, "hello") == 0);
}

int main(void)
{
    test_server_lifecycle_hal_and_echo();
    test_register_find_process_and_stats();
    test_response_helpers_and_parsers();
    return 0;
}
