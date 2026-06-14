# 工具链

XinYi 的构建、测试和开发工具。

---

## 📖 导航

- [构建系统](#构建系统)
- [测试框架](#测试框架)
- [CI/CD](#cicd)
- [代码风格](#代码风格)

---

## 构建系统

### CMake

XinYi 使用 CMake 作为主要构建系统。

```bash
# 配置
mkdir build && cd build
cmake .. -DBUILD_TESTING=ON

# 构建
make -j$(nproc)

# 测试
make test
```

### Makefile

也提供 GNU Make 支持：

```bash
# 构建所有
make

# 构建特定组件
make crypto
make osal

# 清理
make clean
make distclean
```

### Kconfig

使用 Kconfig 进行编译时配置：

```bash
# 交互式配置
make menuconfig

# 设置特定选项
make CONFIG_CRYPTO_AES=y
```

---

## 测试框架

### Unity

XinYi 使用 Unity 单元测试框架。

```c
#include "unity.h"

void test_my_function(void) {
    int result = my_function(42);
    TEST_ASSERT_EQUAL(0, result);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_my_function);
    return UNITY_END();
}
```

### 运行测试

```bash
# 所有测试
ctest --output-on-failure

# 特定测试
ctest -R test_crypto --output-on-failure

# 详细输出
ctest --verbose
```

### 覆盖率

```bash
# 生成覆盖率报告
./tools/scripts/coverage.sh

# 查看 HTML 报告
open coverage-report.html
```

---

## CI/CD

### GitHub Actions

XinYi 使用 GitHub Actions 进行持续集成和部署。

**工作流**:
- `.github/workflows/ci-cd.yml` - 主 CI/CD 工作流
- `.github/workflows/deploy-docs.yml` - 文档部署

**功能**:
- ✅ 多平台构建 (Ubuntu/Windows/macOS)
- ✅ 自动化测试
- ✅ 代码质量检查
- ✅ 文档生成和部署

---

## 代码风格

### 规范

参考 [代码风格规范](../contribute/code-style.md)。

### 工具

```bash
# 格式化代码
./tools/scripts/format_code.sh

# 检查风格
./tools/scripts/check_style.sh
```

### 配置

- `.clang-format` - Clang 格式化配置
- `.editorconfig` - 编辑器配置

---

## 实用工具

### 构建脚本

| 脚本 | 说明 |
|------|------|
| `tools/scripts/build.sh` | Linux/macOS 构建 |
| `tools/scripts/build.bat` | Windows 构建 |
| `tools/scripts/run_tests.sh` | 运行测试 |
| `tools/scripts/coverage.sh` | 生成覆盖率 |
| `tools/scripts/format_code.sh` | 格式化代码 |
| `tools/scripts/check_style.sh` | 检查风格 |

---

*最后更新：2026-02-28 | 维护者：XinYi Team*
