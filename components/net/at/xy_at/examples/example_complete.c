/**
 * @file example_complete.c
 * @brief Complete XY AT Framework Example (Client + Server)
 * @version 1.0.0
 *
 * This example demonstrates:
 * 1. AT Client connecting to a GSM modem
 * 2. AT Server providing custom AT commands
 * 3. Configuration via xy_at_cfg.h
 */

#include "../xy_at.h"
#include "../../../../osal/xy_os.h"
#include <stdio.h>
#include <stdint.h>

/* ==================== HAL Implementation (UART) ==================== */

// Example UART HAL for client (connects to modem)
static int modem_uart_get_char(char *ch, uint32_t timeout)
{
    // TODO: Implement UART receive with timeout
    // Example: return uart1_read_char(ch, timeout);
    (void)ch;
    (void)timeout;
    return -1; // Not implemented
}

static size_t modem_uart_send(const char *data, size_t len)
{
    // TODO: Implement UART send
    // Example: return uart1_write(data, len);
    printf("TX Modem: %.*s", (int)len, data);
    return len;
}

static size_t modem_uart_recv(char *data, size_t len)
{
    // TODO: Implement UART receive
    // Example: return uart1_read(data, len);
    (void)data;
    (void)len;
    return 0;
}

// Example UART HAL for server (provides AT interface)
static int host_uart_get_char(char *ch, uint32_t timeout)
{
    // TODO: Implement UART receive from host
    // Example: return uart2_read_char(ch, timeout);
    (void)ch;
    (void)timeout;
    return -1;
}

static size_t host_uart_send(const char *data, size_t len)
{
    // TODO: Implement UART send to host
    // Example: return uart2_write(data, len);
    printf("TX Host: %.*s", (int)len, data);
    return len;
}

/* ==================== AT Client Example (GSM Modem) ==================== */

#ifdef XY_AT_USING_CLIENT

static xy_at_client_t *g_modem_client = NULL;

// URC Handler: Signal Quality (+CSQ)
static void on_signal_quality_urc(xy_at_client_t *client, const char *data,
                                  size_t size)
{
    int rssi, ber;
    if (sscanf(data, "+CSQ: %d,%d", &rssi, &ber) == 2) {
        printf("[URC] Signal Quality: RSSI=%d, BER=%d\n", rssi, ber);
    }
}

// URC Handler: Network Registration (+CREG)
static void on_network_registration_urc(xy_at_client_t *client,
                                        const char *data, size_t size)
{
    int n, stat;
    if (sscanf(data, "+CREG: %d,%d", &n, &stat) == 2) {
        printf("[URC] Network Registration: stat=%d\n", stat);
    }
}

// URC table
static const xy_at_urc_t modem_urc_table[] = {
    { "+CSQ:", NULL, on_signal_quality_urc },
    { "+CREG:", NULL, on_network_registration_urc },
    { "+CPIN:", NULL, NULL }, // Add handler as needed
};

static int init_modem_client(void)
{
    // Create client
    g_modem_client = xy_at_client_create("gsm_modem", 256, 1024);
    if (!g_modem_client) {
        printf("Failed to create modem client\n");
        return -1;
    }

    // Set HAL interface
    xy_at_client_set_hal(
        g_modem_client, modem_uart_get_char, modem_uart_send, modem_uart_recv);

    // Set URC handlers
    xy_at_set_urc_table(g_modem_client, modem_urc_table,
                        sizeof(modem_urc_table) / sizeof(modem_urc_table[0]));

    printf("Modem client initialized\n");
    return 0;
}

