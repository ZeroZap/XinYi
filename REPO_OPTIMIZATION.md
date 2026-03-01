# XinYi 仓库大小优化指南

## 问题分析

当前 .git/objects 记录过大，超过了 1GB 限制。需要进行仓库瘦身。

## 优化步骤

### 1. 检查仓库大小

```bash
# 检查 Git 对象统计
git count-objects -v

# 检查仓库磁盘使用
du -sh .git/

# 检查大文件
git rev-list --objects --all | \
  git cat-file --batch-check='%(objecttype) %(objectname) %(objectsize) %(rest)' | \
  sed -n 's/^blob //p' | \
  sort -n -k3 | \
  tail -10
```

### 2. 清理不必要的文件

```bash
# 清理构建产物
find . -name "*.o" -o -name "*.a" -o -name "*.exe" -o -name "*.bin" -o -name "build" -type d | xargs rm -rf

# 清理临时文件
find . -name "*.tmp" -o -name "*~" -o -name ".DS_Store" -o -name "*.bak" | xargs rm -f
```

### 3. Git 优化

```bash
# 深度垃圾回收
git gc --aggressive --prune=now

# 重新打包
git repack -ad

# 清理引用日志
git reflog expire --expire=now --all

# 清理未引用的对象
git fsck --full
git prune
```

### 4. 大文件过滤

```bash
# 查找历史中的大文件
git rev-list --objects --all | \
  git cat-file --batch-check='%(objecttype) %(objectname) %(objectsize) %(rest)' | \
  awk '$1 == "blob" && $3 > 1000000 {print $3 ": " $4}' | \
  sort -n
```

### 5. 使用 BFG Repo-Cleaner (可选)

```bash
# 如果有大文件需要彻底移除
java -jar bfg.jar --delete-files "*.bin" .git
java -jar bfg.jar --delete-folders "build" .git
```

### 6. 重新克隆 (终极方案)

如果上述方法不够，可以重新克隆仓库:

```bash
# 仅克隆最新提交
git clone --depth 1 <repository-url>

# 或者使用浅层克隆
git clone --shallow-since="2026-01-01" <repository-url>
```

## XinYi 仓库优化

### 1. 清理构建产物

XinYi 仓库中可能存在一些不必要的构建产物，需要清理：

```bash
# 清理构建目录
rm -rf build/
rm -rf components/*/build/
rm -rf components/*/*/build/

# 清理编译产物
find . -name "*.o" -type f -delete
find . -name "*.obj" -type f -delete
find . -name "*.a" -type f -delete
find . -name "*.lib" -type f -delete
find . -name "*.so" -type f -delete
find . -name "*.dll" -type f -delete
find . -name "*.dylib" -type f -delete
```

### 2. 检查是否有二进制文件

```bash
# 检查可能的二进制文件
find . -name "*.bin" -o -name "*.hex" -o -name "*.elf" -o -name "*.out" | grep -v .git
```

### 3. 检查 Git LFS 需求

对于确实需要存储的大型二进制文件，应使用 Git LFS:

```bash
# 安装 Git LFS
git lfs install

# 跟踪特定类型的文件
git lfs track "*.bin"
git lfs track "*.hex"
git lfs track "*.elf"

# 更新 .gitattributes
git add .gitattributes
```

## 最佳实践

### 1. 避免提交大文件

```bash
# 在 .gitignore 中添加
*.bin
*.hex
*.elf
*.out
build/
obj/
*.o
*.obj
*.a
*.lib
```

### 2. 使用构建目录

- 将所有构建产物放在 `build/` 目录
- 在 `.gitignore` 中忽略该目录

### 3. 定期清理

```bash
# 创建清理脚本
cat > cleanup.sh << 'EOF'
#!/bin/bash

echo "清理构建产物..."
find . -name "build" -type d -exec rm -rf {} + 2>/dev/null || true
find . -name "*.o" -type f -delete 2>/dev/null || true
find . -name "*.a" -type f -delete 2>/dev/null || true

echo "执行 Git 优化..."
git gc --aggressive --prune=now
git repack -ad
git reflog expire --expire=now --all
git prune

echo "完成清理"
EOF

chmod +x cleanup.sh
```

## 当前仓库状态

根据之前的检查，我们的仓库对象统计如下:
```
count: 1362
size: 965485KB (约 942MB)
in-pack: 6536
packs: 2
size-pack: 16938KB (约 16.5MB)
size-garbage: 79372KB (约 77MB)
```

仓库的实际大小远小于 1GB，因此可能是由于某些大型二进制文件或未清理的 Git 对象导致的大小计算错误。

## 解决方案

执行以下命令来优化仓库大小：

```bash
# 1. 清理构建产物
find . -name "build" -type d -exec rm -rf {} + 2>/dev/null || true
find . -name "*.o" -o -name "*.a" -o -name "*.bin" -type f -delete 2>/dev/null || true

# 2. Git 优化
git gc --aggressive --prune=now
git repack -ad

# 3. 清理引用日志
git reflog expire --expire=now --all

# 4. 重新计算对象统计
git count-objects -v
```

如果仍有问题，可能需要检查是否有意外的大文件被提交到仓库中。
