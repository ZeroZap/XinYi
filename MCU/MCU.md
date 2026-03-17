## 简介
这是一个关于底层支持的官方 MCU SDK 子仓库的集合

## MCU SDK 列表

### ST (STMicroelectronics)
| 系列 | 子模块 | 代表型号 | QEMU 支持 |
|------|--------|---------|----------|
| STM32F1 | `STM32F1/` | STM32F100, STM32F103 | ✅ stm32vldiscovery |
| STM32F4 | `STM32F4/` | STM32F405, STM32F407 | ✅ olimex-stm32-h405 |
| STM32U5 | `STM32U5/` | STM32U575, STM32U585 | ❌ (需实际硬件) |

### TI (Texas Instruments)
| 系列 | 子模块 | 代表型号 | QEMU 支持 |
|------|--------|---------|----------|
| LM3S | `TI/LM3S/` | LM3S6965 | ✅ lm3s6965evb |

### WCH (沁恒)
| 系列 | 子模块 | 代表型号 | QEMU 支持 |
|------|--------|---------|----------|
| CH32 | `wch/` | CH32V307, CH32F20x | ❌ (需实际硬件) |

### HC (小华)
| 系列 | 子模块 | 代表型号 | QEMU 支持 |
|------|--------|---------|----------|
| HC32 | _待添加_ | HC32L021, HC32F460 | ❌ (需实际硬件) |

---

## 📁 目录结构

```
MCU/
├── MCU.md                  # 本文档
├── ST/                     # STMicroelectronics
│   ├── STM32F1/           # STM32CubeF1 官方 SDK
│   ├── STM32F4/           # STM32CubeF4 官方 SDK
│   └── STM32U5/           # STM32CubeU5 官方 SDK
├── TI/                     # Texas Instruments
│   └── LM3S/              # LM3S6965 QEMU 支持 (自建 HAL)
└── wch/                   # 沁恒半导体
```

---

## 🔧 使用方式

### 作为 Git 子模块引用

```bash
cd XinYi/MCU/ST

# STM32F1 系列
git submodule add https://github.com/STMicroelectronics/STM32CubeF1.git STM32F1

# STM32F4 系列
git submodule add https://github.com/STMicroelectronics/STM32CubeF4.git STM32F4
```

### 在 CMake 中引用

```cmake
# STM32F1 HAL
include_directories(${XINYI_ROOT}/MCU/ST/STM32F1/Drivers/STM32F1xx_HAL_Driver/Inc)
include_directories(${XINYI_ROOT}/MCU/ST/STM32F1/Drivers/CMSIS/Device/ST/STM32F1xx/Include)

# STM32F4 HAL
include_directories(${XINYI_ROOT}/MCU/ST/STM32F4/Drivers/STM32F4xx_HAL_Driver/Inc)
include_directories(${XINYI_ROOT}/MCU/ST/STM32F4/Drivers/CMSIS/Device/ST/STM32F4xx/Include)
```

---

## 🚀 QEMU 验证

| QEMU 开发板 | MCU | SDK | 测试状态 |
|-----------|-----|-----|---------|
| `lm3s6965evb` | LM3S6965 | XinYi HAL | ✅ 已验证 |
| `stm32vldiscovery` | STM32F100 | STM32CubeF1 | ✅ 已验证 |
| `olimex-stm32-h405` | STM32F405 | STM32CubeF4 | ✅ 已验证 |

---

## 🧪 测试工程

### STM32F1 - stm32vldiscovery

**位置**: `MCU/ST/STM32F1/QEMU/stm32vldiscovery_test/`

**测试内容**:
- ✅ GPIO 输出 (LED 闪烁 - PC8/PC9)
- ✅ ARM 半主机调试输出
- ✅ 向量表和启动代码

**运行**:
```bash
cd MCU/ST/STM32F1/QEMU/stm32vldiscovery_test
qemu-system-arm -M stm32vldiscovery -nographic -kernel stm32vldiscovery.elf -semihosting
```

### STM32F4 - olimex-stm32-h405

**位置**: `MCU/ST/STM32F4/QEMU/olimex_stm32_h405_test/`

**测试内容**:
- ✅ GPIO 输出 (LED 闪烁 - PC13)
- ✅ ARM 半主机调试输出
- ✅ Cortex-M4F FPU 配置

**运行**:
```bash
cd MCU/ST/STM32F4/QEMU/olimex_stm32_h405_test
qemu-system-arm -M olimex-stm32-h405 -nographic -kernel olimex_stm32_h405.elf -semihosting
```

### TI LM3S - lm3s6965evb

**位置**: `MCU/TI/LM3S/`

**测试内容**:
- ✅ GPIO 输出
- ✅ 半主机输出

---

**最后更新**: 2026-03-17

---

**最后更新**: 2026-03-17