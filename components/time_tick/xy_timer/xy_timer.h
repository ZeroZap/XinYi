#ifndef _XY_TIMER_H_
#define _XY_TIMER_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef void *xy_timer_ref;
#if (PLATFORM == PLATFORM_C51)
// typedef void (*timer_proc)(void *params);
typedef void (*timer_proc)(xy_timer_ref xdata timer_handler,
                           void xdata *params) reentrant;
// typedef void (*timer_proc)(xy_timer_ref   timer_handler, void  *params)
// reentrant;
#else
typedef void (*timer_proc)(xy_timer_ref timer_handler, void *params) reentrant;
#endif


void xy_timer_init(void);
xy_uint32_t xy_timer_get_tick(void);
xy_uint32_t xy_timer_get_tick_from_isr(void);
void xy_timer_set_tick(xy_uint32_t tick);
xy_uint32_t xy_timer_get_nexttick(void);

xy_timer_ref xy_timer_create(xy_uint32_t cnt, xy_uint32_t reload,
                             timer_proc pfunc, void *params);
void xy_timer_ticks(void);
void xy_timer_kill(xy_timer_ref timer_handler);
void xy_timer_change_cnt(xy_timer_ref timer_handler, xy_uint32_t cnt);
void xy_timer_change_reload(xy_timer_ref timer_handler, xy_uint32_t reload);
void xy_timer_change_func(xy_timer_ref timer_handler, timer_proc pfunc);
timer_proc xy_timer_get_func(xy_timer_ref timer_handler);


#ifdef __cplusplus
}
#endif

#endif
