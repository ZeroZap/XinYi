# XinYi Trace Component

**状态**: ✅ host-guarded logging runtime

## 1. 组件定位

`components/trace` 是 XinYi 的轻量日志/trace runtime。当前主线实现集中在
`xy_log`：提供编译期日志等级宏、`xy_log_str()` / `xy_log_raw()` 字符输出、
`xy_log_init()` 与动态等级缓存 helper。可检查的 `xy_log_try_set_dynamic_level()` 会明确返回
阈值更新是否被当前 Debug/Release ceiling 接受；既有 void setter 保留兼容。它依赖项目内
`xy_stdio` 格式化输出，并通过
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
| `trace_component` | `test_trace` | `xy_log_str/raw` NULL 与零长 guard、弱字符输出 seam、`xy_log_init()`、public log macro 输出、Debug 构建动态等级接受/拒绝结果与运行时过滤。 |
| `trace_release_contract` | `test_trace_release` | Release 默认 WARN ceiling、ERROR/WARN 更新成功、INFO/DEBUG/越界更新明确拒绝，以及外部越界动态等级 fail-closed。 |

常用验证命令：

```bash
cmake -B build/pc -S . -DHAL_PLATFORM=PC -DCMAKE_BUILD_TYPE=Release
cmake --build build/pc --target xy_trace -j$(nproc)

cmake -B build/tests/unit -S tests/unit
cmake --build build/tests/unit --target test_trace test_trace_release -j$(nproc)
ctest --test-dir build/tests/unit --output-on-failure \
    -R '^(trace_component|trace_release_contract)$'
make test-unit
git diff --check
```

## 4. 维护边界

- 不要在没有 root Kconfig 设计的情况下把 nested `XY_TRACE_ENABLE` 接成可关闭核心日志开关；许多组件默认依赖 `xy_log.h` / `xy_trace`。
- `XY_LOG_LEVEL` 负责编译期裁剪；链接 `xy_trace` 的 consumer 由 target 公开的 `XY_LOG_DYNAMIC_FILTER=1` 启用运行时阈值，public macros 会调用 `xy_log_dynamic_level()` 再决定是否输出。
- Release 构建的动态阈值上限是 WARN；INFO/DEBUG 更新被拒绝，外部强符号覆盖或内存扰动留下的越界值会在 getter 中 fail-closed 到 WARN。非 Release 上限是 DEBUG。
- `xy_log_char()` 是 platform/host 输出 seam；host 测试可重定义该符号观察输出，平台 backend 应提供真实字符 sink。
- 动态等级 storage 是内部实现细节；需要确认配置是否生效的调用方应使用
  `xy_log_try_set_dynamic_level()`，兼容调用可继续使用 `xy_log_set_dynamic_level()`，读取使用
  `xy_log_dynamic_level()`；不要声明或直接改写 backing symbol。
- 触碰 C/H 时按仓库风格格式化，并至少运行 focused `trace_component`、`trace_release_contract`、`make test-unit` 与 `git diff --check`。

## 5. 当前结论

Trace 不再是“只有源码、无事实源”的空白组件：根构建会产出 `xy_trace`，Debug/Release 动态过滤契约均有 focused host CTest 护栏。后续只按真实输出 backend、并发/ISR 丢日志策略或 shell/command 需求做 proposal + 小回归；不要把历史 `xy_cmd` 文档直接升级为 runtime 入口。
