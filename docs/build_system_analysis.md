# 历史构建系统分析

> **状态**：superseded 历史报告。本文只保留早期构建系统观察，不作为当前构建命令、组件选择或成熟度的事实源。
>
> 当前事实源是 root Makefile、CMakeLists.txt 与 Kconfig，以及
> `docs/validation/kconfig-cmake-configuration-matrix.md`、
> `docs/validation/component-evidence-matrix.md` 和 `AGENTS.md`。
> Host/PC/compile-only 不构成实板、安全或 production-ready 证据。

## 历史背景

该报告最初按目录中是否存在 CMake、Kconfig 或 Makefile 来评价“统一性”，并给出通用模板和静态评分。后续验证证明这种方法不可靠：

- 文件存在不代表组件进入 root product target；
- focused CTest、root PC build 与目标交叉编译是不同证据层级；
- Kconfig symbol 必须通过项目生成器和 root CMake selection 验证；
- component-local Makefile 可能是历史入口，不能替代 root Makefile；
- 通用示例选项和命令若未由当前仓库定义，不应作为可执行指南。

因此，原有逐目录完成标记、8.5/10 静态评分、虚构的 component build targets、通用 `XY_COMPONENT_FEATURE_*` 示例和另建 CI workflow 的建议均已移除。

## 当前受检边界

### Canonical 入口

- 默认 PC Release build：`make`
- Host unit suite：`make test-unit`
- STM32U5 target build：`make HAL_PLATFORM=STM32U5`
- 配置生成：root Kconfig 经项目生成器输出 `.config`、`autoconf.h` 与 `config.cmake`
- CI：`.github/workflows/unit-tests.yml`

命令和平台值以 root Makefile、CMakeLists.txt、Kconfig 与 `AGENTS.md` 为准；历史 component-local 构建文件仅在被当前入口引用时才构成 active build evidence。

### 已验证事实

- canonical Host suite 和 PC root build 由 CI/Release 证据行记录；实际测试数量以 CTest 当次发现结果为准，不在本文硬编码。
- Kconfig/CMake 配置矩阵已验证 all-off、逐组件、Display 子功能、Net opt-in 与 STM32U5 默认组合；具体组合、SHA 和证据边界见 `docs/validation/kconfig-cmake-configuration-matrix.md`。
- explicit unsupported 或缺依赖的组合应 fail-closed，不能用空 target、empty CTest 或无说明的忽略错误路径制造假绿。
- `projects/`、component-local Makefile、source directory 或第三方目录存在，不自动表示 root-linked、target-compilable、runtime-validated 或 release-qualified。

## 仍待完成

- reference board 的 clean link/runtime 与 HIL；
- SDK/submodule、工具链及 supported project 的可复现 clean-checkout 构建；
- SBOM、许可证与 release artifact 重建；
- 对继续保留的 component-local Makefile 做 active/legacy/archive 分类。

这些项目应进入 Sprint tracker 后按 focused probe、对应 full/target gate、path-limited commit 闭环；不得从本历史报告直接推导优先级或完成状态。
