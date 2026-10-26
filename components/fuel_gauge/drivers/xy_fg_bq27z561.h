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
#include "xy_sensor_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief BQ27Z561 default I2C/SMBus address.
 */
#define BQ27Z561_ADDR 0x55

/**
 * @brief BQ27Z561 per-instance driver data.
 *
 * Callers that register more than one BQ27Z561 provide one of these per
 * instance; the driver stores bus configuration, the driver-private init
 * flag, cached sample data, cached alert thresholds, and the private
 * average-current cache here.
 */
typedef struct {
    xy_sensor_bus_t bus;
    bool initialized;
    xy_fuel_gauge_data_t data;
    xy_fuel_gauge_alert_t alert;
    int16_t average_current_ma;
} xy_fg_bq27z561_data_t;

/**
 * @brief Register a caller-owned BQ27Z561 instance.
 *
 * Unlike @ref xy_fuel_gauge_bq27z561_register, this API lets a caller register
 * multiple BQ27Z561 gauges by supplying its own device and per-instance data
 * storage. The driver fills `fg->api` and `fg->data`, then registers the
 * instance by name.
 *
 * @param fg Caller-owned device instance; `fg->name` must be a unique,
 *        non-NULL name.
 * @param priv Caller-owned per-instance data storage.
 * @param i2c_handle I2C/SMBus handle provided by the board/sensor bus layer.
 * @param addr I2C/SMBus address; pass 0 to use @ref BQ27Z561_ADDR.
 * @return XY_FG_OK on success, or XY_FG_ERROR_* on failure.
 */
int xy_fuel_gauge_bq27z561_register_instance(xy_fuel_gauge_t *fg,
                                             xy_fg_bq27z561_data_t *priv,
                                             void *i2c_handle, uint8_t addr);

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
