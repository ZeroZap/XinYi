# XinYi 框架使用指南

## 快速开始

### 1. 项目结构

```
XinYi/
├── components/           # 核心组件
│   ├── kernel/           # 内核组件 (osal)
│   ├── hal/              # 硬件抽象层
│   ├── clib/             # C 标准库
│   ├── crypto/           # 密码学
│   ├── dm/               # 数据管理
│   ├── net/              # 网络
│   └── trace/            # 跟踪日志
├── third_party/          # 第三方库
│   ├── freertos/
│   ├── rt-thread/
│   └── unity/            # 测试框架
├── tests/                # 统一测试入口
├── docs/                 # 文档
└── .qwen/                # 智能代理
    └── smart_agent.sh
```

### 2. 智能代理系统

#### 项目管理
```bash
# 查看组件状态
./.qwen/smart_agent.sh pm status

# 查看待办任务
./.qwen/smart_agent.sh pm tasks

# 搜索代码
./.qwen/smart_agent.sh pm search "xy_hal_uart_init"

# 项目统计
./.qwen/smart_agent.sh pm stats
```

#### 架构分析
```bash
# 审查组件
./.qwen/smart_agent.sh arch review hal

# 依赖分析
./.qwen/smart_agent.sh arch deps osal

# 代码质量检查
./.qwen/smart_agent.sh arch check
```

#### 开发辅助
```bash
# 创建新组件
./.qwen/smart_agent.sh dev create my_component

# 生成文档
./.qwen/smart_agent.sh dev docs hal

# 生成测试
./.qwen/smart_agent.sh test gen hal
```

## 代码使用示例

### 1. OSAL 使用

```c
#include "xy_os.h"

void app_main(void)
{
    // 初始化 OS
    xy_os_kernel_init();
    
    // 创建线程 (RT-Thread/FreeRTOS 后端)
    #if !defined(XY_OS_BACKEND_BAREMETAL)
    xy_os_thread_attr_t attr = {
        .name = "main_thread",
        .priority = XY_OS_PRIORITY_NORMAL,
        .stack_size = 2048,
    };
    xy_os_thread_new(my_task, NULL, &attr);
    #endif
    
    // 启动调度器 (RT-Thread/FreeRTOS 后端)
    #if !defined(XY_OS_BACKEND_BAREMETAL)
    xy_os_kernel_start();
    #else
    // 裸机模式下直接运行主循环
    while (1) {
        // 应用逻辑
        xy_os_delay(1000);
    }
    #endif
}
```

### 2. HAL 使用

```c
#include "xy_hal.h"

void hal_example(void)
{
    // GPIO 使用
    xy_hal_pin_config_t config = {
        .mode = XY_HAL_PIN_MODE_OUTPUT,
        .pull = XY_HAL_PIN_PULL_NONE,
        .otype = XY_HAL_PIN_OTYPE_PP,
        .speed = XY_HAL_PIN_SPEED_HIGH,
    };
    xy_hal_pin_init(GPIOA, 5, &config);
    xy_hal_pin_write(GPIOA, 5, XY_HAL_PIN_HIGH);
    
    // UART 使用
    xy_hal_uart_config_t uart_config = {
        .baudrate = 115200,
        .wordlen = XY_HAL_UART_WORDLEN_8B,
        .stopbits = XY_HAL_UART_STOPBITS_1,
        .parity = XY_HAL_UART_PARITY_NONE,
        .flowctrl = XY_HAL_UART_FLOWCTRL_NONE,
        .mode = XY_HAL_UART_MODE_TX_RX,
    };
    xy_hal_uart_init(&huart1, &uart_config);
    
    const char *msg = "Hello World\r\n";
    xy_hal_uart_send(&huart1, (uint8_t *)msg, strlen(msg), 1000);
}
```

### 3. 构建系统

#### CMake 构建
```bash
# 选择 RTOS 后端
mkdir build && cd build
cmake .. -DRTOS_BACKEND=freertos
make
```

#### 配置选项
```bash
# RTOS 选择
-DRTOS_BACKEND=baremetal    # 裸机模式
-DRTOS_BACKEND=freertos     # FreeRTOS
-DRTOS_BACKEND=rtthread     # RT-Thread
-DRTOS_BACKEND=cmsis_rtx    # CMSIS-RTX

# 构建选项
-DBUILD_TESTING=ON          # 构建测试
-DCMAKE_BUILD_TYPE=Debug    # 调试构建
```

## 优化特性

### 1. 多后端支持
- **Bare-metal**: 无 RTOS，最小资源占用
- **FreeRTOS**: 业界标准 RTOS
- **RT-Thread**: 国产 RTOS，功能丰富
- **CMSIS-RTX**: ARM 官方 RTOS

### 2. 统一接口
- 所有组件使用统一 API
- 标准化错误码
- 一致的参数命名

### 3. 智能开发
- 智能代理系统
- 统一测试框架
- 自动化构建

## 文档资源

- [API 参考](docs/api/)
- [架构文档](docs/design/)
- [使用指南](docs/getting-started/)
- [组件状态](COMPONENTS_STATUS.md)
- [测试布局](docs/test_layout_analysis.md)
- [构建系统](docs/build_system_analysis.md)

## 维护者

- **团队**: XinYi Team
- **邮箱**: zerozap2020@gmail.com
- **主页**: https://github.com/zerozap

## 许可证

Apache License 2.0
