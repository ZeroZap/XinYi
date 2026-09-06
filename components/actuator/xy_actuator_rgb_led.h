#ifndef XY_ACTUATOR_RGB_LED_H
#define XY_ACTUATOR_RGB_LED_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "xy_actuator.h"
#include "xy_hal_gpio.h"

typedef void (*xy_actuator_rgb_led_delay_fn)(uint32_t milliseconds);

typedef enum {
    XY_ACTUATOR_RGB_OFF = 0U,
    XY_ACTUATOR_RGB_RED = 1U,
    XY_ACTUATOR_RGB_GREEN = 2U,
    XY_ACTUATOR_RGB_BLUE = 4U,
    XY_ACTUATOR_RGB_YELLOW = XY_ACTUATOR_RGB_RED | XY_ACTUATOR_RGB_GREEN,
    XY_ACTUATOR_RGB_MAGENTA = XY_ACTUATOR_RGB_RED | XY_ACTUATOR_RGB_BLUE,
    XY_ACTUATOR_RGB_CYAN = XY_ACTUATOR_RGB_GREEN | XY_ACTUATOR_RGB_BLUE,
    XY_ACTUATOR_RGB_WHITE = XY_ACTUATOR_RGB_RED | XY_ACTUATOR_RGB_GREEN |
                            XY_ACTUATOR_RGB_BLUE,
} xy_actuator_rgb_color_t;

typedef struct {
    xy_actuator_rgb_color_t color;
    uint32_t duration_ms;
} xy_actuator_rgb_led_step_t;

typedef struct {
    xy_hal_gpio_port_t port;
    uint8_t red_pin;
    uint8_t green_pin;
    uint8_t blue_pin;
    bool active_low;
    bool initialized;
    xy_actuator_rgb_color_t color;
    xy_actuator_rgb_led_delay_fn delay_ms;
} xy_actuator_rgb_led_t;

actuator_err_t xy_actuator_rgb_led_init(xy_actuator_rgb_led_t *led);
actuator_err_t xy_actuator_rgb_led_set(xy_actuator_rgb_led_t *led,
                                       xy_actuator_rgb_color_t color);
actuator_err_t xy_actuator_rgb_led_off(xy_actuator_rgb_led_t *led);
actuator_err_t xy_actuator_rgb_led_play(xy_actuator_rgb_led_t *led,
                                        const xy_actuator_rgb_led_step_t *steps,
                                        size_t step_count);

#endif /* XY_ACTUATOR_RGB_LED_H */
