/**
 * @file xy_bq25620.h
 * @brief TI BQ25620 Standalone I2C 1-Cell Li-Ion Battery Charger Driver
 * @version 1.0.0
 * @date 2026-03-17
 * 
 * @note BQ25620 是一款独立的 I2C 控制的 1 节锂离子电池充电器
 * 
 * 主要特性:
 * - 输入工作电压范围：3.9V - 18V
 * - 充电电流：最高 3.5A
 * - 充电电压：4.2V (默认)
 * - I2C 接口配置
 * - 热调节和过温保护
 * - 输入过压保护
 * - 电池温度监测 (TS 引脚)
 */

#ifndef XY_BQ25620_H
#define XY_BQ25620_H

#include "xy_charger_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== BQ25620 Register Definitions ==================== */

/**
 * @brief BQ25620 寄存器地址
 */
typedef enum {
    BQ25620_REG_CHG_CTRL_1    = 0x02,  /**< 16-bit charge-current limit */
    BQ25620_REG_CHG_CTRL_3    = 0x04,  /**< 16-bit charge-voltage limit */
    BQ25620_REG_CHG_CTRL_4    = 0x06,  /**< 16-bit input-current limit */
    BQ25620_REG_CHG_CTRL_2    = 0x10,  /**< 16-bit pre-charge current */
    BQ25620_REG_CHG_CTRL_5    = 0x12,  /**< 16-bit termination current */
    BQ25620_REG_CHG_CTRL_0    = 0x14,  /**< Charge Control 0 */
    BQ25620_REG_CHG_CTRL_6    = 0x16,  /**< Charger Control 1 */
    BQ25620_REG_ADC_STAT_0    = 0x1D,  /**< Charger Status 0 */
    BQ25620_REG_CHG_STAT_0    = 0x1E,  /**< Charger Status 1 */
    BQ25620_REG_CHG_STAT_1    = 0x1F,  /**< Fault Status 0 */
    BQ25620_REG_DEVICE_ID     = 0x38,  /**< Part Information 寄存器 */
} bq25620_reg_t;

/* ==================== Register Bit Definitions ==================== */

/* Charger Status 1 (0x1E) */
#define BQ25620_STAT_CHG_MASK       (0x03U << 3) /**< CHG_STAT[4:3] */
#define BQ25620_STAT_CHG_IDLE       (0x00U << 3) /**< 未充电或已终止 */
#define BQ25620_STAT_CHG_FAST       (0x01U << 3) /**< 涓流/预充/恒流 */
#define BQ25620_STAT_CHG_CV         (0x02U << 3) /**< 恒压 */
#define BQ25620_STAT_CHG_TOPOFF     (0x03U << 3) /**< Top-off timer active */
#define BQ25620_STAT_VBUS_MASK      0x07U        /**< VBUS_STAT[2:0] */

/* Fault Status 0 (0x1F) */
#define BQ25620_FAULT_INPUT_OVP     (0x01U << 7) /**< VBUS fault */
#define BQ25620_FAULT_BAT_OVP       (0x01U << 6) /**< BAT OCP/OVP */
#define BQ25620_FAULT_SYS           (0x01U << 5) /**< SYS UVP/OVP */
#define BQ25620_FAULT_OTG           (0x01U << 4) /**< OTG fault */
#define BQ25620_FAULT_THERMAL       (0x01U << 3) /**< Thermal shutdown */
#define BQ25620_FAULT_TS_MASK       0x07U        /**< TS_STAT[2:0] */

/* Charge Control 0 (0x14) */
#define BQ25620_VRECHG              (0x01U << 0) /**< 0=100mV, 1=200mV */

/* Charge Current Limit (0x02, 16-bit little-endian) */
#define BQ25620_ICHG_MASK           (0x3FU << 6) /**< ICHG[5:0] in bits 11:6 */
#define BQ25620_ICHG_STEP_mA        80U
#define BQ25620_ICHG_MIN_mA         80U
#define BQ25620_ICHG_MAX_mA         3520U

/* Pre-charge (0x10) and termination (0x12), both 16-bit little-endian */
#define BQ25620_ITERM_MASK          (0x3FU << 3) /**< ITERM[5:0] in bits 8:3 */
#define BQ25620_IPRECHG_MASK        (0x1FU << 4) /**< IPRECHG[4:0] in bits 8:4 */

/* Charge Voltage Limit (0x04, 16-bit little-endian) */
#define BQ25620_VREG_MASK           (0x1FFU << 3) /**< VREG[8:0] in bits 11:3 */
#define BQ25620_VREG_STEP_mV        10           /**< 充电电压步长 (mV) */
#define BQ25620_VREG_MIN_mV         3500         /**< 最小充电电压 (mV) */
#define BQ25620_VREG_MAX_mV         4800U        /**< 最大充电电压 (mV) */

