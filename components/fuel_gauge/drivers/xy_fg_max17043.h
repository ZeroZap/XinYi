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
#include "xy_sensor_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MAX17043 default I2C address.
 */
#define MAX17043_ADDR 0x36

/**
 * @brief MAX17043 per-instance driver data.
 *
 * Callers that register more than one MAX17043 provide one of these per
 * instance; the driver stores bus configuration, the driver-private init
 * flag, cached sample data, and cached alert thresholds here.
 */
typedef struct {
    xy_sensor_bus_t bus;
    bool initialized;
    xy_fuel_gauge_data_t data;
    xy_fuel_gauge_alert_t alert;
} xy_fg_max17043_data_t;

/**
 * @brief Register a caller-owned MAX17043 instance.
 *
 * Unlike @ref xy_fuel_gauge_max17043_register, this API lets a caller register
 * multiple MAX17043 gauges by supplying its own device and per-instance data
 * storage. The driver fills `fg->api` and `fg->data`, then registers the
 * instance by name.
 *
 * @param fg Caller-owned device instance; `fg->name` must be a unique,
 *        non-NULL name.
 * @param priv Caller-owned per-instance data storage.
 * @param i2c_handle I2C handle provided by the board/sensor bus layer.
 * @param addr I2C address; pass 0 to use @ref MAX17043_ADDR.
 * @return XY_FG_OK on success, or XY_FG_ERROR_* on failure.
 */
int xy_fuel_gauge_max17043_register_instance(xy_fuel_gauge_t *fg,
                                             xy_fg_max17043_data_t *priv,
                                             void *i2c_handle, uint8_t addr);

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
