# XinYi 快速入门指南

**版本**: v1.0.0  
**最后更新**: 2026-03-13  
**适用**: 嵌入式开发者 / 物联网工程师

---

## 🚀 5 分钟快速开始

### 1. 克隆仓库

```bash
git clone https://github.com/ZeroZap/XinYi.git
cd XinYi
```

### 2. 选择示例项目

```bash
# STM32U5 示例 (推荐)
cd examples/stm32u5_demo

# 或 PC 仿真 (无需硬件)
cd examples/pc_simulator
```

### 3. 编译项目

```bash
# STM32U5 (需要 ARM GCC)
make

# PC 仿真 (需要 GCC)
make PLATFORM=PC
```

### 4. 运行/烧录

```bash
# STM32U5 - 烧录到开发板
make flash

# PC 仿真 - 直接运行
./build/xy_demo
```

---

## 📋 前置要求

### 硬件要求 (可选)

| 开发板 | MCU | 说明 |
|--------|-----|------|
| NUCLEO-U575ZI | STM32U575 | 官方推荐 |
| NUCLEO-F446RE | STM32F446 | 经典款 |
| CH32V307VCT6 | WCH CH32 | 国产替代 |

### 软件要求

| 工具 | 版本 | 必需 | 下载 |
|------|------|------|------|
| **ARM GCC** | 9-2020-q4 | ✅ (STM32) | [下载](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm) |
| **GCC** | 9.0+ | ✅ (PC 仿真) | `sudo apt install gcc` |
| **CMake** | 3.10+ | ✅ | `sudo apt install cmake` |
| **OpenOCD** | 0.11+ | ⚠️ (烧录) | `sudo apt install openocd` |
| **ST-Link** | - | ⚠️ (ST 烧录) | `sudo apt install stlink-tools` |

### 验证安装

```bash
# 检查 ARM GCC
arm-none-eabi-gcc --version
# 输出：arm-none-eabi-gcc (GNU Arm Embedded Toolchain 9-2020-q4-major) 9.3.1

# 检查 CMake
cmake --version
# 输出：cmake version 3.16.3

# 检查 OpenOCD
openocd --version
# 输出：Open On-Chip Debugger 0.11.0
```

---

## 🏗️ 项目结构

```
XinYi/
├── components/          # 核心组件
│   ├── kernel/         # 内核 (OSAL/服务)
│   ├── hal/            # 硬件抽象层
│   ├── device/         # 设备框架
│   ├── sensor/         # 传感器驱动
│   ├── crypto/         # 密码学
│   ├── net/            # 网络协议
│   ├── fota/           # 固件升级
│   └── ...
│
├── examples/           # 示例项目
│   ├── stm32u5_demo/   # STM32U5 示例
│   ├── pc_simulator/   # PC 仿真
│   └── ...
│
├── projects/           # 实际项目
├── tests/              # 测试
├── docs/               # 文档
└── scripts/            # 工具脚本
```

---

## 💻 第一个应用

### 示例：点亮 LED

创建文件 `examples/my_first_app/main.c`:

```c
#include "xy_os.h"
#include "xy_hal.h"
#include "xy_log.h"

/* LED 引脚 (根据开发板修改) */
#define LED_PIN     GPIO_PIN_5
#define LED_PORT    GPIOA

void app_thread(void *arg)
{
    (void)arg;
    
    xy_log_i("My First XinYi App!\n");
    
    /* 配置 LED 引脚 */
    xy_hal_pin_config_t led_cfg = {
        .mode = XY_HAL_PIN_MODE_OUTPUT,
        .pull = XY_HAL_PIN_PULL_NONE,
        .otype = XY_HAL_PIN_OTYPE_PP,
        .speed = XY_HAL_PIN_SPEED_LOW
    };
    xy_hal_pin_init(LED_PORT, LED_PIN, &led_cfg);
    
    /* 闪烁 LED */
    while (1) {
        xy_hal_pin_write(LED_PORT, LED_PIN, 1);  /* LED ON */
        xy_os_delay(500);                         /* 延时 500ms */
        
        xy_hal_pin_write(LED_PORT, LED_PIN, 0);  /* LED OFF */
        xy_os_delay(500);                         /* 延时 500ms */
        
        xy_log_d("LED toggled\n");
    }
}

int main(void)
{
    /* 初始化系统 */
    xy_os_kernel_init();
    xy_log_init();
    
    xy_log_i("System initialized\n");
    
    /* 创建应用任务 */
    xy_os_thread_t thread;
    xy_os_thread_create(&thread, "app_thread", app_thread, NULL,
                        2, 512);  /* 优先级 2, 栈 512 字节 */
    
    /* 启动调度器 */
    xy_os_kernel_start();
    
    return 0;
}
```

