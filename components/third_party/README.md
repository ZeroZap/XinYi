# 第三方库集成指南 (纯净版)

**版本**: 1.0.0  
**日期**: 2026-03-05  
**许可证政策**: ✅ 仅 MIT/Apache-2.0/BSD-2/3

---

## 📦 第三方库存放位置

```
components/
├── third_party/           # 第三方库根目录
│   ├── network/           # 网络协议栈
│   │   ├── lwip/          # TCP/IP (BSD-3) ✅
│   │   ├── picohttp/      # HTTP 服务器 (MIT) ✅
│   │   └── paho-mqtt/     # MQTT (EPL-1.0) ⚠️
│   ├── filesystem/        # 文件系统
│   │   ├── fatfs/         # FatFS (BSD-2) ✅
│   │   ├── littlefs/      # LittleFS (BSD-3) ✅
│   │   └── spiffs/        # SPIFFS (MIT) ✅
│   ├── usb/               # USB 协议栈
│   │   ├── tinyusb/       # TinyUSB (MIT) ✅
│   │   └── musb/          # MUSB (MIT) ✅
│   ├── graphics/          # 图形库
│   │   ├── lvgl/          # LVGL (MIT) ✅
│   │   └── u8g2/          # U8g2 (BSD-3) ✅
│   └── crypto/            # 加密库
│       ├── mbedtls/       # MbedTLS (Apache-2.0) ✅
│       └── tinycrypt/     # TinyCrypt (BSD-3) ✅
│
├── lib/                   # 编译后的库文件
│   ├── include/           # 头文件
│   └── lib/               # 库文件
│
└── docs/                  # 文档
    └── third_party/       # 第三方库文档
```

---

## ✅ 许可证白名单

### 完全兼容 (可直接使用)

| 许可证 | 兼容性 | 商用 | 修改 | 分发 |
|--------|--------|------|------|------|
| **MIT** | ✅ 100% | ✅ | ✅ | ✅ |
| **Apache-2.0** | ✅ 100% | ✅ | ✅ | ✅ |
| **BSD-2-Clause** | ✅ 100% | ✅ | ✅ | ✅ |
| **BSD-3-Clause** | ✅ 100% | ✅ | ✅ | ✅ |
| **ISC** | ✅ 100% | ✅ | ✅ | ✅ |

### 有条件兼容 (需审查)

| 许可证 | 兼容性 | 商用 | 修改 | 分发 | 条件 |
|--------|--------|------|------|------|------|
| **EPL-1.0** | ⚠️ 部分 | ✅ | ✅ | ⚠️ | 修改需开源 |
| **MPL-2.0** | ⚠️ 部分 | ✅ | ✅ | ⚠️ | 修改需开源 |

### 不兼容 (禁止使用) ❌

| 许可证 | 原因 |
|--------|------|
| **GPL-2.0/3.0** | 传染性，需开源整个项目 |
| **LGPL-2.1/3.0** | 动态链接要求 |
| **AGPL-3.0** | 网络服务也需开源 |
| **SSPL** | MongoDB 专有协议 |

---

## 🔧 集成方式

### 方式 1: Git Submodule (推荐)

```bash
# 添加第三方库为子模块
cd components/third_party/network
git submodule add https://github.com/lwip-tcpip/lwip.git lwip

# 更新子模块
git submodule update --init --recursive
```

### 方式 2: 直接复制

```bash
# 下载并解压到对应目录
wget https://github.com/lvgl/lvgl/archive/master.zip
unzip master.zip -d components/third_party/graphics/
mv components/third_party/graphics/lvgl-master \
   components/third_party/graphics/lvgl
```

### 方式 3: 包管理器 (仅开发)

```bash
# 使用 vcpkg (仅用于开发测试)
vcpkg install lwip
vcpkg install fatfs

# 复制头文件和库文件
cp -r vcpkg/installed/x64-linux/include/* components/lib/include/
cp -r vcpkg/installed/x64-linux/lib/* components/lib/lib/

# 注意：不要提交 vcpkg 包管理器本身
echo "lib/" >> ../../.gitignore
```

---

## 📋 推荐第三方库清单

### 网络协议栈 (100% 兼容)

| 库名 | 用途 | 许可证 | 状态 |
|------|------|--------|------|
| **LwIP** | TCP/IP | BSD-3 | ✅ 推荐 |
| **picoHTTP** | HTTP 服务器 | MIT | ✅ 推荐 |
| **NanoHTTP** | HTTP 客户端 | BSD-2 | ✅ 推荐 |
| **Eclipse Paho** | MQTT | EPL-1.0 | ⚠️ 需审查 |

### 文件系统 (100% 兼容)

| 库名 | 用途 | 许可证 | 状态 |
|------|------|--------|------|
| **FatFS** | FAT 文件系统 | BSD-2 | ✅ 推荐 |
| **LittleFS** | 掉电安全 | BSD-3 | ✅ 推荐 |
| **SPIFFS** | SPI Flash | MIT | ✅ 推荐 |

### USB 协议栈 (100% 兼容)

| 库名 | 用途 | 许可证 | 状态 |
|------|------|--------|------|
| **TinyUSB** | USB Device/Host | MIT | ✅ 推荐 |
| **MUSB** | USB 设备 | MIT | ✅ 可选 |

### 图形库 (100% 兼容)

| 库名 | 用途 | 许可证 | 状态 |
|------|------|--------|------|
| **LVGL** | 图形界面 | MIT | ✅ 推荐 |
| **U8g2** | 单色显示 | BSD-3 | ✅ 推荐 |
| **Guider** | LVGL GUI 工具 | MIT | ✅ 可选 |

