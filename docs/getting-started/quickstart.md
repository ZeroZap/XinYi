# 快速开始

欢迎使用 XinYi 嵌入式框架！本指南将帮助你在 5 分钟内上手。

---

## 📋 前置要求

### 硬件要求

- 任意 STM32 开发板（推荐 STM32U5/Nucleo 系列）
- 或 PC（用于仿真测试）

### 软件要求

| 工具 | 版本 | 说明 |
|------|------|------|
| GCC | 9.0+ | 或 ARM 工具链 |
| CMake | 3.10+ | 构建系统 |
| Git | 2.0+ | 版本控制 |
| Python | 3.8+ | 工具脚本 |

---

## 🚀 5 分钟快速上手

### 步骤 1: 克隆项目

```bash
git clone https://github.com/ZeroZap/XinYi.git
cd XinYi
git submodule update --init --recursive
```

### 步骤 2: 创建第一个项目

创建 `my_project/main.c`：

```c
#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_INFO
#include "xy_log.h"
#include "xy_os.h"

int main(void) {
    xy_log_i("Hello, XinYi!\n");
    
    xy_os_kernel_init();
    
    xy_log_i("XinYi initialized successfully!\n");
    
    while (1) {
        xy_os_delay(1000);
        xy_log_i("Tick...\n");
    }
    
    return 0;
}
```

### 步骤 3: 构建项目

```bash
# 创建构建目录
mkdir build && cd build

# 配置 CMake
cmake .. \
    -DBUILD_TESTING=ON \
    -DCMAKE_BUILD_TYPE=Release

# 构建
make -j$(nproc)
```

### 步骤 4: 运行测试

```bash
# 运行所有测试
make test

# 或运行特定测试
ctest -R test_crypto --output-on-failure
```

### 步骤 5: 运行项目

```bash
# PC 仿真
./build/my_project

# 或部署到开发板
make flash
```

---

## 📁 项目结构

```
XinYi/
├── components/          # 核心组件
│   ├── kernel/osal/    # OS 抽象层
│   ├── hal/            # 硬件抽象层
│   ├── crypto/         # 密码学
│   └── ...
├── tests/              # 单元测试
├── projects/           # 应用示例
├── utils/script/       # 工具脚本
└── docs/               # 文档
```

---

## 🛠️ 常用命令

### 构建相关

```bash
# 完整构建
make

# 构建特定组件
make crypto
make osal

# 清理
make clean
make distclean
```

### 测试相关

```bash
# 运行所有测试
make test

# 运行特定测试
ctest -R test_crypto

# 生成覆盖率
./utils/script/coverage.sh
```

### 代码质量

```bash
# 格式化代码
./utils/script/format_code.sh

# 检查代码风格
./utils/script/check_style.sh
```

---

## 📖 下一步

### 👶 新手路径

1. [了解项目特性](getting-started/features.md)
2. [搭建开发环境](getting-started/toolchain.md)
3. [运行示例代码](samples/hello-world.md)

### 🔧 开发者路径

1. [阅读组件文档](components/index.md)
2. [查看 API 参考](api/index.md)
3. [学习示例代码](samples/index.md)

### 📟 硬件工程师路径

1. [查看支持的板卡](hardware/boards.md)
2. [阅读移植指南](hardware/porting.md)
3. [配置设备树](hardware/device-tree.md)

---

## ❓ 遇到问题？

### 常见问题

**Q: 编译失败怎么办？**

A: 检查以下几点：
1. 确认 CMake 版本 >= 3.10
2. 确认子模块已初始化
3. 清理构建目录重新构建

**Q: 测试失败怎么办？**

A: 检查以下几点：
1. 确认 Unity 框架已正确引入
2. 查看测试输出日志
3. 在 GitHub 报告问题

**Q: 如何贡献代码？**

A: 参考 [贡献指南](../contribute/index.md)

---

## 📞 获取帮助

- 📚 [文档索引](index.md)
- ❓ [常见问题](../about/faq.md)
- 💬 [GitHub Discussions](https://github.com/ZeroZap/XinYi/discussions)
- 🐛 [报告问题](https://github.com/ZeroZap/XinYi/issues)

---

*最后更新：2026-02-28 | 文档版本：1.0*
