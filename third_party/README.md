# Third Party RTOS 管理方案

## 目录结构

```
XinYi/
├── components/
│   └── kernel/
│       └── osal/                    # OS 抽象层 (仅适配层代码)
│           ├── include/
│           │   ├── xy_os.h          # 统一 API 接口
│           │   ├── xy_os_cfg.h      # 配置文件
│           │   ├── xy_os_tick.h     # Tick 模块
│           │   └── xy_os_timer_sw.h # 软件定时器
│           ├── src/
│           │   ├── xy_os_tick.c
│           │   └── xy_os_timer_sw.c
│           ├── backend/
│           │   ├── baremetal/
│           │   │   └── osal_baremetal.c
│           │   ├── freertos/
│           │   │   └── osal_freertos.c    # 仅适配层，不包含 FreeRTOS 源码
│           │   ├── rtthread/
│           │   │   └── osal_rtthread.c    # 仅适配层，不包含 RT-Thread 源码
│           │   └── cmsis_rtx/
│           │       └── osal_cmsis_rtx.c   # 仅适配层，不包含 RTX 源码
│           ├── tests/
│           ├── docs/
│           ├── CMakeLists.txt
│           └── Kconfig
│
├── third_party/                     # ✅ 新增：第三方库统一管理
│   ├── README.md                    # 第三方库说明
│   ├── Kconfig                      # 第三方库配置
│   ├── CMakeLists.txt               # 第三方库构建
│   │
│   ├── freertos/                    # FreeRTOS
│   │   ├── LICENSE
│   │   ├── README.md
│   │   ├── Kconfig                  # FreeRTOS 配置选项
│   │   ├── CMakeLists.txt           # FreeRTOS 构建配置
│   │   ├── include/                 # FreeRTOS 头文件
│   │   │   ├── FreeRTOS.h
│   │   │   ├── task.h
│   │   │   └── ...
│   │   ├── portable/                # 移植层
│   │   │   ├── GCC/
│   │   │   │   └── ARM_CM4F/
│   │   │   │       ├── port.c
│   │   │   │       └── portmacro.h
│   │   │   └── MemMang/
│   │   │       └── heap_4.c
│   │   └── src/                     # FreeRTOS 源码
│   │       ├── tasks.c
│   │       ├── queue.c
│   │       └── ...
│   │
│   ├── rt-thread/                   # RT-Thread
│   │   ├── LICENSE
│   │   ├── README.md
│   │   ├── Kconfig
│   │   ├── CMakeLists.txt
│   │   ├── include/
│   │   │   ├── rtthread.h
│   │   │   └── ...
│   │   ├── src/
│   │   │   ├── ipc.c
│   │   │   ├── kservice.c
│   │   │   └── ...
│   │   └── libcpu/
│   │       └── ARM/
│   │           └── Cortex-M4/
│   │
│   └── cmsis-rtx/                   # CMSIS-RTX
│       ├── LICENSE
│       ├── README.md
│       ├── Kconfig
│       ├── CMakeLists.txt
│       ├── Include/
│       │   ├── rtx_os.h
│       │   └── ...
│       └── Source/
│           ├── rtx_kernel.c
│           └── ...
│
└── MCU/                             # MCU SDK
    ├── ST/
    │   └── STM32U5/
    └── ...
```

## 优势分析

### 当前方案 (RTOS 源码在 osal 内部)

```
❌ 问题:
1. 目录冗余：每个后端都包含完整的 RTOS 源码
2. 版本混乱：难以追踪 RTOS 版本
3. 更新困难：更新 RTOS 需要修改 osal 目录
4. 许可问题：混合了不同许可证的代码
5. 代码重复：多个后端可能包含相同的 RTOS 源码
```

### 新方案 (third_party 独立管理)

```
✅ 优势:
1. 清晰分离：OSAL 仅包含适配层，第三方源码独立管理
2. 版本管理：每个 RTOS 有独立的版本和许可证
3. 易于更新：更新 RTOS 不影响 OSAL 代码
4. 可复用性：其他项目可直接使用 third_party 中的 RTOS
5. 许可证清晰：每个 RTOS 有自己的 LICENSE 文件
6. 灵活选择：可以根据项目需求选择 RTOS，不影响 OSAL
```

## 构建配置

### CMake 集成

```cmake
# 在顶层 CMakeLists.txt 中

# 第三方库配置
option(USE_FREERTOS "Use FreeRTOS" OFF)
option(USE_RTTHREAD "Use RT-Thread" OFF)
option(USE_CMSIS_RTX "Use CMSIS-RTX" OFF)

# 添加第三方库
if(USE_FREERTOS)
    add_subdirectory(third_party/freertos)
    set(OSAL_BACKEND "freertos" CACHE STRING "OSAL backend")
endif()

if(USE_RTTHREAD)
    add_subdirectory(third_party/rt-thread)
    set(OSAL_BACKEND "rtthread" CACHE STRING "OSAL backend")
endif()

if(USE_CMSIS_RTX)
    add_subdirectory(third_party/cmsis-rtx)
    set(OSAL_BACKEND "cmsis_rtx" CACHE STRING "OSAL backend")
endif()

# 添加 OSAL
add_subdirectory(components/kernel/osal)
target_link_libraries(your_app xy_osal)

# 根据后端链接对应的 RTOS
if(USE_FREERTOS)
    target_link_libraries(your_app freertos_kernel)
elseif(USE_RTTHREAD)
    target_link_libraries(your_app rtthread)
elseif(USE_CMSIS_RTX)
    target_link_libraries(your_app rtx5)
endif()
```

