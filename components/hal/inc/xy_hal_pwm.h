/**
 * @file xy_hal_pwm.h
 * @brief PWM Hardware Abstraction Layer
 * @version 2.0
 * @date 2025-10-26
 */

#ifndef XY_HAL_PWM_H
#define XY_HAL_PWM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "xy_hal.h"

/* PWM Channel */
typedef enum {
    XY_HAL_PWM_CHANNEL_1 = 0, /**< Channel 1 */
    XY_HAL_PWM_CHANNEL_2,     /**< Channel 2 */
    XY_HAL_PWM_CHANNEL_3,     /**< Channel 3 */
    XY_HAL_PWM_CHANNEL_4,     /**< Channel 4 */
} xy_hal_pwm_channel_t;

/* PWM Polarity */
typedef enum {
    XY_HAL_PWM_POLARITY_HIGH = 0, /**< Active high */
    XY_HAL_PWM_POLARITY_LOW,      /**< Active low */
} xy_hal_pwm_polarity_t;

/* PWM Configuration Structure */
typedef struct {
    uint32_t frequency;  /**< PWM frequency in Hz */
    uint32_t duty_cycle; /**< Duty cycle (0-10000, represents 0.00%-100.00%) */
    xy_hal_pwm_polarity_t polarity; /**< Output polarity */
} xy_hal_pwm_config_t;

/**
 * @brief Initialize PWM
 * @param timer Timer instance used for PWM
 * @param channel PWM channel
 * @param config PWM configuration
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_pwm_init(void *timer, xy_hal_pwm_channel_t channel,
                               const xy_hal_pwm_config_t *config);

/**
 * @brief Deinitialize PWM
 * @param timer Timer instance
 * @param channel PWM channel
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_pwm_deinit(void *timer, xy_hal_pwm_channel_t channel);

/**
 * @brief Start PWM output
 * @param timer Timer instance
 * @param channel PWM channel
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_pwm_start(void *timer, xy_hal_pwm_channel_t channel);

/**
 * @brief Stop PWM output
 * @param timer Timer instance
 * @param channel PWM channel
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_pwm_stop(void *timer, xy_hal_pwm_channel_t channel);

/**
 * @brief Set PWM duty cycle
 * @param timer Timer instance
 * @param channel PWM channel
 * @param duty_cycle Duty cycle (0-10000, represents 0.00%-100.00%)
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_pwm_set_duty_cycle(void *timer,
                                         xy_hal_pwm_channel_t channel,
                                         uint32_t duty_cycle);

/**
 * @brief Get PWM duty cycle
 * @param timer Timer instance
 * @param channel PWM channel
 * @return Duty cycle (0-10000), negative on error
 */
int xy_hal_pwm_get_duty_cycle(void *timer, xy_hal_pwm_channel_t channel);

/**
 * @brief Set PWM frequency
 * @param timer Timer instance
 * @param frequency Frequency in Hz
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_pwm_set_frequency(void *timer, uint32_t frequency);

/**
 * @brief Get PWM frequency
 * @param timer Timer instance
 * @return Frequency in Hz, negative on error
 */
int xy_hal_pwm_get_frequency(void *timer);

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_PWM_H */