static void test_modem_client(void)
{
    if (!g_modem_client)
        return;

    xy_at_response_t *resp = xy_at_create_resp(512, 0, 5000);
    if (!resp)
        return;

    printf("\n=== Testing Modem Client ===\n");

    // Test 1: Basic AT command
    printf("Test 1: AT\n");
    if (xy_at_exec_cmd(g_modem_client, resp, "AT") == XY_AT_RESP_OK) {
        printf("  OK\n");
    }

    // Test 2: Get manufacturer
    printf("Test 2: AT+CGMI\n");
    if (xy_at_exec_cmd(g_modem_client, resp, "AT+CGMI") == XY_AT_RESP_OK) {
        const char *mfr = xy_at_resp_get_line(resp, 1);
        if (mfr)
            printf("  Manufacturer: %s\n", mfr);
    }

    // Test 3: Get IMEI
    printf("Test 3: AT+GSN\n");
    if (xy_at_exec_cmd(g_modem_client, resp, "AT+GSN") == XY_AT_RESP_OK) {
        const char *imei = xy_at_resp_get_line(resp, 1);
        if (imei)
            printf("  IMEI: %s\n", imei);
    }

    // Test 4: Get signal quality
    printf("Test 4: AT+CSQ\n");
    if (xy_at_exec_cmd(g_modem_client, resp, "AT+CSQ") == XY_AT_RESP_OK) {
        int rssi, ber;
        const char *line = xy_at_resp_get_line_by_prefix(resp, "+CSQ:");
        if (line
            && xy_at_resp_parse_line_args(line, "+CSQ: %d,%d", &rssi, &ber)
                   == 2) {
            printf("  RSSI: %d, BER: %d\n", rssi, ber);
        }
    }

    xy_at_delete_resp(resp);
}

#endif /* XY_AT_USING_CLIENT */

/* ==================== AT Server Example (Custom Commands) ====================
 */

#ifdef XY_AT_USING_SERVER

static xy_at_server_t *g_at_server = NULL;

// AT Command: AT
static xy_at_result_t cmd_at_exec(void)
{
    return XY_AT_RESULT_OK;
}

// AT Command: ATI (Information)
static xy_at_result_t cmd_ati_exec(void)
{
    xy_at_server_printfln(g_at_server, "XinYi AT Server");
    xy_at_server_printfln(g_at_server, "Version: %s", xy_at_get_version());
    xy_at_server_printfln(g_at_server, "Build: %s %s", __DATE__, __TIME__);
    return XY_AT_RESULT_OK;
}

// AT Command: ATE (Echo control)
static xy_at_result_t cmd_ate_setup(const char *args)
{
    int enable;
    if (xy_at_parse_int(args, &enable) == 0) {
        xy_at_server_set_echo(g_at_server, enable != 0);
        return XY_AT_RESULT_OK;
    }
    return XY_AT_RESULT_PARSE_ERR;
}

// AT Command: AT+VER (Version)
static xy_at_result_t cmd_ver_test(void)
{
    xy_at_server_printfln(g_at_server, "+VER: (string)");
    return XY_AT_RESULT_OK;
}

static xy_at_result_t cmd_ver_query(void)
{
    xy_at_server_printfln(g_at_server, "+VER: \"%s\"", xy_at_get_version());
    return XY_AT_RESULT_OK;
}

// AT Command: AT+LED (LED control)
static xy_at_result_t cmd_led_test(void)
{
    xy_at_server_printfln(g_at_server, "+LED: (0,1)");
    return XY_AT_RESULT_OK;
}

static xy_at_result_t cmd_led_query(void)
{
    // TODO: Read actual LED state
    int led_state = 0; // Example
    xy_at_server_printfln(g_at_server, "+LED: %d", led_state);
    return XY_AT_RESULT_OK;
}

static xy_at_result_t cmd_led_setup(const char *args)
{
    int state;
    if (xy_at_parse_int(args, &state) == 0) {
        if (state == 0 || state == 1) {
            // TODO: Control LED
            printf("[LED] Set to %d\n", state);
            return XY_AT_RESULT_OK;
        }
    }
    return XY_AT_RESULT_PARSE_ERR;
}

// AT Command: AT+ADC (Read ADC)
static xy_at_result_t cmd_adc_test(void)
{
    xy_at_server_printfln(g_at_server, "+ADC: (0-4095)");
    return XY_AT_RESULT_OK;
}

static xy_at_result_t cmd_adc_query(void)
{
    // TODO: Read actual ADC value
    uint16_t adc_value = 2048; // Example
    xy_at_server_printfln(g_at_server, "+ADC: %d", adc_value);
    return XY_AT_RESULT_OK;
}

// AT Command: AT+TEMP (Read Temperature)
static xy_at_result_t cmd_temp_query(void)
{
    // TODO: Read actual temperature
    int temp = 25; // Example: 25°C
    xy_at_server_printfln(g_at_server, "+TEMP: %d", temp);
    return XY_AT_RESULT_OK;
}

