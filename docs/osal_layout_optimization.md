# OSAL 布局优化方案

## 问题分析

### 当前布局问题

```
当前布局 (不合理):
components/kernel/osal/
├── baremetal/xy_os_baremetal.c      # ✅ 裸机后端
├── freertos/
│   ├── FreeRTOS/                    # ❌ 完整 FreeRTOS 源码 (应在 third_party)
│   └── xy_os_freertos.c             # ✅ 适配层
├── rt-thread/
│   ├── components/                  # ❌ 完整 RT-Thread 源码
│   ├── include/
│   ├── libcpu/
│   ├── src/
│   └── xy_os_rtthread.c             # ✅ 适配层
├── CMSIS-RTX/                       # ❌ 完整 RTX 源码
├── examples/
├── Kconfig
├── Kconfig.osal                     # ⚠️ 重复
└── Makefile                         # ⚠️ 通用模板
```

**问题总结**:
1. 🔴 **RTOS 源码冗余**: FreeRTOS/RT-Thread/RTX 源码占据大量空间
2. 🔴 **版本混乱**: 难以追踪每个 RTOS 的版本
3. 🔴 **许可问题**: 不同许可证的代码混合
4. 🟡 **misc 位置不当**: `xy_tick` 和 `xy_timer_sw` 在 `kernel/misc` 而非 osal 内部
5. 🟡 **Kconfig 重复**: `Kconfig` 和 `Kconfig.osal` 功能重复
6. 🟡 **缺少 include 目录**: 公共头文件未集中管理

---

## 优化方案

### 新布局结构

```
优化后布局:
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
│           ├── backend/             # 后端适配层 (仅 OSAL 代码)
│           │   ├── baremetal/
│           │   │   └── osal_baremetal.c
│           │   ├── freertos/
│           │   │   └── osal_freertos.c
│           │   ├── rtthread/
│           │   │   └── osal_rtthread.c
│           │   └── cmsis_rtx/
│           │       └── osal_cmsis_rtx.c
│           ├── tests/               # 单元测试
│           ├── docs/                # 文档
│           ├── examples/            # 示例
│           ├── CMakeLists.txt
│           └── Kconfig
│
└── third_party/                     # ✅ 第三方库统一管理
    ├── README.md
    ├── Kconfig
    ├── CMakeLists.txt
    ├── freertos/                    # FreeRTOS
    │   ├── LICENSE
    │   ├── include/
    │   ├── src/
    │   └── portable/
    ├── rt-thread/                   # RT-Thread
    │   ├── LICENSE
    │   ├── include/
    │   ├── src/
    │   └── libcpu/
    └── cmsis-rtx/                   # CMSIS-RTX
        ├── LICENSE
        ├── Include/
        └── Source/
```

---

## 优化优势

| 方面 | 当前方案 | 优化方案 |
|------|----------|----------|
| **代码分离** | ❌ RTOS 源码与 OSAL 混合 | ✅ 清晰分离 |
| **版本管理** | ❌ 难以追踪 RTOS 版本 | ✅ 每个 RTOS 独立版本 |
| **许可证** | ❌ 混合许可证 | ✅ 每个 RTOS 独立 LICENSE |
| **更新维护** | ❌ 更新 RTOS 影响 OSAL | ✅ 更新 RTOS 不影响 OSAL |
| **可复用性** | ❌ RTOS 源码绑定 OSAL | ✅ RTOS 可独立使用 |
| **目录大小** | ❌ 大量 RTOS 源码冗余 | ✅ OSAL 目录精简 |

---

## 迁移步骤

### 步骤 1: 创建 third_party 目录

```bash
mkdir third_party
mkdir third_party/freertos
mkdir third_party/rt-thread
mkdir third_party/cmsis-rtx
```

### 步骤 2: 移动 RTOS 源码

```bash
# 移动 FreeRTOS
mv components/kernel/osal/freertos/FreeRTOS/* third_party/freertos/

# 移动 RT-Thread
mv components/kernel/osal/rt-thread/* third_party/rt-thread/

# 移动 CMSIS-RTX
mv components/kernel/osal/CMSIS-RTX/* third_party/cmsis-rtx/
```

### 步骤 3: 重组 osal 目录

