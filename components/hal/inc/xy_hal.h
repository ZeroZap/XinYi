/**
 * @file xy_hal.h
 * @brief XinYi Hardware Abstraction Layer main header
 * @version 2.0
 * @date 2026-02-27
 */

#ifndef XY_HAL_H
#define XY_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/**
 * @brief HAL 版本宏定义
 */
#define XY_HAL_VERSION_MAJOR    2
#define XY_HAL_VERSION_MINOR    0
#define XY_HAL_VERSION_PATCH    0
#define XY_HAL_VERSION_STRING   "2.0.0"

/**
 * @brief HAL 错误码
 *
 * 所有 HAL 函数返回这些标准化错误码。
 * 成功由 XY_HAL_OK (0) 表示，错误为负值。
 */
typedef enum {
    XY_HAL_OK                  = 0,   /**< 成功 */
    XY_HAL_ERROR               = -1,  /**< 通用错误 */
    XY_HAL_ERROR_INVALID_PARAM = -2,  /**< 无效参数 */
    XY_HAL_ERROR_NOT_SUPPORT   = -3,  /**< 不支持的功能 */
    XY_HAL_ERROR_TIMEOUT       = -4,  /**< 操作超时 */
    XY_HAL_ERROR_BUSY          = -5,  /**< 资源忙 */
    XY_HAL_ERROR_NO_MEM        = -6,  /**< 内存不足 */
    XY_HAL_ERROR_IO            = -7,  /**< I/O 错误 */
    XY_HAL_ERROR_NOT_INIT      = -8,  /**< 未初始化 */
    XY_HAL_ERROR_ALREADY_INIT  = -9,  /**< 已初始化 */
    XY_HAL_ERROR_NO_RESOURCE   = -10, /**< 资源不可用 */
    XY_HAL_ERROR_FAIL          = -11, /**< 操作失败 */
} xy_hal_error_t;

/**
 * @brief HAL 状态码 (已废弃，使用 xy_hal_error_t)
 */
typedef enum {
    XY_HAL_STATUS_OK          = 0,
    XY_HAL_STATUS_ERROR       = -1,
    XY_HAL_STATUS_TIMEOUT     = -2,
    XY_HAL_STATUS_BUSY        = -3,
    XY_HAL_STATUS_NOT_SUPPORT = -4,
    XY_HAL_STATUS_NO_MEM      = -5,
    XY_HAL_STATUS_NOT_INIT    = -6,
} xy_hal_status_t;

/* Legacy type alias for compatibility */
typedef xy_hal_error_t xy_error_t;
#define XY_ERROR_OK              XY_HAL_OK
#define XY_ERROR_INVALID_PARAM   XY_HAL_ERROR_INVALID_PARAM

/**
 * @brief HAL 基础句柄结构
 */
typedef struct {
    void *instance;     /**< 硬件实例指针 */
    void *user_data;    /**< 用户数据 */
    uint8_t initialized;/**< 初始化标志 */
} xy_hal_handle_t;

/* 包含所有子模块头文件 */
#include "xy_hal_sys.h"
#include "xy_hal_pin.h"
#include "xy_hal_gpio.h"
#include "xy_hal_uart.h"
#include "xy_hal_spi.h"
#include "xy_hal_i2c.h"
#include "xy_hal_i2s.h"
#include "xy_hal_can.h"
#include "xy_hal_timer.h"
#include "xy_hal_pwm.h"
#include "xy_hal_rtc.h"
#include "xy_hal_dma.h"
#include "xy_hal_lp_timer.h"
#include "xy_hal_flash.h"
#include "xy_hal_adc.h"
#include "xy_hal_dac.h"
#include "xy_hal_wdg.h"
#include "xy_hal_exti.h"
#include "xy_hal_rng.h"
#include "xy_hal_ir.h"
#include "xy_hal_tgpio.h"

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_H */
