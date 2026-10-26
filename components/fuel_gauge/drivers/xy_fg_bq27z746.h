/**
 * @file xy_fg_bq27z746.h
 * @brief BQ27Z746 Fuel Gauge Driver Interface
 * @version 1.0.0
 * @date 2026-03-05
 * 
 * Texas Instruments BQ27Z746
 * - Impedance Track™ 技术
 * - 支持 1 节串联电池
 * - 高精度电量计
 */

#ifndef XY_FG_BQ27Z746_H
#define XY_FG_BQ27Z746_H

#include "xy_fuel_gauge.h"
#include "xy_sensor_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief BQ27Z746 I2C 地址
 */
#define BQ27Z746_ADDR           0x55

/**
 * @brief BQ27Z746 per-instance driver data.
 *
 * Callers that register more than one BQ27Z746 provide one of these per
 * instance; the driver stores bus configuration, the driver-private init
 * flag, cached sample data, cached alert thresholds, the private
 * average-current cache, cached flags, and the device-type bytes here.
 */
typedef struct {
    xy_sensor_bus_t bus;
    bool initialized;
    xy_fuel_gauge_data_t data;
    xy_fuel_gauge_alert_t alert;
    int16_t average_current_ma;
    uint16_t flags;
    uint8_t device_type[2];
} xy_fg_bq27z746_data_t;

/**
 * @brief Register a caller-owned BQ27Z746 instance.
 *
 * Unlike @ref xy_fuel_gauge_bq27z746_register, this API lets a caller register
 * multiple BQ27Z746 gauges by supplying its own device and per-instance data
 * storage. The driver fills `fg->api` and `fg->data`, then registers the
 * instance by name.
 *
 * @param fg Caller-owned device instance; `fg->name` must be a unique,
 *        non-NULL name.
 * @param priv Caller-owned per-instance data storage.
 * @param i2c_handle I2C/SMBus handle provided by the board/sensor bus layer.
 * @param addr I2C/SMBus address; pass 0 to use @ref BQ27Z746_ADDR.
 * @return XY_FG_OK on success, or XY_FG_ERROR_* on failure.
 */
int xy_fuel_gauge_bq27z746_register_instance(xy_fuel_gauge_t *fg,
                                              xy_fg_bq27z746_data_t *priv,
                                              void *i2c_handle, uint8_t addr);

/**
 * @brief 注册 BQ27Z746 电量计（单例兼容包装）
 * @param i2c_handle I2C 句柄
 * @param addr I2C 地址 (可选，默认 0x55)
 * @return 状态码
 */
int xy_fuel_gauge_bq27z746_register(void *i2c_handle, uint8_t addr);

/**
 * @brief 获取充电状态
 * @param fg 电量计设备
 * @return true=充电中，false=未充电
 */
bool xy_fuel_gauge_bq27z746_is_charging(xy_fuel_gauge_t *fg);

/**
 * @brief 获取充满状态
 * @param fg 电量计设备
 * @return true=充满，false=未充满
 */
bool xy_fuel_gauge_bq27z746_is_full(xy_fuel_gauge_t *fg);

/**
 * @brief 获取告警标志
 * @param fg 电量计设备
 * @return 标志位
 */
uint16_t xy_fuel_gauge_bq27z746_get_flags(xy_fuel_gauge_t *fg);

#ifdef __cplusplus
}
#endif

#endif /* XY_FG_BQ27Z746_H */
