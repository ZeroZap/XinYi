/**
 * @file xy_net_platform.c
 * @brief XinYi Network Platform Abstraction Implementation
 */

#include "xy_net_platform.h"
#include <stdlib.h>

#ifdef HAL_PLATFORM_PC
#include <time.h>
#endif

#ifdef XY_OSAL_ENABLED
#include "xy_osal.h"
#endif

XY_NET_TICK_TYPE xy_net_get_tick(void)
{
#ifdef XY_OSAL_ENABLED
    // Use OSAL timer if available
    return (XY_NET_TICK_TYPE)xy_os_get_time_ms();
#elif defined(HAL_PLATFORM_PC)
    // Use system clock for PC simulation. Avoid CLOCKS_PER_SEC / 1000 because
    // some embedded C libraries define CLOCKS_PER_SEC below 1000, which folds
    // to zero at compile time even though this branch is PC-only in practice.
    return (XY_NET_TICK_TYPE)((clock() * 1000U) / CLOCKS_PER_SEC);
#else
    // No generic bare-metal tick source is available here. Platform ports should
    // provide OSAL or override this weak abstraction when real time is required.
    return 0;
#endif
}

void xy_net_delay_ms(uint32_t ms)
{
#ifdef XY_OSAL_ENABLED
    xy_os_task_delay(ms);
#else
    // Simple busy wait for PC (not recommended for production)
    XY_NET_TICK_TYPE start = xy_net_get_tick();
    while ((xy_net_get_tick() - start) < ms) {
        // Busy wait
    }
#endif
}

void* xy_net_malloc(size_t size)
{
    return malloc(size);
}

void xy_net_free(void* ptr)
{
    free(ptr);
}