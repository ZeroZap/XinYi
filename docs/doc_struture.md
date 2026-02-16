## docs
存放的基础的文档

## 各自目录下的md
各自目录下，有对应的 md 文件，比如 components下有 components.md 介绍组件的情况

## 文档生成规则
- 仓库目录下 ReadMe.md 为主页，根据
- code_style
- 其他的


├── .github/                  # GitHub 专用配置（CI/CD、PR模板等）
│   ├── workflows/
│   │   └── ci.yml
│   └── PULL_REQUEST_TEMPLATE.md
├── cmake/                    # CMake 辅助模块（如查找依赖）
├── docs/                     # 所有文档（含AI生成的API文档、设计文档）
│   ├── api/                  # Doxygen 生成的HTML（通常忽略在git中，但可保留生成脚本）
│   ├── design/               # 设计文档（可AI生成初稿）
│   ├── rules/                # 编码规范、AI规则说明
│   └── Doxyfile               # Doxygen 配置文件
├── include/                   # 公共头文件
│   └── mylib/
│       └── mylib.h
├── src/                       # 源代码
│   ├── module1/
│   │   ├── module1.c
│   │   └── module1_private.h
│   └── main.c
├── tests/                     # 测试代码
│   ├── unit/
│   │   ├── test_module1.c
│   │   └── CMakeLists.txt
│   └── CMakeLists.txt
├── scripts/                   # 辅助脚本（代码生成、格式化等）
│   ├── generate_docs.sh
│   └── run_clang_format.sh
├── .clang-format              # 代码格式化规则
├── .clang-tidy                # 静态分析规则
├── .editorconfig              # 编辑器通用配置
├── .gitignore
├── CMakeLists.txt             # 顶层CMake配置
├── README.md                  # 项目简介（可AI辅助生成）
└── LICENSE