#include "xy_sys.h"

#if defined(__GNUC__) || defined(__clang__)
#define XY_SYS_WEAK __attribute__((weak))
#else
#define XY_SYS_WEAK
#endif

XY_SYS_WEAK void xy_sys_init(void)
{
    // Initialize system components
}

XY_SYS_WEAK int xy_sys_reset(int reset_by)
{
    (void)reset_by;
    // Reset is platform/board-specific; default host-safe stub reports unsupported/no-op.
    return 0;
}

XY_SYS_WEAK int xy_sys_reboot_reason(void *data)
{
    (void)data;
    // Get reboot reason
    return 0;
}

XY_SYS_WEAK int xy_sys_get_chip_id(void *data)
{
    (void)data;
    // Get chip ID
    return 0;
}

XY_SYS_WEAK int xy_sys_get_mac_addr(void *data)
{
    (void)data;
    // Get MAC address
    return 0;
}

int xy_sys_get_sw_ver(void *data)
{
    (void)data;
    // Get software version
    return 0;
}

int xy_sys_get_hw_ver(void *data)
{
    (void)data;
    // Get hardware version
    return 0;
}
