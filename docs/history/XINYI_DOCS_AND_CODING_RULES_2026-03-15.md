# XinYi 文档与代码规范整理报告

**日期**: 2026-03-15 23:05 GMT+8  
**整理者**: Zero (ese)  
**目的**: 整理 XinYi 项目文档目录结构和代码相关规范

---

## 📁 文档目录结构

### 1. 根目录 docs/

```
docs/
├── getting-started/          # 入门指南
│   ├── introduction.md       # 项目介绍
│   ├── toolchain.md          # 工具链配置
│   ├── DEVELOPER_GUIDE.md    # 开发者指南
│   ├── QUICK_START.md        # 快速开始
│   └── quickstart.md         # 快速开始 (重复)
│
├── hardware/                 # 硬件相关
│   ├── index.md              # 硬件索引
│   ├── boards.md             # 开发板支持
│   ├── porting.md            # 移植指南
│   └── rtos_selection_guide.md  # RTOS 选型指南
│
├── misc/                     # 杂项文档
│   ├── ai-prompts/           # AI 提示词
│   │   ├── review_code.md    # 代码审查提示
│   │   ├── generate_function.md  # 函数生成提示
│   │   ├── fix-bug.md        # 修复 Bug 提示
│   │   └── xx_prompts.md     # 其他提示
│   ├── ai-skills/            # AI 技能
│   │   └── code-reviwer/SKILL.md
│   ├── FAQs/                 # 常见问题
│   │   └── RT-Thread Nano 与完整版有何区别.md
│   ├── 6A 工作流详解.md       # 开发流程
│   ├── C 代码仓库 AI 生成标准模块设计方案.md
│   ├── Software Design and Requirements.md
│   ├── emojis.md             # 表情符号使用
│   └── link.md               # 相关链接
│
├── samples/                  # 示例文档
│   └── index.md
│
├── 核心分析文档
│   ├── completeness_analysis.md          # 完整性分析
│   ├── zephyr_device_analysis.md         # Zephyr 设备分析
│   ├── build_system_unified_analysis.md  # 构建系统统一分析
│   ├── device_architecture_comparison.md # 设备架构对比
│   └── component-roadmap.md              # 组件路线图
│
├── 构建与开发
│   ├── STM32_BUILD_ENV_SETUP.md          # STM32 构建环境
│   └── PENDING_TASKS.md                  # 待办任务
│
└── 报告文档
    ├── DOCS_REORGANIZATION_REPORT.md     # 文档重组报告
    └── DEVICE_DRIVER_EXTENSION_REPORT_2026-03-15.md  # 驱动扩展报告
```

---

### 2. 组件文档 components/*/

#### 设备模型 components/device/
```
components/device/
├── DESIGN_SPEC.md                      # 设计规范 (44KB)
├── DEVICE_ARCHITECTURE.md              # 架构设计 (24KB)
├── DEVICE_MODEL_IMPROVEMENT_REPORT_2026-03-15.md  # 模型改进报告
└── DEVICE_MODEL_PM_ASYNC_REPORT_2026-03-15.md     # 电源管理报告
```

#### 传感器 components/sensor/
```
components/sensor/
├── sensor.md                           # 传感器概述
├── sensor_address.md                   # 传感器地址表
├── drivers.md                          # 驱动说明
├── ReadMe.md                           # 自述文件
├── SENSOR_GUIDE.md                     # 传感器指南
├── SENSOR_DEV_PLAN_2026-03-14.md       # 驱动开发计划
├── SENSOR_OPTIMIZATION_PLAN.md         # 优化计划
├── DRIVER_MIGRATION_GUIDE.md           # 驱动迁移指南
├── SENSOR_COMPLETION_REPORT.md         # 完成报告
├── LOW_POWER_SENSORS.md                # 低功耗传感器
└── SENSOR_DEV_PLAN_2026-03-14.md       # 开发计划
```

#### GUI components/gui/
```
components/gui/
├── README.md                           # GUI 使用说明
└── docs/                               # GUI 文档
    ├── Charlie.md                      # Charlie 文档
    ├── led strip effects.md            # LED 灯带效果
    ├── led strip effects-multi-protocols.md
    ├── screen_effects-1to32bit.md      # 屏幕效果
    └── sceen_effects-1or32bit.md
```

