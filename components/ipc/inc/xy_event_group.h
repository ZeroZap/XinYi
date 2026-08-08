/**
 * @file xy_event_group.h
 * @brief IPC event-group convenience wrapper over OSAL event flags.
 */

#ifndef XY_EVENT_GROUP_H
#define XY_EVENT_GROUP_H

#include <stdint.h>

#include "xy_os.h"

#ifdef __cplusplus
extern "C" {
#endif

#define XY_IPC_EVENT_OK              0
#define XY_IPC_EVENT_ERROR           (-1)
#define XY_IPC_EVENT_INVALID_PARAM   (-2)
#define XY_IPC_EVENT_NOT_INITIALIZED (-3)
#define XY_IPC_EVENT_TIMEOUT         (-4)

#define XY_IPC_EVENT_WAIT_ANY XY_OS_FLAGS_WAIT_ANY
#define XY_IPC_EVENT_WAIT_ALL XY_OS_FLAGS_WAIT_ALL
#define XY_IPC_EVENT_NO_CLEAR XY_OS_FLAGS_NO_CLEAR

typedef uint32_t xy_ipc_event_bits_t;

typedef struct xy_ipc_event_group {
    xy_os_event_flags_id_t os_flags;
    char name[32];
    uint8_t initialized;
} xy_ipc_event_group_t;

int xy_ipc_event_group_init(xy_ipc_event_group_t *group, const char *name);
int xy_ipc_event_group_deinit(xy_ipc_event_group_t *group);
int xy_ipc_event_group_set(xy_ipc_event_group_t *group, xy_ipc_event_bits_t bits,
                           xy_ipc_event_bits_t *after_set);
int xy_ipc_event_group_clear(xy_ipc_event_group_t *group, xy_ipc_event_bits_t bits,
                             xy_ipc_event_bits_t *before_clear);
int xy_ipc_event_group_get(xy_ipc_event_group_t *group, xy_ipc_event_bits_t *current);
int xy_ipc_event_group_wait(xy_ipc_event_group_t *group, xy_ipc_event_bits_t bits,
                            uint32_t options, uint32_t timeout_ms,
                            xy_ipc_event_bits_t *matched);

#ifdef __cplusplus
}
#endif

#endif /* XY_EVENT_GROUP_H */
