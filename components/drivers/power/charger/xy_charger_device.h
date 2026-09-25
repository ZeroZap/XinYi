/**
 * @file xy_charger_device.h
 * @brief Device-model charger driver interface
 * @version 1.0.0
 * @date 2026-03-17
 * 
 * @note 统一的充电器管理框架
 */

#ifndef XY_CHARGER_DEVICE_H
#define XY_CHARGER_DEVICE_H

#include "xy_device.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Charger Types ==================== */

/**
 * @brief 充电器状态
 */
typedef enum {
    XY_CHARGER_DEVICE_STATE_IDLE = 0,      /**< 空闲 */
    XY_CHARGER_DEVICE_STATE_PRE_CHARGE,    /**< 预充电 */
    XY_CHARGER_DEVICE_STATE_FAST_CHARGE,   /**< 快充 */
    XY_CHARGER_DEVICE_STATE_CONSTANT_VOLT, /**< 恒压充电 */
    XY_CHARGER_DEVICE_STATE_CHARGE_DONE,   /**< 充电完成 */
    XY_CHARGER_DEVICE_STATE_FAULT,         /**< 故障 */
} xy_charger_device_state_t;

/**
 * @brief 充电器故障类型
 */
typedef enum {
    XY_CHARGER_DEVICE_FAULT_NONE = 0,      /**< 无故障 */
    XY_CHARGER_DEVICE_FAULT_INPUT_OVP,     /**< 输入过压 */
    XY_CHARGER_DEVICE_FAULT_THERMAL,       /**< 过热 */
    XY_CHARGER_DEVICE_FAULT_CHARGE_TIMEOUT,/**< 充电超时 */
    XY_CHARGER_DEVICE_FAULT_BAT_OVP,       /**< 电池过压 */
    XY_CHARGER_DEVICE_FAULT_COLD,          /**< 电池过冷 */
    XY_CHARGER_DEVICE_FAULT_HOT,           /**< 电池过热 */
    XY_CHARGER_DEVICE_FAULT_UNKNOWN,       /**< 未识别的硬件故障编码 */
} xy_charger_device_fault_t;

/**
 * @brief 充电器配置
 */
typedef struct {
    uint32_t input_current_limit;   /**< 输入电流限制 (mA) */
    uint32_t charge_current;        /**< 充电电流 (mA) */
    uint32_t charge_voltage;        /**< 充电电压 (mV) */
    uint32_t precharge_current;     /**< 预充电电流 (mA) */
    uint32_t termination_current;   /**< 终止电流 (mA) */
    uint16_t recharge_threshold;    /**< 再充电阈值 (mV 低于 VREG) */
    bool auto_recharge;             /**< 自动再充电使能 */
} xy_charger_device_config_t;

/**
 * @brief 充电器状态信息
 */
typedef struct {
    xy_charger_device_state_t state;       /**< 充电状态 */
    xy_charger_device_fault_t fault;       /**< 故障类型 */
    uint32_t configured_charge_voltage;    /**< 充电电压设定值 (mV)，非 ADC 测量值 */
    uint32_t configured_charge_current;    /**< 充电电流设定值 (mA)，非 ADC 测量值 */
    uint32_t configured_input_current_limit; /**< 输入限流设定值 (mA)，非 ADC 测量值 */
    bool power_good;                /**< 电源良好 */
    bool charging;                  /**< 充电中 */
    bool done;                      /**< 充电完成 */
} xy_charger_device_status_t;

/**
 * @brief 充电器设备结构
 */
typedef struct {
    xy_device_t base;               /**< 设备基类 */

    /* 硬件操作接口 */
    int (*hw_init)(void *hw_data);
    int (*hw_read_status)(void *hw_data, xy_charger_device_status_t *status);
    int (*hw_set_config)(void *hw_data, const xy_charger_device_config_t *config);
    int (*hw_enable)(void *hw_data, bool enable);
    int (*hw_read_reg)(void *hw_data, uint8_t reg, uint8_t *value);

    void *hw_data;                  /**< 硬件数据 */
} xy_charger_device_t;

/*
 * This header defines the shared Device-model types and hardware-operation
 * contract only. Chip-specific drivers own their public lifecycle and control
 * APIs; do not declare generic functions here without a canonical
 * implementation owner.
 */

#ifdef __cplusplus
}
#endif

#endif /* XY_CHARGER_DEVICE_H */
