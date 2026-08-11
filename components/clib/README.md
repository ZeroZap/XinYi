# CLIB Component

**状态**: ✅ host-guarded core runtime / 嵌入式 libc 子集

## 1. 组件定位

`components/clib/xy_clib` 是 XinYi 的项目内 C library/runtime 子集，为 embedded-facing 组件提供稳定的基础类型、字符串/内存、格式化输出、ctype、ring-buffer、filter/sort 和常用数学工具。上层组件应优先包含 `xy_clib.h` 或具体 `xy_*` 子头，再按需使用标准库设施，避免在 MCU 目标上引入不可控 libc 假设。

该组件仍作为核心 runtime 被根 `CMakeLists.txt` 手动加入；当前没有把 `xy_clib` 做成可关闭的 root Kconfig feature。历史 nested `components/clib/xy_clib/Kconfig` 不是当前 PC/STM32U5 生成配置事实源，除非先完成核心依赖禁用模型设计，否则不要直接把它接成可选开关。

## 2. 当前目录与构建入口

| 路径 | 角色 |
| --- | --- |
| `components/clib/xy_clib/inc/xy_clib.h` | aggregate include；聚合 CLIB 子头。 |
| `components/clib/xy_clib/xy_common.{h,c}` | 基础整型辅助、BCD/bit 宏等 common helpers。 |
| `components/clib/xy_clib/xy_string.{h,c}` | `xy_strlen`、`xy_memcpy`、`xy_memmove`、`xy_str*` 子集。 |
| `components/clib/xy_clib/xy_stdio.{h,c}` | `xy_sprintf` / `xy_snprintf` 与格式化输出子集。 |
| `components/clib/xy_clib/xy_ctype.{h,c}` | `xy_is*`、大小写转换 helpers。 |
| `components/clib/xy_clib/xy_rb.{h,c}` | ring-buffer helpers，含静态与动态 buffer API。 |
| `components/clib/xy_clib/xy_filter.{h,c}` | amplitude/median/recursive/lag/debounce filters。 |
| `components/clib/xy_clib/xy_sort.{h,c}` | u16 排序与 binary search helpers。 |
| `components/clib/xy_clib/xy_math.{h,c}` / `xy_stdlib.{h,c}` | math/stdlib 扩展；部分 API 尚未纳入 focused CTest。 |
| `components/clib/xy_clib/CMakeLists.txt` | `xy_xy_clib` static library 入口。 |

根构建会通过顶层 `CMakeLists.txt` 的 nested component 逻辑加入 `components/clib/xy_clib`。

## 3. Host 测试护栏

主线 PC unit suite 已注册：

| CTest | Target | 覆盖范围 |
| --- | --- | --- |
| `clib_component` | `test_clib` | common/string/ctype/stdio/ring-buffer/filter/sort 核心契约。 |
| `clib_alloc_shim` | `test_clib_alloc_shim` | host malloc-backed CLIB allocation shim；供多个 fake-heavy unit tests 复用。 |

常用验证命令：

```bash
cmake -B build/tests/unit -S tests/unit
cmake --build build/tests/unit --target test_clib -j$(nproc)
ctest --test-dir build/tests/unit --output-on-failure -R '^clib_component$'
make test-unit
git diff --check
```

## 4. 维护边界

- CLIB 是核心 runtime 依赖；不要在没有 proposal 的情况下把它改为可关闭组件。
- 不要恢复已删除的 component-local/private Unity 或 stale runner；当前事实源是 `tests/unit/clib/test_clib_core.c`。
- 若扩展 `xy_math` / `xy_stdlib` 等尚未充分覆盖的 API，优先给 `test_clib_core.c` 增加 focused public-contract 用例，再修改实现。
- 嵌入式目标的内存分配策略仍需由平台/项目决定；host tests 使用 test shim，不能代表 MCU heap/pool 策略已经完成。
- 触碰 C/H 时按仓库风格格式化，并至少运行 focused `clib_component`、`make test-unit` 与 `git diff --check`。

## 5. 当前结论

CLIB 不再是“功能待补充”的空白组件：核心 runtime 入口、root build 集成与 host CTest 护栏均已存在。后续只按具体 API 缺口、真实构建失败或新增 runtime 需求做小 slice；不要进行大规模 libc 替换或配置模型迁移。
