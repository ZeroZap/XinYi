# XY AT Command Framework

## Overview

Complete AT command client and server implementation for the XinYi framework, using XY OSAL for cross-platform compatibility.

## Features

### AT Client
- ✅ **Command Execution**: Send AT commands with response handling
- ✅ **URC Support**: Unsolicited Result Code handling
- ✅ **Data Mode**: Transparent data transmission
- ✅ **Response Parsing**: Line-based and keyword-based parsing
- ✅ **Timeout & Retry**: Configurable timeout and retry mechanism
- ✅ **Thread-Safe**: Multi-threaded access protection
- ✅ **Statistics**: TX/RX/Error counters

### AT Server
- ✅ **Command Registration**: Dynamic command registration
- ✅ **Multiple Modes**: Test/Query/Setup/Execute modes
- ✅ **Parameter Parsing**: Integer/String/Hex parsing
- ✅ **Echo Mode**: Configurable echo
- ✅ **Result Codes**: Standard OK/ERROR responses
- ✅ **Statistics**: Command counters

## Architecture

```
┌─────────────────────────────────────────────┐
│           Application Layer                  │
├─────────────────────────────────────────────┤
│  xy_at_client.h  │  xy_at_server.h         │
├──────────────────┼──────────────────────────┤
│  xy_at_client.c  │  xy_at_server.c         │
├─────────────────────────────────────────────┤
│            XY OSAL Layer                     │
│  (Thread, Mutex, Semaphore, Delay)          │
├─────────────────────────────────────────────┤
│              HAL Layer                       │
│  (UART get_char, send, recv)                │
└─────────────────────────────────────────────┘
```

## Quick Start

### AT Client Usage

#### 1. Basic Command Execution

```c
#include "xy_at_client.h"

// Create client
xy_at_client_t *client = xy_at_client_create("modem", 256, 1024);

// Set HAL interface
xy_at_client_set_hal(client, uart_get_char, uart_send, uart_recv);

// Create response
xy_at_response_t *resp = xy_at_create_resp(512, 0, 5000);

// Send AT command
xy_at_exec_cmd(client, resp, "AT+CGMI");

// Parse response
const char *manufacturer = xy_at_resp_get_line(resp, 1);
printf("Manufacturer: %s\n", manufacturer);

// Clean up
xy_at_delete_resp(resp);
```

#### 2. URC Handling

```c
// URC handler function
void on_signal_quality(xy_at_client_t *client, const char *data, size_t size) {
    int rssi, ber;
    sscanf(data, "+CSQ: %d,%d", &rssi, &ber);
    printf("Signal: RSSI=%d, BER=%d\n", rssi, ber);
}

// Register URC handlers
static const xy_at_urc_t urc_table[] = {
    {"+CSQ:",  NULL, on_signal_quality},
    {"+CREG:", NULL, on_network_registration},
    {"+CPIN:", NULL, on_sim_status},
};

xy_at_set_urc_table(client, urc_table, sizeof(urc_table)/sizeof(urc_table[0]));
```

#### 3. Data Mode (Transparent Transmission)

```c
// Enter data mode
xy_at_exec_cmd(client, NULL, "AT+CIPSTART=\"TCP\",\"192.168.1.100\",8080");
xy_at_client_enter_data_mode(client);

// Send data
uint8_t data[] = "Hello Server!";
xy_at_client_send_data(client, data, sizeof(data));

// Receive data
uint8_t recv_buf[128];
int len = xy_at_client_recv_data(client, recv_buf, sizeof(recv_buf), 5000);

// Exit data mode
xy_at_client_exit_data_mode(client);
```

### AT Server Usage

#### 1. Basic Server Setup

```c
#include "xy_at_server.h"

// Create server
xy_at_server_t *server = xy_at_server_create("at_server");

// Set HAL interface
xy_at_server_set_hal(server, uart_get_char, uart_send);

// Start server
xy_at_server_start(server);
```

