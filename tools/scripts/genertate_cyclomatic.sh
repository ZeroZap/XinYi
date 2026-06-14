#!/bin/bash

# 检查是否传入了路径参数
if [ -z "$1" ]; then
  echo "错误：请传入一个路径作为参数。"
  exit 1
fi

# 获取传入的路径
target_dir="$1"

# 检查路径是否存在
if [ ! -d "$target_dir" ]; then
  echo "错误：路径 $target_dir 不存在。"
  exit 1
fi

# 定义需要忽略的目录（支持多个目录，用空格分隔）
ignore_dirs=("build" "test" "third_party")

# 构建 find 命令的忽略条件
find_args=()
for dir in "${ignore_dirs[@]}"; do
  find_args+=(-not \( -path "*/$dir/*" -prune \))
done

# 创建输出文件
output_file="$target_dir/output.csv"

# 使用 find 查找文件，并通过 xargs 分批次传递给 lizard
echo "正在运行 lizard 分析..."
find "$target_dir" "${find_args[@]}" -type f \( -name "*.h" -o -name "*.c" \) -print0 | xargs -0 lizard -o "$output_file"

# 检查 output.csv 是否存在
if [ ! -f "$output_file" ]; then
  echo "错误：未生成 output.csv 文件。"
  exit 1
fi

# 在 output.csv 的首行插入表头
echo "正在插入表头..."
{
  echo "NLOC,CCN,Token Count,Parameter Count,Length,Function Name,File Path,Function Signature,Start Line,End Line"
  cat "$output_file"
} > temp.csv && mv temp.csv "$output_file"

echo "分析完成，结果已保存到 $output_file"