# RTOS 选择指南

## 快速选择

| 应用场景 | 推荐 RTOS | 理由 |
|----------|-----------|------|
| **简单应用/启动阶段** | Bare-metal | 无需 RTOS，最小资源占用 |
| **通用嵌入式应用** | FreeRTOS | 生态成熟，文档丰富 |
| **物联网应用** | RT-Thread | 组件丰富，网络栈完善 |
| **ARM 生态项目** | CMSIS-RTX | ARM 官方支持，CMSIS 标准 |

---

## RTOS 详细对比

### FreeRTOS

**官网**: https://www.freertos.org/

**许可证**: MIT

**特点**:
- ✅ 全球最流行的嵌入式 RTOS
- ✅ 文档丰富，社区活跃
- ✅ 支持架构广泛
- ✅ 代码简洁，易于理解

**资源占用**:
- Flash: ~10KB (最小配置)
- RAM: ~2KB (最小配置)

**适用场景**:
- 通用嵌入式控制
- 工业控制
- 消费电子
- 需要快速开发的项目

**配置示例**:
```cmake
# CMake 配置
set(RTOS_BACKEND "freertos" CACHE STRING "RTOS backend")
set(FREERTOS_HEAP_TYPE 4)  # 推荐 heap_4
set(FREERTOS_TICK_RATE 1000)
```

**优点**:
- 简单易用，学习曲线低
- 移植简单，支持 MCU 多
- 社区资源丰富

**缺点**:
- 功能相对基础
- 网络/文件系统等需额外组件
- 内存池功能较弱

---

### RT-Thread

**官网**: https://www.rt-thread.io/

**许可证**: Apache-2.0

**特点**:
- ✅ 国产 RTOS，中文文档完善
- ✅ 组件丰富（网络、文件系统、GUI）
- ✅ 支持 POSIX API
- ✅ 物联网生态完善

**资源占用**:
- Flash: ~20KB (标准配置)
- RAM: ~4KB (标准配置)

**适用场景**:
- 物联网设备
- 需要网络功能的应用
- 需要文件系统的设备
- 需要 GUI 的设备

**配置示例**:
```cmake
# CMake 配置
set(RTOS_BACKEND "rtthread" CACHE STRING "RTOS backend")
set(RTTHREAD_TICK_RATE 1000)
set(RTTHREAD_USE_COMPONENTS ON)
```

**优点**:
- 组件丰富，开箱即用
- 网络栈完善（LwIP 集成）
- 支持 POSIX API
- 中文社区活跃

**缺点**:
- 代码量相对较大
- 学习曲线稍高
- 国际影响力有限

---

### CMSIS-RTOS2 (RTX5)

**官网**: https://github.com/ARM-software/CMSIS_5

**许可证**: Apache-2.0

**特点**:
- ✅ ARM 官方开发
- ✅ CMSIS 标准 API
- ✅ 与 MDK-ARM 深度集成
- ✅ 支持 TrustZone

**资源占用**:
- Flash: ~8KB (最小配置)
- RAM: ~1.5KB (最小配置)

**适用场景**:
- ARM Cortex-M 项目
- 使用 MDK-ARM 开发
- 需要 TrustZone 支持
- 追求代码标准化

**配置示例**:
```cmake
# CMake 配置
set(RTOS_BACKEND "cmsis_rtx" CACHE STRING "RTOS backend")
set(RTX_TICK_RATE 1000)
set(RTX_OS_STACK_SIZE 1024)
```

**优点**:
- ARM 官方支持
- CMSIS 标准接口
- 资源占用小
- 与 STM32CubeMX 集成

**缺点**:
- 主要支持 ARM 架构
- 社区相对较小
- 文档以英文为主

---

### Bare-metal (裸机)

**许可证**: 项目自有

**特点**:
- ✅ 无 RTOS 依赖
- ✅ 最小资源占用
- ✅ 确定性最高
- ✅ 适合简单应用

**资源占用**:
- Flash: ~1KB (仅 OSAL)
- RAM: ~100B

**适用场景**:
- 简单控制应用
- 对成本敏感的产品
- 启动引导程序
- 实时性要求极高的场景

**配置示例**:
```cmake
# CMake 配置
set(RTOS_BACKEND "baremetal" CACHE STRING "RTOS backend")
```

**优点**:
- 无 RTOS 开销
- 代码执行可预测
- 调试简单

**缺点**:
- 无多任务支持
- 需要手动管理时序
- 代码复用性低

---

## 决策树

```
开始
│
├─ 需要多任务支持吗？
│   ├─ 否 → 选择 Bare-metal
│   └─ 是 → 继续
│
├─ 使用 ARM Cortex-M 吗？
│   ├─ 否 → 选择 FreeRTOS
│   └─ 是 → 继续
│
├─ 需要网络/文件系统吗？
│   ├─ 是 → 选择 RT-Thread
│   └─ 否 → 继续
│
├─ 使用 MDK-ARM 开发吗？
│   ├─ 是 → 选择 CMSIS-RTX
│   └─ 否 → 选择 FreeRTOS
```

---

## 项目示例配置

### 项目 A：智能传感器（资源受限）

**需求**:
- MCU: STM32L4 (100KB Flash, 20KB RAM)
- 功能：传感器采集、低功耗、BLE 传输
- 无网络需求

