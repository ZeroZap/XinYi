# XinYi Kernel host guard 状态同步（2026-08-08）

## 背景

`docs/component-roadmap.md` 仍把 Kernel Service 归为“60% / 系统监控/定时器待完善”，但当前仓库已经具备一组主线 host CTest 护栏：

- `osal_baremetal`：Bare-metal OSAL primitives、tick/delay、软件定时器单线程契约。
- `kernel_autotask`：`xy_autotask` 初始化、触发、空闲超时、暂停/恢复、tick wraparound、回调与统计契约。
- `bootreason_check`：portable bootreason guard/override 路径。

本次同步只更新 Kernel README，不迁移目录、不改 backend 实现、不碰 MCU/third_party/vendor 树。

## 当前结论

Kernel 组件应按“host-guarded / backend 与板级实证待补”理解：

| 子模块 | 当前状态 | 后续边界 |
| --- | --- | --- |
| OSAL | CMSIS-like public API 与 bare-metal host contract 已有主线 CTest。 | 真实线程调度、ISR 语义、低功耗唤醒仍需 backend/板级证据。 |
| Misc | `xy_sysmon` 与 `xy_autotask` 可发现；AutoTask 已有 focused CTest。 | `xy_sysmon` 后续只按真实指标/平台失败补最小回归。 |
| Service | `bootreason_check` 有 portable host CTest。 | RTC/backup-register/复位源等真实来源必须由 BSP/project smoke 记录。 |

## 不做的事

- 不把 OSAL/backend 目录做大规模重排；旧 `docs/design/osal_layout_optimization.md` 属于高风险迁移 proposal，不能直接在 cron slice 中执行。
- 不把 host fake 输出升级为 RTOS/backend/板级通过证据。
- 不新增 root `tests/CMakeLists.txt` 或 cron/kanban；active unit suite 仍是 `make test-unit` → `tests/unit/CMakeLists.txt`。

## 推荐下一步

若继续 Kernel 方向，优先选择一个可回滚的小 slice：

1. 为 `xy_sysmon` 补一个 focused host CTest（只覆盖现有 public contract 与 null/threshold guard）。
2. 为某个 OSAL backend 做 compile-only probe，并明确这不是调度/ISR 实证。
3. 根据真实板卡日志补 bootreason validation record；没有硬件证据时保持 pending。
