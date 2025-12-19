#ifndef _XY_TICK_H_
#define _XY_TICK_H_
#include "xy_typedef.h"
#ifdef __cplusplus
extern "C" {
#endif

xy_u32_t xy_tick_increase(void);
xy_u32_t xy_ticks_now(void);
xy_u32_t xy_ticks_set(xy_u32_t tick);
xy_u32_t xy_ticks_get(void);
xy_u32_t xy_ticks_get_from_isr(void);
xy_u32_t xy_ticks_since(xy_u32_t tick);

#ifdef __cplusplus
}
#endif


#endif