### 编译运行

```bash
# 创建构建目录
mkdir -p examples/my_first_app/build
cd examples/my_first_app/build

# CMake 配置
cmake .. -DCMAKE_TOOLCHAIN_FILE=../../../cmake/arm-gcc.cmake

# 编译
make

# 烧录 (STM32)
make flash

# 或直接运行 (PC 仿真)
./my_first_app
```

---

## 🔧 常用组件使用

### 1. OSAL (操作系统抽象层)

```c
#include "xy_os.h"

/* 创建任务 */
xy_os_thread_t thread;
xy_os_thread_create(&thread, "my_task", my_function, NULL, 
                    3, 1024);

/* 创建互斥量 */
xy_os_mutex_t mutex;
xy_os_mutex_create(&mutex);

/* 创建信号量 */
xy_os_sem_t sem;
xy_os_sem_create(&sem, 0, 10);

/* 创建消息队列 */
xy_os_msgqueue_t mq;
xy_os_msgqueue_create(&mq, 10, sizeof(my_msg_t));

/* 延时 */
xy_os_delay(100);  /* 延时 100ms */

/* 获取系统 tick */
uint32_t tick = xy_os_tick_get();
```

### 2. HAL (硬件抽象层)

```c
#include "xy_hal.h"

/* GPIO 操作 */
xy_hal_pin_init(GPIOA, GPIO_PIN_5, &config);
xy_hal_pin_write(GPIOA, GPIO_PIN_5, 1);
int level = xy_hal_pin_read(GPIOA, GPIO_PIN_5);

/* UART 操作 */
xy_hal_uart_send(&huart1, data, len, 1000);
xy_hal_uart_recv(&huart1, buffer, len, 1000);

/* I2C 操作 */
xy_hal_i2c_master_transmit(&hi2c1, addr, data, len, 1000);
xy_hal_i2c_master_receive(&hi2c1, addr, buffer, len, 1000);

/* SPI 操作 */
xy_hal_spi_transmit(&hspi1, data, len, 1000);
xy_hal_spi_receive(&hspi1, buffer, len, 1000);
```

### 3. Device (设备框架)

```c
#include "xy_device.h"
#include "xy_device_core.h"

/* 初始化 I2C 设备 (如 SHT30) */
xy_i2c_device_t sht30;
xy_i2c_device_init(&sht30, &hi2c1, 0x44, 1000);

/* 注册到设备管理 */
xy_device_registry_register(&sht30.base);

/* 查找设备 */
xy_device_t *dev = xy_device_find_by_name("sht30");

/* 设备电源管理 */
xy_device_sleep(&sht30.base);  /* 进入休眠 */
xy_device_wake(&sht30.base);   /* 唤醒 */
```

### 4. Sensor (传感器)

```c
#include "xy_sht30.h"

/* 初始化 SHT30 温湿度传感器 */
xy_sht30_t sht30;
xy_sht30_init(&sht30, &hi2c1, 0x44);

/* 读取数据 */
xy_sht30_read_all(&sht30);

/* 获取温度 (0.01°C 精度) */
int16_t temp = sht30.temperature;  /* 2500 = 25.00°C */

/* 获取湿度 (0.01%RH 精度) */
int16_t humid = sht30.humidity;    /* 5000 = 50.00%RH */
```

