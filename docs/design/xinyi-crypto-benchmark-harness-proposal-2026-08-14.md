# XinYi Crypto 可复现 benchmark harness proposal

**日期**: 2026-08-14  
**状态**: proposal / no benchmark claim  
**范围**: `components/crypto/` 的 host-guarded 软件实现、未来 MCU benchmark harness、review manifest 与 source ownership map 的证据边界。

## 1. 背景

Crypto 组件已经进入 `contract-guarded` 阶段：root `xy_tiny_crypto`、focused crypto CTest、source ownership map 与 review manifest 已经守住 API 向量、placeholder 边界和 duplicate-source ownership。下一类容易误推进的工作是“优化”或“汇编加速”：如果没有统一 benchmark harness，很容易把 host CTest、单次 PC timing、或未记录编译参数的 MCU smoke 输出误写成性能/安全结论。

因此优化前应先固定 benchmark harness 的输入、输出、平台、门禁与非目标。本 proposal 只定义 harness 形状，不实现汇编、不改算法、不改变 `COMPONENT_CRYPTO` 默认关闭策略。

## 2. 目标

1. 为后续 Crypto 优化建立可复现 benchmark 证据格式。
2. 区分 correctness gate、performance measurement、security/provenance review 与 hardware validation。
3. 保持 C fallback 可用：任何优化实现都必须能与当前 C/module source 做向量对照。
4. 避免把 host CTest 或未校准 timing 当作 MCU 性能结论。

## 3. 非目标 / 禁止事项

- 不在本 proposal 中启用或实现 Cortex-M0/M4 汇编、T-table、硬件加速或外部 crypto 库。
- 不把 `crypto_*` host CTest 通过升级为 `security-reviewed`、`provenance-reviewed` 或 `hardware-validated`。
- 不改变 `components/crypto/crypto_review_manifest.json` 中的 review-pending / limited / rejected 证据等级。
- 不默认启用 `COMPONENT_CRYPTO`，不重命名 `xy_tiny_crypto`，不批量删除 `components/crypto/src/`。
- 不使用真实密钥、生产证书、secure-boot signing key 或产品身份材料作为 benchmark 输入。

## 4. Harness 分层建议

| 层级 | 作用 | 允许证据 | 不允许声称 |
| --- | --- | --- | --- |
| correctness gate | 证明优化路径与 C fallback 输出一致 | focused CTest、公开 KAT、root smoke | 性能提升、安全审计通过 |
| host micro-benchmark | 检查 harness 稳定性和 API overhead | PC 时间、迭代次数、编译器版本 | MCU 周期数、功耗、硬件通过 |
| MCU cycle benchmark | 记录目标 MCU 上的 cycle/us/bytes-per-second | target、clock、compiler flags、缓存/中断状态、样本统计 | 安全/来源审查、侧信道证明 |
| hardware/security review | 审查实现来源、常数时间、RNG/nonce/key policy | review record、logic/trace、人工结论 | 由 benchmark 自动推出 |

## 5. 最小 benchmark 输入集

后续实现 harness 时，每个算法组至少应包含：

- API 名称和 source ownership：root runtime source、focused-test source、是否 wrapper。
- 输入大小：0、1、短消息、block 边界、multi-block、典型 payload（例如 64B/1KiB/4KiB）。
- 迭代次数和 warm-up 次数。
- 固定 test key/nonce/seed：仅用于 benchmark，不可复用生产材料。
- 输出 digest/tag/ciphertext 校验：benchmark 前后都要验证 correctness，防止只测空 loop。
- 编译信息：compiler、flags、`HAL_PLATFORM`、commit hash、dirty 状态。

## 6. 后续实现边界

建议把后续实现拆成小 slice：

1. **host-only skeleton**：新增 `tests/unit/crypto/crypto_benchmark_manifest.json` 与一个 policy smoke，先校验 manifest 字段与 no-claim wording，不计时。
2. **PC timing prototype**：增加 opt-in CLI 或 CTest-disabled helper，只输出 PC timing metadata；默认 `make test-unit` 不因机器性能波动失败。
3. **STM32U5 compile probe**：只证明 benchmark harness 可编译，不声称硬件 timing。
4. **真实 MCU run record**：手工/自动记录 UART/SWO/log 输出、clock 配置、样本统计与 dirty 状态，写入 `docs/validation/`。
5. **优化实现 slice**：每次只换一个算法/一个 backend，保留 C fallback 和 focused correctness gate。

## 7. 建议验证命令

本 proposal 为 docs-only slice，验证口径：

```bash
make test-unit
git diff --check
```

后续若实现 harness，应追加：

```bash
cd build/tests/unit && ctest --output-on-failure -R '^crypto_'
cmake --build build/pc --target xy_tiny_crypto -j$(nproc)
make HAL_PLATFORM=STM32U5 -j$(nproc)
```

## 8. 回滚方式

未提交时：

```bash
git checkout -- docs/design/xinyi-crypto-benchmark-harness-proposal-2026-08-14.md
```

提交后：

```bash
git revert <commit>
```
