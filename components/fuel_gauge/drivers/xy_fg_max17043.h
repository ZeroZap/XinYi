/**
 * @file xy_fg_max17043.h
 * @brief MAX17043 Fuel Gauge Driver Interface
 * @version 1.0.0
 * @date 2026-08-06
 *
 * Maxim Integrated MAX17043
 * - ModelGauge single-cell Li-Ion fuel gauge
 * - VCELL/SOC/CRATE register based host contract
 */

#ifndef XY_FG_MAX17043_H
#define XY_FG_MAX17043_H

#include "xy_fuel_gauge.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MAX17043 default I2C address.
 */
#define MAX17043_ADDR 0x36

/**
 * @brief Register the MAX17043 fuel gauge device.
 *
 * The driver owns a static device instance. Re-registering after a successful
 * registration returns an error and does not reconfigure the bus.
 *
 * @param i2c_handle I2C handle provided by the board/sensor bus layer.
 * @param addr I2C address; pass 0 to use @ref MAX17043_ADDR.
 * @return XY_FG_OK on success, or XY_FG_ERROR_* on failure.
 */
int xy_fuel_gauge_max17043_register(void *i2c_handle, uint8_t addr);

#ifdef __cplusplus
}
#endif

#endif /* XY_FG_MAX17043_H */
