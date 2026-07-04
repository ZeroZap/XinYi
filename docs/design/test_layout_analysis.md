# 测试布局分析与优化方案

## 当前测试布局状态

### 布局模式对比

| 模式 | 路径示例 | 优点 | 缺点 | 适用场景 |
|------|---------|------|------|----------|
| **组件内测试** | `components/crypto/test/` | 测试与源码就近，易于维护 | 测试代码可能污染组件目录 | 小型组件、独立模块 |
| **统一测试目录** | `tests/unit/` | 测试集中管理，便于 CI/CD | 测试与源码分离，更新可能滞后 | 大型项目、多组件联合测试 |
| **混合模式** | 两者结合 | 灵活 | 需要规范管理 | 复杂项目 |

### 当前项目测试分布

```
当前布局 (混合模式 - 需要规范):

XinYi/
├── components/
│   ├── clib/
│   │   └── xy_clib/
│   │       ├── test/                    ✅ 组件内测试
│   │       ├── test_filter.c            ✅ 组件内测试
│   │       └── test_sort.c              ✅ 组件内测试
│   │
│   ├── crypto/
│   │   ├── test/                        ✅ 组件内测试
│   │   ├── xy_25519/
│   │   │   └── test_xy_25519_m0.c       ⚠️ 源码目录内测试
│   │   └── curve25519-*/test/           ⚠️ 第三方源码带测试
│   │
│   ├── dm/
│   │   ├── fee-test.c                   ⚠️ 源码同级测试
│   │   └── xy_eeprom/eflash_test.c      ⚠️ 子目录内测试
│   │
│   ├── kernel/osal/
│   │   └── tests/                       ✅ 组件内测试 (新规范)
│   │
│   ├── net/
│   │   └── xy_iso7816/
│   │       └── xy_iso7816.c             # ISO7816 实现
│   │
│   └── sensor/
│       └── sensor_self_test.c           ⚠️ 组件根目录测试
│
├── tests/                               📋 统一测试入口
│   ├── unit/                            ✅ PC 单元测试
│   ├── qemu_stm32f4/                    ✅ QEMU STM32F4 测试
│   ├── qemu_ch32v/                      ✅ QEMU CH32V 测试
│   └── unity/                           ✅ 测试框架
│
└── projects/
    └── */test*.c                        ⚠️ 项目内测试
```

---

## 问题分析

### 1. 测试代码重复

**问题**: `xy_clib` 测试同时存在于两处
- `components/clib/xy_clib/test/`
- 历史 `UniTest/component/xy_clib/test/`

**影响**: 
- 维护成本翻倍
- 可能出现测试结果不一致

### 2. 测试文件命名不统一

| 模式 | 示例 | 出现位置 |
|------|------|----------|
| `test_*.c` | `test_filter.c` | clib |
| `*_test.c` | `fee-test.c`, `eflash_test.c` | dm |
| `test*.c` | `test.c` | crypto |

**影响**: 难以批量处理测试文件

### 3. 测试目录层级不一致

```
components/crypto/test/           # 组件级测试目录
components/clib/xy_clib/test/     # 子组件级测试目录
components/dm/fee-test.c          # 文件级测试
```

### 4. 第三方源码测试混合

```
components/crypto/curve25519-*/test/  # 第三方测试
```

**问题**: 第三方测试可能与项目测试框架不兼容

---

## 推荐方案

### 方案 A: 组件内测试 (推荐) ✅

```
XinYi/
├── components/
│   ├── crypto/
│   │   ├── src/               # 源码
│   │   ├── include/           # 头文件
│   │   ├── tests/             # 测试代码
│   │   │   ├── test_aes.c
│   │   │   ├── test_hmac.c
│   │   │   └── CMakeLists.txt
│   │   └── CMakeLists.txt
│   │
│   └── kernel/osal/
│       ├── src/
│       ├── include/
│       └── tests/
│           ├── test_osal.c
│           └── CMakeLists.txt
│
├── tests/                     # 可选：集成测试/系统测试
│   ├── integration/
│   └── system/
│
└── third_party/unity/         # 测试框架
```

**优点**:
- 测试与源码就近，易于同步更新
- 每个组件独立管理测试
- 符合现代 C 项目最佳实践

**缺点**:
- 需要统一规范

### 方案 B: 统一测试目录

```
XinYi/
├── components/
│   ├── crypto/
│   │   └── src/
│   └── kernel/
│       └── osal/
│
├── tests/                     # 所有测试集中在此
│   ├── unit/
│   │   ├── crypto/
│   │   │   ├── test_aes.c
│   │   │   └── test_hmac.c
│   │   └── osal/
│   │       └── test_osal.c
│   ├── integration/
│   └── system/
│
└── third_party/unity/
```

**优点**:
- 测试集中管理
- 便于 CI/CD 集成

**缺点**:
- 测试与源码分离，更新可能滞后
- 大型项目测试目录会非常庞大

