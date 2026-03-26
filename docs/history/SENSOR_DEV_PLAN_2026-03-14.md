# XinYi 传感器驱动开发计划

**创建日期**: 2026-03-14  
**来源**: XinSor 传感器选型文档 + Zephyr/RT-Thread/Linux 传感器收集  
**目标**: 扩展 XinYi 传感器驱动库，覆盖常用传感器型号

---

## 📊 现有驱动清单 (XinYi)

| 型号 | 类型 | 状态 | 文件位置 |
|------|------|------|---------|
| AHT20 | 温湿度 | ✅ 已实现 | `sensor_aht20.c/h` |
| BME280 | 温湿压 | ✅ 已实现 | `sensor_bpm280.c` |
| BMP280 | 温压 | ✅ 已实现 | `sensor_bmp280.h` |
| MPU6050 | 6 轴 IMU | ⏳ 待确认 | - |
| ICM20608 | 6 轴 IMU | ✅ 已实现 | `sensor_icm20608.c/h` |
| LIS2DH12 | 3 轴加速度 | ⏳ 待确认 | - |
| SC7A20 | 3 轴加速度 | ⏳ 待确认 | - |
| BMA400 | 3 轴加速度 | ✅ 已实现 | `sensor_bma400.c/h` |
| ADXL362 | 3 轴加速度 | ✅ 已实现 | `sensor_adxl362.c/h` |
| QMC5883L | 3 轴磁力计 | ⏳ 待确认 | - |
| CCS811 | 气体 (eCO2/TVOC) | ✅ 已实现 | `sensor_ccs811.c/h` |
| AP3216C | 光学 (ALS+PS) | ✅ 已实现 | `sensor_ap3216c.c/h` |
| APDS9960 | 光学 (RGB+Gesture) | ✅ 已实现 | `sensor_apds9960.c/h` |
| VL53L0X | ToF 测距 | ⏳ 待确认 | - |
| ADT7420 | 温度 | ✅ 已实现 | `sensor_adt7420.c/h` |

---

## 🎯 第一阶段：高优先级驱动 (2026-03-15 ~ 2026-03-20)

### 1. BMI088 - 6 轴 IMU (无人机/机器人)

**优先级**: 🔥 最高  
**价格**: ¥35-50  
**应用**: 无人机飞控、机器人平衡

**文件规划**:
```
components/sensor/drivers/imu/
├── sensor_bmi088.c      # 驱动实现 (~8KB)
├── sensor_bmi088.h      # 头文件 (~2KB)
└── examples/bmi088_demo/
    └── main.c           # 示例代码
```

**技术要点**:
- SPI 接口 (加速度 + 陀螺仪独立 CS)
- 低温漂，抗振动
- ±24g / ±2000°/s 量程
- 温度补偿

**开发工时**: 4-6 小时

---

### 2. SHT40 - 温湿度 (高精度)

**优先级**: 🔥 最高  
**价格**: ¥50-65  
**应用**: 医疗、工业环境监测

**文件规划**:
```
components/sensor/drivers/humidity/
├── sensor_sht40.c       # 驱动实现 (~6KB)
├── sensor_sht40.h       # 头文件 (~1.5KB)
└── examples/sht40_demo/
    └── main.c
```

**技术要点**:
- I2C 接口
- ±0.1°C / ±1%RH 高精度
- 快速测量 (8ms)
- 低功耗

**开发工时**: 3-4 小时

---

### 3. VL53L1X - ToF 测距

**优先级**: 🔥 高  
**价格**: ¥15-20  
**应用**: 机器人避障、液位检测

**文件规划**:
```
components/sensor/drivers/distance/
├── sensor_vl53l1x.c     # 驱动实现 (~10KB)
├── sensor_vl53l1x.h     # 头文件 (~2KB)
└── examples/vl53l1x_demo/
    └── main.c
```

**技术要点**:
- I2C 接口
- 0-4m 测距
- 区域测量模式
- 多传感器同步

**开发工时**: 4-5 小时

---

### 4. LPS22HB - 气压 (防水)

**优先级**: 💰 高  
**价格**: ¥12-18  
**应用**: 户外设备、无人机定高

**文件规划**:
```
components/sensor/drivers/pressure/
├── sensor_lps22hb.c     # 驱动实现 (~5KB)
├── sensor_lps22hb.h     # 头文件 (~1.5KB)
└── examples/lps22hb_demo/
    └── main.c
```

**技术要点**:
- I2C/SPI接口
- 防水设计
- ±0.25 hPa 精度 (±2m)
- 低功耗 (1.8μA)

**开发工时**: 3-4 小时

---

### 5. SGP40 - VOC 气体

**优先级**: 💰 高  
**价格**: ¥35-45  
**应用**: 室内空气质量监测

**文件规划**:
```
components/sensor/drivers/gas/
├── sensor_sgp40.c       # 驱动实现 (~7KB)
├── sensor_sgp40.h       # 头文件 (~2KB)
└── examples/sgp40_demo/
    └── main.c
```