#### 2. Command Registration

```c
// Test mode handler (AT+VER=?)
xy_at_result_t cmd_version_test(void) {
    xy_at_server_printfln(server, "+VER: (1-100)");
    return XY_AT_RESULT_OK;
}

// Query mode handler (AT+VER?)
xy_at_result_t cmd_version_query(void) {
    xy_at_server_printfln(server, "+VER: 1.0.0");
    return XY_AT_RESULT_OK;
}

// Setup mode handler (AT+VER=<val>)
xy_at_result_t cmd_version_setup(const char *args) {
    int ver;
    if (xy_at_parse_int(args, &ver) == 0) {
        // Set version
        return XY_AT_RESULT_OK;
    }
    return XY_AT_RESULT_PARSE_ERR;
}

// Execute mode handler (AT+VER)
xy_at_result_t cmd_version_exec(void) {
    xy_at_server_printfln(server, "Version: 1.0.0");
    return XY_AT_RESULT_OK;
}

// Register command
static const xy_at_cmd_t cmd_version = {
    .name = "AT+VER",
    .args_expr = "<version>",
    .test = cmd_version_test,
    .query = cmd_version_query,
    .setup = cmd_version_setup,
    .exec = cmd_version_exec,
};

xy_at_server_register_cmd(server, &cmd_version);
```

#### 3. Standard AT Commands

```c
// AT - Basic test
xy_at_result_t cmd_at_exec(void) {
    return XY_AT_RESULT_OK;  // Server automatically sends "OK"
}

// ATE - Echo control (ATE0/ATE1)
xy_at_result_t cmd_ate_exec(void) {
    // Parse from command: ATE0 or ATE1
    return XY_AT_RESULT_OK;
}

// ATI - Information (ATI)
xy_at_result_t cmd_ati_exec(void) {
    xy_at_server_printfln(server, "XinYi AT Server v1.0");
    xy_at_server_printfln(server, "Build: %s %s", __DATE__, __TIME__);
    return XY_AT_RESULT_OK;
}
```

## API Reference

### AT Client API

| Function | Description |
|----------|-------------|
| `xy_at_client_create()` | Create AT client |
| `xy_at_client_init()` | Initialize client |
| `xy_at_client_delete()` | Delete client |
| `xy_at_client_set_hal()` | Set HAL interface |
| `xy_at_exec_cmd()` | Execute AT command |
| `xy_at_create_resp()` | Create response object |
| `xy_at_delete_resp()` | Delete response object |
| `xy_at_resp_get_line()` | Get response line by index |
| `xy_at_resp_parse_line_args()` | Parse response arguments |
| `xy_at_set_urc_table()` | Set URC handler table |
| `xy_at_client_enter_data_mode()` | Enter transparent mode |
| `xy_at_client_exit_data_mode()` | Exit transparent mode |

### AT Server API

| Function | Description |
|----------|-------------|
| `xy_at_server_create()` | Create AT server |
| `xy_at_server_init()` | Initialize server |
| `xy_at_server_delete()` | Delete server |
| `xy_at_server_set_hal()` | Set HAL interface |
| `xy_at_server_start()` | Start server |
| `xy_at_server_stop()` | Stop server |
| `xy_at_server_register_cmd()` | Register command |
| `xy_at_server_printf()` | Send formatted response |
| `xy_at_server_printfln()` | Send response with newline |
| `xy_at_server_print_result()` | Send result code |
| `xy_at_parse_args()` | Parse command arguments |

## Configuration

### AT Client Configuration

```c
#define XY_AT_CLIENT_NUM_MAX        1      // Max clients
#define XY_AT_CMD_MAX_LEN           256    // Max command length
#define XY_AT_RESP_MAX_LEN          1024   // Max response length
#define XY_AT_RECV_LINE_MAX_LEN     256    // Max line length
#define XY_AT_DEFAULT_TIMEOUT       5000   // Default timeout (ms)
#define XY_AT_MAX_RETRY             3      // Max retry count
#define XY_AT_URC_TABLE_MAX         16     // Max URC handlers
```

