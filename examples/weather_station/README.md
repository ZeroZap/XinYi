# 智能气象站示例

**版本**: 1.0  
**日期**: 2026-03-01

---

## 📋 概述

综合演示 XinYi 框架的多传感器融合、PID 控制、消息队列、FOTA 升级等功能。

### 功能特性

- ✅ SHT30 温湿度传感器
- ✅ MPU6050 六轴姿态传感器
- ✅ OLED 显示屏
- ✅ PID 温度控制
- ✅ 消息队列 IPC
- ✅ FOTA 固件升级
- ✅ 多任务实时系统

---

## 🏗️ 系统架构

```
┌─────────────────────────────────────────────┐
│              应用层 (Application)            │
│  传感器任务 │ PID 任务 │ 显示任务 │ FOTA 任务  │
├─────────────────────────────────────────────┤
│              IPC 通信层 (Message Queue)       │
├─────────────────────────────────────────────┤
│              设备驱动层 (Device Drivers)      │
│   SHT30  │  MPU6050  │  OLED  │  FOTA        │
├─────────────────────────────────────────────┤
│              OSAL 层 (OS Abstraction)        │
│        FreeRTOS / RT-Thread / Bare-metal    │
└─────────────────────────────────────────────┘
```

---

## 🚀 快速开始

### 1. 克隆项目

```bash
cd XinYi/examples/weather_station
```

### 2. 构建项目

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 3. 运行

```bash
# PC 仿真
./build/weather_station

# 或部署到开发板
make flash
```

---

## 📊 任务调度

| 任务 | 优先级 | 周期 | 说明 |
|------|--------|------|------|
| Sensor | 5 | 1000ms | 传感器数据采集 |
| PID | 6 | 100ms | 温度 PID 控制 |
| Display | 4 | 5000ms | OLED 显示刷新 |
| FOTA | 3 | 1000ms | 固件升级检测 |

---

## 📝 配置说明

### 温度控制参数

```c
#define TEMP_TARGET       25.0f    /* 目标温度 */
#define PID_KP            2.0f     /* 比例增益 */
#define PID_KI            0.5f     /* 积分增益 */
#define PID_KD            1.0f     /* 微分增益 */
```

### 消息队列配置

```c
xy_mq_config_t mq_cfg = {
    .msg_size = sizeof(sensor_data_t),
    .max_msgs = 10,
    .overwrite_old = true,
};
```

---

## 🔗 相关文档

- [SHT30 驱动](../../components/sensor/xy_sht30.c)
- [MPU6050 驱动](../../components/sensor/xy_mpu6050.c)
- [PID 控制器](../../components/pid/xy_pid.c)
- [消息队列](../../components/dm/xy_mq.c)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