**推荐**: FreeRTOS

**配置**:
```cmake
# 顶层 CMakeLists.txt
set(MCU_FAMILY "STM32L4")
set(RTOS_BACKEND "freertos")
set(FREERTOS_HEAP_TYPE 4)
set(FREERTOS_TICK_RATE 1000)
set(FREERTOS_MINIMAL_STACK_SIZE 64)  # 减小系统任务栈

# OSAL 配置
set(OSAL_BACKEND "freertos")
set(XY_OS_FEATURE_THREAD ON)
set(XY_OS_FEATURE_TIMER ON)
set(XY_OS_FEATURE_MEMORY_POOL OFF)  # 禁用不需要的功能
```

---

### 项目 B：物联网网关

**需求**:
- MCU: STM32H7 (高性能)
- 功能：多协议、网络、文件系统、GUI

**推荐**: RT-Thread

**配置**:
```cmake
# 顶层 CMakeLists.txt
set(MCU_FAMILY "STM32H7")
set(RTOS_BACKEND "rtthread")
set(RTTHREAD_TICK_RATE 1000)
set(RTTHREAD_USE_COMPONENTS ON)
set(RTTHREAD_CONSOLE_ENABLE ON)

# OSAL 配置
set(OSAL_BACKEND "rtthread")
set(XY_OS_FEATURE_THREAD ON)
set(XY_OS_FEATURE_MESSAGE_QUEUE ON)
set(XY_OS_FEATURE_MEMORY_POOL ON)
```

---

### 项目 C：工业控制器

**需求**:
- MCU: STM32U5
- 功能：实时控制、多轴电机、确定性

**推荐**: FreeRTOS 或 CMSIS-RTX

**配置**:
```cmake
# 顶层 CMakeLists.txt
set(MCU_FAMILY "STM32U5")
set(RTOS_BACKEND "freertos")
set(FREERTOS_CONFIG_PATH "config/FreeRTOSConfig.h")
set(FREERTOS_MAX_PRIORITIES 32)

# OSAL 配置
set(OSAL_BACKEND "freertos")
set(XY_OS_FEATURE_THREAD ON)
set(XY_OS_FEATURE_MUTEX ON)
set(XY_OS_FEATURE_SEMAPHORE ON)
set(XY_OS_PARAM_CHECK ON)  # 生产环境可关闭
```

---

### 项目 D：启动引导程序

**需求**:
- MCU: 任意 STM32
- 功能：固件升级、简单交互
- 无多任务需求

**推荐**: Bare-metal

**配置**:
```cmake
# 顶层 CMakeLists.txt
set(MCU_FAMILY "STM32U5")
set(RTOS_BACKEND "baremetal")

# OSAL 配置
set(OSAL_BACKEND "baremetal")
set(XY_OS_FEATURE_THREAD OFF)  # 禁用不需要的功能
set(XY_OS_FEATURE_MUTEX OFF)
set(XY_OS_FEATURE_SEMAPHORE OFF)
set(XY_OS_FEATURE_TIMER ON)    # 保留定时器
set(XY_OS_FEATURE_DELAY ON)
```

---

## 迁移指南

### 从 Bare-metal 迁移到 FreeRTOS

1. **修改 CMake 配置**:
```cmake
# 修改前
set(RTOS_BACKEND "baremetal")

# 修改后
set(RTOS_BACKEND "freertos")
```

2. **修改应用代码**:
```c
// Bare-metal 代码
int main(void) {
    xy_os_kernel_init();
    while (1) {
        do_something();
        xy_os_delay(1000);
    }
}

// FreeRTOS 代码
static void main_thread(void *arg) {
    while (1) {
        do_something();
        xy_os_delay(1000);
    }
}

int main(void) {
    xy_os_kernel_init();
    
    // 创建主线程
    xy_os_thread_attr_t attr = {
        .name = "main",
        .priority = XY_OS_PRIORITY_NORMAL,
        .stack_size = 2048,
    };
    xy_os_thread_new(main_thread, NULL, &attr);
    
    // 启动调度器
    xy_os_kernel_start();
}
```

3. **重新编译**:
```bash
rm -rf build
mkdir build && cd build
cmake .. -DRTOS_BACKEND=freertos
make
```

---

## 性能对比

| RTOS | 上下文切换 | 中断延迟 | 最小内存 | 代码大小 |
|------|-----------|----------|----------|----------|
| **Bare-metal** | N/A | ~100ns | ~100B | ~1KB |
| **FreeRTOS** | ~3μs | ~200ns | ~1.5KB | ~10KB |
| **RT-Thread** | ~4μs | ~250ns | ~2KB | ~20KB |
| **CMSIS-RTX** | ~2μs | ~180ns | ~1.5KB | ~8KB |

*数据基于 STM32F4 @ 168MHz*

---

## 总结

| 因素 | FreeRTOS | RT-Thread | CMSIS-RTX | Bare-metal |
|------|----------|-----------|-----------|------------|
| **易用性** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| **功能丰富** | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐ |
| **资源占用** | ⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| **社区支持** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐ |
| **文档质量** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ |
| **学习曲线** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |

**推荐**:
- 初学者：FreeRTOS
- 物联网项目：RT-Thread
- ARM 生态：CMSIS-RTX
- 简单应用：Bare-metal
