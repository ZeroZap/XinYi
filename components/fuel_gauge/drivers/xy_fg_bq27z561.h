/**
 * @file xy_fg_bq27z561.h
 * @brief BQ27Z561 Fuel Gauge Driver Interface
 * @version 1.0.0
 * @date 2026-08-06
 *
 * Texas Instruments BQ27Z561
 * - Impedance Track™ technology
 * - Single-cell Li-Ion fuel gauge
 */

#ifndef XY_FG_BQ27Z561_H
#define XY_FG_BQ27Z561_H

#include "xy_fuel_gauge.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief BQ27Z561 default I2C/SMBus address.
 */
#define BQ27Z561_ADDR 0x55

/**
 * @brief Register the BQ27Z561 fuel gauge device.
 *
 * The driver owns a static device instance. Re-registering after a successful
 * registration returns an error and does not reconfigure the bus.
 *
 * @param i2c_handle I2C/SMBus handle provided by the board/sensor bus layer.
 * @param addr I2C/SMBus address; pass 0 to use @ref BQ27Z561_ADDR.
 * @return XY_FG_OK on success, or XY_FG_ERROR_* on failure.
 */
int xy_fuel_gauge_bq27z561_register(void *i2c_handle, uint8_t addr);

#ifdef __cplusplus
}
#endif

#endif /* XY_FG_BQ27Z561_H */
