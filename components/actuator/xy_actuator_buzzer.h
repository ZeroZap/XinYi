#ifndef XY_ACTUATOR_BUZZER_H
#define XY_ACTUATOR_BUZZER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "xy_actuator.h"
#include "xy_hal_gpio.h"

typedef void (*xy_actuator_buzzer_delay_fn)(uint32_t milliseconds);

typedef struct {
    bool on;
    uint32_t duration_ms;
} xy_actuator_buzzer_step_t;

typedef struct {
    xy_hal_gpio_port_t port;
    uint8_t pin;
    bool active_high;
    bool initialized;
    bool is_on;
    xy_actuator_buzzer_delay_fn delay_ms;
} xy_actuator_buzzer_t;

actuator_err_t xy_actuator_buzzer_init(xy_actuator_buzzer_t *buzzer);
actuator_err_t xy_actuator_buzzer_on(xy_actuator_buzzer_t *buzzer);
actuator_err_t xy_actuator_buzzer_off(xy_actuator_buzzer_t *buzzer);
actuator_err_t xy_actuator_buzzer_pulse(xy_actuator_buzzer_t *buzzer, uint32_t duration_ms);
actuator_err_t xy_actuator_buzzer_play(xy_actuator_buzzer_t *buzzer,
                                       const xy_actuator_buzzer_step_t *steps,
                                       size_t step_count);

#endif /* XY_ACTUATOR_BUZZER_H */
