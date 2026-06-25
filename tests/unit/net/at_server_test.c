/**
 * @file at_server_test.c
 * @brief AT Server Unit Tests
 * @version 1.0.0
 * @date 2026-03-22
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "unity.h"

#include "xy_hal.h"
#include "xy_os.h"
#include "xy_log.h"
#include "xy_ats.h"

static char mock_tx_buffer[2048];
static size_t mock_tx_len = 0;
static int mock_get_char_index = 0;
static const char *mock_input = NULL;
static at_server_t *global_test_server = NULL;
static int test_led_state = 0;

static int mock_get_char(char *ch, uint32_t timeout)
{
    (void)timeout;
    if (mock_input && mock_get_char_index < (int)strlen(mock_input)) {
        *ch = mock_input[mock_get_char_index++];
        return 1;
    }
    return 0;
}

static size_t mock_send(const char *data, size_t len)
{
    if (mock_tx_len + len < sizeof(mock_tx_buffer)) {
        memcpy(mock_tx_buffer + mock_tx_len, data, len);
        mock_tx_len += len;
    }
    return len;
}

static at_result_t test_led_query(void)
{
    if (global_test_server) {
        at_server_printf(global_test_server, "+LED: %d", test_led_state);
    }
    return ATS_RESULT_OK;
}

static at_result_t test_led_setup(const char *args)
{
    if (!args || strlen(args) == 0) {
        return ATS_RESULT_FAIL;
    }

    int state = atoi(args);
    if (state == 0 || state == 1) {
        test_led_state = state;
        return ATS_RESULT_OK;
    }
    return ATS_RESULT_FAIL;
}

void setUp(void)
{
    memset(mock_tx_buffer, 0, sizeof(mock_tx_buffer));
    mock_tx_len = 0;
    mock_get_char_index = 0;
    mock_input = NULL;
    global_test_server = NULL;
    test_led_state = 0;
}

void tearDown(void)
{
}

void test_ats_init(void)
{
    at_server_t *server = at_server_create("test_server");
    TEST_ASSERT_NOT_NULL(server);

    int result = at_server_set_hal(server, mock_get_char, mock_send);
    TEST_ASSERT_EQUAL_INT(0, result);

    result = at_server_start(server);
    TEST_ASSERT_EQUAL_INT(0, result);

    at_server_stop(server);
    at_server_delete(server);
}

void test_ats_command_execution(void)
{
    at_server_t *server = at_server_create("test_server");
    TEST_ASSERT_NOT_NULL(server);

    int result = at_server_set_hal(server, mock_get_char, mock_send);
    TEST_ASSERT_EQUAL_INT(0, result);

    at_cmd_t led_cmd = {
        .args_expr = NULL,
        .test = NULL,
        .query = test_led_query,
        .setup = test_led_setup,
        .exec = NULL
    };
    strncpy(led_cmd.name, "LED", sizeof(led_cmd.name) - 1);
    led_cmd.name[sizeof(led_cmd.name) - 1] = '\0';

    result = at_server_register_cmd(server, &led_cmd);
    TEST_ASSERT_EQUAL_INT(0, result);

    global_test_server = server;

    result = at_server_start(server);
    TEST_ASSERT_EQUAL_INT(0, result);

    test_led_state = 1;
    TEST_ASSERT_EQUAL_INT(0, at_server_process_command(server, "AT+LED?"));
    TEST_ASSERT_NOT_NULL(strstr(mock_tx_buffer, "+LED: 1"));

    mock_tx_len = 0;
    mock_tx_buffer[0] = '\0';
    TEST_ASSERT_EQUAL_INT(0, at_server_process_command(server, "AT+LED=0"));
    TEST_ASSERT_EQUAL_INT(0, test_led_state);

    at_server_stop(server);
    at_server_delete(server);
}

void test_ats_hash_table(void)
{
    at_server_t *server = at_server_create("test_server");
    TEST_ASSERT_NOT_NULL(server);

    const char *cmd_names[] = {"AT+CMD1", "AT+CMD2", "AT+CMD3", "AT+CMD4", "AT+CMD5"};
    at_cmd_t cmds[5];

    for (int i = 0; i < 5; i++) {
        cmds[i] = (at_cmd_t){
            .args_expr = NULL,
            .test = NULL,
            .query = test_led_query,
            .setup = NULL,
            .exec = NULL
        };
        strncpy(cmds[i].name, cmd_names[i], sizeof(cmds[i].name) - 1);
        cmds[i].name[sizeof(cmds[i].name) - 1] = '\0';

        int result = at_server_register_cmd(server, &cmds[i]);
        TEST_ASSERT_EQUAL_INT(0, result);
    }

    for (int i = 0; i < 5; i++) {
        at_cmd_t *found = at_server_find_cmd(server, cmd_names[i]);
        TEST_ASSERT_NOT_NULL(found);
        TEST_ASSERT_EQUAL_STRING(cmd_names[i], found->name);
    }

    at_server_delete(server);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_ats_init);
    RUN_TEST(test_ats_command_execution);
    RUN_TEST(test_ats_hash_table);
    return UNITY_END();
}
