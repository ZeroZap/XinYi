# XinYi Net Component

## Overview

The `net` component provides network communication protocols for embedded systems, including AT commands, Modbus, MQTT, CAN, and LTE modules.

## Component Structure

```
components/net/
├── at/                          # AT Command implementations
│   ├── AT-Command-V2/          # Version 2 AT command framework
│   ├── FreeRTOS-Cellular-Interface/  # FreeRTOS cellular support
│   ├── atc/                    # Basic AT command parser
│   ├── ats/                    # AT command server
│   └── rtthread-at/           # RT-Thread AT package
├── xy_mqtt/                    # MQTT protocol (INCOMPLETE)
├── modbus/                     # Modbus RTU Slave (Production Ready)
├── modbus_tiny/               # Lightweight Modbus RTU
├── modbus_full/               # Full-featured Modbus (RTU + TCP + ASCII)
├── smbus/                      # SMBus protocol
├── xy_iso7816/                # ISO7816 smart card
├── src/                        # Source implementations
│   ├── nano_modbus.c          # Unified Modbus API
│   ├── xy_can.c              # CAN bus implementation
│   ├── xy_lte.c              # LTE/4G module driver (host-guarded)
│   ├── xy_lte_uart_adapter.c # Callback-backed LTE UART transport adapter
│   ├── xy_net.c              # Network core
│   └── xy_net_platform.c     # Platform abstraction
├── inc/                        # Public headers
│   ├── nano_modbus.h
│   ├── xy_can.h
│   ├── xy_lte.h
│   ├── xy_lte_uart_adapter.h
│   ├── xy_net.h
│   └── xy_net_config.h
├── CMakeLists.txt
├── Kconfig
└── Makefile
```

## Module Status

| Module | Status | Description |
|--------|--------|-------------|
| **Modbus** | ✅ Production Ready | Full RTU slave implementation with examples |
| **Modbus Tiny** | ✅ Production Ready | Lightweight version for constrained MCUs |
| **Modbus Full** | ✅ Production Ready | RTU + TCP + ASCII support |
| **CAN** | ✅ Implemented | FIFO-based host-guarded CAN core; not enabled in `xy_net` by default |
| **LTE** | 🟡 Host-guarded | Public API, fakeable AT transport seam, and callback-backed UART adapter are covered by focused CTests; still disabled by default until a real HAL UART binding is proven |
| **MQTT Client** | 🟡 Host-guarded | `src/xy_mqtt_client.c` covers CONNECT/CONNACK, QoS0/1 publish, subscribe, keepalive helpers; legacy `xy_mqtt/` remains deprecated |
| **AT Client/Server** | 🟡 Host-guarded | Lightweight AT client/server cores have Unity/CTest coverage; larger vendor-style AT trees are not part of the default library |

## AT Client/Server Architecture

### Available Implementations

1. **AT-Command-V2** - Modern AT command framework with command registration
2. **ats** - Lightweight AT command server
3. **atc** - Basic AT command client
4. **FreeRTOS-Cellular-Interface** - FreeRTOS integration for cellular modules
5. **rtthread-at** - RT-Thread OS AT package

### Quick Start (AT Command V2)

```c
#include "at_command_v2.h"

// Define command handler
at_cmd_status_t handle_test(at_context_t *ctx, char *resp) {
    at_response(ctx, "OK");
    return AT_OK;
}

// Register and run
at_init();
at_register("AT+TEST", handle_test);
at_process();
```

## Modbus RTU Slave

### Features
- **Function Codes**: 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0F, 0x10
- **Memory Footprint**: ~720 bytes RAM, ~1.6 KB flash
- **Configurable**: Coil, discrete, holding, input register counts

### Configuration

```c
#define MB_SLAVE_ADDRESS      1
#define MB_UART_BAUDRATE      9600
#define MB_COIL_COUNT         64
#define MB_HOLDING_REG_COUNT  32
```

### Basic Usage

```c
#include "mb_slave.h"

static mb_slave_t modbus;

int main(void) {
    mb_slave_init(&modbus, 1, 9600);

    while (1) {
        mb_slave_poll(&modbus, mb_get_time_ms());
    }
}

// UART RX interrupt
void UART1_IRQHandler(void) {
    if (UART1->SR & UART_SR_RXNE) {
        mb_slave_receive_byte(&modbus, UART1->DR);
    }
}
```

### Data Access

```c
// Set holding register
mb_slave_set_holding_register(&modbus, 0, 1234);

// Get input register
uint16_t value = mb_slave_get_input_register(&modbus, 0);

// Set callbacks
mb_slave_set_register_callback(&modbus, on_register_changed);
```

### See Also
- [Modbus README](modbus/README.md) - Complete documentation with examples

## CAN Bus

### Features
- FIFO-based TX/RX buffers
- Configurable baudrate
- Interrupt-driven reception
- Callback support