/* Input Current Limit (0x06, 16-bit little-endian) */
#define BQ25620_ILIM_MASK           (0xFFU << 4) /**< IINDPM[7:0] in bits 11:4 */
#define BQ25620_ILIM_STEP_mA        20U          /**< 输入电流步长 (mA) */
#define BQ25620_ILIM_MIN_mA         100          /**< 最小输入电流 (mA) */
#define BQ25620_ILIM_MAX_mA         3200U        /**< 最大输入电流 (mA) */

/* Charger Control 1 (0x16) */
#define BQ25620_EN_CHG              (0x01U << 5) /**< Charger enable in REG0x16 */

/* CHG_CTRL_7 (0x09) -  Miscellaneous */
#define BQ25620_FORCE_DPDM          (0x01 << 7)  /**< 强制 DPDM 检测 */
#define BQ25620_EN_OC               (0x01 << 4)  /**< 过流保护使能 */

/* Part Information (0x38) */
#define BQ25620_PART_NUMBER_MASK    (0x07U << 3) /**< PN[5:3] */
#define BQ25620_PART_NUMBER         (0x00U << 3) /**< BQ25620 */
#define BQ25622_PART_NUMBER         (0x01U << 3) /**< BQ25622 */
#define BQ25620_DEVICE_REV_MASK     0x07U        /**< DEV_REV[2:0] */

/* BQ25620 uses fixed 7-bit I2C address 0x6B. */
#define BQ25620_I2C_ADDR            0x6BU

/* ==================== BQ25620 Device Structure ==================== */

/**
 * @brief BQ25620 设备结构
 */
typedef struct {
    xy_charger_device_t base;              /**< 充电器基类 */
    void *i2c_handle;               /**< I2C 句柄 */
    uint8_t i2c_addr;               /**< 7-bit I2C 地址 (固定 0x6B) */
    uint32_t owner_cookie;           /**< 已提交 owner 身份，用于原子 re-init */
} xy_bq25620_t;

/* ==================== BQ25620 API ==================== */

/**
 * @brief 初始化 BQ25620
 * @param dev BQ25620 设备句柄
 * @param i2c_handle I2C 句柄
 * @param i2c_addr 7-bit I2C 地址 (0x6B)
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bq25620_init(xy_bq25620_t *dev, void *i2c_handle, uint8_t i2c_addr);

/**
 * @brief 反初始化 BQ25620
 * @param dev BQ25620 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bq25620_deinit(xy_bq25620_t *dev);

/**
 * @brief 读取 BQ25620 寄存器
 * @param dev BQ25620 设备句柄
 * @param reg 寄存器地址
 * @param value 寄存器值输出
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bq25620_read_reg(xy_bq25620_t *dev, uint8_t reg, uint8_t *value);

/**
 * @brief 读取 BQ25620 设备 ID
 * @param dev BQ25620 设备句柄
 * @param id 设备 ID 输出
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bq25620_get_device_id(xy_bq25620_t *dev, uint8_t *id);

/**
 * @brief 获取充电状态
 * @param dev BQ25620 设备句柄
 * @param status 状态输出
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bq25620_get_status(xy_bq25620_t *dev, xy_charger_device_status_t *status);

/**
 * @brief 原子入口校验并应用完整充电配置
 * @param dev BQ25620 设备句柄
 * @param config 完整配置；所有值必须在范围内且可由寄存器步长精确表示
 * @return XY_DEVICE_OK 成功；参数无效时零总线访问；传输失败时返回首个错误
 */
int xy_bq25620_configure(xy_bq25620_t *dev,
                         const xy_charger_device_config_t *config);

/**
 * @brief 设置充电电流
 * @param dev BQ25620 设备句柄
 * @param current_mA 充电电流 (mA)
 * @return XY_DEVICE_OK 成功；超出 80..3520 mA 或不对齐 80 mA 步长时返回
 * XY_DEVICE_INVALID_PARAM 且不访问总线
 */
int xy_bq25620_set_charge_current(xy_bq25620_t *dev, uint32_t current_mA);

/**
 * @brief 设置充电电压
 * @param dev BQ25620 设备句柄
 * @param voltage_mV 充电电压 (mV)
 * @return XY_DEVICE_OK 成功；超出 3500..4800 mV 或不对齐 10 mV 步长时返回
 * XY_DEVICE_INVALID_PARAM 且不访问总线
 */
int xy_bq25620_set_charge_voltage(xy_bq25620_t *dev, uint32_t voltage_mV);

/**
 * @brief 设置输入电流限制
 * @param dev BQ25620 设备句柄
 * @param current_mA 输入电流限制 (mA)
 * @return XY_DEVICE_OK 成功；超出 100..3200 mA 或不对齐 20 mA 步长时返回
 * XY_DEVICE_INVALID_PARAM 且不访问总线
 */
int xy_bq25620_set_input_limit(xy_bq25620_t *dev, uint32_t current_mA);

/**
 * @brief 启动充电
 * @param dev BQ25620 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bq25620_start_charge(xy_bq25620_t *dev);

/**
 * @brief 停止充电
 * @param dev BQ25620 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bq25620_stop_charge(xy_bq25620_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* XY_BQ25620_H */
