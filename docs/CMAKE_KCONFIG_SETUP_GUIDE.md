# CMake + Kconfig 配置系统部署指南

**版本**: 1.0  
**日期**: 2026-03-18  
**状态**: ✅ 已部署

---

## 📋 概述

XinYi Framework 采用 CMake + Kconfig 配置系统，类似 Zephyr/ESP-IDF 的现代化构建系统。

### 特性
- ✅ Kconfig 配置菜单
- ✅ CMake 自动检测组件
- ✅ 多平台支持 (PC/STM32/WCH/HC32)
- ✅ 条件编译
- ✅ 配置头文件生成

---

## 🏗️ 目录结构

```
XinYi/
├── CMakeLists.txt          # 顶层 CMake 配置
├── Kconfig                 # 顶层 Kconfig 配置
├── cmake/
│   ├── Kconfig.cmake       # Kconfig 集成脚本
│   ├── kconfig_parser.py   # Kconfig 解析器
│   ├── arm-gcc.cmake       # ARM GCC 工具链
│   └── platform/           # 平台配置
├── components/
│   ├── sensor/
│   │   ├── Kconfig         # 组件级 Kconfig
│   │   └── CMakeLists.txt
│   ├── crypto/
│   │   ├── Kconfig
│   │   └── CMakeLists.txt
│   └── ...
├── projects/
│   ├── Bank/
│   │   ├── Kconfig         # 项目级 Kconfig
│   │   └── CMakeLists.txt
│   └── ...
└── build/                  # 构建输出目录
```

---

## 🔧 快速开始

### 1. 基础构建 (PC 平台)

```bash
cd XinYi
mkdir build && cd build
cmake ..
make
```

### 2. 配置选项

```bash
mkdir build && cd build
cmake .. \
    -DCONFIG_PLATFORM_PC=y \
    -DCONFIG_BUILD_TESTING=y \
    -DCONFIG_COMPONENT_SENSOR=y
make
```

### 3. 嵌入式构建 (STM32U5)

```bash
mkdir build_stm32u5 && cd build_stm32u5
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-gcc.cmake \
    -DCONFIG_PLATFORM_STM32U5=y \
    -DCONFIG_HAL_STM32=y
make
```

---

## 📁 Kconfig 配置

### 顶层 Kconfig (Kconfig)

```kconfig
menu "XinYi Framework Configuration"

config XY_VERSION
    string
    default "1.0.0"

menu "Platform Selection"

config PLATFORM_PC
    bool "PC Simulation"
    default y if ARCH_X86_64

config PLATFORM_STM32
    bool "STM32 Series"
    
config PLATFORM_STM32U5
    bool "STM32U5 Series"
    depends on PLATFORM_STM32
    select CORTEX_M33

config PLATFORM_WCH
    bool "WCH CH32 Series"

config PLATFORM_HC32
    bool "Huada HC32 Series"

endmenu

menu "Core Components"

config COMPONENT_DEVICE
    bool "Device Framework"
    default y

config COMPONENT_SENSOR
    bool "Sensor Drivers"
    default y

config COMPONENT_CHARGER
    bool "Charger Management"
    default n

endmenu

endmenu
```

### 组件级 Kconfig (components/sensor/Kconfig)

```kconfig
menu "Sensor Drivers"

config SENSOR_DHT11
    bool "DHT11/DHT22 Temperature & Humidity"
    default y
    help
      DHT11/DHT22 digital temperature and humidity sensor.

config SENSOR_BME280
    bool "BME280 Environmental Sensor"
    default n
    help
      Bosch BME280 temperature, humidity, and pressure sensor.

config SENSOR_BMI270
    bool "BMI270 6-Axis IMU"
    default n
    help
      Bosch BMI270 accelerometer and gyroscope for wearables.

endmenu
```

---

## 🔨 CMake 集成

### 顶层 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.12)

# Kconfig Integration
if(EXISTS ${CMAKE_SOURCE_DIR}/Kconfig)
    include(${CMAKE_SOURCE_DIR}/cmake/Kconfig.cmake)
endif()

project(XY_Framework VERSION 1.0.0 LANGUAGES C)

# Build Options from Kconfig
if(DEFINED CONFIG_COMPONENT_SENSOR)
    set(BUILD_SENSOR ${CONFIG_COMPONENT_SENSOR})
endif()

