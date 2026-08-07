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
# PC 平台完整构建（默认本地构建）
make

# 等价 CMake 命令：HAL_PLATFORM 取值为 PC/STM32F4/STM32U5/WCH/HC32
cmake -B build/pc -S . -DHAL_PLATFORM=PC -DCMAKE_BUILD_TYPE=Release
cmake --build build/pc -j"$(nproc)"

# 运行 PC 单元测试
make test-unit

# 构建单个组件目标（先完成对应配置）
make configure
cmake --build build/pc --target xy_sensor -j"$(nproc)"
cmake --build build/pc --target xy_actuator -j"$(nproc)"
cmake --build build/pc --target xy_net -j"$(nproc)"
```

### 清理

```bash
# 使用仓库维护的清理入口删除生成目录/配置缓存
make distclean
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

### CMake / Kconfig 选项

当前构建入口使用统一平台变量 `HAL_PLATFORM`，可选值为 `PC`、`STM32F4`、`STM32U5`、`WCH`、`HC32`。组件开关由根 `Kconfig` 生成到 `.config`、`autoconf.h` 和 `config.cmake`，不要再使用旧的 `PLATFORM_PC` / `XY_CONFIG_*_ENABLED` 命令行示例作为事实源。

| 选项 | 说明 | 默认值 |
|------|------|--------|
| `HAL_PLATFORM=PC` | PC 仿真平台 | 默认 |
| `HAL_PLATFORM=STM32U5` | 主 MCU 平台 | 手动选择 |
| `BUILD_TESTS=ON` | 目标平台测试构建 | OFF |
| `FOTA=ON` | FOTA 构建变体 | OFF |

### 推荐构建配置

```bash
# PC 平台默认 Release 构建
make

# STM32U5 平台
make HAL_PLATFORM=STM32U5
```

---

## 🐛 遇到问题？

1. **构建失败**: 先以根 `Makefile` 为准运行 `make` 或 `make test-unit`，不要套用旧的 `-DPLATFORM_*` 示例
2. **找不到库**: 先确认根 `Kconfig` 组件符号和生成的 `build/<platform>/config.cmake` 是否启用
3. **查看 BUILD_GUIDE.md**: `less docs/BUILD_GUIDE.md`

---

## 📞 获取帮助

- 查看 [docs/BUILD_GUIDE.md](docs/BUILD_GUIDE.md) - 构建指南
- 查看 [COMPONENTS_STATUS.md](COMPONENTS_STATUS.md) - 组件状态
- 提交 Issue: https://github.com/ZeroZap/XinYi/issues

---

**Happy Coding!** ⚡