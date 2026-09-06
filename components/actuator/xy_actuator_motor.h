#ifndef XY_ACTUATOR_MOTOR_H
#define XY_ACTUATOR_MOTOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "xy_actuator.h"
#include "xy_hal_gpio.h"

typedef void (*xy_actuator_motor_delay_fn)(uint32_t milliseconds);

typedef enum {
    XY_ACTUATOR_MOTOR_STANDBY = 0,
    XY_ACTUATOR_MOTOR_FORWARD,
    XY_ACTUATOR_MOTOR_REVERSE,
    XY_ACTUATOR_MOTOR_BRAKE,
} xy_actuator_motor_mode_t;

typedef struct {
    xy_actuator_motor_mode_t mode;
    uint32_t duration_ms;
} xy_actuator_motor_step_t;

typedef struct {
    xy_hal_gpio_port_t ina_port;
    uint8_t ina_pin;
    xy_hal_gpio_port_t inb_port;
    uint8_t inb_pin;
    uint32_t break_before_make_ms;
    bool initialized;
    xy_actuator_motor_mode_t mode;
    xy_actuator_motor_delay_fn delay_ms;
} xy_actuator_motor_t;

actuator_err_t xy_actuator_motor_init(xy_actuator_motor_t *motor);
actuator_err_t xy_actuator_motor_set_mode(xy_actuator_motor_t *motor,
                                          xy_actuator_motor_mode_t mode);
actuator_err_t xy_actuator_motor_standby(xy_actuator_motor_t *motor);
actuator_err_t xy_actuator_motor_forward_pulse(xy_actuator_motor_t *motor,
                                               uint32_t duration_ms);
actuator_err_t xy_actuator_motor_play(xy_actuator_motor_t *motor,
                                      const xy_actuator_motor_step_t *steps,
                                      size_t step_count);

#endif /* XY_ACTUATOR_MOTOR_H */