#### 加密组件 components/crypto/
```
components/crypto/
├── ReadMe.md                           # 加密组件说明
├── crypto.md                           # 加密概述
├── xy_crc/crc.md                       # CRC 算法
├── xy_chacha/README.md                 # ChaCha 加密
├── xy_25519/
│   ├── README.md                       # Curve25519 说明
│   ├── README_M0.md                    # M0 平台说明
│   └── README_RISCV.md                 # RISC-V 平台说明
└── xy_tiny_boot_crypto.md              # 小 boot 加密
```

#### 系统组件 components/sys/
```
components/sys/xy_state_machine/
└── README.md                           # 状态机说明
```

---

## 📋 代码规范与 Rule

### 1. 编码规范

#### 命名规范
- **文件名**: 小写 + 下划线 (e.g., `xy_hal_gpio.h`)
- **函数名**: 小写 + 下划线 (e.g., `xy_hal_gpio_init()`)
- **类型名**: 小写 + 下划线 + `_t` 后缀 (e.g., `xy_hal_gpio_config_t`)
- **宏定义**: 大写 + 下划线 (e.g., `XY_HAL_GPIO_MODE_INPUT`)
- **变量名**: 小写 + 下划线 (e.g., `gpio_pin`)

#### 文件组织
```
组件名/
├── inc/                    # 公共头文件
│   ├── xy_*.h
│   └── ...
├── src/                    # 源文件
│   ├── xy_*.c
│   └── ...
├── docs/                   # 文档
│   └── *.md
├── tests/                  # 测试代码
│   └── test_*.c
├── CMakeLists.txt          # 构建配置
└── README.md               # 组件说明
```

#### 头文件保护
```c
#ifndef XY_COMPONENT_NAME_H
#define XY_COMPONENT_NAME_H

#ifdef __cplusplus
extern "C" {
#endif

// 内容

#ifdef __cplusplus
}
#endif

#endif /* XY_COMPONENT_NAME_H */
```

---

### 2. 注释规范

#### Doxygen 注释
```c
/**
 * @file xy_hal_gpio.h
 * @brief GPIO 硬件抽象层
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 统一 GPIO API，适用于所有平台
 */

/**
 * @brief 初始化 GPIO
 * @param port GPIO 端口
 * @param pin 引脚号
 * @param config 配置结构
 * @return XY_HAL_OK 成功，其他值失败
 * 
 * @par 使用示例:
 * @code
 * xy_hal_gpio_init(GPIOA, 5, &cfg);
 * @endcode
 */
xy_hal_error_t xy_hal_gpio_init(xy_hal_gpio_port_t port, uint8_t pin,
                                const xy_hal_gpio_config_t *config);
```

#### 函数内注释
```c
/* ==================== 章节分隔符 ==================== */

/**
 * @brief 功能说明
 */
static void function_name(void)
{
    /* 单行注释 */
    
    /*
     * 多行注释
     * 用于复杂逻辑说明
     */
}
```

---

### 3. 错误处理规范

#### 统一错误码
```c
typedef enum {
    XY_OK                     = 0,   /**< 成功 */
    XY_ERROR                  = -1,  /**< 通用错误 */
    XY_ERROR_INVALID_PARAM    = -2,  /**< 参数错误 */
    XY_ERROR_NOT_SUPPORT      = -3,  /**< 不支持 */
    XY_ERROR_TIMEOUT          = -4,  /**< 超时 */
    XY_ERROR_BUSY             = -5,  /**< 忙 */
    XY_ERROR_NO_MEM           = -6,  /**< 内存不足 */
    XY_ERROR_IO               = -7,  /**< IO 错误 */
    XY_ERROR_NOT_INIT         = -8,  /**< 未初始化 */
    XY_ERROR_NOT_FOUND        = -9,  /**< 未找到 */
} xy_error_t;
```

#### 错误处理模式
```c
xy_error_t function_name(void)
{
    /* 参数检查 */
    if (!param) {
        return XY_ERROR_INVALID_PARAM;
    }
    
    /* 主逻辑 */
    if (error_condition) {
        return XY_ERROR_IO;
    }
    
    return XY_OK;
}
```

---

### 4. 内存管理规范

#### 动态内存
```c
/* 分配 */
void *ptr = malloc(size);
if (!ptr) {
    return XY_ERROR_NO_MEM;
}

/* 使用后立即释放 */
free(ptr);
ptr = NULL;
```

#### 静态内存
```c
/* 静态缓冲区 */
static uint8_t buffer[256];

/* 静态对象池 */
static object_t object_pool[OBJECT_MAX_COUNT];
```