static int init_at_server(void)
{
    // Create server
    g_at_server = xy_at_server_create("custom_server");
    if (!g_at_server) {
        printf("Failed to create AT server\n");
        return -1;
    }

    // Set HAL interface
    xy_at_server_set_hal(g_at_server, host_uart_get_char, host_uart_send);

    // Register standard commands
    static const xy_at_cmd_t cmd_at = {
        .name = "AT",
        .exec = cmd_at_exec,
    };
    xy_at_server_register_cmd(g_at_server, &cmd_at);

    static const xy_at_cmd_t cmd_ati = {
        .name = "ATI",
        .exec = cmd_ati_exec,
    };
    xy_at_server_register_cmd(g_at_server, &cmd_ati);

    static const xy_at_cmd_t cmd_ate = {
        .name  = "ATE",
        .setup = cmd_ate_setup,
    };
    xy_at_server_register_cmd(g_at_server, &cmd_ate);

    // Register custom commands
    static const xy_at_cmd_t cmd_ver = {
        .name  = "AT+VER",
        .test  = cmd_ver_test,
        .query = cmd_ver_query,
    };
    xy_at_server_register_cmd(g_at_server, &cmd_ver);

    static const xy_at_cmd_t cmd_led = {
        .name  = "AT+LED",
        .test  = cmd_led_test,
        .query = cmd_led_query,
        .setup = cmd_led_setup,
    };
    xy_at_server_register_cmd(g_at_server, &cmd_led);

    static const xy_at_cmd_t cmd_adc = {
        .name  = "AT+ADC",
        .test  = cmd_adc_test,
        .query = cmd_adc_query,
    };
    xy_at_server_register_cmd(g_at_server, &cmd_adc);

    static const xy_at_cmd_t cmd_temp = {
        .name  = "AT+TEMP",
        .query = cmd_temp_query,
    };
    xy_at_server_register_cmd(g_at_server, &cmd_temp);

    // Start server
    if (xy_at_server_start(g_at_server) != 0) {
        printf("Failed to start AT server\n");
        return -1;
    }

    printf("AT server initialized and started\n");
    printf("Registered commands:\n");
    printf("  AT, ATI, ATE\n");
    printf("  AT+VER=?, AT+VER?\n");
    printf("  AT+LED=?, AT+LED?, AT+LED=<0|1>\n");
    printf("  AT+ADC=?, AT+ADC?\n");
    printf("  AT+TEMP?\n");

    return 0;
}

#endif /* XY_AT_USING_SERVER */

/* ==================== Main Application ==================== */

int main(void)
{
    printf("\n");
    printf("========================================\n");
    printf(" XY AT Framework Complete Example\n");
    printf(" Version: %s\n", xy_at_get_version());
    printf("========================================\n\n");

    // Initialize XY OSAL
    xy_os_kernel_init();

    // Initialize AT framework
    xy_at_init();

#ifdef XY_AT_USING_CLIENT
    // Initialize and test AT client
    printf("--- AT Client Example ---\n");
    if (init_modem_client() == 0) {
        test_modem_client();
    }
    printf("\n");
#endif

#ifdef XY_AT_USING_SERVER
    // Initialize AT server
    printf("--- AT Server Example ---\n");
    init_at_server();
    printf("\n");
#endif

    printf("Configuration:\n");
    printf("  XY_AT_USING_CLIENT: %d\n", XY_AT_USING_CLIENT);
    printf("  XY_AT_USING_SERVER: %d\n", XY_AT_USING_SERVER);
    printf("  XY_AT_CMD_MAX_LEN: %d\n", XY_AT_CMD_MAX_LEN);
    printf("  XY_AT_RESP_MAX_LEN: %d\n", XY_AT_RESP_MAX_LEN);
    printf("  XY_AT_DEFAULT_TIMEOUT: %d ms\n", XY_AT_DEFAULT_TIMEOUT);
    printf("\n");

    // Start RTOS scheduler
    xy_os_kernel_start();

    // Never reached
    while (1) {
        xy_os_delay(1000);
    }

    return 0;
}
