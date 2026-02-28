# 提交 PR 指南

**最后更新**: 2026-02-28

---

## 📋 概述

本指南介绍如何为 XinYi 项目提交 Pull Request (PR)。

---

## 🚀 提交流程

### 1. Fork 项目

在 GitHub 上点击 Fork 按钮创建你的副本。

### 2. 克隆仓库

```bash
git clone https://github.com/YOUR_USERNAME/XinYi.git
cd XinYi
git remote add upstream https://github.com/ZeroZap/XinYi.git
```

### 3. 创建分支

```bash
# 同步主分支
git checkout main
git pull upstream main

# 创建功能分支
git checkout -b feature/your-feature-name
```

**分支命名规范**:
- `feature/xxx` - 新功能
- `fix/xxx` - Bug 修复
- `docs/xxx` - 文档更新
- `refactor/xxx` - 重构
- `test/xxx` - 测试相关

### 4. 进行修改

- 遵循 [代码风格](code-style.md)
- 添加必要的测试
- 更新相关文档

### 5. 提交更改

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

### 6. 推送并创建 PR

```bash
git push origin feature/your-feature-name
```

然后在 GitHub 上创建 Pull Request。

---

## 📝 PR 模板

```markdown
## 描述
简要描述此 PR 的目的。

## 变更类型
- [ ] 新功能
- [ ] Bug 修复
- [ ] 文档更新
- [ ] 重构
- [ ] 测试

## 测试
- [ ] 已添加单元测试
- [ ] 所有测试通过
- [ ] 已在目标平台测试

## 检查清单
- [ ] 遵循代码风格
- [ ] 已格式化代码
- [ ] 已更新文档
- [ ] 无编译警告

## 相关问题
Fixes #123
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

## 📚 相关文档

- [贡献指南](index.md)
- [代码风格](code-style.md)
- [编码规范](../rules/RULEBOOK.md)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
