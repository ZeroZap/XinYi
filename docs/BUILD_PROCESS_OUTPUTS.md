# XinYi Build Process And Output Directories

本文档说明 XinYi 当前主要构建入口、输出目录，以及哪些入口已经统一到根 `build/` 目录，哪些入口仍可能在子目录生成构建产物。

## 目标

构建产物应尽量集中到仓库根目录下的 `build/`：

```text
XinYi/
└── build/
    ├── pc/
    ├── stm32f4/
    ├── stm32u5/
    ├── wch/
    ├── hc32/
    ├── tests/unit/
    └── qemu/
        ├── stm32f4/
        └── ch32v/
```

这样可以避免根目录出现大量 `build_xxx/`，也避免 examples/tests/components 内部散落 `build/`。

## 根 Makefile 构建流程

默认构建：

```bash
make
```

流程：

```text
make
└── cmake -B build/pc -S . -DHAL_PLATFORM=PC -DCMAKE_BUILD_TYPE=Release ...
    └── cmake --build build/pc -j<N>
```

输出目录：

```text
build/pc
```

平台构建：

```bash
make HAL_PLATFORM=STM32F4
make HAL_PLATFORM=STM32U5
make HAL_PLATFORM=WCH
make HAL_PLATFORM=HC32
```

输出目录：

| 命令 | 输出目录 |
|------|----------|
| `make` | `build/pc` |
| `make HAL_PLATFORM=STM32F4` | `build/stm32f4` |
| `make HAL_PLATFORM=STM32U5` | `build/stm32u5` |
| `make HAL_PLATFORM=WCH` | `build/wch` |
| `make HAL_PLATFORM=HC32` | `build/hc32` |

可覆盖目录：

```bash
make BUILD_ROOT=out
make BUILD_DIR=out/custom-pc
```

## 单元测试构建流程

命令：

```bash
make test-unit
```

流程：

```text
make test-unit
└── cmake -B build/tests/unit -S tests/unit
    └── cmake --build build/tests/unit -j<N>
        └── ctest --output-on-failure
```

输出目录：

```text
build/tests/unit
```

状态：已统一，不再使用 `tests/unit/build`。

## QEMU 测试构建流程

STM32F4 QEMU：

```bash
make test-qemu
```

输出目录：

```text
build/qemu/stm32f4
```

CH32V QEMU：

```bash
make test-qemu-ch32v
```

输出目录：

```text
build/qemu/ch32v
```

状态：已统一，ELF 和 `.log` 不再生成到 `tests/qemu_*` 源目录中。

## examples 构建建议

`examples/*/CMakeLists.txt` 仍是独立 CMake 工程入口。不要在 example 目录内部执行 `mkdir build`，推荐从仓库根目录指定统一输出目录：

```bash
cmake -B build/examples/component_demo -S examples/component_demo
cmake --build build/examples/component_demo -j$(nproc)
```

通用模式：

```bash
cmake -B build/examples/<example-name> -S examples/<example-name>
cmake --build build/examples/<example-name> -j$(nproc)
```

当前 `make distclean` 会清理常见历史目录：

```text
examples/*/build
examples/*/*/build
```

判断：examples 还没有统一的根 Makefile wrapper，若开发者手动在 `examples/foo/build` 构建，仍会在 example 内部生成 build。建议后续新增 `make example NAME=<example>` 统一入口。

## components/tests 独立构建入口

部分组件和测试目录仍有独立 `CMakeLists.txt` 或 Makefile。推荐用根 `build/` 指定输出目录：

```bash
cmake -B build/components/hal -S components/hal
cmake --build build/components/hal -j$(nproc)
```

```bash
cmake -B build/tests/unit -S tests/unit
cmake --build build/tests/unit --target test_actuator_framework -j$(nproc)
ctest --test-dir build/tests/unit -R '^actuator_framework$' --output-on-failure
```

当前 `make distclean` 会清理常见历史目录：

```text
tests/*/build
components/*/build
components/*/*/build
```

判断：根 Makefile 管理的单元测试已统一；其他独立组件/测试入口仍取决于开发者使用的 `cmake -B` 路径。

## vendor 和第三方目录

`MCU/`、`third_party/` 下存在厂商或上游 Makefile/CMake 工程。这些目录不由根 Makefile 重构，避免误删或破坏 vendor SDK。

判断：vendor 内部 build 目录属于外部 SDK 行为，不建议纳入根 `distclean` 的激进清理范围。

## distclean 行为

命令：

```bash
make distclean
```

清理范围：

```text
tmp
build
build_*
tests/unit/build
examples/*/build
examples/*/*/build
tests/*/build
components/*/build
components/*/*/build
```

并调用：

```bash
make -C tests/qemu_stm32f4 clean
make -C tests/qemu_ch32v clean
```

注意：`distclean` 会删除构建缓存。需要保留当前构建缓存时，不要执行。

## 当前结论

已统一的入口：

| 入口 | 状态 | 输出目录 |
|------|------|----------|
| `make` | 已统一 | `build/pc` |
| `make HAL_PLATFORM=<platform>` | 已统一 | `build/<platform>` |
| `make test-unit` | 已统一 | `build/tests/unit` |
| `make test-qemu` | 已统一 | `build/qemu/stm32f4` |
| `make test-qemu-ch32v` | 已统一 | `build/qemu/ch32v` |

仍需开发者遵守约定的入口：

| 入口 | 风险 | 推荐输出目录 |
|------|------|--------------|
| `examples/*` 独立 CMake | 若在 example 内执行 `mkdir build` 会生成嵌套 build | `build/examples/<name>` |
| `components/*` 独立 CMake | 若在 component 内执行 `mkdir build` 会生成嵌套 build | `build/components/<name>` |
| `tests/*` 独立 CMake | 若在 test 内执行 `mkdir build` 会生成嵌套 build | `build/tests/<name>` |
| vendor SDK Makefile | 外部行为，不建议统一改造 | 保持 vendor 默认 |

建议后续改进：

1. 增加 `make example NAME=<name>`，统一 examples 构建入口。
2. 增加 `make component-test NAME=<name>`，统一组件独立测试入口。
3. 对文档中旧的 `cd examples/foo && mkdir build` 逐步替换为 `cmake -B build/examples/foo -S examples/foo`。
