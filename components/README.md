# XinYi Components

**版本**: 2.0.0
**日期**: 2026-05-01
**状态**: Host-guarded development baseline；各组件等级见
[`docs/validation/component-evidence-matrix.md`](../docs/validation/component-evidence-matrix.md)

---

## 🏗️ 组件架构

```
components/
├── hal/              # 硬件抽象层 (MCU 外设)
├── kernel/           # 内核/OSAL
├── device/           # 设备框架层
├── drivers/          # 器件驱动层 (按类别组织)
├── sensor/          # 传感器组件
├── actuator/        # 执行器组件
├── power/           # 电源管理组件
├── gui/             # 图形界面组件
├── net/             # 网络组件
├── crypto/          # 加密组件
├── dm/              # 数据管理组件
└── ...
```

### 分层说明

| 层次 | 目录 | 说明 |
|------|------|------|
| **硬件抽象层** | `hal/` | MCU 外设驱动 (UART/SPI/I2C/GPIO) |
| **设备框架层** | `device/` | 统一设备管理框架 |
| **器件驱动层** | `drivers/` | 外设器件驱动 (传感器/显示/存储) |
| **组件层** | `sensor/` 等 | 高级 API 和业务逻辑 |

---

## 📁 Drivers 目录结构

```
components/drivers/
├── sensor/              # 传感器驱动
│   ├── temperature/     # 温湿度传感器
│   │   ├── sht30/       # SHT30/SHT40
│   │   └── dht11/       # DHT11
│   ├── motion/          # 运动传感器
│   │   └── mpu6050/     # MPU6050
│   ├── pressure/        # 气压传感器
│   │   └── bmp280/      # BMP280
│   ├── adc/             # ADC 芯片
│   │   └── ads1115/     # ADS1115
│   ├── light/           # 光线传感器
│   └── ...
├── display/             # 显示驱动
│   ├── oled/            # OLED 显示
│   │   └── ssd1306/     # SSD1306
│   ├── lcd/            # LCD 显示
│   ├── led/            # LED 驱动
│   └── rgb/            # RGB LED (WS2812)
├── storage/            # 存储驱动
│   ├── eeprom/         # EEPROM 24xx 系列
│   ├── flash/          # W25Qxx Flash
│   └── sdcard/         # SDCard
├── power/              # 电源管理驱动
│   └── bq25620/        # BQ25620 充电器
├── wireless/           # 无线驱动
│   └── rc522/          # RC522 RFID
└── system/            # 系统驱动
    ├── key/            # 按键
    ├── rtc/            # RTC
    └── watchdog/       # 看门狗
```

---

## 🔌 驱动清单与证据边界

> 下表的 `✅` 仅表示源码/目录存在，不代表进入 root product target，也不代表硬件通过。
> Active owner、root source selection 与验证等级以 root Kconfig/CMake 与组件证据台账为准。

### 传感器驱动 (Sensor Drivers)

| 型号 | 类型 | 接口 | 当前边界 |
|------|------|------|------|
| SHT30 | 温湿度 | I2C | Device-model canonical owners 之一；Host contract；实板 pending |
| MPU6050 | IMU | I2C | Device-model canonical owner；Host contract；实板 pending |
| BMP280 | 气压 | I2C | Device-model canonical owner；Host contract；实板 pending |
| ADS1115 | ADC | I2C | Device-model canonical owner；Host contract；实板 pending |
| 其他 Sensor source | 多类 | 多类 | legacy root 或 experimental test-only；见 active-source manifest |

### 显示驱动 (Display Drivers)

| 型号 | 类型 | 接口 | 当前边界 |
|------|------|------|------|
| SSD1306 | OLED | I2C | Host transaction/adapter contract；实板 pending |
| WS2812 | RGB LED | GPIO/SPI | Host transaction contract；实板 pending |

### 存储驱动 (Storage Drivers)

| 型号 | 类型 | 接口 | 当前边界 |
|------|------|------|------|
| 24xx | EEPROM | I2C | Host transaction/error contract；掉电与实板 pending |
| W25Qxx | Flash | SPI | Host contract；目标 Flash 时序/实板 pending |

### 电源驱动 (Power Drivers)

| 型号 | 类型 | 功能 | 当前边界 |
|------|------|------|------|
| BQ25620 | 充电器 | 配置/状态控制 | standalone legacy-maintained owner；Host contract；实板 pending |

---

## 📦 组件列表

### 核心组件

| 组件 | 说明 | 状态 |
|------|------|------|
| `kernel/` | OS 抽象层；Bare-metal Host contract，FreeRTOS `compile-guarded-runtime-pending` | H1/C1 前置 |
| `hal/` | 硬件抽象层（逐平台证据见矩阵） | Host/部分 QEMU |
| `device/` | 统一设备管理框架 | Host-guarded |
| `drivers/` | 器件驱动层（多数硬件证据 pending） | Host-guarded |

### 功能组件

| 组件 | 说明 | 状态 |
|------|------|------|
| `sensor/` | legacy compatibility + Device canonical owners | Host-guarded；实板 pending |
| `actuator/` | 执行器框架 | Host-guarded；GPIO/PWM 实板 pending |
| `charger/` | standalone 充电管理 | legacy-maintained；实板/安全 pending |
| `fuel_gauge/` | standalone 电量计 | Host-guarded；SMBus 实板 pending |
| `pid/` | PID 控制 | Host-guarded；plant/HIL pending |
| `fota/` | 固件升级 | Host fail-closed；bootloader/security/实板 pending |
| `gui/` | 图形界面 | Host-guarded；显示/视觉/性能 pending |
| `crypto/` | 加密库 | Host contract；security review pending |
| `dm/` | 数据管理 | Host-guarded；真实 Flash durability pending |
| `pm/` | 电源管理 | Host-guarded；功耗/唤醒实证 pending |

---

## 🔧 构建配置

### Kconfig 选项

不要从本页复制可能漂移的符号清单。使用 root `Kconfig` 与各组件 `Kconfig`，并通过
`KCONFIG_OVERRIDES` 显式选择；非法父子组合必须在配置阶段 fail-closed。

### CMake 构建

root `CMakeLists.txt` 负责组件发现和 Kconfig 选择。应用不应绕过 root selection 直接
`add_subdirectory()` 某个 driver 子目录；先通过正常配置生成 source/target，再构建所需 target。

---

## 📚 相关文档

- [ARCHITECTURE_REFACTORING_PLAN.md](ARCHITECTURE_REFACTORING_PLAN.md) - 架构重组方案
- [REFACTORING_SUMMARY.md](REFACTORING_SUMMARY.md) - 重组工作总结
- [COMPONENT_ARCHITECTURE.md](COMPONENT_ARCHITECTURE.md) - 组件架构分析
- [DISPLAY_ARCHITECTURE.md](DISPLAY_ARCHITECTURE.md) - 显示架构

---

## 🔍 与业界标准对比

| 标准 | 驱动组织方式 | XinYi 采用 |
|------|-------------|-----------|
| Zephyr | 按类型/厂商 | ✅ 类似 |
| Linux Kernel | 按类型 | ✅ 类似 |
| RT-Thread | 按类型/厂商 | ✅ 类似 |

---

**维护者**: XinYi Team
**许可证**: Apache License 2.0
