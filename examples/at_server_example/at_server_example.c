/**
 * @file at_server_example.c
 * @brief AT Server Usage Example
 * @version 1.0.0
 * @date 2026-03-21
 * 
 * This example demonstrates how to use the AT Server with hash table command mapping.
 * It creates a simple AT server that supports basic commands for testing.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Include XinYi framework headers
#include "xy_ats.h"  // AT Server header directly
#include "xy_os.h"   // OS Abstraction Layer
#include "xy_log.h"  // Logging system

// Global variables for the example
static int g_led_state = 0;
static int g_pwm_value = 50;

/* ==================== Simulated UART Interface ==================== */

/**
 * @brief Simulated UART receive function
 * In real hardware, this would read from UART
 */
static int example_get_char(char *ch, uint32_t timeout)
{
    // For PC simulation, we'll simulate receiving characters
    // In real implementation, this would call xy_hal_uart_read()
    
    static int simulated_input_index = 0;
    static const char *simulated_input = "AT+LED=1\r\n";
    
    if (simulated_input_index < strlen(simulated_input)) {
        *ch = simulated_input[simulated_input_index++];
        return 1; // Success
    }
    
    // Reset for next test
    simulated_input_index = 0;
    return 0; // Timeout
}

/**
 * @brief Simulated UART send function  
 * In real hardware, this would write to UART
 */
static size_t example_send(const char *data, size_t len)
{
    // For PC simulation, print to stdout
    // In real implementation, this would call xy_hal_uart_write()
    printf("%.*s", (int)len, data);
    fflush(stdout);
    return len;
}

/* ==================== AT Command Handlers ==================== */

/**
 * @brief Test handler for AT+LED command
 * Returns: +LED: (0-1)
 */
static at_result_t led_test_handler(void)
{
    at_server_t *server = at_server_get_by_name("example_server");
    if (server) {
        at_server_printfln(server, "+LED: (0-1)");
    }
    return ATS_RESULT_OK;
}

/**
 * @brief Query handler for AT+LED command  
 * Returns: +LED: <state>
 */
static at_result_t led_query_handler(void)
{
    at_server_t *server = at_server_get_by_name("example_server");
    if (server) {
        at_server_printf(server, "+LED: %d", g_led_state);
    }
    return ATS_RESULT_OK;
}

/**
 * @brief Setup handler for AT+LED command
 * Sets LED state: AT+LED=<state>
 */
static at_result_t led_setup_handler(const char *args)
{
    if (!args || strlen(args) == 0) {
        return ATS_RESULT_FAIL;
    }
    
    int state = atoi(args);
    if (state == 0 || state == 1) {
        g_led_state = state;
        XY_LOG_I("LED state set to: %d\n", state);
        return ATS_RESULT_OK;
    }
    
    return ATS_RESULT_FAIL;
}

/**
 * @brief Execute handler for AT+RST command
 * Resets the system: AT+RST
 */
static at_result_t rst_exec_handler(void)
{
    at_server_t *server = at_server_get_by_name("example_server");
    if (server) {
        at_server_printfln(server, "System resetting...");
    }
    
    XY_LOG_I("System reset requested\n");
    // In real implementation, this would trigger a system reset
    // For simulation, just log it
    
    return ATS_RESULT_OK;
}

/**
 * @brief Test handler for AT+PWM command
 * Returns: +PWM: (0-100)
 */
static at_result_t pwm_test_handler(void)
{
    at_server_t *server = at_server_get_by_name("example_server");
    if (server) {
        at_server_printfln(server, "+PWM: (0-100)");
    }
    return ATS_RESULT_OK;
}

/**
 * @brief Query handler for AT+PWM command
 * Returns: +PWM: <value>
 */
static at_result_t pwm_query_handler(void)
{
    at_server_t *server = at_server_get_by_name("example_server");
    if (server) {
        at_server_printf(server, "+PWM: %d", g_pwm_value);
    }
    return ATS_RESULT_OK;
}

/**
 * @brief Setup handler for AT+PWM command  
 * Sets PWM value: AT+PWM=<value>
 */
static at_result_t pwm_setup_handler(const char *args)
{
    if (!args || strlen(args) == 0) {
        return ATS_RESULT_FAIL;
    }
    
    int value = atoi(args);
    if (value >= 0 && value <= 100) {
        g_pwm_value = value;
        XY_LOG_I("PWM value set to: %d\n", value);
        return ATS_RESULT_OK;
    }
    
    return ATS_RESULT_FAIL;
}

/* ==================== Main Function ==================== */

int main(void)
{
    XY_LOG_I("AT Server Example Started\n");
    
    // Create AT server instance
    at_server_t *server = at_server_create("example_server");
    if (!server) {
        XY_LOG_E("Failed to create AT server\n");
        return -1;
    }
    
    // Set HAL interface (simulated UART)
    if (at_server_set_hal(server, example_get_char, example_send) != 0) {
        XY_LOG_E("Failed to set HAL interface\n");
        at_server_delete(server);
        return -1;
    }
    
    // Register AT commands
    at_cmd_t led_cmd = {
        .name = "AT+LED",
        .args_expr = NULL,
        .test = led_test_handler,
        .query = led_query_handler,
        .setup = led_setup_handler,
        .exec = NULL
    };
    
    at_cmd_t rst_cmd = {
        .name = "AT+RST",
        .args_expr = NULL,
        .test = NULL,
        .query = NULL,
        .setup = NULL,
        .exec = rst_exec_handler
    };
    
    at_cmd_t pwm_cmd = {
        .name = "AT+PWM",
        .args_expr = NULL,
        .test = pwm_test_handler,
        .query = pwm_query_handler,
        .setup = pwm_setup_handler,
        .exec = NULL
    };
    
    if (at_server_register_cmd(server, &led_cmd) != 0) {
        XY_LOG_E("Failed to register AT+LED command\n");
    }
    
    if (at_server_register_cmd(server, &rst_cmd) != 0) {
        XY_LOG_E("Failed to register AT+RST command\n");
    }
    
    if (at_server_register_cmd(server, &pwm_cmd) != 0) {
        XY_LOG_E("Failed to register AT+PWM command\n");
    }
    
    XY_LOG_I("Registered commands:\n");
    XY_LOG_I("- AT+LED=?     : Test LED command\n");
    XY_LOG_I("- AT+LED?      : Query LED state\n");
    XY_LOG_I("- AT+LED=<0|1> : Set LED state\n");
    XY_LOG_I("- AT+RST       : Reset system\n");
    XY_LOG_I("- AT+PWM=?     : Test PWM command\n");
    XY_LOG_I("- AT+PWM?      : Query PWM value\n");
    XY_LOG_I("- AT+PWM=<0-100>: Set PWM value\n");
    
    // Start AT server
    if (at_server_start(server) != 0) {
        XY_LOG_E("Failed to start AT server\n");
        at_server_delete(server);
        return -1;
    }
    
    XY_LOG_I("AT Server started successfully!\n");
    XY_LOG_I("Simulating AT command: AT+LED=1\\r\\n\n");
    
    // Wait for server to process commands
    sleep(2);
    
    // Get and display statistics
    uint32_t processed, ok, error;
    at_server_get_stats(server, &processed, &ok, &error);
    XY_LOG_I("AT Server Statistics:\n");
    XY_LOG_I("- Commands processed: %u\n", processed);
    XY_LOG_I("- OK responses: %u\n", ok);
    XY_LOG_I("- Error responses: %u\n", error);
    
    // Stop and cleanup
    at_server_stop(server);
    at_server_delete(server);
    
    XY_LOG_I("AT Server Example Completed\n");
    return 0;
}