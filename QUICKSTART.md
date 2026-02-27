# XinYi Framework - 快速入门指南

欢迎使用 XinYi 嵌入式框架！本指南将帮助您快速开始。

---

## 🚀 5 分钟快速开始

### 1. 克隆项目

```bash
git clone <repository-url>
cd XinYi
git submodule update --init --recursive
```

### 2. 构建项目

#### Linux/macOS

```bash
# 使用构建脚本
./utils/script/build.sh

# 或手动构建
mkdir build && cd build
cmake .. -DBUILD_TESTING=ON
make -j$(nproc)
```

#### Windows

```cmd
REM 使用构建脚本
utils\script\build.bat

REM 或手动构建
mkdir build && cd build
cmake .. -DBUILD_TESTING=ON
cmake --build . --config Release
```

### 3. 运行测试

#### Linux/macOS

```bash
# 使用测试脚本
./utils/script/run_tests.sh

# 或手动运行
cd build
ctest --output-on-failure
```

#### Windows

```cmd
REM 使用测试脚本
utils\script\test.bat

REM 或手动运行
cd build
ctest -C Release --output-on-failure
```

---

## 📁 项目结构

```
XinYi/
├── components/          # 核心组件
│   ├── crypto/         # 密码学
│   ├── clib/           # C 库
│   ├── dm/             # 数据管理
│   ├── net/            # 网络协议
│   ├── hal/            # 硬件抽象层
│   ├── osal/           # OS 抽象层
│   └── ...
├── tests/              # 单元测试
├── projects/           # 应用项目
├── utils/script/       # 工具脚本
└── docs/               # 文档
```

---

## 🛠️ 常用命令

### 构建

```bash
# 完整构建
make

# 构建单个组件
make crypto
make osal

# 清理
make clean
make distclean
```

### 测试

```bash
# 运行所有测试
make test

# 运行特定测试
ctest -R test_crypto --output-on-failure

# 生成覆盖率报告
./utils/script/coverage.sh
```

### 代码质量

```bash
# 格式化代码
./utils/script/format_code.sh

# 检查代码风格
./utils/script/check_style.sh
```

---

## 📖 文档导航

| 文档 | 说明 |
|------|------|
| [ReadMe.md](ReadMe.md) | 主文档 |
| [COMPONENTS_STATUS.md](COMPONENTS_STATUS.md) | 组件状态 |
| [docs/rules/100-code_style/xy_code_style.md](docs/rules/100-code_style/xy_code_style.md) | 代码风格 |

---

## 🔧 配置选项

### CMake 配置

```bash
cmake .. \
    -DBUILD_TESTING=ON \
    -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -DTEST_COVERAGE=ON
```

### Kconfig 配置

```bash
# 交互式配置（需要 kconfig-frontends）
make menuconfig

# 设置特定选项
make CONFIG_CRYPTO_AES=y
```

---

## 📊 组件一览

| 组件 | 测试用例 | 状态 |
|------|---------|------|
| osal | 17 | ✅ |
| hal | 11 | ✅ |
| crypto | 28 | ✅ |
| clib | 21 | ✅ |
| dm | 24 | ✅ |
| net | 22 | ✅ |
| sensor | 18 | ✅ |
| ipc | 14 | ✅ |
| pm | 19 | ✅ |
| pid | 20 | ✅ |
| addc | 24 | ✅ |
| trace | 10 | ✅ |

**总计**: 228 个测试用例

---

## 🐛 遇到问题？

1. **构建失败**: 检查 CMake 版本 (需要 3.10+)
2. **测试失败**: 确认 Unity 框架已初始化
3. **代码风格**: 运行 `format_code.sh` 格式化

---

## 📞 获取帮助

- 查看组件 README.md
- 查看单元测试示例
- 提交 Issue

---

**Happy Coding!** 🎉
