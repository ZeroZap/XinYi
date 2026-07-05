# 贡献指南

欢迎为 XinYi 嵌入式框架做贡献！

---

## 🤝 如何贡献

### 1. Fork 项目

在 GitHub 上点击 Fork 按钮创建你的副本。

### 2. 克隆仓库

```bash
git clone https://github.com/YOUR_USERNAME/XinYi.git
cd XinYi
```

### 3. 创建分支

```bash
git checkout -b feature/your-feature-name
```

### 4. 进行修改

- 遵循 [代码风格](code-style.md)
- 添加必要的测试
- 更新相关文档

### 5. 提交更改

```bash
git add .
git commit -m "feat: add your feature description"
```

### 6. 推送并创建 PR

```bash
git push origin feature/your-feature-name
```

然后在 GitHub 上创建 Pull Request。

---

## 📝 提交信息规范

### 格式

```
<type>(<scope>): <subject>

<body>

<footer>
```

### Type 类型

| Type | 说明 |
|------|------|
| `feat` | 新功能 |
| `fix` | Bug 修复 |
| `docs` | 文档更新 |
| `style` | 代码格式 |
| `refactor` | 重构 |
| `test` | 测试相关 |
| `chore` | 构建/工具 |

### 示例

```
feat(crypto): add SHA-512 support

- Implement SHA-512 algorithm
- Add unit tests
- Update documentation

Fixes: #123
```

---

## 🧪 测试要求

### 运行测试

```bash
# 构建并运行所有测试
mkdir build && cd build
cmake .. -DBUILD_TESTING=ON
make test

# 运行特定测试
ctest --test-dir build/tests/unit -R '^crypto_' --output-on-failure
```

### 代码覆盖率

```bash
# 生成覆盖率报告
./tools/scripts/coverage.sh
```

### 测试要求

- 新功能必须添加测试
- 现有代码修改不能破坏现有测试
- 目标覆盖率：>80%

---

## 💻 代码风格

### 基本要求

- 使用 C99 标准
- 4 空格缩进（禁用制表符）
- 小写命名 + 下划线分隔
- 所有函数必须有 Doxygen 注释

### 示例

```c
/**
 * @brief 函数简要描述
 * @param param1 参数 1 描述
 * @return 返回值描述
 */
int my_function(int param1) {
    xy_log_d("调试信息\n");
    return 0;
}
```

### 格式化代码

```bash
# 格式化所有代码
./tools/scripts/format_code.sh

# 检查代码风格
./tools/scripts/check_style.sh
```

---

## 📖 文档要求

### 代码注释

- 所有公共函数必须有 Doxygen 注释
- 复杂逻辑需要添加行内注释
- 使用英文或中文均可

### 文档更新

- 新功能需要更新相关文档
- API 变更需要更新 API 参考
- 添加示例代码

---

## 🐛 报告 Bug

### Bug 报告模板

```markdown
**描述问题**
简要描述问题

**复现步骤**
1. ...
2. ...
3. ...

**期望行为**
描述期望的结果

**环境信息**
- 编译器：GCC 10.2
- 平台：STM32U5
- 版本：v1.0

**附加信息**
截图、日志等
```

---

## 💡 功能建议

### 建议模板

```markdown
**问题描述**
描述你遇到的问题或需求

**建议方案**
描述你的建议

**替代方案**
描述其他可能的解决方案

**附加信息**
相关参考资料
```

---

## 🔍 Code Review

### 审查要点

- [ ] 代码遵循项目风格
- [ ] 添加了必要的测试
- [ ] 更新了相关文档
- [ ] 没有引入编译警告
- [ ] 没有破坏现有功能

### 反馈处理

- 及时回复审查意见
- 根据意见修改代码
- 推送新的提交

---

## 📜 许可证

本项目采用 Apache License 2.0。提交代码即表示你同意将代码在此许可证下发布。

---

## 📞 获取帮助

- 📚 [文档索引](../index.md)
- ❓ [常见问题](../about/faq.md)
- 💬 [GitHub Discussions](https://github.com/ZeroZap/XinYi/discussions)

---

*感谢你的贡献！* 🎉