# Auto-detect Components
file(GLOB COMPONENT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/components/*)
foreach(component_dir ${COMPONENT_DIRS})
    if(IS_DIRECTORY ${component_dir})
        if(EXISTS ${component_dir}/CMakeLists.txt)
            add_subdirectory(${component_dir})
        endif()
    endif()
endforeach()

# Testing
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
```

### Kconfig.cmake 集成脚本

```cmake
# Find Python for Kconfig parsing
find_package(Python3 COMPONENTS Interpreter)

# Parse Kconfig and generate config.h
if(Python3_Interpreter_FOUND)
    execute_process(
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_SOURCE_DIR}/cmake/kconfig_parser.py
                ${CMAKE_SOURCE_DIR}/Kconfig
                ${CMAKE_BINARY_DIR}/config.h
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif()

# Include generated config
include_directories(${CMAKE_BINARY_DIR})
```

---

## 📊 配置示例

### 项目级配置 (projects/Bank/Kconfig)

```kconfig
# Bank 项目特定配置

menu "Bank Project Configuration"

config BANK_DISPLAY_LCD
    bool "LCD Display"
    default y
    select COMPONENT_DISPLAY

config BANK_RTC_ENABLE
    bool "RTC Support"
    default y
    select COMPONENT_RTC

config BANK_CHARGER_BQ25620
    bool "BQ25620 Charger"
    default y
    select COMPONENT_CHARGER

config BANK_BATTERY_CAPACITY
    int "Battery Capacity (mAh)"
    default 2000
    range 500 10000

endmenu
```

---

## 🧪 构建验证

### 1. 检查配置

```bash
cd build
cmake .. -LH  # 列出所有配置选项
```

### 2. 查看生成的配置

```bash
cat build/config.h  # 查看生成的配置头文件
```

### 3. 构建测试

```bash
make VERBOSE=1  # 详细编译输出
ctest --output-on-failure  # 运行测试
```

---

## 🎯 平台配置

### PC 平台 (Linux/macOS/Windows)

```bash
mkdir build_pc && cd build_pc
cmake .. \
    -DCONFIG_PLATFORM_PC=y \
    -DCONFIG_BUILD_TESTING=y \
    -DCMAKE_BUILD_TYPE=Debug
make
```

### STM32U5 (Cortex-M33)

```bash
mkdir build_u5 && cd build_u5
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-gcc.cmake \
    -DCONFIG_PLATFORM_STM32U5=y \
    -DCONFIG_CORTEX_M33=y \
    -DCONFIG_FPU=y \
    -DCMAKE_BUILD_TYPE=Release
make
```

### HC32L021 (Cortex-M0+)

```bash
mkdir build_hc32 && cd build_hc32
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-gcc.cmake \
    -DCONFIG_PLATFORM_HC32=y \
    -DCONFIG_CORTEX_M0PLUS=y \
    -DCONFIG_HAL_HC32=y
make
```

---

## 📝 最佳实践

### 1. 配置分层

- **顶层 Kconfig**: 框架级配置
- **组件 Kconfig**: 组件级配置
- **项目 Kconfig**: 项目特定配置

### 2. 依赖管理

```kconfig
config COMPONENT_CHARGER
    bool "Charger Management"
    select COMPONENT_DEVICE      # 自动启用 Device 框架
    select COMPONENT_I2C         # 自动启用 I2C
    depends on !PLATFORM_PC      # PC 平台不可用
```

### 3. 条件编译

```c
#ifdef CONFIG_COMPONENT_SENSOR
    #include "xy_sensor.h"
#endif

#if defined(CONFIG_SENSOR_DHT11)
    xy_dht11_init(&dht11, gpio);
#endif
```

---

## 🔍 故障排查

### 问题 1: Kconfig 配置未生效

**检查**:
```bash
cat build/config.h  # 确认配置已生成
grep CONFIG_SENSOR_DHT11 build/config.h
```

**解决**:
```bash
rm -rf build
mkdir build && cd build
cmake .. -DCONFIG_SENSOR_DHT11=y
make
```

### 问题 2: 组件未自动检测

**检查**:
```bash
ls components/sensor/CMakeLists.txt  # 确认文件存在
```

**解决**:
```cmake
# 在顶层 CMakeLists.txt 中添加
if(EXISTS ${component_dir}/CMakeLists.txt)
    add_subdirectory(${component_dir})
endif()
```

---

## 📚 参考资料

- [CMake 官方文档](https://cmake.org/documentation/)
- [Kconfig 语法](https://www.kernel.org/doc/html/latest/kbuild/kconfig-language.html)
- [Zephyr 构建系统](https://docs.zephyrproject.org/latest/build/kconfig/index.html)

---

**维护者**: XinYi Team  
**更新日期**: 2026-03-18
