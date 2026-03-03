/**
 * @file xy_wiki.h
 * @brief XinYi Wiki Documentation Index
 * @version 1.0.0
 * @date 2026-03-01 YOLO 通宵
 */

#ifndef XY_WIKI_H
#define XY_WIKI_H

/**
 * @mainpage XinYi 嵌入式框架 Wiki
 * 
 * @section intro_sec 简介
 * XinYi 是一个模块化、生产级的嵌入式 C 框架。
 * 
 * @section features_sec 核心特性
 * - 模块化架构
 * - 跨平台支持 (STM32/HC32/WCH)
 * - 多 RTOS 后端 (FreeRTOS/RT-Thread/Bare-metal)
 * - 完善的驱动库
 * - 丰富的中间件
 * 
 * @section quickstart_sec 快速开始
 * @code
 * #include "xy_os.h"
 * #include "xy_log.h"
 * 
 * int main(void) {
 *     xy_os_kernel_init();
 *     xy_log_i("Hello XinYi!\n");
 *     xy_os_kernel_start();
 *     return 0;
 * }
 * @endcode
 * 
 * @author XinYi Team
 * @date 2026-03-01
 * @version 2.0
 */

/**
 * @page architecture 系统架构
 * 
 * @section arch_overview 架构概览
 * 
 * @verbatim
 * +---------------------+
 * |   Application Layer |
 * +---------------------+
 * |   Middleware Layer  |
 * +---------------------+
 * |      OSAL Layer     |
 * +---------------------+
 * |      HAL Layer      |
 * +---------------------+
 * |   Hardware Layer    |
 * +---------------------+
 * @endverbatim
 * 
 * @section components_sec 核心组件
 * 
 * | 组件 | 说明 | 状态 |
 * |------|------|------|
 * | OSAL | OS 抽象层 | ✅ |
 * | HAL | 硬件抽象层 | ✅ |
 * | Crypto | 密码学库 | ✅ |
 * | DM | 数据管理 | ✅ |
 * | NET | 网络协议 | ✅ |
 */

/**
 * @page drivers 设备驱动
 * 
 * @section sensor_drivers 传感器驱动
 * - SHT30: 温湿度传感器
 * - HDC1080: 温湿度传感器
 * - MLX90614: 红外温度传感器
 * - MPU6050: 六轴 IMU
 * - ADS1115: 16 位 ADC
 * 
 * @section display_drivers 显示驱动
 * - SSD1306: OLED 显示屏 (128x64)
 * 
 * @section memory_drivers 存储驱动
 * - W25Qxx: SPI Flash
 */

/**
 * @page protocols 协议栈
 * 
 * @section comm_protocols 通信协议
 * - CAN Bus: 控制器局域网
 * - Modbus RTU: 工业协议
 * - JSON: 数据交换格式
 * 
 * @section file_systems 文件系统
 * - VFS: 虚拟文件系统抽象层
 */

/**
 * @page tools 开发工具
 * 
 * @section code_quality 代码质量
 * - code_quality_checker.py: 静态分析工具
 * - benchmark.py: 性能基准测试
 * 
 * @section autotask 自主任务
 * - 自主任务调度系统
 * - TODO 自动执行
 */

#endif
