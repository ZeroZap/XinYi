# 第三方库集成指南

**版本**: 1.0.0  
**日期**: 2026-03-05

---

## 📦 第三方库存放位置

```
components/
├── third_party/           # 第三方库根目录
│   ├── network/           # 网络协议栈
│   │   ├── lwip/          # TCP/IP 协议栈
│   │   ├── http_parser/   # HTTP 解析
│   │   └── mqtt/          # MQTT 客户端
│   ├── filesystem/        # 文件系统
│   │   ├── fatfs/         # FatFS
│   │   ├── littlefs/      # LittleFS
│   │   └── spiffs/        # SPIFFS
│   ├── usb/               # USB 协议栈
│   │   ├── tinyusb/       # TinyUSB
│   │   └── libusb/        # LibUSB (Host)
│   ├── bluetooth/         # 蓝牙协议栈
│   │   ├── nimble/        # NimBLE
│   │   └── bluez/         # BlueZ (Linux)
│   ├── wireless/          # 无线通信
│   │   ├── esp_wifi/      # ESP WiFi
│   │   └── lorawan/       # LoRaWAN
│   ├── graphics/          # 图形库
│   │   ├── lvgl/          # LVGL
│   │   └── u8g2/          # U8g2
│   └── audio/             # 音频处理
│       ├── opus/          # Opus 编解码
│       └── libmad/        # MP3 解码
│
├── lib/                   # 编译后的库文件
│   ├── include/           # 头文件
│   └── lib/               # 库文件
│
└── docs/                  # 文档
    └── third_party/       # 第三方库文档
```

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
```

### 方式 3: 包管理器

```bash
# 使用 vcpkg (如果支持)
vcpkg install lwip
vcpkg install fatfs

# 复制到头文件和库目录
cp -r vcpkg/installed/x64-linux/include/* components/lib/include/
cp -r vcpkg/installed/x64-linux/lib/* components/lib/lib/
```

---

## 📋 第三方库清单

### 网络协议栈

| 库名 | 用途 | 许可证 | 集成方式 |
|------|------|--------|---------|
| **LwIP** | TCP/IP | BSD-3 | Submodule |
| **mbedTLS** | TLS/SSL | Apache-2.0 | Submodule |
| **Eclipse Paho** | MQTT | EPL-1.0 | Submodule |
| **http_parser** | HTTP 解析 | MIT | Direct |

### 文件系统

| 库名 | 用途 | 许可证 | 集成方式 |
|------|------|--------|---------|
| **FatFS** | FAT 文件系统 | BSD-2 | Direct |
| **LittleFS** | 掉电安全 | BSD-3 | Submodule |
| **SPIFFS** | SPI Flash | MIT | Submodule |

### USB 协议栈

| 库名 | 用途 | 许可证 | 集成方式 |
|------|------|--------|---------|
| **TinyUSB** | USB Device/Host | MIT | Submodule |
| **libusb** | USB Host (PC) | LGPL-2.1 | Package |

### 蓝牙协议栈

| 库名 | 用途 | 许可证 | 集成方式 |
|------|------|--------|---------|
| **NimBLE** | BLE Host | Apache-2.0 | Submodule |
| **BlueZ** | BLE Host (Linux) | GPL-2.0 | Package |

### 图形库

| 库名 | 用途 | 许可证 | 集成方式 |
|------|------|--------|---------|
| **LVGL** | 图形界面 | MIT | Submodule |
| **U8g2** | 单色显示 | BSD-3 | Direct |

---

## 🔧 CMake 集成

### CMakeLists.txt 示例

```cmake
# 第三方库配置
option(XY_USE_LWIP "Use LwIP TCP/IP stack" ON)
option(XY_USE_FATFS "Use FatFS filesystem" ON)
option(XY_USE_TINYUSB "Use TinyUSB stack" OFF)
option(XY_USE_LVGL "Use LVGL graphics" OFF)

# LwIP 集成
if(XY_USE_LWIP)
    add_subdirectory(third_party/network/lwip)
    target_link_libraries(xy_core PRIVATE lwip)
endif()

# FatFS 集成
if(XY_USE_FATFS)
    add_subdirectory(third_party/filesystem/fatfs)
    target_link_libraries(xy_core PRIVATE fatfs)
endif()

# TinyUSB 集成
if(XY_USE_TINYUSB)
    add_subdirectory(third_party/usb/tinyusb)
    target_link_libraries(xy_core PRIVATE tinyusb)
endif()

# LVGL 集成
if(XY_USE_LVGL)
    add_subdirectory(third_party/graphics/lvgl)
    target_link_libraries(xy_gui PRIVATE lvgl)
endif()
```

---

## 📝 许可证兼容性

### 兼容许可证

| 许可证 | 兼容性 | 说明 |
|--------|--------|------|
| **MIT** | ✅ 完全兼容 | 可商用 |
| **Apache-2.0** | ✅ 完全兼容 | 可商用 |
| **BSD-2/3** | ✅ 完全兼容 | 可商用 |
| **LGPL-2.1** | ⚠️ 动态链接 | 需动态链接 |
| **GPL-2.0** | ❌ 不兼容 | 传染性 |

### 许可证检查清单

- [ ] 检查许可证类型
- [ ] 确认商用许可
- [ ] 添加许可证声明
- [ ] 保留版权声明
- [ ] 遵守分发要求

---

## 🎯 集成优先级

### 高优先级 (本周)

1. **LwIP** - TCP/IP 协议栈
2. **FatFS** - FAT 文件系统
3. **TinyUSB** - USB 协议栈

### 中优先级 (下周)

4. **NimBLE** - BLE 协议栈
5. **LVGL** - 图形库
6. **LittleFS** - 掉电安全文件系统

### 低优先级 (可选)

7. **SPIFFS** - SPI Flash 文件系统
8. **http_parser** - HTTP 解析
9. **U8g2** - 单色显示

---

## 📊 集成进度

| 类别 | 计划 | 已集成 | 进度 |
|------|------|--------|------|
| **网络** | 4 | 0 | 0% |
| **文件系统** | 3 | 0 | 0% |
| **USB** | 2 | 0 | 0% |
| **蓝牙** | 2 | 0 | 0% |
| **图形** | 2 | 0 | 0% |
| **总计** | 13 | 0 | 0% |

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