### 5. Crypto (密码学)

```c
#include "xy_crypto.h"

/* AES-128 加密 */
uint8_t key[16] = {0};
uint8_t iv[16] = {0};
uint8_t data[16] = "Hello, XinYi!";
uint8_t encrypted[16];

xy_aes_set_key(&ctx, key, 128);
xy_aes_crypt_cbc(&ctx, AES_ENCRYPT, 16, iv, data, encrypted);

/* SHA-256 哈希 */
uint8_t hash[32];
xy_sha256(data, len, hash);

/* CRC16 校验 */
uint16_t crc = xy_crc16(data, len);
```

---

## 🎯 后端选择

### OSAL 后端配置

在 `xy_os_cfg.h` 中配置:

```c
/* 选择 OSAL 后端 */
#define OSAL_BACKEND_FREERTOS
// #define OSAL_BACKEND_RTTHREAD
// #define OSAL_BACKEND_CMSIS_RTX
// #define OSAL_BACKEND_BAREMETAL
```

### HAL 平台配置

在 CMake 中配置:

```cmake
set(HAL_PLATFORM "STM32" CACHE STRING "HAL Platform")
# 可选：STM32 / WCH / HC32 / PC
```

或在 Makefile 中:

```makefile
HAL_PLATFORM = STM32
# 可选：STM32 / WCH / HC32 / PC
```

---

## 📚 下一步学习

### 推荐阅读顺序

1. **架构理解** → `docs/architecture_analysis.md`
2. **OSAL 使用** → `docs/components/kernel/osal/README.md`
3. **HAL 移植** → `docs/components/hal/PORTING_GUIDE.md`
4. **设备开发** → `docs/components/device/DEVICE_ARCHITECTURE.md`
5. **组件生态** → `docs/COMPONENT_STATUS_REPORT.md`

### 示例项目

| 示例 | 路径 | 说明 |
|------|------|------|
| Blinky | `examples/stm32u5_demo/` | LED 闪烁 |
| Sensor | `examples/sensor_demo/` | 传感器读取 |
| Network | `examples/net_demo/` | 网络通信 |
| FOTA | `examples/fota_demo/` | 固件升级 |

### 高级主题

- [ ] 多任务编程 (OSAL)
- [ ] 设备驱动开发 (Device)
- [ ] 低功耗设计 (PM)
- [ ] 安全启动 (FOTA)
- [ ] 网络协议栈 (Net)

---

## 🤝 获取帮助

### 文档

- 本地文档：`docs/` 目录
- 在线文档：(待部署)

### 社区

- GitHub: https://github.com/ZeroZap/XinYi
- Issues: https://github.com/ZeroZap/XinYi/issues
- Discussions: https://github.com/ZeroZap/XinYi/discussions

### 常见问题

**Q: 编译失败 "arm-none-eabi-gcc: command not found"**

A: 安装 ARM GCC 工具链:
```bash
# Ubuntu
sudo apt install gcc-arm-none-eabi

# 或从 ARM 官网下载
# https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm
```

**Q: 烧录失败 "No ST-Link detected"**

A: 检查:
1. ST-Link 连接是否正常
2. 驱动是否安装
3. 尝试 `sudo openocd -f interface/stlink.cfg -f target/stm32u5.cfg`

**Q: 如何选择 OSAL 后端？**

A: 根据项目需求:
- FreeRTOS: 成熟稳定，生态丰富
- RT-Thread: 国产 RTOS，组件丰富
- CMSIS-RTX: ARM 标准，轻量级
- Bare-metal: 最简单，无 RTOS

---

## 📝 检查清单

开始开发前，确保完成:

- [ ] 工具链安装验证
- [ ] 仓库克隆成功
- [ ] 示例项目编译通过
- [ ] 开发板连接正常 (如有)
- [ ] 阅读架构文档
- [ ] 选择合适后端

---

**祝你开发愉快！** 🎉

如有问题，请提交 Issue 或参与 Discussions。

---

*XinYi - 为嵌入式而生* ⚡