**技术要点**:
- I2C 接口
- VOC 指数输出
- 内置温湿度补偿
- 长期稳定性

**开发工时**: 3-4 小时

---

## 📋 第二阶段：中优先级驱动 (2026-03-21 ~ 2026-03-31)

| 型号 | 类型 | 价格 | 应用 | 工时 |
|------|------|------|------|------|
| BMI270 | 6 轴 IMU | ¥25-35 | 可穿戴，AI 运动识别 | 5h |
| LSM6DSO | 6 轴 IMU | ¥25-35 | 机器学习核心 | 5h |
| SCD40 | CO2 (NDIR) | ¥120-150 | 室内空气质量 | 6h |
| BME688 | 气体+AI | ¥80-100 | 气体识别 | 8h |
| TSL2591 | 光照 | ¥15-20 | 高动态范围 | 3h |
| MAX30102 | 心率血氧 | ¥15-25 | 可穿戴医疗 | 6h |
| DPS310 | 气压 | ¥25-35 | 超高精度±2cm | 4h |

---

## 🔧 第三阶段：补充驱动 (2026-04-01 ~)

### IMU 系列
- [ ] BMI160 (可穿戴低功耗)
- [ ] ICM42688 (低功耗高精度)
- [ ] LSM9DS1 (9 轴)
- [ ] BMM150 (磁力计)

### 温湿度系列
- [ ] SHT31/SHT35 (高精度)
- [ ] HDC1080 (低功耗)
- [ ] SI7021 (工业级)

### 气体系列
- [ ] SCD30 (CO2 NDIR)
- [ ] ENS160 (eCO2 低成本)
- [ ] PMS5003 (PM2.5)

### 光学系列
- [ ] APDS9960 (RGB+ 手势)
- [ ] VEML6030 (低功耗光照)
- [ ] TCS34725 (颜色识别)

---

## 📁 驱动目录结构规划

```
components/sensor/
├── inc/
│   ├── xy_sensor.h              # 主头文件
│   ├── xy_sensor_device.h       # 设备接口
│   ├── xy_sensor_attr.h         # 属性定义
│   └── xy_sensor_type.h         # 传感器类型
│
├── drivers/
│   ├── imu/                     # IMU 驱动
│   │   ├── sensor_bmi088.c/h
│   │   ├── sensor_bmi270.c/h
│   │   ├── sensor_lsm6dso.c/h
│   │   └── ...
│   ├── humidity/                # 温湿度驱动
│   │   ├── sensor_sht40.c/h
│   │   ├── sensor_sht3x.c/h
│   │   └── ...
│   ├── pressure/                # 气压驱动
│   │   ├── sensor_lps22hb.c/h
│   │   ├── sensor_dps310.c/h
│   │   └── ...
│   ├── gas/                     # 气体驱动
│   │   ├── sensor_sgp40.c/h
│   │   ├── sensor_scd40.c/h
│   │   └── ...
│   ├── distance/                # 距离驱动
│   │   ├── sensor_vl53l1x.c/h
│   │   └── ...
│   └── optical/                 # 光学驱动
│       ├── sensor_tsl2591.c/h
│       └── ...
│
├── examples/
│   ├── bmi088_demo/
│   ├── sht40_demo/
│   ├── vl53l1x_demo/
│   └── ...
│
└── docs/
    ├── DRIVER_DEVELOPMENT_GUIDE.md
    ├── SENSOR_SUPPORT_MATRIX.md
    └── ...
```

---

## 🛠️ 开发规范

### 驱动模板
```c
// sensor_xxx.h
#ifndef SENSOR_XYZ_H
#define SENSOR_XYZ_H

#include "xy_sensor_device.h"

// 设备结构体
typedef struct {
    xy_sensor_device_t base;
    // 设备特定成员
} sensor_xyz_t;

// API 函数
xy_ret_t sensor_xyz_init(sensor_xyz_t *dev, xy_i2c_t *i2c, uint8_t addr);
xy_ret_t sensor_xyz_read(sensor_xyz_t *dev, xyz_data_t *data);
xy_ret_t sensor_xyz_deinit(sensor_xyz_t *dev);

#endif
```

### 文档要求
每个驱动需包含:
- [ ] 数据手册链接
- [ ] 接线图
- [ ] 使用示例
- [ ] API 说明
- [ ] 注意事项

---

## 📊 进度追踪

| 阶段 | 驱动数量 | 开始日期 | 结束日期 | 状态 |
|------|---------|---------|---------|------|
| 第一阶段 | 5 | 2026-03-15 | 2026-03-20 | 🟡 进行中 |
| 第二阶段 | 7 | 2026-03-21 | 2026-03-31 | ⏳ 待开始 |
| 第三阶段 | 15+ | 2026-04-01 | TBD | ⏳ 待开始 |

---

## 📝 与 XinSor 知识库联动

每个驱动开发完成后:
1. 更新 XinSor 对应索引文档的"XinYi 驱动状态"
2. 添加驱动文档链接
3. 补充示例代码到 XinSor 参考设计

---

**维护**: XinYi 嵌入式团队  
**最后更新**: 2026-03-14