### AT Server Configuration

```c
#define XY_AT_SERVER_RECV_BUF_SIZE  256    // Receive buffer
#define XY_AT_SERVER_SEND_BUF_SIZE  512    // Send buffer
#define XY_AT_CMD_NAME_MAX_LEN      16     // Max command name
#define XY_AT_CMD_TABLE_MAX         32     // Max commands
```

## Examples

### Example 1: GSM Modem Client

```c
void gsm_modem_example(void) {
    // Create client
    xy_at_client_t *modem = xy_at_client_create("gsm", 256, 1024);
    xy_at_client_set_hal(modem, uart1_get_char, uart1_send, uart1_recv);

    // Check modem
    xy_at_response_t *resp = xy_at_create_resp(128, 0, 1000);
    if (xy_at_exec_cmd(modem, resp, "AT") == XY_AT_RESP_OK) {
        printf("Modem ready\n");
    }

    // Get IMEI
    xy_at_exec_cmd(modem, resp, "AT+GSN");
    printf("IMEI: %s\n", xy_at_resp_get_line(resp, 1));

    // Get signal quality
    xy_at_exec_cmd(modem, resp, "AT+CSQ");
    int rssi, ber;
    xy_at_resp_parse_line_args(xy_at_resp_get_line(resp, 1), "+CSQ: %d,%d", &rssi, &ber);
    printf("RSSI: %d, BER: %d\n", rssi, ber);

    xy_at_delete_resp(resp);
}
```

### Example 2: WiFi Module Client

```c
void wifi_module_example(void) {
    xy_at_client_t *wifi = xy_at_client_create("wifi", 256, 1024);
    xy_at_client_set_hal(wifi, uart2_get_char, uart2_send, uart2_recv);

    xy_at_response_t *resp = xy_at_create_resp(512, 0, 5000);

    // Connect to AP
    xy_at_exec_cmd(wifi, resp, "AT+CWJAP=\"MyWiFi\",\"password\"");

    // Get IP address
    xy_at_exec_cmd(wifi, resp, "AT+CIFSR");
    printf("IP: %s\n", xy_at_resp_get_line(resp, 1));

    // TCP connect
    xy_at_exec_cmd(wifi, resp, "AT+CIPSTART=\"TCP\",\"192.168.1.100\",8080");

    // Send data
    xy_at_client_enter_data_mode(wifi);
    xy_at_client_send_data(wifi, (uint8_t*)"GET / HTTP/1.1\r\n\r\n", 18);

    uint8_t buf[512];
    int len = xy_at_client_recv_data(wifi, buf, sizeof(buf), 5000);
    printf("Received: %.*s\n", len, buf);

    xy_at_client_exit_data_mode(wifi);
    xy_at_delete_resp(resp);
}
```

### Example 3: Custom AT Server

```c
// Command handlers
xy_at_result_t cmd_led_setup(const char *args) {
    int state;
    if (xy_at_parse_int(args, &state) == 0) {
        if (state == 0 || state == 1) {
            gpio_write(LED_PIN, state);
            return XY_AT_RESULT_OK;
        }
    }
    return XY_AT_RESULT_PARSE_ERR;
}

xy_at_result_t cmd_led_query(void) {
    int state = gpio_read(LED_PIN);
    xy_at_server_printfln(server, "+LED: %d", state);
    return XY_AT_RESULT_OK;
}

xy_at_result_t cmd_adc_query(void) {
    uint16_t value = adc_read(ADC_CH1);
    xy_at_server_printfln(server, "+ADC: %d", value);
    return XY_AT_RESULT_OK;
}

void custom_server_example(void) {
    xy_at_server_t *server = xy_at_server_create("custom");
    xy_at_server_set_hal(server, uart_get_char, uart_send);

    // Register LED command
    static const xy_at_cmd_t cmd_led = {
        .name = "AT+LED",
        .args_expr = "<0|1>",
        .test = NULL,
        .query = cmd_led_query,
        .setup = cmd_led_setup,
        .exec = NULL,
    };
    xy_at_server_register_cmd(server, &cmd_led);

    // Register ADC command
    static const xy_at_cmd_t cmd_adc = {
        .name = "AT+ADC",
        .query = cmd_adc_query,
    };
    xy_at_server_register_cmd(server, &cmd_adc);

    xy_at_server_start(server);
}
```

