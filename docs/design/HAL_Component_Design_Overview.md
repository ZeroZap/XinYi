# XY HAL 组件设计概览

**文档版本**: 1.0  
**最后更新**: 2026-02-28  
**作者**: XinYi 项目团队

---

## 一、组件概述

### 1.1 核心定位

XY HAL（Hardware Abstraction Layer）是 XinYi 嵌入式框架的硬件抽象层，提供统一的、平台无关的接口来访问 MCU 外设。通过该层，应用代码可以在不同 MCU 平台间无缝移植。

### 1.2 设计原则

- **接口统一**: 所有平台使用相同的函数名和参数风格
- **实现分离**: 平台特定代码隔离在独立目录
- **编译时选择**: 通过构建系统选择目标平台和系列
- **易于扩展**: 新增平台只需实现对应的 C 文件

### 1.3 当前状态

- ✅ 接口定义完成（14 个外设头文件）
- ✅ STM32F4 实现完成
- ✅ 错误码体系建立
- ⏳ 其他 STM32 系列待实现（U0/L4/H7）
- ⏳ 其他平台待实现（HC32/CH32/ESP32）

---

## 二、架构设计

### 2.1 目录结构

```
components/hal/
├── inc/                          # 平台无关接口定义
│   ├── xy_hal.h                 # 主头文件 + 错误码定义
│   ├── xy_hal_pin.h             # GPIO/引脚接口
│   ├── xy_hal_uart.h            # UART 串口接口
│   ├── xy_hal_spi.h             # SPI 总线接口
│   ├── xy_hal_i2c.h             # I2C 总线接口
│   ├── xy_hal_timer.h           # 定时器接口
│   ├── xy_hal_pwm.h             # PWM 输出接口
│   ├── xy_hal_rtc.h             # 实时时钟接口
│   ├── xy_hal_dma.h             # DMA 传输接口
│   ├── xy_hal_lp_timer.h        # 低功耗定时器接口
│   ├── xy_hal_flash.h           # Flash 存储接口
│   ├── xy_hal_gpio.h            # GPIO 扩展接口
│   ├── xy_hal_sys.h             # 系统接口
│   └── xy_hal_tgpio.h           # 触摸 GPIO 接口
│
├── stm32/                        # STM32 平台实现
│   ├── stm32f4/                  # STM32F4 系列（已完成）
│   │   ├── xy_hal_pin.c
│   │   ├── xy_hal_uart.c
│   │   ├── xy_hal_spi.c
│   │   ├── xy_hal_i2c.c
│   │   ├── xy_hal_timer.c
│   │   ├── xy_hal_pwm.c
│   │   ├── xy_hal_rtc.c
│   │   ├── xy_hal_dma.c
│   │   └── xy_hal_lp_timer.c
│   ├── stm32u0/                  # STM32U0 系列（待实现）
│   ├── stm32l4/                  # STM32L4 系列（待实现）
│   ├── stm32h7/                  # STM32H7 系列（待实现）
│   ├── stm32_hal.h              # STM32 HAL 辅助头文件
│   ├── CMakeLists.txt           # CMake 构建配置
│   ├── Makefile.template        # Make 构建模板
│   ├── README.md                # STM32 实现说明
│   └── example_usage.c          # 使用示例
│
├── ch32/                         # CH32 平台实现（待实现）
│   └── ch32x03x/
│
├── hc32/                         # HC32 平台实现（待实现）
│
├── PC/                           # PC 仿真层
│   ├── win32/                    # Windows 仿真
│   └── linux/                    # Linux 仿真
│
├── README.md                     # 主文档
├── IMPLEMENTATION_SUMMARY.md     # 实现总结
├── INTEGRATION_GUIDE.md          # 集成指南
├── RESTRUCTURING_NOTES.md        # 重构说明
├── RETURN_TYPE_UPDATE.md         # 返回类型更新
├── ERROR_CODES.md                # 错误码参考
├── ERROR_CODES_CHANGELOG.md      # 错误码变更日志
└── hal_status.md                 # 支持状态
```

### 2.2 接口设计特点

#### 统一的函数命名规范

```c
xy_hal_<peripheral>_<operation>()
```

#### 统一的错误处理

```c
typedef enum {
    XY_HAL_OK                = 0,
    XY_HAL_ERROR             = -1,
    XY_HAL_ERROR_INVALID_PARAM = -2,
    XY_HAL_ERROR_NOT_SUPPORT = -3,
    XY_HAL_ERROR_TIMEOUT     = -4,
    XY_HAL_ERROR_BUSY        = -5,
    XY_HAL_ERROR_NO_MEM      = -6,
    XY_HAL_ERROR_IO          = -7,
    XY_HAL_ERROR_NOT_INIT    = -8,
    XY_HAL_ERROR_ALREADY_INIT = -9,
    XY_HAL_ERROR_NO_RESOURCE = -10,
    XY_HAL_ERROR_FAIL        = -11,
} xy_hal_error_t;
```

#### 返回类型约定

- **控制操作**: 返回 `xy_hal_error_t`（成功返回 0，失败返回负数）
- **查询操作**: 返回 `int`（成功返回数据值，失败返回负数）

