#!/bin/bash

#!/bin/bash

# 定义输出文件
output_file="stack_usage.csv"

# 清空或创建输出文件
> "$output_file"

# 递归查找所有 .su 文件并合并
find . -type f -name "*.su" | while read -r file; do
    # 将每个 .su 文件的内容追加到输出文件中
    cat "$file" >> "$output_file"
done

echo "所有 .su 文件已合并到 $output_file"

# 处理逻辑
sed -i -E '
  # 匹配路径部分（盘符路径或相对路径），保留冒号
  s#([A-Za-z]:[^:]+|\.{2,}[^:]+):#\1,#g;
  # 匹配数字前后的冒号，替换为逗号
  s#:([0-9]+):#,\1,#g;
  s#:([0-9]+)#,\1#g;
  s#([0-9]+):#\1,#g;
  # 将空格和制表符替换为逗号
  s/[ \t]/,/g;
' "$output_file"


echo "格式化处理完成，文件已直接修改。"