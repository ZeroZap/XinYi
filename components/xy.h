/**
 * @file xy.h
 * @brief XinYi Framework Master Header
 * @version 2.0.0
 * @date 2026-03-02
 */

#ifndef XINYI_H
#define XINYI_H

/**
 * @mainpage XinYi 嵌入式框架
 * 
 * @section intro 简介
 * XinYi 是一个模块化、生产级的嵌入式 C 框架
 * 
 * @section features 核心特性
 * - 模块化架构
 * - 跨平台支持 (STM32/WCH/HC32)
 * - 多 RTOS 后端
 * - 完善的驱动库
 * - 丰富的中间件
 * 
 * @section quickstart 快速开始
 * @code
 * #include "xy.h"
 * 
 * int main(void) {
 *     xy_os_kernel_init();
 *     xy_log_i("Hello XinYi!\n");
 *     xy_os_kernel_start();
 *     return 0;
 * }
 * @endcode
 */

/* ==================== 版本信息 ==================== */

#define XINYI_VERSION_MAJOR     2
#define XINYI_VERSION_MINOR     0
#define XINYI_VERSION_PATCH     0
#define XINYI_VERSION_STRING    "2.0.0"

/* ==================== 核心组件 ==================== */

/* 系统类型定义 */
#include "xy_typedef.h"

/* 通用定义 */
#include "xy_def.h"

/* ==================== OSAL (OS 抽象层) ==================== */

#ifdef CONFIG_OSAL
#include "kernel/osal/inc/xy_os_sys.h"
#endif

/* ==================== HAL (硬件抽象层) ==================== */

#ifdef CONFIG_HAL
#include "hal/inc/xy_hal.h"
#endif

/* ==================== 基础库 ==================== */

#ifdef CONFIG_CLIB
/* xy_clib 功能 */
#endif

#ifdef CONFIG_CRYPTO
/* 密码学功能 */
#endif

/* ==================== 设备驱动 ==================== */

#ifdef CONFIG_DEVICE
#include "device/inc/xy_device.h"
#endif

#ifdef CONFIG_SENSOR
/* 传感器驱动 */
#include "sensor/inc/xy_sht30.h"
#include "sensor/inc/xy_sht40.h"
#include "sensor/inc/xy_hdc1080.h"
#include "sensor/inc/xy_aht20.h"
#include "sensor/inc/xy_mpu6050.h"
#include "sensor/inc/xy_bmp280.h"
#include "sensor/inc/xy_oled_ssd1306.h"
#include "sensor/inc/xy_bh1750.h"
#include "sensor/inc/xy_tsl2561.h"
#endif

#ifdef CONFIG_POWER
/* 电源管理 */
#include "sensor/inc/xy_bq25620.h"
#include "sensor/inc/xy_coulomb.h"
#include "sensor/inc/xy_ina226.h"
#include "sensor/inc/xy_max17043.h"
#include "sensor/inc/xy_ltc2945.h"
#endif

/* ==================== 中间件 ==================== */

#ifdef CONFIG_PID
#include "pid/inc/xy_pid.h"
#include "pid/inc/xy_pid_auto.h"
#endif

#ifdef CONFIG_FOTA
#include "fota/inc/xy_fota.h"
#include "fota/inc/xy_fota_secure.h"
#endif

#ifdef CONFIG_GUI
#include "gui/inc/xy_gui.h"
#include "gui/inc/xy_font.h"
#endif

#ifdef CONFIG_DM
/* 数据管理 */
#include "dm/inc/xy_json.h"
#include "dm/inc/xy_fs.h"
#endif

#ifdef CONFIG_IPC
/* IPC 通信 */
#include "ipc/inc/xy_mq.h"
#endif

/* ==================== 系统服务 ==================== */

#ifdef CONFIG_SYSMON
#include "kernel/misc/inc/xy_sysmon.h"
#endif

#ifdef CONFIG_AUTOTASK
#include "kernel/misc/inc/xy_autotask.h"
#endif

/* ==================== 调试工具 ==================== */

#ifdef CONFIG_LOG
#include "trace/xy_log/inc/xy_log.h"
#endif

#ifdef CONFIG_ASSERT
#include "clib/xy_clib/xy_assert.h"
#endif

#endif /* XINYI_H */