```bash
# 创建新结构
mkdir components/kernel/osal/include
mkdir components/kernel/osal/src
mkdir components/kernel/osal/backend

# 移动头文件
mv components/kernel/osal/xy_os.h components/kernel/osal/include/
mv components/kernel/osal/xy_os_cfg.h components/kernel/osal/include/
mv components/kernel/misc/xy_tick.h components/kernel/osal/include/xy_os_tick.h
mv components/kernel/misc/xy_timer_sw.h components/kernel/osal/include/xy_os_timer_sw.h

# 移动源文件
mv components/kernel/misc/xy_tick.c components/kernel/osal/src/xy_os_tick.c
mv components/kernel/misc/xy_timer_sw.c components/kernel/osal/src/xy_os_timer_sw.c

# 移动后端适配层
mv components/kernel/osal/baremetal/xy_os_baremetal.c components/kernel/osal/backend/baremetal/osal_baremetal.c
mv components/kernel/osal/freertos/xy_os_freertos.c components/kernel/osal/backend/freertos/osal_freertos.c
mv components/kernel/osal/rt-thread/xy_os_rtthread.c components/kernel/osal/backend/rtthread/osal_rtthread.c
```

### 步骤 4: 更新构建配置

更新 `components/kernel/osal/CMakeLists.txt`:
- 包含目录指向 `include/`
- 源文件指向 `src/` 和 `backend/`
- 第三方头文件指向 `third_party/`

### 步骤 5: 清理冗余文件

```bash
# 删除重复的 Kconfig
rm components/kernel/osal/Kconfig

# 删除空的 RTOS 目录
rm -rf components/kernel/osal/freertos/FreeRTOS
rm -rf components/kernel/osal/rt-thread/components
rm -rf components/kernel/osal/rt-thread/include
rm -rf components/kernel/osal/rt-thread/libcpu
rm -rf components/kernel/osal/rt-thread/src
rm -rf components/kernel/osal/CMSIS-RTX
```

---

## 构建集成

### 顶层 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.12)
project(my_project C)

# MCU 配置
set(MCU_FAMILY "STM32U5" CACHE STRING "MCU Family")

# RTOS 选择
set(RTOS_BACKEND "freertos" CACHE STRING "RTOS backend")

# 添加第三方库
add_subdirectory(third_party)

# 添加 OSAL
set(OSAL_BACKEND ${RTOS_BACKEND} CACHE STRING "OSAL backend")
add_subdirectory(components/kernel/osal)

# 添加应用
add_executable(my_app src/main.c)
target_link_libraries(my_app xy_osal)

# 根据 RTOS 链接对应库
if(RTOS_BACKEND STREQUAL "freertos")
    target_link_libraries(my_app freertos_kernel)
elseif(RTOS_BACKEND STREQUAL "rtthread")
    target_link_libraries(my_app rtthread)
elseif(RTOS_BACKEND STREQUAL "cmsis_rtx")
    target_link_libraries(my_app rtx5)
endif()
```

### 使用示例

```bash
# 裸机构建
cmake .. -DRTOS_BACKEND=baremetal -DOSAL_BACKEND=baremetal

# FreeRTOS 构建
cmake .. -DRTOS_BACKEND=freertos -DOSAL_BACKEND=freertos

# RT-Thread 构建
cmake .. -DRTOS_BACKEND=rtthread -DOSAL_BACKEND=rtthread
```

---

## 文件对比

### OSAL 目录大小对比

| 项目 | 优化前 | 优化后 | 变化 |
|------|--------|--------|------|
| **目录数** | ~50 | ~10 | -80% |
| **文件数** | ~500 | ~50 | -90% |
| **代码行数** | ~100K | ~5K | -95% |

### third_party 目录

| RTOS | 目录数 | 文件数 | 许可证 |
|------|--------|--------|--------|
| **FreeRTOS** | ~20 | ~100 | MIT |
| **RT-Thread** | ~30 | ~200 | Apache-2.0 |
| **CMSIS-RTX** | ~10 | ~50 | Apache-2.0 |

---

## 总结

### 优化成果

✅ **清晰的代码分离**: OSAL 适配层与 RTOS 源码完全分离
✅ **独立的版本管理**: 每个 RTOS 有独立的版本和许可证
✅ **易于维护**: 更新 RTOS 不影响 OSAL 代码
✅ **灵活选择**: 根据项目需求选择 RTOS
✅ **许可证清晰**: 每个 RTOS 有自己的 LICENSE 文件
✅ **精简的 OSAL 目录**: 仅包含适配层代码

### 下一步

1. ✅ 创建 third_party 目录结构
2. ✅ 创建 Kconfig 和 CMakeLists.txt
3. ⏳ 移动 RTOS 源码到 third_party
4. ⏳ 更新 OSAL 构建配置
5. ⏳ 添加单元测试
6. ⏳ 生成 Doxygen 文档

---

## 参考

- [Third Party README](../../third_party/README.md)
- [OSAL README](components/kernel/osal/README.md)
- [构建指南](docs/build_guide.md)
