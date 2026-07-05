# XinYi 贡献指南

**版本**: 1.0.0  
**日期**: 2026-03-02

---

## 📋 目录

1. [开发环境搭建](#开发环境搭建)
2. [代码规范](#代码规范)
3. [提交流程](#提交流程)
4. [测试要求](#测试要求)
5. [文档规范](#文档规范)

---

## 🛠️ 开发环境搭建

### 必需工具

| 工具 | 版本 | 用途 |
|------|------|------|
| GCC | 9.0+ | 编译器 |
| CMake | 3.10+ | 构建系统 |
| Git | 2.0+ | 版本控制 |
| clang-format | 10.0+ | 代码格式化 |

### 可选工具

| 工具 | 用途 |
|------|------|
| STM32CubeIDE | STM32 开发 |
| WCH-LinkE | WCH 调试器 |
| VS Code | 代码编辑 |

---

## 📝 代码规范

### 命名规范

```c
/* ✅ 正确 */
int my_function(void);
int my_variable;
#define MAX_BUFFER_SIZE 256
typedef struct { ... } my_struct_t;

/* ❌ 错误 */
int MyFunction(void);      /* 驼峰命名 */
int myVariable;            /* 驼峰命名 */
```

### 注释规范

```c
/**
 * @brief 函数简要描述
 * @param param1 参数 1 描述
 * @param param2 参数 2 描述
 * @return 返回值描述
 */
int my_function(int param1, int param2);
```

### 格式化

使用 clang-format 自动格式化：

```bash
clang-format -i path/to/file.c
```

---

## 🔄 提交流程

### 1. Fork 项目

在 GitHub 上点击 Fork 按钮。

### 2. 克隆仓库

```bash
git clone https://github.com/YOUR_USERNAME/XinYi.git
cd XinYi
git remote add upstream https://github.com/ZeroZap/XinYi.git
```

### 3. 创建分支

```bash
git checkout -b feature/your-feature-name
```

**分支命名规范**:
- `feature/xxx` - 新功能
- `fix/xxx` - Bug 修复
- `docs/xxx` - 文档更新
- `refactor/xxx` - 重构

### 4. 提交更改

```bash
git add .
git commit -m "feat: add your feature description"
```

**提交信息格式**:
```
<type>(<scope>): <subject>

<body>

<footer>
```

**Type 类型**:
- `feat` - 新功能
- `fix` - Bug 修复
- `docs` - 文档更新
- `style` - 代码格式
- `refactor` - 重构
- `test` - 测试相关
- `chore` - 构建/工具

### 5. 推送并创建 PR

```bash
git push origin feature/your-feature-name
```

然后在 GitHub 上创建 Pull Request。

---

## ✅ 测试要求

### 单元测试

所有新功能必须添加单元测试：

```c
void test_my_function(void)
{
    /* 测试代码 */
    TEST_ASSERT_EQUAL(expected, actual);
}
```

### 测试覆盖率

| 组件类型 | 行覆盖率 | 分支覆盖率 |
|---------|---------|-----------|
| 核心组件 | >90% | >80% |
| 一般组件 | >80% | >70% |
| 辅助工具 | >70% | >60% |

### 运行测试

```bash
# 运行所有测试
make test

# 运行特定测试
ctest --test-dir build/tests/unit -R '^crypto_' --output-on-failure

# 生成覆盖率报告
make coverage
```

---

## 📖 文档规范

### README 模板

```markdown
# 组件名称

**版本**: 1.0.0  
**日期**: 2026-03-02

## 📋 概述

简要描述组件功能。

## 🚀 快速开始

### 初始化

```c
int ret = my_component_init(&handle, &config);
```

### 使用示例

```c
my_component_read(&handle, &data);
```

## 📊 API 参考

| 函数 | 说明 |
|------|------|
| `my_component_init` | 初始化 |
| `my_component_read` | 读取数据 |

## 🔗 相关文档

- [相关文档 1](link1.md)
- [相关文档 2](link2.md)
```

---

## 🎯 PR 检查清单

提交 PR 前请确认：

- [ ] 代码遵循项目规范
- [ ] 已运行 clang-format 格式化
- [ ] 添加了必要的单元测试
- [ ] 所有测试通过
- [ ] 更新了相关文档
- [ ] 无编译警告
- [ ] 在目标平台测试过

---

## 📞 获取帮助

- 📚 [文档索引](docs/README.md)
- ❓ [常见问题](docs/about/faq.md)
- 💬 [GitHub Discussions](https://github.com/ZeroZap/XinYi/discussions)
- 🐛 [报告问题](https://github.com/ZeroZap/XinYi/issues)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
