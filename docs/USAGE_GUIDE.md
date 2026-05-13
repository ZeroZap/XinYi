# XinYi 框架使用指南

**版本**: 2.0.0  
**日期**: 2026-03-02

---

## 📋 目录

1. [快速开始](#快速开始)
2. [组件配置](#组件配置)
3. [使用示例](#使用示例)
4. [最佳实践](#最佳实践)

---

## 🚀 快速开始

### 1. 包含主头文件

```c
#include "xy.h"
```

### 2. 最小系统

```c
#include "xy.h"

int main(void) {
    /* 初始化系统 */
    xy_os_kernel_init();
    
    /* 启动内核 */
    xy_os_kernel_start();
    
    return 0;
}
```

### 3. 使用日志

```c
#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_INFO
#include "xy.h"

int main(void) {
    xy_log_i("System starting...\n");
    
    /* 你的代码 */
    
    return 0;
}
```

---

## ⚙️ 组件配置

### Kconfig 配置

```kconfig
# 启用 OSAL
CONFIG_OSAL=y

# 启用 HAL
CONFIG_HAL=y

# 启用传感器
CONFIG_SENSOR=y

# 启用电源管理
CONFIG_POWER=y

# 启用 FOTA
CONFIG_FOTA=y

# 启用 GUI
CONFIG_GUI=y

# 启用日志
CONFIG_LOG=y
```

### CMake 配置

```cmake
# 在 CMakeLists.txt 中
set(CONFIG_OSAL ON)
set(CONFIG_HAL ON)
set(CONFIG_SENSOR ON)
set(CONFIG_LOG ON)

# 添加 XinYi
add_subdirectory(components)
include_directories(components)
```

---

## 📖 使用示例

### 传感器读取

```c
#include "xy.h"

static xy_sht30_t sht30;

void sensor_task(void *arg) {
    (void)arg;
    
    /* 初始化传感器 */
    xy_sht30_init(&sht30, i2c_handle, 0x44);
    
    while (1) {
        /* 读取温湿度 */
        xy_sht30_read(&sht30);
        
        xy_log_i("T: %d.%02d°C, H: %d.%02d%%RH\n",
                 sht30.data.temperature / 100,
                 sht30.data.temperature % 100,
                 sht30.data.humidity / 100,
                 sht30.data.humidity % 100);
        
        xy_os_delay(1000);
    }
}
```

### PID 温度控制

```c
#include "xy.h"

static xy_pid_t pid;
static xy_sht30_t sensor;

void pid_task(void *arg) {
    float target = 25.0f;
    float current;
    float output;
    
    /* 配置 PID */
    xy_pid_config_t cfg = {
        .kp = 2.0f,
        .ki = 0.5f,
        .kd = 1.0f,
    };
    xy_pid_init(&pid, &cfg);
    xy_pid_set_setpoint(&pid, target);
    
    while (1) {
        /* 读取当前温度 */
        xy_sht30_read(&sensor);
        current = sensor.data.temperature / 100.0f;
        
        /* 计算 PID 输出 */
        xy_pid_compute(&pid, current, &output);
        
        /* 控制加热器 */
        // heater_set_power(output);
        
        xy_os_delay(100);
    }
}
```

### FOTA 安全升级

```c
#include "xy.h"

static xy_fota_secure_t fota;

void fota_task(void *arg) {
    /* 配置安全 FOTA */
    xy_fota_secure_config_t cfg = {
        .pub_key = g_root_pub_key,
        .slot0_addr = 0x08010000,
        .slot1_addr = 0x08040000,
        .slot_size = 0x30000,
        .dual_bank = true,
    };
    
    xy_fota_secure_init(&fota, &cfg, &flash_ops);
    
    /* 等待升级包 */
    while (1) {
        if (upgrade_available()) {
            /* 下载并验证固件 */
            xy_fota_secure_verify(&fota, fw_pkg, pkg_size);
            
            /* 解密并写入 */
            xy_fota_secure_decrypt_and_write(&fota, encrypted, size, offset);
            
            /* 标记有效 */
            xy_fota_secure_mark_valid(&fota, 1);
            
            /* 重启生效 */
            NVIC_SystemReset();
        }
        
        xy_os_delay(1000);
    }
}
```

### 消息队列 IPC

```c
#include "xy.h"

static xy_mq_t mq;

void sender_task(void *arg) {
    xy_mq_msg_t msg;
    msg.id = 1;
    msg.priority = XY_MQ_PRIORITY_NORMAL;
    msg.data = data;
    msg.len = sizeof(data);
    
    while (1) {
        xy_mq_send(&mq, &msg, 100);
        xy_os_delay(1000);
    }
}

void receiver_task(void *arg) {
    xy_mq_msg_t msg;
    
    while (1) {
        if (xy_mq_recv(&mq, &msg, 100) == XY_MQ_OK) {
            /* 处理消息 */
            process_message(&msg);
        }
    }
}
```

---

## 🎯 最佳实践

### 1. 错误处理

```c
int ret;

ret = xy_sht30_init(&sensor, i2c, 0x44);
if (ret != XY_SHT30_OK) {
    xy_log_e("Sensor init failed: %d\n", ret);
    return ret;
}
```

### 2. 资源管理

```c
/* 使用完释放资源 */
xy_mq_deinit(&mq);
xy_fota_secure_deinit(&fota);
```

### 3. 日志级别

```c
/* 生产环境使用 INFO */
#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_INFO

/* 开发调试使用 DEBUG */
// #define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG
```

### 4. 看门狗喂狗

```c
void idle_task(void *arg) {
    while (1) {
        /* 喂看门狗 */
        IWDG_Reload();
        xy_os_delay(100);
    }
}
```

---

## 📊 组件依赖图

```
xy.h (主头文件)
├── OSAL (OS 抽象层)
├── HAL (硬件抽象层)
├── 传感器驱动
│   ├── SHT30/SHT40
│   ├── HDC1080/AHT20
│   ├── MPU6050/BMP280
│   └── BH1750/TSL2561
├── 电源管理
│   ├── BQ25620 (充电)
│   ├── INA226 (功率)
│   └── MAX17043 (电量计)
├── 中间件
│   ├── PID 控制
│   ├── FOTA 升级
│   ├── GUI 显示
│   └── IPC 通信
└── 系统服务
    ├── 系统监控
    └── 自动任务
```

---

## 🔗 相关文档

- [API 参考](docs/api/API_REFERENCE.md)
- [Wiki 文档](docs/wiki/xy_wiki.h)
- [组件状态](COMPONENTS_STATUS.md)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