### 加密库 (100% 兼容)

| 库名 | 用途 | 许可证 | 状态 |
|------|------|--------|------|
| **MbedTLS** | TLS/SSL | Apache-2.0 | ✅ 推荐 |
| **TinyCrypt** | 基础加密 | BSD-3 | ✅ 推荐 |
| **WolfSSL** | TLS/SSL | GPL-2.0 | ❌ 禁止 |

### 蓝牙协议栈 (需审查)

| 库名 | 用途 | 许可证 | 状态 |
|------|------|--------|------|
| **NimBLE** | BLE Host | Apache-2.0 | ✅ 推荐 |
| **BlueZ** | BLE Host (Linux) | GPL-2.0 | ❌ 禁止 |

### 无线通信 (100% 兼容)

| 库名 | 用途 | 许可证 | 状态 |
|------|------|--------|------|
| **ESP WiFi** | ESP32 WiFi | Apache-2.0 | ✅ 推荐 |
| **LoRaMac** | LoRaWAN | Apache-2.0 | ✅ 推荐 |

---

## 🔧 CMake 集成

### CMakeLists.txt 示例

```cmake
# 第三方库配置
option(XY_USE_LWIP "Use LwIP TCP/IP stack" ON)
option(XY_USE_FATFS "Use FatFS filesystem" ON)
option(XY_USE_TINYUSB "Use TinyUSB stack" OFF)
option(XY_USE_LVGL "Use LVGL graphics" OFF)
option(XY_USE_MBEDTLS "Use MbedTLS crypto" ON)

# LwIP 集成 (BSD-3)
if(XY_USE_LWIP)
    add_subdirectory(third_party/network/lwip)
    target_link_libraries(xy_core PRIVATE lwip)
endif()

# FatFS 集成 (BSD-2)
if(XY_USE_FATFS)
    add_subdirectory(third_party/filesystem/fatfs)
    target_link_libraries(xy_core PRIVATE fatfs)
endif()

# TinyUSB 集成 (MIT)
if(XY_USE_TINYUSB)
    add_subdirectory(third_party/usb/tinyusb)
    target_link_libraries(xy_core PRIVATE tinyusb)
endif()

# LVGL 集成 (MIT)
if(XY_USE_LVGL)
    add_subdirectory(third_party/graphics/lvgl)
    target_link_libraries(xy_gui PRIVATE lvgl)
endif()

# MbedTLS 集成 (Apache-2.0)
if(XY_USE_MBEDTLS)
    add_subdirectory(third_party/crypto/mbedtls)
    target_link_libraries(xy_crypto PRIVATE mbedtls)
endif()
```

---

## 📝 许可证检查清单

### 添加新库前检查

- [ ] 检查 LICENSE 文件
- [ ] 确认许可证类型
- [ ] 确认商用许可
- [ ] 确认修改权限
- [ ] 确认分发要求
- [ ] 添加到许可证清单
- [ ] 更新 CMakeLists.txt
- [ ] 添加版权声明

### 许可证声明模板

```c
/**
 * @file xxx.c
 * @brief XXX 功能实现
 * 
 * @copyright Copyright (c) 2026 XinYi Team
 * @copyright Copyright (c) 2023 Third Party Author
 * 
 * @license MIT License
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 */
```

---

## 🎯 集成优先级

### 高优先级 (本周)

1. **LwIP** (BSD-3) - TCP/IP 协议栈
2. **FatFS** (BSD-2) - FAT 文件系统
3. **TinyUSB** (MIT) - USB 协议栈

### 中优先级 (下周)

4. **NimBLE** (Apache-2.0) - BLE 协议栈
5. **LVGL** (MIT) - 图形库
6. **LittleFS** (BSD-3) - 掉电安全文件系统

### 低优先级 (可选)

7. **SPIFFS** (MIT) - SPI Flash 文件系统
8. **picoHTTP** (MIT) - HTTP 服务器
9. **U8g2** (BSD-3) - 单色显示

---

## 📊 集成进度

| 类别 | 计划 | 已集成 | 进度 |
|------|------|--------|------|
| **网络** | 3 | 0 | 0% |
| **文件系统** | 3 | 0 | 0% |
| **USB** | 2 | 0 | 0% |
| **图形** | 2 | 0 | 0% |
| **加密** | 2 | 0 | 0% |
| **蓝牙** | 1 | 0 | 0% |
| **总计** | 13 | 0 | 0% |

---

## ⚠️ 禁止使用的库

### GPL 许可证 ❌

| 库名 | 用途 | 替代方案 |
|------|------|---------|
| **BlueZ** | BLE Host | NimBLE (Apache-2.0) |
| **WolfSSL** | TLS/SSL | MbedTLS (Apache-2.0) |
| **GNU TLS** | TLS/SSL | MbedTLS (Apache-2.0) |
| **Readline** | 命令行 | linenoise (BSD-2) |

### LGPL 许可证 ❌

| 库名 | 用途 | 替代方案 |
|------|------|---------|
| **FFmpeg** | 音视频 | 无 (暂不需要) |
| **SQLite** | 数据库 | 无 (使用 FatFS) |
| **libusb** | USB Host | TinyUSB (MIT) |

---

## 📚 相关文档

- [SPDX 许可证列表](https://spdx.org/licenses/)
- [OSI 认证许可证](https://opensource.org/licenses)
- [GitHub 许可证选择](https://choosealicense.com/)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
