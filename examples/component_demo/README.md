# XinYi 组件演示示例

**说明**: 综合演示 XinYi 各核心组件的使用方法

**运行平台**: PC (Linux/macOS/Windows)

---

## 🚀 快速开始

### 编译

```bash
cd examples/component_demo
mkdir -p build && cd build
cmake .. -DPLATFORM=PC
make
```

### 运行

```bash
./component_demo
```

---

## 📋 演示内容

| 组件 | 功能 | 说明 |
|------|------|------|
| **OSAL** | 任务/信号量/队列 | 多任务调度演示 |
| **HAL** | GPIO/UART 模拟 | PC 仿真层演示 |
| **Device** | 设备注册表 | 设备管理演示 |
| **Sensor** | SHT30/MPU6050 | 传感器读取模拟 |
| **Crypto** | AES/SHA256/CRC | 加密算法演示 |
| **Log** | 日志系统 | 分级日志输出 |

---

## 📊 预期输出

```
=================================================
  XinYi Component Demo
  Version: 1.0.0
=================================================

[INFO] System initialized
[INFO] OSAL backend: PC Simulator
[INFO] Device registry initialized (max 32 devices)

--- OSAL Demo ---
[INFO] Creating tasks...
[DEBUG] Task 'worker_1' created (priority=3)
[DEBUG] Task 'worker_2' created (priority=3)
[INFO] Tasks running...

--- HAL Demo ---
[DEBUG] GPIO PA5 set to 1
[DEBUG] GPIO PA5 set to 0
[INFO] UART TX: "Hello, XinYi!"

--- Device Demo ---
[INFO] Registering devices...
[DEBUG] Device 'sht30_1' registered (I2C)
[DEBUG] Device 'mpu6050_1' registered (I2C)
[INFO] Device count: 2
[INFO] Device list:
  sht30_1      I2C          RefCnt=0   PM=ACTIVE
  mpu6050_1    I2C          RefCnt=0   PM=ACTIVE

--- Sensor Demo ---
[DEBUG] SHT30: Temperature=25.50°C, Humidity=50.00%RH
[DEBUG] MPU6050: Accel=(0.00, 0.00, 1.00)g

--- Crypto Demo ---
[DEBUG] AES-128 encrypt: "Hello" -> [encrypted]
[DEBUG] SHA256: "XinYi" -> 5a1f2b3c...
[DEBUG] CRC16: 0xABCD

--- Power Management Demo ---
[DEBUG] Putting 'sht30_1' to sleep...
[DEBUG] Device 'sht30_1' entered SLEEP mode
[DEBUG] Waking up 'sht30_1'...
[DEBUG] Device 'sht30_1' woke up

=================================================
  Demo completed successfully!
=================================================
```

---

## 🔧 配置选项

### CMake 选项

```bash
# 启用详细日志
cmake .. -DLOG_LEVEL=DEBUG

# 禁用特定组件演示
cmake .. -DDEMO_CRYPTO=OFF
cmake .. -DDEMO_SENSOR=OFF
```

---

## 📁 文件结构

```
component_demo/
├── README.md           # 本文件
├── CMakeLists.txt      # 构建配置
├── main.c              # 主程序
├── demo_osal.c         # OSAL 演示
├── demo_hal.c          # HAL 演示
├── demo_device.c       # Device 演示
├── demo_sensor.c       # Sensor 演示
└── demo_crypto.c       # Crypto 演示
```

---

## 🎯 学习目标

完成本示例后，你将了解:

- ✅ OSAL 基本 API 使用
- ✅ HAL 接口调用方法
- ✅ 设备注册表管理
- ✅ 传感器驱动使用
- ✅ 加密算法调用
- ✅ 电源管理框架

---

## 📚 相关文档

- [OSAL 使用指南](../../docs/components/kernel/osal/README.md)
- [HAL 移植指南](../../docs/components/hal/PORTING_GUIDE.md)
- [设备框架设计](../../docs/components/device/DEVICE_ARCHITECTURE.md)
- [快速入门](../../docs/getting-started/QUICK_START.md)

---

*XinYi - 为嵌入式而生* ⚡