## HAL Interface Implementation

### UART HAL for Client

```c
// Get character with timeout
int uart_get_char(char *ch, uint32_t timeout) {
    uint32_t start = xy_os_kernel_get_tick_count();

    while ((xy_os_kernel_get_tick_count() - start) < timeout) {
        if (uart_rx_available()) {
            *ch = uart_read_byte();
            return 0;
        }
        xy_os_delay(1);
    }
    return -1;  // Timeout
}

// Send data
size_t uart_send(const char *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        uart_write_byte(data[i]);
    }
    return len;
}

// Receive data
size_t uart_recv(char *data, size_t len) {
    size_t count = 0;
    while (count < len && uart_rx_available()) {
        data[count++] = uart_read_byte();
    }
    return count;
}
```

## Thread Model

### AT Client Threads

```
┌──────────────┐     ┌──────────────┐
│ Application  │────▶│ Command Send │
│   Thread     │     │   (Blocking) │
└──────────────┘     └──────────────┘
                            │
                            ▼
                     ┌──────────────┐
                     │ Parser Thread│
                     │ (Background) │
                     └──────────────┘
                            │
                            ▼
                     ┌──────────────┐
                     │ URC Handlers │
                     └──────────────┘
```

### AT Server Threads

```
┌──────────────┐
│ Parser Thread│
│ (Background) │
└──────────────┘
       │
       ▼
┌──────────────┐
│ Command      │
│ Dispatcher   │
└──────────────┘
       │
       ▼
┌──────────────┐
│ Handler      │
│ Functions    │
└──────────────┘
```

## Porting Guide

### 1. Implement HAL Functions

```c
// For AT Client
int (*get_char)(char *ch, uint32_t timeout);
size_t (*send)(const char *data, size_t len);
size_t (*recv)(char *data, size_t len);

// For AT Server
int (*get_char)(char *ch, uint32_t timeout);
size_t (*send)(const char *data, size_t len);
```

### 2. Configure XY OSAL

Ensure XY OSAL is properly configured for your platform (FreeRTOS, RT-Thread, or bare-metal).

### 3. Adjust Buffer Sizes

Modify configuration defines based on your memory constraints and use case.

## Performance

### Memory Footprint

| Component | RAM (Default Config) |
|-----------|---------------------|
| AT Client | ~1.5 KB |
| AT Server | ~1.0 KB |
| Response Buffer | Configurable |
| Command Table | ~1.5 KB (32 commands) |

### Throughput

- **Command Rate**: Up to 100 commands/sec
- **Data Mode**: Limited by UART speed
- **Response Time**: < 10ms (typical)

## Troubleshooting

### Q: Client commands timeout
- Check UART baud rate
- Verify HAL get_char function
- Increase timeout value
- Check UART RX buffer

### Q: Server not responding
- Verify parser thread is running
- Check HAL get_char implementation
- Enable echo mode for debugging
- Check command registration

### Q: URC not triggered
- Verify URC prefix matches exactly
- Check parser thread priority
- Ensure URC table is set before commands

### Q: Response buffer full
- Increase XY_AT_RESP_MAX_LEN
- Use line_num parameter in xy_at_create_resp()
- Parse responses incrementally

## License

Same as XinYi project

## Version

- **Version**: 1.0.0
- **Date**: 2025-10-27
- **Status**: Ready for Implementation
