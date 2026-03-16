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
| `CMAKE_TOOLCHAIN_FILE` | - | 工具链文件路径 |
| `BUILD_TESTING` | ON | 构建测试用例 |
| `TEST_COVERAGE` | OFF | 启用代码覆盖率 |
| `HAL_PLATFORM` | STM32 | HAL 平台 (STM32/WCH/HC32/PC) |
| `OSAL_BACKEND` | FreeRTOS | OSAL 后端 (FreeRTOS/RT-Thread/RTX/Baremetal) |

#### 平台特定配置

```bash
# STM32U5
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-gcc.cmake \
         -DHAL_PLATFORM=STM32

# WCH CH32U5
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/wch-gcc.cmake \
         -DHAL_PLATFORM=WCH

# HC32
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/hc32-gcc.cmake \
         -DHAL_PLATFORM=HC32

# PC 仿真
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/native-gcc.cmake \
         -DHAL_PLATFORM=PC
```

### Make (传统)

**优势**:
- ✅ 简单直接
- ✅ 无需额外工具
- ✅ 适合小项目

```bash
# 编译
make all

# 清理
make clean

# 测试
make test

# 烧录
make flash

# 配置
make menuconfig
```

---

## ⚙️ Kconfig 配置系统

### 交互式配置

```bash
make menuconfig
```

### 配置选项示例

```
XinYi Configuration
├─ HAL Platform Selection
│  ├─ STM32 (STM32F4/STM32U5/STM32H7)
│  ├─ WCH (CH32V307/CH32U5)
│  ├─ HC32 (HC32F460)
│  └─ PC Simulator
│
├─ OSAL Backend Selection
│  ├─ FreeRTOS
│  ├─ RT-Thread
│  ├─ CMSIS-RTX
│  └─ Bare-metal
│
├─ Component Selection
│  ├─ Crypto (AES/SHA/CRC)
│  ├─ Network (MQTT/Modbus)
│  ├─ Sensor (DHT11/SHT30)
│  └─ GUI (Basic/Advanced)
│
└─ Debug Options
   ├─ Enable Logging
   ├─ Enable Asserts
   └─ Enable Stack Check
```

### 命令行配置

```bash
# 启用特定功能
make CONFIG_CRYPTO_AES=y
make CONFIG_NETWORK_MQTT=y
make CONFIG_SENSOR_DHT11=y

# 禁用功能
make CONFIG_GUI=n
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
