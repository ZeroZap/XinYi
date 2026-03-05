# Git 推送指南

**日期**: 2026-03-05  
**状态**: ⚠️ 需要 SSH 密钥配置

---

## 📊 当前状态

### 本地提交

- **总提交数**: 54 个 (领先远程)
- **最近提交**: 6ac25969 - docs: 添加近期修改总结报告
- **用户**: zerozap <zerozap2020@gmail.com>

### 远程仓库

- **URL**: git@github.com:ZeroZap/XinYi.git
- **分支**: main
- **状态**: ⚠️ SSH 密钥未配置

---

## ⚠️ 推送失败原因

```
git@github.com: Permission denied (publickey).
fatal: Could not read from remote repository.
```

**原因**: SSH 密钥未配置或无效

---

## 🔧 解决方案

### 方案 1: 配置 SSH 密钥 (推荐)

```bash
# 1. 生成 SSH 密钥
ssh-keygen -t ed25519 -C "zerozap2020@gmail.com"

# 2. 添加 SSH 密钥到 ssh-agent
eval "$(ssh-agent -s)"
ssh-add ~/.ssh/id_ed25519

# 3. 复制公钥内容
cat ~/.ssh/id_ed25519.pub

# 4. 添加到 GitHub
# 访问：https://github.com/settings/keys
# 点击 "New SSH key"，粘贴公钥内容

# 5. 测试连接
ssh -T git@github.com
```

### 方案 2: 使用 HTTPS 推送

```bash
# 1. 切换为 HTTPS
git remote set-url origin https://github.com/ZeroZap/XinYi.git

# 2. 推送
git push origin main

# 3. 输入 GitHub 用户名和密码 (或 Personal Access Token)
```

### 方案 3: 使用 Git Credential Manager (Windows)

```bash
# 1. 启用 Git Credential Manager
git config --global credential.helper manager

# 2. 切换为 HTTPS
git remote set-url origin https://github.com/ZeroZap/XinYi.git

# 3. 推送 (会自动弹出登录窗口)
git push origin main
```

---

## 📦 待推送内容

### 主要修改

1. **Fuel Gauge 组件** 🔋
   - 新增 `components/fuel_gauge/`
   - ~1,300 行代码

2. **Sensor 框架** 📊
   - 新增 `components/sensor/inc/` (5 个头文件)
   - 新增 `components/sensor/core/` (4 个实现)
   - ~2,500 行代码

3. **Sensor 驱动迁移** 🚗
   - 14 个驱动迁移
   - ~3,000 行代码

4. **TODO 修复** 🔧
   - 28 个 TODO 修复
   - 100% 完成

5. **文档** 📚
   - 10+ 新增文档
   - ~5,000 行文档

### 提交统计

```
feat: 30+ 个
fix: 20+ 个
docs: 10+ 个
refactor: 5+ 个
总计：65+ 个提交
```

---

## ✅ 推送后验证

```bash
# 1. 检查远程分支
git branch -r

# 2. 查看 GitHub
# 访问：https://github.com/ZeroZap/XinYi

# 3. 验证提交
git log origin/main --oneline -10
```

---

## 📝 快速推送命令

```bash
# 完整推送流程
cd /path/to/XinYi

# 如果使用 HTTPS
git remote set-url origin https://github.com/ZeroZap/XinYi.git
git push origin main

# 如果使用 SSH (配置好密钥后)
git push origin main

# 强制推送 (如果需要)
git push -f origin main
```

---

## 🎯 推送清单

- [ ] 配置 SSH 密钥或切换 HTTPS
- [ ] 执行 git push
- [ ] 验证 GitHub 仓库
- [ ] 检查 CI/CD (如有)
- [ ] 通知团队成员

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
