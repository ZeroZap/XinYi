/**
 * @file at_server_test.c
 * @brief AT Server Unit Tests
 * @version 1.0.0
 * @date 2026-03-22
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

// Include XinYi framework headers
#include "xy_hal.h"
#include "xy_os.h"
#include "xy_log.h"
#include "xy_ats.h"

// Global test variables
static char mock_tx_buffer[2048];
static size_t mock_tx_len = 0;
static int mock_get_char_index = 0;
static const char *mock_input = NULL;
static at_server_t *global_test_server = NULL;  // Add global server reference

// Mock HAL functions
static int mock_get_char(char *ch, uint32_t timeout)
{
    if (mock_input && mock_get_char_index < strlen(mock_input)) {
        *ch = mock_input[mock_get_char_index++];
        return 1;
    }
    return 0; // Timeout
}

static size_t mock_send(const char *data, size_t len)
{
    if (mock_tx_len + len < sizeof(mock_tx_buffer)) {
        memcpy(mock_tx_buffer + mock_tx_len, data, len);
        mock_tx_len += len;
    }
    return len;
}

// Test command handlers
static int test_led_state = 0;

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

// Test case: Initialize AT Server
void test_ats_init(void)
{
    // //"Running test_ats_init...\n");
    
    at_server_t *server = at_server_create("test_server");
    assert(server != NULL);
    
    int result = at_server_set_hal(server, mock_get_char, mock_send);
    assert(result == 0);
    
    result = at_server_start(server);
    assert(result == 0);
    
    at_server_stop(server);
    at_server_delete(server);
    
    // //"test_ats_init PASSED\n");
}

// Test case: Register and execute commands
void test_ats_command_execution(void)
{
    //"Running test_ats_command_execution...\n");
    
    at_server_t *server = at_server_create("test_server");
    assert(server != NULL);
    
    int result = at_server_set_hal(server, mock_get_char, mock_send);
    assert(result == 0);
    
    // Register test command - use base name without AT+ prefix
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
    assert(result == 0);
    
    // Set global server reference for test handlers
    global_test_server = server;
    
    result = at_server_start(server);
    assert(result == 0);
    
    // Test query command
    mock_tx_len = 0;
    test_led_state = 1;
    at_server_process_command(server, "AT+LED?");
    assert(strstr(mock_tx_buffer, "+LED: 1") != NULL);
    
    // Test setup command
    mock_tx_len = 0;
    mock_tx_buffer[0] = '\0';
    at_server_process_command(server, "AT+LED=0");
    test_led_state = 0; // Should be set by handler
    
    at_server_stop(server);
    at_server_delete(server);
    
    //"test_ats_command_execution PASSED\n");
}

// Test case: Hash table performance
void test_ats_hash_table(void)
{
    //"Running test_ats_hash_table...\n");
    
    at_server_t *server = at_server_create("test_server");
    assert(server != NULL);
    
    // Register multiple commands to test hash table
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
        assert(result == 0);
    }
    
    // Verify all commands can be found
    for (int i = 0; i < 5; i++) {
        at_cmd_t *found = at_server_find_cmd(server, cmd_names[i]);
        assert(found != NULL);
        assert(strcmp(found->name, cmd_names[i]) == 0);
    }
    
    at_server_delete(server);
    
    //"test_ats_hash_table PASSED\n");
}

int main(void)
{
    //"=== AT Server Unit Tests ===\n");
    
    test_ats_init();
    test_ats_command_execution();
    test_ats_hash_table();
    
    //"=== All AT Server Tests PASSED ===\n");
    return 0;
}