### Usage

```c
#include "xy_can.h"

xy_can_t can;
xy_can_config_t config = {
    .baudrate = 500000,
    .rx_fifo_size = 16,
    .tx_fifo_size = 16,
};

xy_can_init(&can, hw_handle, &config);
xy_can_start(&can);

// Send message
xy_can_msg_t msg = {.id = 0x100, .len = 8};
xy_can_send(&can, &msg, 1000);

// Register callback
xy_can_register_rx_callback(&can, on_can_receive, user_data);
```

## LTE/4G Module

⚠️ **This module is host-guarded but still disabled by default**

### Status
- Header exposes the LTE public API and the local `xy_lte_transport_t` AT command seam
- `src/xy_lte.c` preserves lifecycle, attach/PDP, callback, and caller-output contracts behind a fakeable transport
- `src/xy_lte_uart_adapter.c` provides a callback-backed UART byte transport adapter with no vendor/HAL UART dependency
- `tests/unit/net/test_lte.c` (`lte_component`) covers fake transport command success/failure, AT/CSQ/SIM/attach/PDP/send/recv contracts, and output/state preservation
- `tests/unit/net/test_lte_uart_adapter.c` (`lte_uart_adapter`) covers adapter guards, byte forwarding, timeout propagation, backend-error propagation, and LTE core AT-command binding through the adapter
- `XY_NET_ENABLE_LTE` remains `0` by default; include `xy_lte.h` / `xy_lte_uart_adapter.h` directly for focused consumers until a real HAL UART binding is designed and compile-probed
- Needs a real HAL UART binding before being exported from `xy_net.h` by default

### Supported Modules (Planned)
- 移远 EC100Y/EC200Y
- 广和通 L610/L630
- 合宙 Air780E

### Planned Features
- [x] Fakeable AT command transport seam
- [x] Host coverage for command failure propagation and state/output preservation
- [x] Callback-backed UART adapter compile probe
- [ ] HAL UART binding compile probe
- [ ] URC buffering/dispatch backed by a real transport
- [ ] Network attachment on validated hardware
- [ ] PDP context management on validated hardware
- [ ] TCP/UDP connections on validated hardware
- [ ] Data transmission on validated hardware
- [ ] Signal quality monitoring on validated hardware

### Focused verification

```bash
cmake --build build/tests/unit --target test_lte -j$(nproc)
cd build/tests/unit && ctest --output-on-failure -R '^lte_component$'

cmake --build build/tests/unit --target test_lte_uart_adapter -j$(nproc)
cd build/tests/unit && ctest --output-on-failure -R '^lte_uart_adapter$'
```

See `docs/design/xinyi-net-lte-transport-proposal-2026-08-05.md` for the transport boundary and enablement criteria.

## MQTT Client

The active MQTT client is `src/xy_mqtt_client.c` with public API in
`src/xy_mqtt_client.h`. It is host-guarded by `mqtt_client` Unity/CTest coverage
for remaining-length encoding, topic filters, client lifecycle validation,
CONNECT/CONNACK flow, and connection-return-code strings. It implements the
small embeddable MQTT 3.1.1 surface used by XinYi network slices:

- CONNECT/CONNACK state transition through caller-provided transport callbacks
- QoS 0/1 PUBLISH packet construction and PUBACK handling
- SUBSCRIBE/UNSUBSCRIBE packet helpers and callback dispatch
- PINGREQ/PINGRESP keep-alive helpers
- Wildcard topic filter matching (`+`, `#`)

The older `xy_mqtt/` subtree is retained as legacy planning/reference material
and should not be treated as the active implementation entrypoint.

### Minimal host-side usage

```c
#include "xy_mqtt_client.h"

static int net_send(void *ctx, const uint8_t *data, size_t len);
static int net_recv(void *ctx, uint8_t *data, size_t len, uint32_t timeout_ms);

xy_mqtt_config_t cfg = {
    .transport_context = socket_or_modem_context,
    .send = net_send,
    .recv = net_recv,
    .keepalive = 30,
    .tx_buffer_size = 256,
    .rx_buffer_size = 256,
};

xy_mqtt_client_t *mqtt = xy_mqtt_client_new(&cfg);
xy_mqtt_connect(mqtt, "xinyi-node", NULL, NULL);
xy_mqtt_process(mqtt, 1000); /* consume CONNACK */
```

### Focused verification

```bash
cmake --build build/tests/unit --target test_mqtt_client -j$(nproc)
cd build/tests/unit && ctest --output-on-failure -R '^mqtt_client$'
```

### Remaining MQTT work

- Add host coverage for successful publish/subscribe/unsubscribe callback paths.
- Align root Kconfig (`PROTO_MQTT`) with the component-local default/library policy
  before enabling MQTT through `xy_net` automatically.
- Do not extend the legacy `xy_mqtt/` stub without first writing a migration
  proposal; prefer the active `xy_mqtt_client` API.

