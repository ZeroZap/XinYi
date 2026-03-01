# .gitignore 配置说明

**版本**: 1.0  
**最后更新**: 2026-02-28

---

## 📋 概述

本文档说明 XinYi 项目的 `.gitignore` 配置规则和最佳实践。

---

## 🗂️ 配置分类

### 1. 构建产物 (Build Artifacts)

**忽略内容**:
- 编译输出目录：`build/`, `obj/`, `output/`
- 目标文件：`*.o`, `*.obj`
- 库文件：`*.a`, `*.lib`, `*.so`, `*.dll`
- 可执行文件：`*.exe`, `*.elf`, `*.bin`, `*.hex`
- 调试文件：`*.map`, `*.lst`

**原因**: 这些文件可以通过源代码重新生成，不应纳入版本控制。

---

### 2. IDE 文件

**忽略内容**:
- VS Code: `.vscode/`
- CLion: `.idea/`
- 编辑器临时文件：`*.swp`, `*.swo`, `*.bak`

**原因**: IDE 配置因开发者而异，不应强制统一。

---

### 3. 日志和临时文件

**忽略内容**:
- 日志文件：`*.log`, `logs/`
- 临时文件：`*.tmp`, `*~`
- 系统文件：`.DS_Store`, `Thumbs.db`

**原因**: 运行时生成的文件，不应纳入版本控制。

---

### 4. 第三方库 (Third-Party)

#### RT-Thread

**忽略内容**:
```gitignore
**/third_party/rt-thread/**/*.a
**/third_party/rt-thread/**/*.lib
**/third_party/rt-thread/**/*.out
**/third_party/rt-thread/**/*.hex
**/third_party/rt-thread/**/build/
**/third_party/rt-thread/**/obj/
```

**保留内容**:
- ✅ 源代码
- ✅ 文档
- ✅ 配置文件

#### FreeRTOS

**忽略内容**:
```gitignore
**/third_party/freertos/**/*.a
**/third_party/freertos/**/*.lib
**/third_party/freertos/**/build/
```

#### CMSIS-RTX

**忽略内容**:
```gitignore
**/third_party/CMSIS-RTX/**/RTE/
**/third_party/CMSIS-RTX/**/output/
**/third_party/CMSIS-RTX/**/*.uv*
**/third_party/CMSIS-RTX/**/*.jlink
```

**原因**: 第三方库的构建产物体积大，且可通过 submodule 获取。

---

### 5. 生成文档

**忽略内容**:
- Doxygen 生成：`docs/api/html/`, `docs/api/latex/`
- 自动生成的文档：`docs/generated/`

**保留内容**:
- ✅ 手写文档
- ✅ Markdown 源文件

---

### 6. 操作系统特定文件

**忽略内容**:
- macOS: `.DS_Store`, `.Spotlight-V100`, `._*`
- Windows: `Thumbs.db`, `Desktop.ini`, `ehthumbs.db`

---

## 🎯 最佳实践

### 1. 检查 .gitignore 效果

```bash
# 查看哪些文件会被忽略
git check-ignore -v *

# 查看特定文件是否被忽略
git check-ignore -v path/to/file
```

### 2. 移除已跟踪的文件

```bash
# 从 Git 缓存中移除（保留本地文件）
git rm --cached path/to/file

# 递归移除目录
git rm -r --cached path/to/directory
```

### 3. 查看被忽略的文件

```bash
# 查看所有被忽略的文件
git ls-files --others -i --exclude-standard

# 查看被忽略但存在的文件
git status --ignored
```

---

## 📝 常见场景

### 场景 1: 忽略所有 .o 文件但保留特定的

```gitignore
# 忽略所有 .o
*.o

# 但保留特定的
!important.o
```

### 场景 2: 忽略目录下所有文件但保留特定子目录

```gitignore
# 忽略 logs 目录下所有文件
logs/*

# 但保留 logs/keep/ 子目录
!logs/keep/
```

### 场景 3: 忽略第三方库但保留配置文件

```gitignore
# 忽略第三方库构建产物
third_party/**/build/

# 但保留配置文件
!third_party/**/CMakeLists.txt
!third_party/**/Kconfig
```

---

## 🔧 维护指南

### 定期清理

```bash
# 清理所有被忽略的文件
git clean -fdx

# 预览将要删除的文件
git clean -fdxn
```

### 更新 .gitignore

1. 编辑 `.gitignore` 文件
2. 运行 `git check-ignore -v *` 验证
3. 提交更改

---

## 📚 参考资源

- [Git 官方文档 - .gitignore](https://git-scm.com/docs/gitignore)
- [GitHub .gitignore 模板](https://github.com/github/gitignore)
- [gitignore.io - 生成 .gitignore](https://www.toptal.com/developers/gitignore)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
