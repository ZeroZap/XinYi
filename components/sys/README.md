# XinYi System Components - 系统组件

**状态**: host-guarded / root buildable / hardware hooks pending
**日期**: 2026-08-12

## 当前事实源

- 根构建会自动发现 `components/sys/CMakeLists.txt`，当前产出 `xy_sys` 静态库，并提供 `sys_component` alias。
- `xy_timer/xy_timer.{h,c}` 提供裸机软件定时器链表、tick 查询、callback 参数和 self-kill 契约。
- `xy_state_machine/xy_st.{h,c}` 提供 bare-metal 默认状态机、timeout transition/delay、timeout reset/cancel 与 NULL guard 契约。
- `xy_sys/xy_sys.{h,c}` 只保留平台可覆盖的 weak 系统信息/重启 stub；真实 reboot reason、chip id、MAC、reset 行为仍需 board/backend 实现。

## Host 单元测试

活跃 focused CTest：

| CTest | 覆盖范围 |
| --- | --- |
| `sys_timer_sm` | timer lifecycle/order、callback 参数、periodic/self-kill、state-machine transition、timeout fire/reset/cancel/NULL guard |

常用验证：

```bash
cmake --build build/tests/unit --target test_sys_timer_sm -j$(nproc)
cd build/tests/unit && ctest --output-on-failure -R '^sys_timer_sm$'
cmake --build build/pc --target xy_sys -j$(nproc)
make test-unit
git diff --check
```

## 边界与 backlog

- 现有 host CTest 只证明定时器/状态机的软件契约，不代表真实硬件 tick ISR、低功耗 wakeup、watchdog/reset 或 bootreason backend 已验证。
- `xy_sys` 默认实现是 weak no-op/stub，板级 reset/chip-id/MAC/reboot-reason 需要在 project/BSP 层覆盖并提供硬件验证记录。
- 若后续需要 RTOS safe state-machine API，应先明确 OSAL mutex/event backend 语义并补 focused host/RTOS probe；不要在本组件内直接依赖 vendor RTOS API。

## 回滚

本轮 root build / README 同步可用以下命令回滚未提交改动：

```bash
git checkout -- components/sys/CMakeLists.txt components/sys/xy_sys/xy_sys.c components/sys/xy_sys/xy_sys.h components/sys/README.md
```
