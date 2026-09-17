#ifndef XY_DEVICE_TIMING_H
#define XY_DEVICE_TIMING_H

#include <stdint.h>
#include "xy_hal_delay.h"

/* Define XY_DEVICE_USE_OSAL for threaded/RTOS builds. */
#ifdef XY_DEVICE_USE_OSAL
#include "xy_os.h"
static inline uint32_t xy_device_now_ms(void)
{
    uint32_t hz = xy_os_kernel_get_tick_freq();
    uint32_t ticks = xy_os_kernel_get_tick_count();
    return hz == 0U ? 0U : (uint32_t)(((uint64_t)ticks * 1000U) / hz);
}

static inline int xy_device_delay_ms(uint32_t ms)
{
    uint32_t hz = xy_os_kernel_get_tick_freq();
    uint32_t ticks;
    if (hz == 0U) {
        return -1;
    }
    ticks = (uint32_t)(((uint64_t)ms * hz + 999U) / 1000U);
    return xy_os_delay(ticks) == XY_OS_OK ? 0 : -1;
}
#else
#include "xy_hal_sys.h"
static inline uint32_t xy_device_now_ms(void)
{
    uint32_t hz = xy_hal_sys_get_tick_freq();
    uint32_t ticks = xy_hal_sys_get_tick_count();
    return hz == 0U ? 0U : (uint32_t)(((uint64_t)ticks * 1000U) / hz);
}

static inline int xy_device_delay_ms(uint32_t ms)
{
    xy_hal_delay_ms(ms);
    return 0;
}
#endif

#endif /* XY_DEVICE_TIMING_H */
