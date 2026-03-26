/**
 * @file at_client_test.c
 * @brief AT Client Unit Tests
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
#include "../components/net/at/atc/core/xy_at_client.h"

// Mock functions for testing
static char mock_response_buffer[1024];
static size_t mock_response_len = 0;
static int mock_send_called = 0;

static int mock_get_char(char *ch, uint32_t timeout)
{
    // This will be populated by test cases
    return 0;
}

static int mock_uart_send(const char *data, size_t len)
{
    mock_send_called++;
    // Store sent data for verification
    if (mock_response_len + len < sizeof(mock_response_buffer)) {
        memcpy(mock_response_buffer + mock_response_len, data, len);
        mock_response_len += len;
    }
    return len;
}

static size_t mock_uart_recv(char *data, size_t len)
{
    // This will be populated by test cases
    return 0;
}

// Test case: Initialize AT Client
void test_atc_init(void)
{
    //"Running test_atc_init...\n");
    
    xy_at_client_t *client = xy_at_client_create("test_client", 256, 256);
    assert(client != NULL);
    
    int result = xy_at_client_set_hal(client, mock_get_char, mock_uart_send, mock_uart_recv);
    assert(result == 0);
    
    // Start client (if needed)
    // result = xy_at_client_start(client);
    // assert(result == 0);
    
    // xy_at_client_stop(client);
    xy_at_client_delete(client);
    
    //"test_atc_init PASSED\n");
}

// Test case: Send basic AT command
void test_atc_send_command(void)
{
    //"Running test_atc_send_command...\n");
    
    xy_at_client_t *client = xy_at_client_create("test_client", 256, 256);
    assert(client != NULL);
    
    int result = xy_at_client_set_hal(client, mock_get_char, mock_uart_send, mock_uart_recv);
    assert(result == 0);
    
    // Reset mock state
    mock_send_called = 0;
    mock_response_len = 0;
    
    // Send AT command using exec_cmd
    xy_at_resp_status_t resp_status = xy_at_exec_cmd(client, NULL, "AT");
    assert(resp_status == XY_AT_RESP_OK);
    assert(mock_send_called > 0);
    
    xy_at_client_delete(client);
    
    //"test_atc_send_command PASSED\n");
}

int main(void)
{
    //"=== AT Client Unit Tests ===\n");
    
    test_atc_init();
    test_atc_send_command();
    
    //"=== All AT Client Tests PASSED ===\n");
    return 0;
}