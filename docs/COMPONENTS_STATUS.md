# XinYi 组件状态汇总

**最后更新**: 2026-03-30  
**版本**: 2.1.0

---

## 快速导航

- [组件总览](#组件总览)
- [新增组件](#新增组件-2026-03-30)
- [详细状态](#详细状态)
- [构建状态](#构建状态)

---

## 组件总览

| 组件 | 目录 | 状态 | 构建 | 备注 |
|------|------|------|------|------|
| **Sensor** | `components/sensor/` | 🟢 稳定 | ✅ | 60+ 传感器驱动 |
| **Actuator** | `components/actuator/` | 🟢 稳定 | ✅ | 舵机/继电器/PWM |
| **SMBus/PMBus** | `components/smbus/` | 🟢 稳定 | ✅ | 电源管理总线 |
| **HAL** | `components/hal/` | 🟢 稳定 | ✅ | 硬件抽象层 |
| **OSAL** | `components/kernel/osal/` | 🟢 稳定 | ✅ | OS 抽象层 |
| **Trace/Log** | `components/trace/` | 🟢 稳定 | ✅ | 日志系统 |
| **Net** | `components/net/` | 🟢 稳定 | ✅ | MQTT/Modbus/AT |
| **Crypto** | `components/crypto/` | 🟢 稳定 | ✅ | AES/CRC/SHA |
| **DM** | `components/dm/` | 🟢 稳定 | ✅ | 数据管理 |
| **PID** | `components/pid/` | 🟢 稳定 | ✅ | PID 控制器 |
| **IPC** | `components/ipc/` | 🟢 稳定 | ✅ | 进程间通信 |
| **PM** | `components/pm/` | 🟢 稳定 | ✅ | 电源管理 |
| **Fuel Gauge** | `components/fuel_gauge/` | 🟡 host-guarded | ✅ | standalone 电量计；SMBus 硬件验证 pending |
| **GUI** | `components/gui/` | 🟡 host-guarded | ✅ | core/widgets/effects/fonts/display-backend adapter 已有 CTest；真实屏幕、字体来源/美术审查 pending |
| **FOTA** | `components/fota/` | 🟢 主线可用 | ✅ | host CTest + smoke example 已闭环；bootloader/board NOR 硬件记录 pending |

**图例**: 🟢 稳定/主线可用 | 🟡 host-guarded 但仍待硬件或人工证据 | ⚠️ 需要工作

---

## 新增组件 (2026-03-30)

### GPS Sensor 驱动

- **文件**: `components/sensor/sensors/sensor_gps.{h,c}`
- **功能**: NMEA 协议解析, 支持 AT6558/LC86L/UBLOX
- **状态**: 🟢 稳定

```c
// 使用示例
gps_device_t gps = {
    .config.baudrate = 9600,
    .config.update_rate = 1,
};
gps_register(&gps);

// UART 输入 NMEA 数据
gps_parse_byte(&gps_dev, byte);

// 读取位置
gps_data_t data;
gps_read_data(&gps_dev, &data);
printf("Lat: %.6f, Lon: %.6f\n", data.latitude, data.longitude);
```

### Actuator 执行器框架

- **文件**: `components/actuator/xy_actuator.{h,c}`
- **功能**: 继电器/舵机/PWM 控制
- **状态**: 🟢 稳定

```c
// 继电器控制
actuator_device_t relay = {
    .name = "relay_1",
    .type = ACTUATOR_TYPE_RELAY,
};
relay_on(&relay);
os_delay_ms(500);
relay_off(&relay);

// 舵机控制
actuator_device_t servo = {
    .name = "servo_1",
    .type = ACTUATOR_TYPE_SERVO,
    .config.servo_min_angle = -90.0f,
    .config.servo_max_angle = 90.0f,
};
servo_set_angle(&servo, 45.0f);

// 急停所有
actuator_emergency_stop_all();
```

### SMBus/PMBus 协议栈

- **文件**: `components/smbus/xy_smbus.{h,c}`, `xy_pmbus.{h,c}`
- **功能**: SMBus 协议, PMBus 电源管理
- **状态**: 🟢 稳定

```c
// SMBus 操作
smbus_write_word(&smbus, 0x40, 0x21, 0x1234);
smbus_read_word(&smbus, 0x40, 0x2B, &value);

// PMBus 操作
pmbus_device_t pmbus = {
    .config.smbus.addr = 0x40,
    .config.vout_mode = PMBUS_VOUT_MODE_LINEAR,
};
pmbus_read_vout(&pmbus, &voltage);
pmbus_read_iout(&pmbus, &current);
pmbus_get_status(&pmbus, &status);
```

---

## 详细状态

### Sensor 传感器框架

**目录**: `components/sensor/`  
**状态**: 🟢 稳定

**功能**:
- [x] 传感器框架核心
- [x] 60+ 传感器驱动 (详见 `sensors/` 目录)
- [x] GPS 驱动 (NMEA 协议)
- [x] FIFO/中断/校准支持
- [x] 示例代码 (`examples/gps_example.c`)

**传感器类型**:
| 类型 | 数量 | 示例 |
|------|------|------|
| 温湿度 | 8 | SHT30, SHT40, AHT20, HDC1080, DHT22 |
| IMU/加速度 | 6 | MPU6050, BMI088, ICM20948, LSM6DSO |
| 气压/高度 | 4 | BMP280, BMP390, DPS368 |
| 光照/颜色 | 6 | BH1750, TCS34725, VCNL4040 |
| 气体传感 | 8 | SGP30, SGP40, MQ135, MQ3 |
| GPS | 1 | AT6558, LC86L, UBLOX |
| 电流/功率 | 5 | INA219, INA226, ACS712 |
| 角度编码 | 2 | AS5600, MA730 |
| 其他 | 20+ | 距离、颜色、紫外线等 |

---

### Actuator 执行器框架

**目录**: `components/actuator/`  
**状态**: 🟢 稳定

**功能**:
- [x] 继电器控制 (开/关/翻转/脉冲)
- [x] 舵机控制 (角度/速度/扫描/归中)
- [x] PWM 输出
- [x] 急停接口
- [x] 批量操作 (全部关闭)

---

### SMBus/PMBus 协议栈

**目录**: `components/smbus/`  
**状态**: 🟢 稳定

**功能**:
- [x] SMBus 字节/字/块读写
- [x] PEC (Packet Error Code) 校验
- [x] 总线扫描
- [x] PMBus 电源管理命令
- [x] Linear/VID/Direct 格式转换
- [x] 状态字解析

---

### HAL 硬件抽象层

**目录**: `components/hal/`  
**状态**: 🟢 稳定

**功能**:
- [x] GPIO, UART, SPI, I2C
- [x] Timer, PWM, RTC, DMA
- [x] ADC, DAC, Flash
- [x] Watchdog, RNG, EXTI
- [x] I2S, CAN

---

### OSAL OS 抽象层

**目录**: `components/kernel/osal/`  
**状态**: 🟢 稳定

**功能**:
- [x] Bare-metal 后端
- [x] FreeRTOS 后端
- [x] RT-Thread 后端
- [x] CMSIS-RTX 配置
- [x] 软件定时器

---

## 构建状态

### 推荐构建目录

| 用途 | 目录 | 说明 |
|------|------|------|
| 开发调试 | `build/` | PC 平台快速验证 |
| 功能测试 | `build_full_test/` | 完整功能测试 |
| STM32F4 | `build_stm32f4_test/` | F4 芯片测试 |
| STM32U5 | `build_stm32u5_validation/` | U5 芯片验证 |

### 构建命令

```bash
# PC 平台
cd XinYi
rm -rf build && mkdir build && cd build
cmake .. -DPLATFORM_PC=ON -DXY_CONFIG_SENSOR_ENABLED=ON \
         -DXY_CONFIG_ACTUATOR_ENABLED=ON -DXY_CONFIG_SMBUS_ENABLED=ON
make -j4
```

### 静态库

| 组件 | 库文件 | 大小 |
|------|--------|------|
| Sensor | `libsensor_component.a` | 311 KB |
| Actuator | `libxy_actuator.a` | 14.6 KB |
| SMBus | `libxy_smbus.a` | 28.5 KB |

---

## 更新记录

| 日期 | 更新内容 |
|------|----------|
| 2026-03-30 | 新增 GPS 传感器驱动 |
| 2026-03-30 | 新增 Actuator 执行器框架 |
| 2026-03-30 | 新增 SMBus/PMBus 协议栈 |
| 2026-03-30 | 修复 60+ 传感器驱动的组织问题 |
| 2026-03-26 | 完成 XinYi 框架构建 (PC 平台) |
| 2026-02-28 | 初始版本, 228 个测试用例 |

---

**维护者**: ZeroZap Team  
**许可证**: Apache License 2.0