### Kconfig 集成

```kconfig
# third_party/Kconfig

menu "Third Party Libraries"

config USE_THIRD_PARTY
    bool "Enable third party libraries"
    default y

choice
    prompt "RTOS Selection"
    default RTOS_NONE
    help
        Select the RTOS to use in your project.
        Only one RTOS can be selected at a time.

config RTOS_NONE
    bool "No RTOS (Bare-metal)"
    help
        No RTOS, use bare-metal mode.

config RTOS_FREERTOS
    bool "FreeRTOS"
    help
        Use FreeRTOS RTOS.
        License: MIT

config RTOS_RTTHREAD
    bool "RT-Thread"
    help
        Use RT-Thread RTOS.
        License: Apache-2.0

config RTOS_CMSIS_RTX
    bool "CMSIS-RTX"
    help
        Use ARM CMSIS-RTOS2 (RTX5).
        License: Apache-2.0

endchoice

# RTOS Configuration
menu "RTOS Configuration"
    depends on RTOS_FREERTOS || RTOS_RTTHREAD || RTOS_CMSIS_RTX

config FREERTOS_HEAP_TYPE
    int "FreeRTOS Heap Type"
    default 4
    range 1 5
    depends on RTOS_FREERTOS

config RTTHREAD_TICK_RATE
    int "RT-Thread Tick Rate (Hz)"
    default 1000
    depends on RTOS_RTTHREAD

endmenu

endmenu
```

## 使用示例

### 项目 A：使用 FreeRTOS

```bash
# 项目配置
cmake .. \
    -DUSE_FREERTOS=ON \
    -DFREERTOS_HEAP_TYPE=4 \
    -DOSAL_BACKEND=freertos
```

### 项目 B：使用 RT-Thread

```bash
# 项目配置
cmake .. \
    -DUSE_RTTHREAD=ON \
    -DRTTHREAD_TICK_RATE=1000 \
    -DOSAL_BACKEND=rtthread
```

### 项目 C：裸机

```bash
# 项目配置
cmake .. \
    -DOSAL_BACKEND=baremetal
```

## 许可证兼容性

| RTOS | 许可证 | 商业使用 | 修改要求 | 分发要求 |
|------|--------|----------|----------|----------|
| **FreeRTOS** | MIT | ✅ 允许 | ❌ 不要求 | ⚠️ 保留许可声明 |
| **RT-Thread** | Apache-2.0 | ✅ 允许 | ❌ 不要求 | ⚠️ 保留许可声明 |
| **CMSIS-RTX** | Apache-2.0 | ✅ 允许 | ❌ 不要求 | ⚠️ 保留许可声明 |

所有 RTOS 都使用宽松许可证，可以安全地用于商业项目。

## 迁移步骤

### 从当前布局迁移到新布局

1. **创建 third_party 目录**
   ```bash
   mkdir third_party
   ```

2. **移动 RTOS 源码**
   ```bash
   # 移动 FreeRTOS
   mv components/kernel/osal/freertos/FreeRTOS third_party/freertos/
   
   # 移动 RT-Thread
   mv components/kernel/osal/rt-thread/* third_party/rt-thread/
   
   # 移动 CMSIS-RTX
   mv components/kernel/osal/CMSIS-RTX/* third_party/cmsis-rtx/
   ```

3. **清理 osal 目录**
   ```bash
   # 仅保留适配层代码
   rm -rf components/kernel/osal/freertos/FreeRTOS
   rm -rf components/kernel/osal/rt-thread/*
   rm -rf components/kernel/osal/CMSIS-RTX/*
   ```

4. **更新构建配置**
   - 更新 `CMakeLists.txt` 链接新的 third_party 目录
   - 更新 `Kconfig` 添加第三方库配置

5. **测试验证**
   - 编译 baremetal 后端
   - 编译 FreeRTOS 后端
   - 编译 RT-Thread 后端

## 总结

将第三方 RTOS 源码移到 `third_party/` 目录是更好的实践：

1. **清晰分离**: OSAL 适配层与 RTOS 源码分离
2. **独立管理**: 每个 RTOS 有独立的版本和配置
3. **易于维护**: 更新 RTOS 不影响 OSAL 代码
4. **灵活选择**: 根据项目需求选择 RTOS
5. **许可证清晰**: 每个 RTOS 有自己的 LICENSE 文件

这种布局更符合现代嵌入式项目的最佳实践。