## Build Instructions

### Using CMake

```bash
# In your project CMakeLists.txt
add_subdirectory(components/net)
target_link_libraries(your_app PRIVATE xy_net)

# Or using Kconfig
# Run `make menuconfig` and enable modules under Components → Net
```

### Using Make

```bash
# Set NET_DIR in your Makefile
NET_DIR = components/net
include $(NET_DIR)/Makefile
```

### Configuration Options

Via Kconfig or defines:

```c
// Generated by root Kconfig for the overall communication menu
#define CONFIG_NETWORK          1
#define CONFIG_PROTO_MQTT       1

// Component-local policy in components/net/inc/xy_net_config.h
#define XY_NET_ENABLE_MODBUS    1
#define XY_NET_ENABLE_MQTT      0  // not auto-exported by xy_net yet
#define XY_NET_ENABLE_CAN       0  // host-guarded, direct opt-in only
#define XY_NET_ENABLE_LTE       0  // host-guarded stub, direct opt-in only

// Modbus configuration
#define MB_COIL_COUNT           64
#define MB_DISCRETE_COUNT       64
#define MB_INPUT_REG_COUNT      32
#define MB_HOLDING_REG_COUNT    32

// CAN configuration
#define XY_CAN_RX_FIFO_SIZE     16
#define XY_CAN_TX_FIFO_SIZE     16
```

`CONFIG_PROTO_MQTT=y` currently records product intent, while
`XY_NET_ENABLE_MQTT=0` keeps the active MQTT client out of the umbrella
`xy_net.h` export until the component-local library policy and root Kconfig are
aligned in a dedicated slice. Include `xy_mqtt_client.h` directly for focused
MQTT users/tests.

## API Reference

### Modbus API

| Function | Description |
|----------|-------------|
| `mb_slave_init()` | Initialize Modbus slave |
| `mb_slave_poll()` | Poll for complete frames |
| `mb_slave_receive_byte()` | Feed received byte |
| `mb_slave_set_holding_register()` | Set holding register |
| `mb_slave_get_holding_register()` | Get holding register |
| `mb_slave_set_coil()` | Set coil state |
| `mb_slave_get_coil()` | Get coil state |
| `mb_crc16()` | Calculate CRC16 |

### CAN API

| Function | Description |
|----------|-------------|
| `xy_can_init()` | Initialize CAN |
| `xy_can_start()` | Start CAN |
| `xy_can_stop()` | Stop CAN |
| `xy_can_send()` | Send CAN message |
| `xy_can_receive()` | Receive CAN message |
| `xy_can_register_rx_callback()` | Register RX callback |

### MQTT Client API

| Function | Description |
|----------|-------------|
| `xy_mqtt_client_new()` | Allocate a client around caller-provided send/recv callbacks |
| `xy_mqtt_connect()` | Build/send CONNECT and enter CONNACK-wait state |
| `xy_mqtt_process()` | Receive and dispatch one packet from the transport |
| `xy_mqtt_publish()` | Publish QoS 0/1 payloads when connected |
| `xy_mqtt_subscribe()` | Subscribe to one wildcard-capable topic filter |
| `xy_mqtt_keepalive_check()` | Send PINGREQ when keep-alive expires |

### LTE API

| Function | Description |
|----------|-------------|
| `xy_lte_init()` | Initialize LTE module |
| `xy_lte_bind_transport()` | Bind a fakeable AT command transport hook |
| `xy_lte_attach()` | Attach to network after command success |
| `xy_lte_connect()` | TCP/UDP connection |
| `xy_lte_send()` | Send data after prompt validation |
| `xy_lte_recv()` | Receive data through bound transport or placeholder zero-fill |
| `xy_lte_get_signal()` | Get signal quality while preserving output on parse/transport failure |

## Troubleshooting

### Modbus
- **No response**: Check UART config (baud rate, parity)
- **CRC errors**: Verify electrical connections, add termination resistor
- **Missed bytes**: Use DMA for UART RX

### CAN
- **No TX interrupts**: Check hardware initialization
- **FIFO overflow**: Increase buffer size or poll more frequently

### LTE
- **AT commands fail**: In host tests, run `lte_component` plus `lte_uart_adapter` and inspect the fake transport command/response queue plus callback-backed UART byte forwarding; on hardware, verify UART wiring and AT command syntax after a HAL UART binding exists.
- **Network attach fails**: Check SIM card and antenna; the current direct-opt-in API only marks `attached` after the attach command path succeeds.
- **Unexpected `xy_net.h` export**: Confirm `XY_NET_ENABLE_LTE` is explicitly set to `1`; default builds intentionally keep LTE out of the umbrella.

## Version

- **Version**: 1.0.0
- **Date**: 2025-10-27
- **Status**: Mixed (Modbus production-ready, others incomplete)
