# 工具链配置

**最后更新**: 2026-02-28

---

## 📋 概述

XinYi 支持多种工具链和构建系统。

---

## 🔧 支持的编译器

| 编译器 | 版本 | 平台 |
|--------|------|------|
| ARM GCC | 9.0+ | STM32/ARM |
| GCC | 9.0+ | Linux/PC |
| Clang | 10.0+ | macOS/Linux |
| IAR | 8.0+ | STM32 |
| Keil | 5.0+ | STM32 |

---

## 🏗️ 构建系统

### CMake

**推荐** 使用 CMake 进行构建：

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Make

传统 Make 构建：

```bash
make all
make test
```

---

## ⚙️ 配置选项

### CMake 选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `BUILD_TESTING` | ON | 构建测试 |
| `TEST_COVERAGE` | OFF | 启用覆盖率 |
| `CMAKE_BUILD_TYPE` | Release | 构建类型 |

### Kconfig 选项

```bash
# 交互式配置
make menuconfig

# 设置特定选项
make CONFIG_CRYPTO_AES=y
```

---

## 📚 相关文档

- [构建系统分析](build_system_analysis.md)
- [快速入门](../getting-started/quickstart.md)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