---

## 三、支持的外设

### 3.1 GPIO/引脚（xy_hal_pin.h）

- 输入/输出/复用/模拟模式
- 上拉/下拉/浮空配置
- 推挽/开漏输出
- 速度配置（低/中/高/超高）
- 外部中断支持（上升沿/下降沿/双边沿）
- 中断回调机制

### 3.2 UART（xy_hal_uart.h）

- 可配置波特率、数据位、停止位、奇偶校验
- 硬件流控（RTS/CTS）
- 阻塞式发送/接收
- DMA 非阻塞传输
- 事件回调机制

### 3.3 SPI（xy_hal_spi.h）

- 4 种 SPI 模式（Mode 0-3）
- 主/从模式
- 8/16 位数据宽度
- MSB/LSB 优先
- 软件/硬件片选
- DMA 传输支持

### 3.4 I2C（xy_hal_i2c.h）

- 7 位/10 位地址模式
- 标准（100kHz）和快速（400kHz）模式
- 内存读写操作（寄存器访问）
- 设备就绪检测
- DMA 传输支持

### 3.5 定时器（xy_hal_timer.h）

- 向上/向下/中心对齐计数模式
- 可配置预分频器和周期
- 自动重载预加载
- 时钟分频配置
- 中断支持
- 捕获/比较事件

### 3.6 PWM（xy_hal_pwm.h）

- 可配置频率
- 精细的占空比控制（0.01% 精度）
- 极性配置
- 动态调整占空比

### 3.7 RTC（xy_hal_rtc.h）

- 时间和日期管理
- BCD 和二进制格式
- 闹钟 A 和闹钟 B
- Unix 时间戳转换
- 中断支持

### 3.8 DMA（xy_hal_dma.h）

- 外设到内存/内存到外设/内存到内存
- 普通/循环模式
- 优先级配置
- 8/16/32 位数据宽度
- 地址自增控制
- 传输完成回调

### 3.9 低功耗定时器（xy_hal_lp_timer.h）

- 内部/外部时钟源
- 8 级预分频器
- 适用于低功耗应用

### 3.10 其他接口

- **Flash**（xy_hal_flash.h）：Flash 存储操作
- **GPIO 扩展**（xy_hal_gpio.h）：GPIO 端口级操作
- **系统**（xy_hal_sys.h）：系统级操作
- **触摸 GPIO**（xy_hal_tgpio.h）：触摸传感器接口

---

## 四、平台支持矩阵

### 4.1 STM32 系列

| 系列 | 目录 | 状态 | 说明 |
|------|------|------|------|
| STM32F4 | `stm32f4/` | ✅ 完成 | F401/F407/F411 等 |
| STM32U0 | `stm32u0/` | ⏳ 待实现 | 超低功耗系列 |
| STM32L4 | `stm32l4/` | ⏳ 待实现 | 低功耗系列 |
| STM32H7 | `stm32h7/` | ⏳ 待实现 | 高性能系列 |
| STM32F1 | `stm32f1/` | 📋 可扩展 | 经典系列 |
| STM32F7 | `stm32f7/` | 📋 可扩展 | 高性能系列 |
| STM32G0 | `stm32g0/` | 📋 可扩展 | 主流系列 |
| STM32G4 | `stm32g4/` | 📋 可扩展 | 数字电源系列 |
| STM32WB | `stm32wb/` | 📋 可扩展 | 无线系列 |
| STM32WL | `stm32wl/` | 📋 可扩展 | LoRa 系列 |

### 4.2 其他平台

| 平台 | 目录 | 状态 | 说明 |
|------|------|------|------|
| CH32 | `ch32/` | ⏳ 待实现 | WCH 国产 MCU |
| HC32 | `hc32/` | ⏳ 待实现 | 华大国产 MCU |
| ESP32 | - | 📋 可扩展 | Espressif WiFi MCU |
| Linux | `PC/linux/` | 📋 可扩展 | Linux 仿真 |
| Win32 | `PC/win32/` | ⏳ 部分实现 | Windows 仿真 |

---

## 五、关键设计决策

### 5.1 函数命名统一

- **决策**: 实现文件中的函数名与接口声明完全一致，不包含平台后缀
- **原因**:
  - 简化函数命名
  - 通过目录结构区分不同系列
  - 便于跨平台代码编写
  - 降低学习成本

### 5.2 编译时平台选择

- **决策**: 通过构建系统变量选择目标平台和系列
- **实现**:
  - CMake：`-DSTM32_SERIES=stm32f4`
  - Make：`STM32_SERIES=stm32f4`
  - IDE：项目配置中指定

### 5.3 错误码体系

- **决策**: 统一的错误码枚举，所有平台遵循相同的错误语义
- **优势**:
  - 一致的错误处理
  - 易于调试和诊断
  - 便于错误传播

### 5.4 返回类型区分

- **控制操作**: 返回 `xy_hal_error_t`（0 表示成功）
- **查询操作**: 返回 `int`（正数表示数据，负数表示错误）
- **优势**: 清晰的语义，避免歧义