---

### 5. 并发与中断安全

#### 临界区保护
```c
/* 进入临界区 */
uint32_t primask = __get_PRIMASK();
__disable_irq();

/* 临界区代码 */

/* 退出临界区 */
__set_PRIMASK(primask);
```

#### 原子操作
```c
/* 使用原子操作 */
__atomic_add_fetch(&counter, 1, __ATOMIC_SEQ_CST);
```

---

### 6. 平台移植规范

#### 条件编译
```c
#if defined(STM32U5)
    /* STM32U5 特定代码 */
#elif defined(CH32U5)
    /* WCH CH32U5 特定代码 */
#elif defined(HC32)
    /* HC32 特定代码 */
#else
    #error "Unsupported platform"
#endif
```

#### 抽象层接口
```c
/* 平台特定实现 */
typedef struct {
    void (*init)(void);
    void (*write)(uint8_t data);
    uint8_t (*read)(void);
} platform_ops_t;

/* 统一调用接口 */
static const platform_ops_t ops = {
    .init = platform_init,
    .write = platform_write,
    .read = platform_read,
};
```

---

### 7. 测试规范

#### 测试文件命名
```
test_<module>.c    # 测试源文件
test_<module>.h    # 测试头文件 (可选)
```

#### 测试用例结构
```c
/**
 * @brief 测试用例：功能描述
 */
static xy_test_result_t test_module_feature(void)
{
    /* 准备测试环境 */
    
    /* 执行测试 */
    XY_TEST_ASSERT(condition);
    
    /* 清理环境 */
    
    return XY_TEST_PASS;
}
```

#### 测试覆盖率要求
- **核心模块**: >80%
- **驱动模块**: >60%
- **工具函数**: >50%

---

### 8. 文档规范

#### README 模板
```markdown
# 组件名称

**版本**: 1.0.0  
**日期**: 2026-03-15  
**维护者**: XinYi Team

## 概述

简要描述组件功能。

## 特性

- ✅ 特性 1
- ✅ 特性 2
- ✅ 特性 3

## 快速开始

```c
#include "xy_component.h"

int main(void)
{
    xy_component_init();
    return 0;
}
```

## API 参考

详见 [API 文档](inc/xy_component.h)

## 示例

详见 [examples/](examples/) 目录。

## 许可证

Apache License 2.0
```

#### 变更日志
```markdown
## [1.0.0] - 2026-03-15

### Added
- 新增功能 1
- 新增功能 2

### Changed
- 修改内容 1
- 修改内容 2

### Fixed
- 修复问题 1
- 修复问题 2
```

---

## 📊 文档统计

| 类别 | 文件数 | 总大小 |
|------|-------|--------|
| **入门指南** | 5 | ~50KB |
| **硬件文档** | 4 | ~30KB |
| **组件文档** | 30+ | ~200KB |
| **分析报告** | 6 | ~100KB |
| **AI 相关** | 8 | ~20KB |
| **总计** | **53+** | **~400KB** |

---

## 🎯 改进建议

### 1. 文档重组
- [ ] 合并重复文档 (如 quickstart.md)
- [ ] 统一文档命名风格
- [ ] 建立文档索引页面

### 2. 规范完善
- [ ] 添加代码审查清单
- [ ] 补充性能优化指南
- [ ] 完善 API 参考文档

### 3. 自动化
- [ ] 集成 Doxygen 自动生成 API 文档
- [ ] 添加文档检查脚本
- [ ] 建立文档更新流程

---

## 📚 核心文档推荐

### 新手必读
1. `docs/getting-started/introduction.md` - 项目介绍
2. `docs/getting-started/QUICK_START.md` - 快速开始
3. `docs/getting-started/toolchain.md` - 工具链配置

### 开发者必读
1. `components/device/DESIGN_SPEC.md` - 设备设计规范
2. `components/device/DEVICE_ARCHITECTURE.md` - 设备架构
3. `components/sensor/SENSOR_GUIDE.md` - 传感器指南

### 架构师必读
1. `docs/device_architecture_comparison.md` - 架构对比
2. `docs/zephyr_device_analysis.md` - Zephyr 分析
3. `docs/completeness_analysis.md` - 完整性分析

---

**报告时间**: 2026-03-15 23:05 GMT+8  
**维护者**: XinYi Team (ese)
