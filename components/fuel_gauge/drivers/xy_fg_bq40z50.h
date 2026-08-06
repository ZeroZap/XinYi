/**
 * @file xy_fg_bq40z50.h
 * @brief BQ40Z50 Fuel Gauge Driver Interface
 * @version 1.0.0
 * @date 2026-03-05
 * 
 * Texas Instruments BQ40Z50
 * - 支持 2-4 节串联电池
 * - Impedance Track™ 技术
 * - 集成保护功能
 */

#ifndef XY_FG_BQ40Z50_H
#define XY_FG_BQ40Z50_H

#include "xy_fuel_gauge.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief BQ40Z50 I2C 地址
 */
#define BQ40Z50_ADDR            0x0B

/**
 * @brief 注册 BQ40Z50 电量计
 * @param i2c_handle I2C 句柄
 * @param addr I2C 地址 (可选，默认 0x0B)
 * @return 状态码
 */
int xy_fuel_gauge_bq40z50_register(void *i2c_handle, uint8_t addr);

/**
 * @brief 读取并返回实时平衡状态（不更新内部快照）
 *
 * 该 helper 通过底层寄存器读取路径处理 SMBus transient NACK，但只把成功读到的值
 * 写给调用者，不修改 fetch() 维护的原子快照缓存。
 *
 * @param fg 电量计实例
 * @param balance_status 输出平衡状态低 8 位
 * @return XY_FG_OK 成功；XY_FG_ERROR_* 失败
 */
int xy_fuel_gauge_bq40z50_read_balance_status(xy_fuel_gauge_t *fg,
                                              uint8_t *balance_status);

/**
 * @brief 获取缓存的电池平衡状态
 */
uint8_t xy_fuel_gauge_bq40z50_get_balance_status(xy_fuel_gauge_t *fg);

/**
 * @brief 获取电池组电压
 * @param fg 电量计设备
 * @param voltage_mv 电压 (mV)
 * @return 状态码
 */
int xy_fuel_gauge_bq40z50_get_battery_voltage(xy_fuel_gauge_t *fg, uint16_t *voltage_mv);

/**
 * @brief 获取单体电压
 * @param fg 电量计设备
 * @param cell_index 单体索引 (1-4)
 * @param voltage_mv 单体电压 (mV)
 * @return 状态码
 */
int xy_fuel_gauge_bq40z50_get_cell_voltage(xy_fuel_gauge_t *fg, 
                                           uint8_t cell_index,
                                           uint16_t *voltage_mv);

/**
 * @brief 获取保护状态
 * @param fg 电量计设备
 * @return 保护状态位掩码
 */
uint32_t xy_fuel_gauge_bq40z50_get_protection_status(xy_fuel_gauge_t *fg);

/**
 * @brief 检查充电状态
 * @param fg 电量计设备
 * @return true 正在充电
 */
bool xy_fuel_gauge_bq40z50_is_charging(xy_fuel_gauge_t *fg);

/**
 * @brief 检查放电状态
 * @param fg 电量计设备
 * @return true 正在放电
 */
bool xy_fuel_gauge_bq40z50_is_discharging(xy_fuel_gauge_t *fg);

/**
 * @brief 检查充满状态
 * @param fg 电量计设备
 * @return true 已充满
 */
bool xy_fuel_gauge_bq40z50_is_full(xy_fuel_gauge_t *fg);

/**
 * @brief 检查保护状态
 * @param fg 电量计设备
 * @return true 存在保护状态
 */
bool xy_fuel_gauge_bq40z50_is_protected(xy_fuel_gauge_t *fg);

#ifdef __cplusplus
}
#endif

#endif /* XY_FG_BQ40Z50_H */
