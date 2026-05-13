# SSH 密钥配置指南

**日期**: 2026-03-05  
**状态**: ⚠️ 需要手动配置

---

## ⚠️ 问题

SSH 密钥不存在或未被正确加载。

---

## 🔧 解决方案

### 方案 1: 手动生成 SSH 密钥 (推荐)

**在 Git Bash 中执行**:

```bash
# 1. 生成 SSH 密钥
ssh-keygen -t ed25519 -C "zerozap2020@gmail.com"

# 按 Enter 接受默认路径
# 按 Enter 跳过密码 (或设置密码)

# 2. 查看公钥
cat ~/.ssh/id_ed25519.pub

# 3. 复制公钥内容
# 选中并复制输出内容 (以 ssh-ed25519 开头)

# 4. 添加到 GitHub
# 访问：https://github.com/settings/keys
# 点击 "New SSH key"
# 粘贴公钥内容，保存

# 5. 测试连接
ssh -T git@github.com

# 6. 推送
git push -u origin main
```

---

### 方案 2: 使用现有 RSA 密钥

**在 Git Bash 中执行**:

```bash
# 1. 生成 RSA 密钥
ssh-keygen -t rsa -b 4096 -C "zerozap2020@gmail.com"

# 2. 查看公钥
cat ~/.ssh/id_rsa.pub

# 3. 添加到 GitHub (同上)

# 4. 推送
git push -u origin main
```

---

### 方案 3: 使用 GitHub Desktop

1. 下载 GitHub Desktop: https://desktop.github.com/
2. 登录 GitHub 账号
3. 添加本地仓库
4. 自动处理认证

---

## 📝 详细步骤

### 步骤 1: 生成 SSH 密钥

打开 **Git Bash**,执行:

```bash
ssh-keygen -t ed25519 -C "zerozap2020@gmail.com"
```

输出示例:
```
Generating public/private ed25519 key pair.
Enter file in which to save the key (/c/Users/86158/.ssh/id_ed25519): 
Enter passphrase (empty for no passphrase): 
Enter same passphrase again: 
Your identification has been saved in /c/Users/86158/.ssh/id_ed25519
Your public key has been saved in /c/Users/86158/.ssh/id_ed25519.pub
```

### 步骤 2: 复制公钥

```bash
cat ~/.ssh/id_ed25519.pub
```

输出示例:
```
ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAI... zerozap2020@gmail.com
```

**复制整行内容!**

### 步骤 3: 添加到 GitHub

1. 访问：https://github.com/settings/keys
2. 点击 **"New SSH key"**
3. Title: 填写描述 (如 "BaoBao Laptop")
4. Key type: 选择 **"Authentication Key"**
5. Key: 粘贴公钥内容
6. 点击 **"Add SSH key"**

### 步骤 4: 测试连接

```bash
ssh -T git@github.com
```

成功输出:
```
Hi ZeroZap! You've successfully authenticated, but GitHub does not provide shell access.
```

### 步骤 5: 推送

```bash
cd /e/github_download/_ZeroZap/Maker/XinYi
git push -u origin main
```

---

## 🎯 快速推送命令

```bash
# 完整流程
cd /e/github_download/_ZeroZap/Maker/XinYi

# 1. 生成密钥 (如果还没有)
ssh-keygen -t ed25519 -C "zerozap2020@gmail.com"

# 2. 查看并复制公钥
cat ~/.ssh/id_ed25519.pub

# 3. 手动添加到 GitHub 后，测试
ssh -T git@github.com

# 4. 推送
git push -u origin main
```

---

## ✅ 验证清单

- [ ] SSH 密钥已生成
- [ ] 公钥已添加到 GitHub
- [ ] ssh -T git@github.com 测试成功
- [ ] git push 成功

---

## 🐛 常见问题

### 问题 1: "Permission denied (publickey)"

**原因**: SSH 密钥未添加或未加载

**解决**:
```bash
# 检查密钥是否存在
ls -la ~/.ssh/

# 加载密钥
ssh-add ~/.ssh/id_ed25519

# 测试
ssh -T git@github.com
```

### 问题 2: "Could not open a connection to your authentication agent"

**原因**: ssh-agent 未运行

**解决**:
```bash
# 启动 ssh-agent
eval $(ssh-agent -s)

# 添加密钥
ssh-add ~/.ssh/id_ed25519
```

### 问题 3: Git Bash 中 cat 命令无效

**解决**: 使用 Windows 命令
```bash
# Windows 命令
cat /c/Users/86158/.ssh/id_ed25519.pub
```

---

## 📞 需要帮助?

如果以上步骤都无法解决，请:

1. 检查 Git 版本：`git --version`
2. 检查 SSH 版本：`ssh -V`
3. 查看详细错误：`ssh -vT git@github.com`

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
