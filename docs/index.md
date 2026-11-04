# XinYi 文档索引

本页只列出**仓库中真实存在**的文档。早期版本索引里的一整套规划目录
（`build-system/`、`testing/`、`architecture/`、`coding-standards/`、每个外设一页的 HAL 文档）
从未落地，链接因此全是死链；现在改以实际目录结构为准。

## 快速导航

### 🚀 快速开始

- [**5 分钟上手**](getting-started/QUICK_START.md) - 第一条可编译、可运行的路径
- [**开发者指南**](getting-started/DEVELOPER_GUIDE.md) - 完整开发流程
- [**工具链与环境**](getting-started/toolchain.md) - 交叉编译器与工具链位置
- [**构建指南**](BUILD_GUIDE.md) - 构建目录说明和常用命令
- [**入门总览**](getting-started/introduction.md) - 项目定位与目录结构

### 📚 组件文档

- [**组件总览**](components/index.md) - 各组件状态、证据边界与 Sprint 事实

| 组件 | 文档 | 组件 | 文档 |
|------|------|------|------|
| OSAL | [简介](components/osal/introduction.md) | HAL | [简介](components/hal/introduction.md) |
| Crypto | [简介](components/crypto/introduction.md) | CLib | [简介](components/clib/introduction.md) |
| DM | [简介](components/dm/introduction.md) | NET | [简介](components/net/introduction.md) |
| Device | [简介](components/device/introduction.md) |
| IPC | [简介](components/ipc/introduction.md) | PM | [简介](components/pm/introduction.md) |
| PID | [简介](components/pid/introduction.md) | ADDC | [简介](components/addc/introduction.md) |
| FOTA | [简介](components/fota/introduction.md) | GUI | [简介](components/gui/introduction.md) |
| Trace | [简介](components/trace/introduction.md) | 驱动模板 | [组件驱动模板](components/device/DRIVER_TEMPLATE.md) |

### 🔧 API 参考

- [**API 索引**](api/index.md) - 按组件浏览的入口
- [**API 参考**](api/API_REFERENCE.md) - 已核对的函数签名
- [**HAL 统一 API**](hal/HAL_UNIFIED_API.md) - 句柄模型与外设接口

### 🧭 硬件与平台

- [**硬件概览**](hardware/index.md) - 支持的 MCU 与外设
- [**开发板支持**](hardware/boards.md) - Pandora STM32L475VE 等板级事实
- [**移植到新平台**](hardware/porting.md) - HAL 后端接入步骤
- [**RTOS 选择指南**](hardware/rtos_selection_guide.md) - FreeRTOS/RT-Thread/RTX 对比
- [**Pandora 板级记录**](hardware/pandora_stm32l475/) - 板级边界与证据

### 🧪 验证与计划

- [**组件证据矩阵**](validation/component-evidence-matrix.md) - 每个组件到 B1/B2 的差距
- [**HAL 平台证据矩阵**](validation/hal-platform-evidence-matrix.md) - 逐平台、逐外设状态
- [**Sprint 跟踪看板**](plans/SPRINT_TRACKER.md) - 当前执行状态与阻塞

### 📐 规范与设计

- [**RULEBOOK**](rules/RULEBOOK.md) - 开发规则
- [**代码风格**](rules/code_style.md) - 详细编码规范
- [**C 编码标准（完整）**](rules/C_Coding_Standard_Full.md)
- [**项目概览**](design/XinYi_Project_Overview.md)
- [**单元测试清单**](design/unit-test-inventory.md) - 活跃 CTest 与覆盖归属

### ❓ 其他

- [**常见问题**](about/faq.md)
- [**更新记录**](about/changelog.md)
- [**贡献指南**](contribute/index.md)
- [**示例代码**](samples/index.md)

---

## 文档状态

| 文档 | 完整度 | 更新日期 | 状态 |
|------|--------|----------|------|
| HAL 组件文档 | 分层证据 | 2026-10-01 | ✅ 事实源已同步 |
| OSAL 组件文档 | 分层证据 | 2026-10-01 | ✅ Pandora bounded runtime |
| Device 组件文档 | host-guarded | 2026-08-11 | ✅ README/xy_device root build/registry+buses+async CTest 已护栏 |
| CLIB 组件文档 | 85% | 2026-02-28 | ✅ 完善 |
| Crypto 组件文档 | 90% | 2026-08-11 | ✅ host-guarded |
| DM 组件文档 | 85% | 2026-08-11 | ✅ host-guarded |
| Net 组件文档 | host-guarded / LTE 硬件待证据 | 2026-08-11 | ✅ 软件契约已护栏 |
| Trace 组件文档 | 85% | 2026-08-11 | ✅ 完善 |
| Sensor 组件文档 | 95% | 2026-08-11 | ✅ tail host coverage 已收口 |
| IPC 组件文档 | host-guarded | 2026-08-11 | ✅ pipe/broker/message queue/observer/event group 已护栏 |
| PM 组件文档 | host-guarded / 功耗待实证 | 2026-08-11 | ✅ README/Kconfig/CMake/CTest 已闭环 |
| FOTA 组件文档 | host-guarded / 硬件待证据 | 2026-10-01 | ✅ host CTest；board/security pending |
| GUI 组件文档 | host-guarded core / 硬件待验证 | 2026-08-11 | ✅ core/widgets/effects/fonts/display-backend adapter 已有护栏 |

> 说明：该表只记录软件文档与 host-side 契约状态；LTE modem、Fuel Gauge SMBus、FOTA bootloader/board NOR、GUI/Display 真实屏幕等硬件结论仍必须来自 `docs/validation/` 下的真实板级记录，不能由 host fake 或 compile-only 结果替代。

---

## 贡献文档

### 编写规范

1. 使用标准 Markdown 格式
2. 遵循 [编码规范](rules/code_style.md)
3. 提供实际代码示例
4. 保持文档与代码同步 —— 相对链接必须能从当前页面打开，`check_doc_relative_links.py` 会在
   `make test-unit` 中判定

### 模板

```markdown
# <组件名称> 文档

## 概述

<组件功能概述>

## API 参考

### 函数

`xy_<module>_<function>(<parameters>)`

<函数说明>

**参数**:
- `param1`: 参数说明

**返回值**:
- `XY_OK`: 成功
- `XY_ERROR_*`: 错误码

## 使用示例

```c
// 代码示例
```

## 配置选项

<配置选项说明>
```

---

## 联系与支持

- **问题反馈**: [Issues](https://github.com/zerozap/issues)
- **文档纠错**: [Pull Requests](https://github.com/zerozap/pulls)
- **技术支持**: zerozap2020@gmail.com

---

**最后更新**: 2026-11-02
**维护者**: XinYi Team
**许可证**: Apache License 2.0
