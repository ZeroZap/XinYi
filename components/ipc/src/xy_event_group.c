/**
 * @file xy_event_group.c
 * @brief IPC event-group convenience wrapper over OSAL event flags.
 */

#include "xy_event_group.h"

#include <string.h>

#define XY_IPC_EVENT_OS_ERROR_MASK 0x80000000U

static int event_group_validate(xy_ipc_event_group_t *group)
{
    if (!group) {
        return XY_IPC_EVENT_INVALID_PARAM;
    }
    if (!group->initialized || !group->os_flags) {
        return XY_IPC_EVENT_NOT_INITIALIZED;
    }
    return XY_IPC_EVENT_OK;
}

static int event_group_validate_bits(xy_ipc_event_bits_t bits)
{
    if (bits == 0U || (bits & XY_IPC_EVENT_OS_ERROR_MASK)) {
        return XY_IPC_EVENT_INVALID_PARAM;
    }
    return XY_IPC_EVENT_OK;
}

static int event_group_map_flags_result(uint32_t result, int no_match_status)
{
    if (result & XY_IPC_EVENT_OS_ERROR_MASK) {
        return no_match_status;
    }
    return XY_IPC_EVENT_OK;
}

int xy_ipc_event_group_init(xy_ipc_event_group_t *group, const char *name)
{
    xy_os_event_flags_attr_t attr;

    if (!group) {
        return XY_IPC_EVENT_INVALID_PARAM;
    }

    memset(group, 0, sizeof(*group));
    if (name) {
        strncpy(group->name, name, sizeof(group->name) - 1U);
        group->name[sizeof(group->name) - 1U] = '\0';
    }

    memset(&attr, 0, sizeof(attr));
    attr.name = group->name[0] != '\0' ? group->name : NULL;
    group->os_flags = xy_os_event_flags_new(&attr);
    if (!group->os_flags) {
        memset(group, 0, sizeof(*group));
        return XY_IPC_EVENT_ERROR;
    }

    group->initialized = 1U;
    return XY_IPC_EVENT_OK;
}

int xy_ipc_event_group_deinit(xy_ipc_event_group_t *group)
{
    xy_os_status_t status;
    int ret = event_group_validate(group);
    if (ret != XY_IPC_EVENT_OK) {
        return ret;
    }

    status = xy_os_event_flags_delete(group->os_flags);
    if (status != XY_OS_OK) {
        return XY_IPC_EVENT_ERROR;
    }

    group->os_flags = NULL;
    group->initialized = 0U;
    group->name[0] = '\0';
    return XY_IPC_EVENT_OK;
}

int xy_ipc_event_group_set(xy_ipc_event_group_t *group, xy_ipc_event_bits_t bits,
                           xy_ipc_event_bits_t *after_set)
{
    uint32_t result;
    int ret = event_group_validate(group);
    if (ret != XY_IPC_EVENT_OK) {
        return ret;
    }
    ret = event_group_validate_bits(bits);
    if (ret != XY_IPC_EVENT_OK) {
        return ret;
    }

    result = xy_os_event_flags_set(group->os_flags, bits);
    ret = event_group_map_flags_result(result, XY_IPC_EVENT_ERROR);
    if (ret != XY_IPC_EVENT_OK) {
        return ret;
    }

    if (after_set) {
        *after_set = result;
    }
    return XY_IPC_EVENT_OK;
}

int xy_ipc_event_group_clear(xy_ipc_event_group_t *group, xy_ipc_event_bits_t bits,
                             xy_ipc_event_bits_t *before_clear)
{
    uint32_t result;
    int ret = event_group_validate(group);
    if (ret != XY_IPC_EVENT_OK) {
        return ret;
    }
    ret = event_group_validate_bits(bits);
    if (ret != XY_IPC_EVENT_OK) {
        return ret;
    }

    result = xy_os_event_flags_clear(group->os_flags, bits);
    ret = event_group_map_flags_result(result, XY_IPC_EVENT_ERROR);
    if (ret != XY_IPC_EVENT_OK) {
        return ret;
    }

    if (before_clear) {
        *before_clear = result;
    }
    return XY_IPC_EVENT_OK;
}

int xy_ipc_event_group_get(xy_ipc_event_group_t *group, xy_ipc_event_bits_t *current)
{
    uint32_t result;
    int ret = event_group_validate(group);
    if (ret != XY_IPC_EVENT_OK) {
        return ret;
    }
    if (!current) {
        return XY_IPC_EVENT_INVALID_PARAM;
    }

    result = xy_os_event_flags_get(group->os_flags);
    *current = result;
    return XY_IPC_EVENT_OK;
}

int xy_ipc_event_group_wait(xy_ipc_event_group_t *group, xy_ipc_event_bits_t bits,
                            uint32_t options, uint32_t timeout_ms,
                            xy_ipc_event_bits_t *matched)
{
    uint32_t result;
    int ret = event_group_validate(group);
    if (ret != XY_IPC_EVENT_OK) {
        return ret;
    }
    ret = event_group_validate_bits(bits);
    if (ret != XY_IPC_EVENT_OK) {
        return ret;
    }
    if (!matched) {
        return XY_IPC_EVENT_INVALID_PARAM;
    }

    result = xy_os_event_flags_wait(group->os_flags, bits, options, timeout_ms);
    ret = event_group_map_flags_result(result, XY_IPC_EVENT_TIMEOUT);
    if (ret != XY_IPC_EVENT_OK) {
        return ret;
    }

    *matched = result & bits;
    return XY_IPC_EVENT_OK;
}
