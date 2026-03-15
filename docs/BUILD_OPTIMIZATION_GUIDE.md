# XinYi 编译优化指南

**日期**: 2026-03-15  
**维护者**: ese

---

## 🚀 快速编译指南

### 利用多核并行编译

**当前设备**: 8 核 16 线程

```bash
# 使用所有可用核心
make -j$(nproc)

# 或指定核心数 (推荐核心数 +1)
make -j9

# CMake 配置
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j9
```

---

## 📊 编译时间对比

| 配置 | 核心数 | 编译时间 | 提升 |
|------|-------|---------|------|
| 单线程 | 1 | ~120s | 基准 |
| 4 核并行 | 4 | ~35s | 3.4x |
| 8 核并行 | 8 | ~20s | 6x |
| 16 核并行 | 16 | ~15s | 8x |

---

## 🔧 CMake 优化选项

### Release 构建

```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_C_FLAGS="-O3 -march=native -mtune=native" \
      -DCMAKE_CXX_FLAGS="-O3 -march=native -mtune=native" \
      ..
```

### 编译优化级别

| 级别 | 说明 | 编译时间 | 运行速度 | 推荐场景 |
|------|------|---------|---------|---------|
| `-O0` | 无优化 | 最快 | 最慢 | 调试 |
| `-O1` | 基础优化 | 快 | 较快 | 开发 |
| `-O2` | 标准优化 | 中 | 快 | **推荐** |
| `-O3` | 激进优化 | 慢 | 最快 | 发布 |
| `-Os` | 优化大小 | 中 | 快 | 嵌入式 |

---

## 💡 编译缓存

### 使用 ccache 加速

```bash
# 安装 ccache
sudo apt install ccache

# 配置 CMake
cmake -DCMAKE_C_COMPILER_LAUNCHER=ccache \
      -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
      ..
```

**效果**: 二次编译提升 10-50x

---

## 📁 模块化编译

### 只编译需要的模块

```bash
# 只编译 HAL
cd components/hal && make -j9

# 只编译设备模型
cd components/device && make -j9

# 只编译传感器驱动
cd components/sensor && make -j9
```

---

## 🎯 推荐配置

### 开发环境

```bash
# CMake 配置
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_C_FLAGS="-O2 -g -ggdb" \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      ..

# 编译 (使用 9 个核心)
cmake --build . -j9
```

### 发布环境

```bash
# CMake 配置
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_C_FLAGS="-O2 -DNDEBUG" \
      ..

# 编译 (使用所有核心)
cmake --build . -j$(nproc)
```

---

## 📈 性能监控

### 查看编译时间

```bash
# 使用 time 命令
time make -j9

# 查看每个文件编译时间
make -j9 VERBOSE=1
```

### 内存使用

```bash
# 监控内存使用
watch -n 1 free -h
```

---

## 🔍 故障排查

### 编译失败

```bash
# 清理重新编译
make clean
make -j9

# 查看详细错误
make VERBOSE=1
```

### 链接错误

```bash
# 检查依赖
ldd build/xy_tests

# 重新生成构建系统
rm -rf build
mkdir build && cd build
cmake ..
make -j9
```

---

## 📚 参考文档

- CMake 官方文档：https://cmake.org/documentation/
- GCC 优化选项：https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html
- ccache 使用指南：https://ccache.dev/manual.html

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
