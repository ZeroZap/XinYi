# 测试布局分析与优化方案

## 当前测试布局状态

### 布局模式对比

| 模式 | 路径示例 | 优点 | 缺点 | 适用场景 |
|------|---------|------|------|----------|
| **组件内测试** | `components/*/test/` | 测试与源码就近，易于维护 | 测试代码可能污染组件目录 | 小型组件、独立模块 |
| **统一测试目录** | `tests/unit/` | 测试集中管理，便于 CI/CD | 测试与源码分离，更新可能滞后 | 大型项目、多组件联合测试 |
| **混合模式** | 两者结合 | 灵活 | 需要规范管理 | 复杂项目 |

### 当前项目测试分布

```
当前布局 (统一 tests/unit 为主，少量历史/上游文件需分类):

XinYi/
├── components/
│   ├── clib/
│   │   └── xy_clib/                     # CLib 实现；测试在 tests/unit/clib/
│   │
│   ├── crypto/
│   │   ├── tests/unit/crypto/            ✅ 统一 PC 单元测试
│   │   ├── xy_25519/                    ✅ 源码旁 M0 测试已迁入 tests/unit/crypto/
│   │   └── curve25519-*/test/           ⚠️ 上游源码带测试（暂不纳入统一单测）
│   │
│   ├── dm/                              ✅ 旧源码旁测试已迁入 tests/unit/dm/
│   │
│   ├── kernel/osal/                     ✅ OSAL 单测由 tests/unit/kernel/ 维护
│   │
│   ├── net/
│   │   └── xy_iso7816/
│   │       └── xy_iso7816.c             # ISO7816 实现
│   │
│   └── sensor/
│       └── sensor_self_test.c           ✅ 生产自检实现；由 tests/unit/sensor/ 覆盖
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

**问题**: `xy_clib` 旧组件内测试已迁入统一 `tests/unit/clib/test_clib_core.c`。

**当前状态**:
- `components/clib/xy_clib/test/` 已删除
- `components/clib/xy_clib/test_filter.c` / `test_sort.c` 已并入 `test_clib_core.c`
- 统一入口：`make test-unit` 或 CTest `clib_component`
- 当前 `tests/unit` 源码库存为 70 个 C 文件，均为 Unity-style，且 0 个未接入 CMake 的源码测试文件。

### 2. 测试文件命名不统一

| 模式 | 示例 | 出现位置 |
|------|------|----------|
| `test_*.c` | `tests/unit/clib/test_clib_core.c` | clib |
| `*_test.c` | 历史 `fee-test.c`, `eflash_test.c` | 已迁入/清理 |
| `test*.c` | `test.c` | 第三方 crypto 上游源码 |

**影响**: 难以批量处理测试文件

### 3. 测试目录层级不一致

```
tests/unit/crypto/                 # Crypto 统一 PC 单元测试
tests/unit/clib/                   # CLib 统一 PC 单元测试
tests/unit/dm/                     # DM 统一 PC 单元测试
```

### 4. 第三方源码测试混合

```
components/crypto/curve25519-*/test/  # 第三方/上游测试
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
- `components/clib/xy_clib/test/` 已删除
- `components/clib/xy_clib/test_filter.c` / `test_sort.c` 已并入 `tests/unit/clib/test_clib_core.c`

**运行**:
```bash
make test-unit
ctest --test-dir build/tests/unit -R '^clib_component$' --output-on-failure
```

### 步骤 2: 规范 crypto 测试

**状态**: 旧 `components/crypto/test/` printf/manual 测试已收敛到仓库级
Unity + CTest 套件，当前入口在 `tests/unit/crypto/`。

**运行**:
```bash
make test-unit
ctest --test-dir build/tests/unit -R '^crypto_' --output-on-failure
```

第三方 `components/crypto/curve25519-*/test/` 保持原位并按 vendor 测试处理。

### 步骤 3: 规范 dm 测试

**状态**: 已收敛到统一 `tests/unit/dm/` 目录；旧 `components/dm/fee-test.c`
和 `components/dm/xy_eeprom/eflash_test.c` 源码旁入口已迁移/清理。

**运行**:
```bash
make test-unit
ctest --test-dir build/tests/unit -R '^dm_' --output-on-failure
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

**当前维护重点**:
1. 保持 `tests/unit` 源码测试全部接入 Unity + CTest 主入口
2. 使用 `tests/unity/` 作为统一 Unity 框架位置
3. 维护 `tests/CMakeLists.txt` 和 `tests/unit/CMakeLists.txt` 统一入口
4. 对 `tests/unit` 外 first-party-looking 测试文件先分类，再决定是否迁移、保留或删除