---

## 六、集成方式

### 6.1 Makefile 集成

```makefile
XY_HAL_DIR = components/hal
XY_HAL_INC = $(XY_HAL_DIR)/inc
XY_HAL_STM32 = $(XY_HAL_DIR)/stm32
STM32_SERIES = stm32f4

C_INCLUDES += -I$(XY_HAL_INC)
C_INCLUDES += -I$(XY_HAL_STM32)
C_SOURCES += $(wildcard $(XY_HAL_STM32)/$(STM32_SERIES)/*.c)
C_DEFS += -DSTM32_HAL_ENABLED
```

### 6.2 CMake 集成

```cmake
set(XY_HAL_DIR ${CMAKE_SOURCE_DIR}/components/hal)
set(STM32_SERIES "stm32f4" CACHE STRING "STM32 Series")

include_directories(
    ${XY_HAL_DIR}/inc
    ${XY_HAL_DIR}/stm32
)

file(GLOB XY_HAL_SOURCES
    "${XY_HAL_DIR}/stm32/${STM32_SERIES}/*.c"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE STM32_HAL_ENABLED)
```

### 6.3 IDE 集成

- Keil MDK：添加 Group，选择对应系列的源文件
- IAR EWARM：添加源文件路径和宏定义
- STM32CubeIDE：配置 CMakeLists.txt

---

## 七、扩展指南

### 7.1 添加新的 STM32 系列

1. **创建目录**: `stm32/stm32xx/`
2. **实现所有外设**: 复制参考实现并修改
3. **函数名保持一致**: 与 `inc/` 中的声明完全相同
4. **适配 STM32 HAL**: 使用对应系列的 HAL API
5. **测试编译**: 验证编译无误

### 7.2 添加新平台

1. **创建平台目录**: `<platform>/`
2. **实现所有接口**: `xy_hal_*.c`
3. **创建平台头文件**: `<platform>_hal.h`
4. **更新构建脚本**: 支持新平台选择
5. **编写文档**: 平台特定的说明

### 7.3 添加新外设

1. **创建接口头文件**: `inc/xy_hal_<peripheral>.h`
2. **定义接口函数**: 遵循命名规范
3. **实现平台代码**: `<platform>/xy_hal_<peripheral>.c`
4. **更新主头文件**: `inc/xy_hal.h`
5. **编写文档和示例**

---

## 八、文档体系

### 8.1 现有文档

- **README.md**: 主文档，概述和使用说明
- **IMPLEMENTATION_SUMMARY.md**: 实现总结
- **INTEGRATION_GUIDE.md**: 集成指南
- **RESTRUCTURING_NOTES.md**: 重构说明
- **RETURN_TYPE_UPDATE.md**: 返回类型更新
- **ERROR_CODES.md**: 错误码参考
- **ERROR_CODES_CHANGELOG.md**: 错误码变更日志
- **hal_status.md**: 支持状态

### 8.2 文档维护原则

- 保持文档与代码同步
- 每个重大变更更新相关文档
- 提供清晰的示例代码
- 记录平台特定的注意事项

---

## 九、质量保证

### 9.1 编译检查

- 所有头文件无编译错误
- 实现文件无编译警告
- 跨平台编译验证

### 9.2 功能测试

- 单元测试（每个外设）
- 集成测试（多外设协作）
- 平台验证（实际硬件）

### 9.3 代码审查

- 接口设计审查
- 实现代码审查
- 文档完整性审查

---

## 十、后续规划

### 10.1 短期（Phase 1）

- [ ] 完成 STM32U0/L4/H7 系列实现
- [ ] 补充 Flash/GPIO 扩展接口实现
- [ ] 完善单元测试

### 10.2 中期（Phase 2）

- [ ] 实现 CH32 平台支持
- [ ] 实现 HC32 平台支持
- [ ] 优化 DMA 传输性能

### 10.3 长期（Phase 3）

- [ ] 支持 ESP32 平台
- [ ] 支持 Nordic nRF 系列
- [ ] 建立完整的测试框架

---

## 十一、关键指标

### 11.1 代码质量

- 接口覆盖率：100%（所有声明都有实现）
- 文档完整度：100%（每个接口都有文档）
- 编译成功率：100%（所有平台编译无误）

### 11.2 性能指标

- 函数调用开销：最小化（直接映射到 HAL）
- 内存占用：最小化（无额外缓冲）
- 中断延迟：保持原生 HAL 水平

### 11.3 可维护性

- 代码重复率：最小化（通过公共代码）
- 文档更新频率：与代码同步
- 测试覆盖率：>80%

---

## 十二、相关资源

### 12.1 参考文档

- STM32 HAL 库文档
- MCU 数据手册
- 硬件设计指南

### 12.2 工具链

- ARM GCC 编译器
- CMake/Make 构建系统
- STM32CubeIDE/Keil/IAR IDE

### 12.3 测试工具

- 逻辑分析仪
- 示波器
- 调试器（J-Link/ST-Link）

---

**文档版本**: 1.0  
**最后更新**: 2026-02-28  
**作者**: XinYi Project Team