---

## 优化建议

### 推荐：方案 A (组件内测试) + 统一测试入口

**目录规范**:
```
components/<component>/
├── src/              # 源码 (可选，如源码已在根目录则不需要)
├── include/          # 公共头文件
├── tests/            # 单元测试 (统一目录名)
│   ├── test_*.c      # 测试文件 (统一前缀)
│   ├── unity/        # 或引用第三方测试框架
│   └── CMakeLists.txt
├── docs/             # 文档
├── CMakeLists.txt    # 组件构建配置
└── README.md
```

**文件命名规范**:
- 测试文件：`test_<module>.c`
- 测试目录：`tests/` (复数形式)

**统一测试入口**:
```
tests/
├── CMakeLists.txt    # 统一测试构建入口
├── run_tests.sh      # 测试运行脚本
└── coverage/         # 覆盖率报告
```

---

## 迁移步骤

### 步骤 1: 统一 xy_clib 测试

**当前**:
- `components/clib/xy_clib/test/` - 保留
- 历史 `UniTest/component/xy_clib/test/` - 已删除/不再使用

**操作**:
```bash
# 备份后删除重复测试
rm -rf tests/unit/build

# 统一测试文件命名
mv components/clib/xy_clib/test_filter.c components/clib/xy_clib/tests/test_filter.c
mv components/clib/xy_clib/test_sort.c components/clib/xy_clib/tests/test_sort.c
```

### 步骤 2: 规范 crypto 测试

**操作**:
```bash
# 创建统一 tests 目录
mkdir -p components/crypto/tests

# 移动测试文件
mv components/crypto/test/*.c components/crypto/tests/
mv components/crypto/xy_25519/test_xy_25519_m0.c components/crypto/tests/

# 第三方测试保持原位 (标记为第三方)
# components/crypto/curve25519-*/test/ 保持不变
```

### 步骤 3: 规范 dm 测试

**操作**:
```bash
mkdir -p components/dm/tests

# 移动分散的测试文件
mv components/dm/fee-test.c components/dm/tests/test_fee.c
mv components/dm/xy_eeprom/eflash_test.c components/dm/tests/test_eflash.c
```

### 步骤 4: 规范 net 测试

**操作**:
```bash
mkdir -p components/net/tests

ISO7816 组件测试已在 `tests/unit/net/test_iso7816.c` 中维护，并由 `make test-unit` 运行。
```

### 步骤 5: 创建统一测试入口

**创建文件**: `tests/CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.12)
project(xy_tests C)

enable_testing()

# 统一 PC 单元测试入口
add_subdirectory(unit)

# 自定义测试目标
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR}/unit --output-on-failure
    DEPENDS run_unit_tests
)
```

---

## 测试框架统一

### 当前状态

| 组件 | 测试框架 | 位置 |
|------|----------|------|
| osal | Unity (自包含) | `osal/tests/unity.*` |
| xy_clib | 自定义 | `tests/unity/` |
| crypto | 自定义 | 各测试文件内 |

### 推荐方案

**统一使用 Unity 框架**:

```
tests/
└── unity/               # Unity 测试框架
    ├── src/
    │   ├── unity.c
    │   ├── unity.h
    │   └── unity_internals.h
    └── README.md
```

**组件测试 CMakeLists.txt 模板**:

```cmake
# components/<name>/tests/CMakeLists.txt

# Unity 框架
set(UNITY_DIR ${CMAKE_SOURCE_DIR}/tests/unity)
include_directories(${UNITY_DIR})

# 测试可执行文件
add_executable(test_<component>
    ${UNITY_DIR}/unity.c
    test_<module>.c
)

# 链接被测组件
target_link_libraries(test_<component>
    <component_lib>
)

# 注册测试
add_test(NAME <component>_test COMMAND test_<component>)
```

---

## 实施优先级

| 优先级 | 任务 | 工作量 | 影响 |
|--------|------|--------|------|
| 🔴 高 | 统一 xy_clib 测试 (去重) | 小 | 大 |
| 🟡 中 | 规范 crypto 测试目录 | 中 | 中 |
| 🟡 中 | 创建统一测试入口 | 小 | 中 |
| 🟢 低 | 统一测试框架 | 大 | 中 |
| 🟢 低 | 规范其他组件 | 中 | 小 |

---

## 总结

**推荐方案**: 组件内 `tests/` 目录 + 统一测试入口

**核心原则**:
1. 测试就近源码，易于维护
2. 统一目录名 `tests/`
3. 统一文件命名 `test_<module>.c`
4. 统一测试框架 (推荐 Unity)
5. 保留第三方测试，但明确标记

**下一步**:
1. 删除重复测试和历史 `UniTest/` 根目录
2. 使用 `tests/unity/` 作为统一 Unity 框架位置
3. 维护 `tests/CMakeLists.txt` 和 `tests/unit/CMakeLists.txt` 统一入口
4. 逐步规范各组件测试目录
