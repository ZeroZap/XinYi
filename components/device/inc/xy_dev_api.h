/**
 * @file xy_dev_api.h
 * @brief Compatibility umbrella for XinYi device APIs
 * @version 2.0
 * @date 2026-02-28
 *
 * @deprecated The generic device framework API now lives in `xy_device.h`.
 * Bus/peripheral-specific compatibility APIs live in their own headers:
 * `xy_dev_i2c.h`, `xy_dev_spi.h`, `xy_dev_uart.h`, `xy_gpio.h`, `xy_adc.h`,
 * and `xy_pwm.h`.
 *
 * This header is kept as an umbrella include for legacy users that previously
 * included `xy_dev_api.h`. Do not add new duplicate type definitions here;
 * doing so creates conflicting ownership with the RT-Thread/Zephyr-like device
 * model where `xy_device.h` owns the generic registry/lifecycle API and typed
 * capability headers own bus/peripheral APIs.
 */

#ifndef XY_DEV_API_H
#define XY_DEV_API_H

#include "xy_device.h"
#include "xy_dev_i2c.h"
#include "xy_dev_spi.h"
#include "xy_dev_uart.h"
#include "xy_gpio.h"
#include "xy_adc.h"
#include "xy_pwm.h"

#endif /* XY_DEV_API_H */
