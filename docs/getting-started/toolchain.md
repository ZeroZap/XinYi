# XinYi 工具链配置指南

**版本**: 1.0.0  
**最后更新**: 2026-03-16  
**维护者**: XinYi Team

---

## 📋 概述

XinYi 支持多种工具链和构建系统，覆盖从 STM32、WCH、HC32 到 PC 仿真的完整开发流程。

### 支持的编译器

| 编译器 | 版本 | 平台 | 必需 | 下载 |
|--------|------|------|------|------|
| **ARM GCC** | 9-2020-q4 | STM32/ARM | ✅ | [ARM 官网](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm) |
| **WCH Toolchain** | 9.3.1 | WCH CH32 | ✅ (WCH) | [沁恒官网](https://www.wch.cn/) |
| **HC32 Toolchain** | 9.3.1 | HC32 | ✅ (HC32) | [小华官网](https://www.hcsemi.com/) |
| **GCC** | 9.0+ | Linux/PC | ✅ (PC) | `sudo apt install gcc` |
| **Clang** | 10.0+ | macOS/Linux | ⚠️ | `brew install llvm` |
| **IAR** | 8.0+ | STM32 | ❌ | [IAR 官网](https://www.iar.com/) |
| **Keil** | 5.0+ | STM32 | ❌ | [Keil 官网](https://www.keil.com/) |

---

## 🔧 ARM GCC 工具链配置

### Ubuntu/Debian 安装

```bash
# 方法 1: apt 安装 (版本可能较旧)
sudo apt update
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi

# 方法 2: 官方最新版本 (推荐)
wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/9-2020q4/gcc-arm-none-eabi-9-2020-q4-major-x86_64-linux.tar.bz2
tar xjf gcc-arm-none-eabi-9-2020-q4-major-x86_64-linux.tar.bz2
sudo mv gcc-arm-none-eabi-9-2020-q4-major /opt/gcc-arm-none-eabi

# 添加到 PATH
echo 'export PATH="/opt/gcc-arm-none-eabi/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

### Windows 安装

1. 下载官方安装包
2. 运行安装程序
3. 添加到系统 PATH: `C:\Program Files (x86)\GNU Tools Arm Embedded\9 2020-q4-major\bin`

### macOS 安装

```bash
brew install --cask gcc-arm-embedded
```

### 验证安装

```bash
arm-none-eabi-gcc --version
# 输出：arm-none-eabi-gcc (GNU Arm Embedded Toolchain 9-2020-q4-major) 9.3.1

arm-none-eabi-gcc -v
# 查看完整配置信息
```

---

## 🔧 WCH 工具链配置

### 位置
```
/usr/share/MRS2/MRS-linux-x64/resources/app/resources/linux/components/WCH/Toolchain/
```

### 环境配置

```bash
export ARM_TOOLCHAIN_ROOT="/usr/share/MRS2/MRS-linux-x64/resources/app/resources/linux/components/WCH/Toolchain/arm-none-eabi-gcc"
export PATH="$ARM_TOOLCHAIN_ROOT/bin:$PATH"
```

### 快速加载脚本

```bash
# 使用项目提供的脚本
source /home/eugene/zerozap/scripts/env.sh

# 或手动配置
source /path/to/XinYi/scripts/wch_env.sh
```

### 验证安装

```bash
arm-none-eabi-gcc --version
# 输出：arm-none-eabi-gcc (GNU Arm Embedded Toolchain 9-2020-q4-major) 9.3.1
```

---

## 🔧 HC32 工具链配置

### 安装步骤

1. 下载 HC32 工具链
2. 解压到 `/opt/hc32-toolchain`
3. 配置环境变量

```bash
export HC32_TOOLCHAIN_ROOT="/opt/hc32-toolchain/gcc"
export PATH="$HC32_TOOLCHAIN_ROOT/bin:$PATH"
```

### 验证安装

```bash
arm-none-eabi-gcc --version
```

---

## 🏗️ 构建系统

### CMake (推荐)

**优势**:
- ✅ 跨平台支持
- ✅ 自动生成 Makefile
- ✅ 依赖管理清晰
- ✅ IDE 集成友好

#### 基本用法

```bash
# 创建构建目录
mkdir build && cd build

# 配置 (STM32U5)
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-gcc.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON

# 编译
make -j$(nproc)

# 运行测试
make test

# 清理
make clean
```

#### CMake 选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `CMAKE_BUILD_TYPE` | Release | 构建类型 (Debug/Release/MinSizeRel) |
| `BUILD_TESTING` | OFF | 构建测试用例 |
| `TEST_COVERAGE` | OFF | 启用代码覆盖率 |
| `HAL_PLATFORM` | PC | HAL 平台 (PC/STM32F4/STM32U5/STM32L4/WCH/HC32) |
| `OSAL_BACKEND` | baremetal | 由 Kconfig 推导 (baremetal/freertos)；直接传值会被根 CMake 拒绝 |

#### 平台特定配置

交叉编译器由 `cmake/platform/<HAL_PLATFORM>.cmake` 选择，所以平台就是唯一开关；不需要
（也不应该）再传 `CMAKE_TOOLCHAIN_FILE`。

```bash
# STM32U5
make HAL_PLATFORM=STM32U5

# STM32L4（Pandora STM32L475 参考板冒烟镜像）
make HAL_PLATFORM=STM32L4 STM32L4_BOARD=pandora_stm32l475

# STM32F4
make HAL_PLATFORM=STM32F4

# WCH CH32V307
make HAL_PLATFORM=WCH

# HC32
make HAL_PLATFORM=HC32

# PC 仿真（默认）
make
```

### Make (传统)

**优势**:
- ✅ 简单直接
- ✅ 无需额外工具
- ✅ 适合小项目

```bash
# 编译（配置当前目录里已启用的全部内容）
make all

# 清理
make clean

# 测试
make test

# 烧录：root Makefile 没有 flash 目标，走带快照门禁的工具
python3 tools/pandora_hil_program.py run --image <image.bin> --address 0x08000000 \
        --snapshot <snapshot> --yes --reset

# 配置：没有 menuconfig 目标，取值在 configure 步给出
make configure KCONFIG_OVERRIDES="COMPONENT_CRYPTO=ON"
```

`make help` 是权威列表：wrapper 接受的全部目标与变量都在那里，本文提到的其余名字都不在
root Makefile 里。

---

## ⚙️ Kconfig 配置系统

### 取值方式

Kconfig 没有交互式界面：`cmake/kconfig_parser.py` 在 configure 时解析一次 root `Kconfig`，
生成 `.config` 与 `config.cmake`。取值只有一条路——在 configure 步给出覆盖：

```bash
# 打开可选组件（多个符号用 ';' 分隔）
make configure KCONFIG_OVERRIDES="COMPONENT_CRYPTO=ON;FOTA_ENABLED=ON"

# 换个后端（只对声明了该符号的 Kconfig 有效，默认 baremetal）
make HAL_PLATFORM=STM32U5 configure KCONFIG_OVERRIDES="OSAL_BACKEND_FREERTOS=ON"
```

覆盖只在 configure 时生效：已有构建目录的缓存不会被后续 `make` 重新生成，需要换配置时
重新执行一次 `make configure`（或换一个 `BUILD_DIR`）。

### 配置项分组

root `Kconfig` 的菜单结构（共 81 个 `config` 条目）：

```
XinYi Framework Configuration
├─ Platform Selection        PLATFORM_*
├─ HAL Configuration         HAL_*
├─ Core Components           COMPONENT_DEVICE / CRYPTO / DM / SENSOR / ACTUATOR / CHARGER
├─ Kernel Configuration      KERNEL_OSAL, OSAL_BACKEND_FREERTOS / BAREMETAL
├─ Device Drivers            DRIVER_SENSOR / DISPLAY(+OLED/LCD/LED) / STORAGE / POWER / WIRELESS / SYSTEM
├─ GUI Subsystem             GUI_ENABLED / SDL / TFT / LVGL / WIDGETS
└─ Communication Protocols   NETWORK / ...
```

### 命令行配置

`make CONFIG_XXX=y` 只是把 `CONFIG_XXX=y` 当成一个与目标无关的 make 变量，什么都不会打开；
root Kconfig 里也**没有** `CONFIG_CRYPTO_AES` / `CONFIG_NETWORK_MQTT` / `CONFIG_SENSOR_DHT11` /
`CONFIG_GUI` 这些符号。真实开关是 Kconfig 里的 `config` 名：

```bash
# 打开一个组件（名字取自 root Kconfig，例如 COMPONENT_CRYPTO / COMPONENT_SENSOR）
make configure KCONFIG_OVERRIDES="COMPONENT_CRYPTO=ON"

# 关掉一个（默认已经是 n 的符号写 n 是幂等的）
make configure KCONFIG_OVERRIDES="GUI_ENABLED=n"
```

---

## 🔌 IDE 集成

### VSCode

#### 推荐扩展
- C/C++ (Microsoft)
- CMake Tools (Microsoft)
- Cortex-Debug (ms-iot)
- PlatformIO (可选)

#### 配置示例 (.vscode/c_cpp_properties.json)

```json
{
    "configurations": [
        {
            "name": "STM32U5",
            "compilerPath": "/opt/gcc-arm-none-eabi/bin/arm-none-eabi-gcc",
            "cStandard": "c11",
            "cppStandard": "c++17",
            "intelliSenseMode": "gcc-arm",
            "includePath": [
                "${workspaceFolder}/components/**",
                "${workspaceFolder}/hal/**",
                "/opt/gcc-arm-none-eabi/arm-none-eabi/include"
            ],
            "defines": [
                "STM32U5",
                "USE_HAL_DRIVER",
                "__ARM_ARCH=7"
            ]
        }
    ]
}
```

### CLion

1. 打开 `CMakeLists.txt`
2. 配置工具链：`File → Settings → Build → Toolchains`
3. 添加 ARM GCC 路径

### Keil MDK

1. 导入 CMake 项目
2. 选择 STM32U5 设备
3. 配置编译器路径

---

## 📦 依赖管理

### Git Submodules

```bash
# 初始化子模块
git submodule update --init --recursive

# 更新子模块
git submodule update --remote
```

### 子模块列表

| 子模块 | 路径 | 说明 |
|--------|------|------|
| CMSIS | `deps/CMSIS` | ARM CMSIS 库 |
| FreeRTOS | `deps/FreeRTOS` | FreeRTOS 源码 |
| RT-Thread | `deps/rt-thread` | RT-Thread 源码 |

---

## 🚀 烧录工具

### ST-Link (STM32)

```bash
# 安装
sudo apt install stlink-tools

# 烧录
st-flash write build/firmware.bin 0x8000000

# 或使用 OpenOCD
openocd -f interface/stlink.cfg -f target/stm32u5.cfg -c "program build/firmware.bin verify reset exit"
```

### WCH-Link (WCH)

```bash
# 使用 WCH 官方工具
WCH-LinkRV -t -f build/firmware.bin

# 或使用 OpenOCD (需要 WCH 支持)
openocd -f interface/wch-link.cfg -f target/ch32v307.cfg
```

### J-Link (通用)

```bash
# 使用 J-Flash
JFlash -openprj project.jflash -open build/firmware.bin,0x8000000 -auto

# 或使用 JLinkExe
JLinkExe -device STM32U575ZI -if SWD -speed 4000 -auto connect loadfile build/firmware.bin reset exit
```

---

## 🔍 故障排查

### 常见问题

#### 1. "arm-none-eabi-gcc: command not found"

**解决**:
```bash
# 检查 PATH
echo $PATH | grep arm

# 重新添加 PATH
export PATH="/opt/gcc-arm-none-eabi/bin:$PATH"

# 永久添加
echo 'export PATH="/opt/gcc-arm-none-eabi/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

#### 2. "Cannot find -lstdc++"

**解决**:
```bash
# 安装 32 位库 (Ubuntu)
sudo apt install lib32stdc++6
```

#### 3. CMake 找不到工具链

**解决**:
```bash
# 使用绝对路径
cmake .. -DCMAKE_TOOLCHAIN_FILE=/absolute/path/to/arm-gcc.cmake
```

#### 4. 烧录失败 "No ST-Link detected"

**解决**:
```bash
# 检查连接
lsusb | grep ST-LINK

# 添加 udev 规则
echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="0483", MODE="0666"' | sudo tee /etc/udev/rules.d/49-stlink.rules
sudo udevadm control --reload-rules
```

---

## 📚 相关文档

- [快速入门](QUICK_START.md)
- [开发者指南](DEVELOPER_GUIDE.md)
- [构建系统分析](../toolchain/build_system_analysis.md)
- [CI/CD 指南](../toolchain/CI_CD_GUIDE.md)

---

## 🔗 外部资源

- [ARM GCC 官方文档](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/documentation)
- [CMake 官方文档](https://cmake.org/documentation/)
- [STM32CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0  
**最后更新**: 2026-03-16
