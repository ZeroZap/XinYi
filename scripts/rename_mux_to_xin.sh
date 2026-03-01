#!/bin/bash
# 批量更新 MuxLink 为 XinLink

echo "=== 开始更新 MuxLink 为 XinLink ==="

# 查找所有包含 MuxLink 的文件
find . -type f -name "*.md" -o -name "*.c" -o -name "*.h" -o -name "*.txt" | while read file; do
    if grep -q "MuxLink" "$file"; then
        echo "更新文件: $file"
        sed -i 's/MuxLink/XinLink/g' "$file"
    fi
done

echo "=== 更新完成 ==="
