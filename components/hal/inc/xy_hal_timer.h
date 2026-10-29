/**
 * @file xy_hal_timer.h
 * @brief Timer Hardware Abstraction Layer
 * @version 2.0
 * @date 2025-10-26
 */

#ifndef XY_HAL_TIMER_H
#define XY_HAL_TIMER_H
#include "xy_hal_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "xy_hal.h"

/* Timer Counter Mode */
typedef enum {
    XY_HAL_TIMER_COUNT_UP = 0,  /**< Count up mode */
    XY_HAL_TIMER_COUNT_DOWN,    /**< Count down mode */
    XY_HAL_TIMER_COUNT_CENTER1, /**< Center-aligned mode 1 */
    XY_HAL_TIMER_COUNT_CENTER2, /**< Center-aligned mode 2 */
    XY_HAL_TIMER_COUNT_CENTER3, /**< Center-aligned mode 3 */
} xy_hal_timer_count_mode_t;

/* Timer Clock Division */
typedef enum {
    XY_HAL_TIMER_CKDIV_1 = 0, /**< No division */
    XY_HAL_TIMER_CKDIV_2,     /**< Division by 2 */
    XY_HAL_TIMER_CKDIV_4,     /**< Division by 4 */
} xy_hal_timer_ckdiv_t;

/* Timer Configuration Structure */
typedef struct {
    uint32_t prescaler;             /**< Prescaler value (0-65535) */
    uint32_t period;                /**< Period value (auto-reload) */
    xy_hal_timer_count_mode_t mode; /**< Counter mode */
    xy_hal_timer_ckdiv_t clock_div; /**< Clock division */
    uint8_t
        auto_reload_preload; /**< Auto-reload preload: 1=enable, 0=disable */
} xy_hal_timer_config_t;

/* Timer Event Types */
typedef enum {
    XY_HAL_TIMER_EVENT_UPDATE = 0, /**< Update event (overflow) */
    XY_HAL_TIMER_EVENT_CC1,        /**< Capture/Compare 1 event */
    XY_HAL_TIMER_EVENT_CC2,        /**< Capture/Compare 2 event */
    XY_HAL_TIMER_EVENT_CC3,        /**< Capture/Compare 3 event */
    XY_HAL_TIMER_EVENT_CC4,        /**< Capture/Compare 4 event */
} xy_hal_timer_event_t;

/* Timer callback */
typedef void (*xy_hal_timer_callback_t)(void *timer, xy_hal_timer_event_t event,
                                        void *arg);

/**
 * @brief Initialize timer
 * @param timer Timer instance
 * @param config Timer configuration
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_timer_init(void *timer,
                                 const xy_hal_timer_config_t *config);

/**
 * @brief Deinitialize timer
 * @param timer Timer instance
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_timer_deinit(void *timer);

/**
 * @brief Start timer
 * @param timer Timer instance
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_timer_start(void *timer);

/**
 * @brief Stop timer
 * @param timer Timer instance
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_timer_stop(void *timer);

/**
 * @brief Get timer counter value
 * @param timer Timer instance
 * @return Counter value, negative on error
 */
int xy_hal_timer_get_counter(void *timer);

/**
 * @brief Set timer counter value
 * @param timer Timer instance
 * @param value Counter value
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_timer_set_counter(void *timer, uint32_t value);

/**
 * @brief Set timer period
 * @param timer Timer instance
 * @param period Period value
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_timer_set_period(void *timer, uint32_t period);

/**
 * @brief Register timer callback
 * @param timer Timer instance
 * @param event Event type
 * @param callback Callback function
 * @param arg User argument
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_timer_register_callback(void *timer,
                                              xy_hal_timer_event_t event,
                                              xy_hal_timer_callback_t callback,
                                              void *arg);

/**
 * @brief Arm one timer interrupt source
 * @param timer Timer instance
 * @param event Event type; the backend arms that source and no other
 * @return 0 on success, negative on error
 *
 * Arming an interrupt does not start the counter - call xy_hal_timer_start() for that. Two
 * backends used to do both from here (HAL_TIM_Base_Start_IT was the mechanism), so a caller that
 * only wanted the interrupt started a timer, and one that disarmed an interrupt stopped
 * timekeeping.
 *
 * The backend also enables the NVIC line for the instance. Enabling the peripheral interrupt while
 * leaving the line masked reports success and never fires, which is what every STM32 backend did
 * before this contract existed. An instance whose line the backend cannot identify is refused
 * rather than armed into a vector that would never arrive.
 */
xy_hal_error_t xy_hal_timer_enable_irq(void *timer, xy_hal_timer_event_t event);

/**
 * @brief Mask one timer interrupt source
 * @param timer Timer instance
 * @param event Event type; only that source is masked
 * @return 0 on success, negative on error
 *
 * Masking an interrupt does not stop the counter. The NVIC line stays enabled and the counter
 * keeps running, so re-arming needs no second setup and does not restart anything.
 */
xy_hal_error_t xy_hal_timer_disable_irq(void *timer,
                                        xy_hal_timer_event_t event);

/**
 * @brief Dispatch a timer interrupt through the selected HAL backend
 * @param timer Platform timer handle associated with the active IRQ
 */
void xy_hal_timer_irq_handler(void *timer);

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_TIMER_H */