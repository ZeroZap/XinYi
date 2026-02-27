# 常见问题 (FAQ)

常见问题解答。

---

## 📖 目录

- [一般问题](#一般问题)
- [安装和构建](#安装和构建)
- [使用问题](#使用问题)
- [故障排除](#故障排除)
- [贡献相关](#贡献相关)

---

## 一般问题

### XinYi 是什么？

XinYi 是一个模块化、生产级的嵌入式 C 框架，专为资源受限的微控制器系统设计。它提供硬件、通信协议、密码学和 RTOS 管理的统一抽象层。

### XinYi 适合什么场景？

适合以下场景：
- 物联网设备
- 工业控制
- 消费电子
- 传感器节点
- 需要跨平台移植的项目

### 项目采用什么许可证？

Apache License 2.0，可商用、可修改、可分发。

### 支持哪些 MCU？

- **STM32 系列** - 完整支持（U5/F4/F1/L4）
- **HC32 系列** - 占位符
- **WCH 系列** - 占位符
- **PC 仿真** - 开发测试

### 支持哪些 RTOS？

- Bare-metal（裸机）
- FreeRTOS
- RT-Thread
- CMSIS-RTX

---

## 安装和构建

### 如何获取项目？

```bash
git clone https://github.com/ZeroZap/XinYi.git
cd XinYi
git submodule update --init --recursive
```

### 构建需要什么工具？

| 工具 | 版本要求 |
|------|---------|
| GCC | 9.0+ |
| CMake | 3.10+ |
| Git | 2.0+ |

### 如何构建项目？

```bash
# 创建构建目录
mkdir build && cd build

# 配置 CMake
cmake .. -DBUILD_TESTING=ON

# 构建
make -j$(nproc)
```

### 如何构建特定组件？

```bash
# 使用 Make
make crypto
make osal

# 使用 CMake
cd build
make xy_crypto
make xy_osal
```

### 如何清理构建？

```bash
# 清理
make clean

# 完全清理
make distclean

# 或删除 build 目录
rm -rf build/
```

---

## 使用问题

### 如何开始使用？

参考 [快速开始指南](../getting-started/quickstart.md)。

### 如何配置 RTOS 后端？

```bash
# 使用 Kconfig
make menuconfig
# 选择 Kernel -> OSAL -> Backend
```

或修改配置文件：
```c
// xy_os_cfg.h
#define XY_OS_BACKEND XY_OS_BACKEND_FREERTOS
```

### 如何配置平台？

```c
// xy_hal_cfg.h
#define XY_HAL_PLATFORM STM32
// #define XY_HAL_PLATFORM HC32
// #define XY_HAL_PLATFORM PC_SIM
```

### 如何使用日志系统？

```c
#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG
#include "xy_log.h"

void my_function(void) {
    xy_log_d("调试信息：%d\n", value);
    xy_log_i("信息：%s\n", str);
    xy_log_e("错误：操作失败\n");
}
```

### 如何运行测试？

```bash
cd build
make test
# 或
ctest --output-on-failure

# 运行特定测试
ctest -R test_crypto --output-on-failure
```

### 如何生成覆盖率报告？

```bash
./utils/script/coverage.sh
```

---

## 故障排除

### 编译失败怎么办？

检查以下几点：
1. 确认 CMake 版本 >= 3.10
2. 确认子模块已初始化：`git submodule update --init`
3. 清理构建目录重新构建

### 测试失败怎么办？

1. 确认 Unity 框架已正确引入
2. 查看测试输出日志
3. 在 GitHub 报告问题

### 如何调试？

```bash
# 启用调试构建
cmake .. -DCMAKE_BUILD_TYPE=Debug

# 使用 GDB
gdb ./build/your_target
```

### 如何启用详细日志？

```c
#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_VERBOSE
#include "xy_log.h"
```

### 代码风格检查失败？

```bash
# 格式化代码
./utils/script/format_code.sh

# 检查风格
./utils/script/check_style.sh
```

---

## 贡献相关

### 如何贡献代码？

参考 [贡献指南](../contribute/index.md)。

### 提交信息有什么要求？

```
<type>(<scope>): <subject>

<body>

<footer>
```

Type 类型：
- `feat` - 新功能
- `fix` - Bug 修复
- `docs` - 文档更新
- `style` - 代码格式
- `refactor` - 重构
- `test` - 测试相关
- `chore` - 构建/工具

### 测试有什么要求？

- 新功能必须添加测试
- 现有代码修改不能破坏现有测试
- 目标覆盖率：>80%

### 代码风格要求？

参考 [代码风格规范](../contribute/code-style.md)。

主要要求：
- C99 标准
- 4 空格缩进
- 小写命名 + 下划线
- Doxygen 注释

---

## 📞 还需要帮助？

- 📚 [文档索引](index.md)
- 💬 [GitHub Discussions](https://github.com/ZeroZap/XinYi/discussions)
- 🐛 [报告问题](https://github.com/ZeroZap/XinYi/issues)

---

*最后更新：2026-02-28 | 维护者：XinYi Team*
