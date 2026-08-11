# XinYi Trace Component

**状态**: ✅ host-guarded logging runtime

## 1. 组件定位

`components/trace` 是 XinYi 的轻量日志/trace runtime。当前主线实现集中在
`xy_log`：提供编译期日志等级宏、`xy_log_str()` / `xy_log_raw()` 字符输出、
`xy_log_init()` 与动态等级缓存 helper。它依赖项目内 `xy_stdio` 格式化输出，并通过
弱符号 `xy_log_char()` 作为最终字符输出 seam，便于 host 单测或平台 backend 覆盖。

当前 trace 不应被当作完整 shell/command framework；`xy_cmd/shell_cmd.md` 只是历史命令说明，
没有主线 CMake/CTest runtime 入口。

## 2. 当前目录与构建入口

| 路径 | 角色 |
| --- | --- |
| `xy_log/inc/xy_log.h` | public logging header；声明日志等级、宏、字符串/raw 输出与动态等级 helper。 |
| `xy_log/inc/xy_stdio.h` | compatibility include；转发到 CLIB 的 `xy_stdio.h`。 |
| `xy_log/src/xy_log.c` | logging runtime；弱 `xy_log_char()` seam、`xy_stdio_printf_init()` 绑定与动态等级缓存。 |
| `Kconfig` | nested trace config（`XY_TRACE_ENABLE` / test / example）；当前不是根 Kconfig 生成配置事实源。 |
| `CMakeLists.txt` | `xy_trace` static library 入口；根 `CMakeLists.txt` 会自动发现并加入。 |

## 3. Host 测试护栏

主线 PC unit suite 已注册：

| CTest | Target | 覆盖范围 |
| --- | --- | --- |
| `trace_component` | `test_trace` | `xy_log_str/raw` NULL 与零长 guard、弱字符输出 seam、`xy_log_init()`、public log macro 输出、动态等级边界。 |

常用验证命令：

```bash
cmake -B build/pc -S . -DHAL_PLATFORM=PC -DCMAKE_BUILD_TYPE=Release
cmake --build build/pc --target xy_trace -j$(nproc)

cmake -B build/tests/unit -S tests/unit
cmake --build build/tests/unit --target test_trace -j$(nproc)
ctest --test-dir build/tests/unit --output-on-failure -R '^trace_component$'
make test-unit
git diff --check
```

## 4. 维护边界

- 不要在没有 root Kconfig 设计的情况下把 nested `XY_TRACE_ENABLE` 接成可关闭核心日志开关；许多组件默认依赖 `xy_log.h` / `xy_trace`。
- `XY_LOG_LEVEL` 是编译期宏过滤；`xy_log_set_dynamic_level()` 当前只维护动态等级缓存，现有 public macros 不按动态等级过滤输出。
- `xy_log_char()` 是 platform/host 输出 seam；host 测试可重定义该符号观察输出，平台 backend 应提供真实字符 sink。
- `g_xy_log_dinamic_level` 保留历史拼写以避免 ABI/extern drift；新增代码应优先使用 `xy_log_set_dynamic_level()` / `xy_log_dynamic_level()`。
- 触碰 C/H 时按仓库风格格式化，并至少运行 focused `trace_component`、`make test-unit` 与 `git diff --check`。

## 5. 当前结论

Trace 不再是“只有源码、无事实源”的空白组件：根构建会产出 `xy_trace`，核心 logging runtime 有 focused host CTest 护栏。后续只按真实输出 backend、动态等级过滤策略或 shell/command 需求做 proposal + 小回归；不要把历史 `xy_cmd` 文档直接升级为 runtime 入口。
