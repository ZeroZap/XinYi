# XinYi Framework - 快速入门指南

**最后更新**: 2026-03-30

---

## 🚀 5 分钟快速开始

### 1. 克隆项目

```bash
git clone https://github.com/ZeroZap/XinYi.git
cd XinYi
```

### 2. PC 平台构建

```bash
cd XinYi
rm -rf build && mkdir build && cd build
cmake .. -DPLATFORM_PC=ON -DXY_CONFIG_SENSOR_ENABLED=ON \
         -DXY_CONFIG_ACTUATOR_ENABLED=ON -DXY_CONFIG_SMBUS_ENABLED=ON
make -j4
```

### 3. 查看所有目标

```bash
make help
```

---

## 📁 项目结构

```
XinYi/
├── components/           # 核心组件
│   ├── sensor/         # 传感器框架 (60+ 驱动)
│   │   ├── sensors/    # 驱动文件
│   │   └── examples/   # 示例代码
│   ├── actuator/       # 执行器框架
│   ├── smbus/          # SMBus/PMBus 协议
│   ├── hal/           # 硬件抽象层
│   ├── osal/          # OS 抽象层
│   ├── trace/         # 日志/命令
│   ├── net/           # 网络协议
│   ├── crypto/        # 密码学
│   └── dm/            # 数据管理
├── bsp/                # 板级支持包
├── docs/               # 文档
│   └── BUILD_GUIDE.md # 构建指南
├── examples/           # 示例代码
└── CMakeLists.txt      # 构建系统
```

---

## 🛠️ 常用命令

### 构建

```bash
# PC 平台完整构建
rm -rf build && mkdir build && cd build
cmake .. -DPLATFORM_PC=ON -DXY_CONFIG_SENSOR_ENABLED=ON \
         -DXY_CONFIG_ACTUATOR_ENABLED=ON -DXY_CONFIG_SMBUS_ENABLED=ON
make -j4

# 构建单个目标
make xy_sensor
make xy_actuator
make xy_smbus
```

### 清理

```bash
# 删除构建目录
rm -rf build

# 删除所有 build_* 目录
for d in build_*; do [ -d "$d" ] && rm -rf "$d"; done
```

---

## 📖 文档导航

| 文档 | 说明 |
|------|------|
| [README.md](README.md) | 主文档 (更新: 2026-03-30) |
| [COMPONENTS_STATUS.md](COMPONENTS_STATUS.md) | 组件状态 (更新: 2026-03-30) |
| [docs/BUILD_GUIDE.md](docs/BUILD_GUIDE.md) | 构建指南 |

---

## 📊 组件一览

| 组件 | 状态 | 说明 |
|------|------|------|
| **Sensor** | 🟢 稳定 | 60+ 传感器驱动 |
| **Actuator** | 🟢 稳定 | 舵机/继电器/PWM |
| **SMBus/PMBus** | 🟢 稳定 | 电源管理总线 |
| **HAL** | 🟢 稳定 | 硬件抽象层 |
| **OSAL** | 🟢 稳定 | OS 抽象层 |
| **Net** | 🟢 稳定 | MQTT/Modbus/AT |
| **Crypto** | 🟢 稳定 | AES/CRC/SHA |
| **GUI** | 🟡 开发中 | 图形界面 |
| **FOTA** | 🟡 开发中 | 固件升级 |

---

## 🔧 配置选项

### CMake 选项

| 选项 | 说明 | 默认值 |
|------|------|--------|
| `PLATFORM_PC` | PC 平台构建 | OFF |
| `PLATFORM_STM32F4` | STM32F4 平台 | OFF |
| `PLATFORM_STM32U5` | STM32U5 平台 | OFF |
| `XY_CONFIG_SENSOR_ENABLED` | 启用传感器 | ON |
| `XY_CONFIG_ACTUATOR_ENABLED` | 启用执行器 | ON |
| `XY_CONFIG_SMBUS_ENABLED` | 启用 SMBus | ON |

### 推荐构建配置

```bash
# PC 平台完整功能
cmake .. -DPLATFORM_PC=ON \
         -DXY_CONFIG_SENSOR_ENABLED=ON \
         -DXY_CONFIG_ACTUATOR_ENABLED=ON \
         -DXY_CONFIG_SMBUS_ENABLED=ON

# STM32F4 平台
cmake .. -DPLATFORM_STM32F4=ON -DXY_HAL_STM32=ON
```

---

## 🐛 遇到问题？

1. **构建失败**: 检查 CMake 版本 (需要 3.10+)
2. **找不到库**: 确认组件已启用 (`XY_CONFIG_*_ENABLED=ON`)
3. **查看 BUILD_GUIDE.md**: `cat docs/BUILD_GUIDE.md`

---

## 📞 获取帮助

- 查看 [docs/BUILD_GUIDE.md](docs/BUILD_GUIDE.md) - 构建指南
- 查看 [COMPONENTS_STATUS.md](COMPONENTS_STATUS.md) - 组件状态
- 提交 Issue: https://github.com/ZeroZap/XinYi/issues

---

**Happy Coding!** ⚡