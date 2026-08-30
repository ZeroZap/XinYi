/**
 * @file xy_net_config.h
 * @brief XinYi Network Component Configuration
 */

#ifndef XY_NET_CONFIG_H
#define XY_NET_CONFIG_H

#include "xy_typedef.h"

#include <stdlib.h>

// Enable/disable network protocols
#ifndef XY_NET_ENABLE_MODBUS
#define XY_NET_ENABLE_MODBUS    1
#endif

#ifndef XY_NET_ENABLE_MQTT
#define XY_NET_ENABLE_MQTT      0  // Disabled by default; active owner is direct opt-in
#endif

#ifndef XY_NET_ENABLE_AT_CLIENT
#define XY_NET_ENABLE_AT_CLIENT 0  // Disabled by default; direct opt-in only
#endif

#ifndef XY_NET_ENABLE_AT_SERVER
#define XY_NET_ENABLE_AT_SERVER 0  // Disabled by default; direct opt-in only
#endif

#ifndef XY_NET_ENABLE_CAN
#define XY_NET_ENABLE_CAN       0  // Disabled by default; direct opt-in only
#endif

#ifndef XY_NET_ENABLE_LTE
#define XY_NET_ENABLE_LTE       0  // Disabled by default; direct opt-in only
#endif

// Platform abstraction
#ifndef XY_NET_TICK_TYPE
#define XY_NET_TICK_TYPE        uint32_t
#endif

// Function to get current tick count (platform specific)
extern XY_NET_TICK_TYPE xy_net_get_tick(void);

// Memory allocation (use system malloc by default)
#ifndef XY_NET_MALLOC
#define XY_NET_MALLOC(size)     malloc(size)
#endif

#ifndef XY_NET_FREE
#define XY_NET_FREE(ptr)        free(ptr)
#endif

#endif // XY_NET_CONFIG_H