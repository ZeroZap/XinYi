# XinYi Crypto 安全等级与来源审查计划

**日期**: 2026-08-12  
**状态**: proposal / review-pending  
**范围**: `components/crypto/` 当前 host-guarded 软件实现、历史 aggregate source、module-directory source、focused crypto CTest 与 public smoke example。

## 1. 背景

Crypto 组件当前已经具备主线构建入口 `xy_tiny_crypto`、root `COMPONENT_CRYPTO` 默认关闭策略、以及 `tests/unit/crypto/` 下的 focused Unity/CTest 护栏。现有测试证明 public API guard path、公开向量和 host-safe smoke flow，但它们不能替代：

- 密码学安全审计；
- 第三方/上游代码来源与许可证审查；
- 侧信道或常数时间证明；
- 硬件加速或真实设备熵源验证；
- placeholder-grade 算法从“可编译/可测”升级为 production-safe。

因此后续 Crypto 推进应先固定 review policy，而不是直接批量整理 `src/` 与 module-directory duplicate source，或把 host CTest 通过误写为安全可用结论。

## 2. 当前证据分级

| 等级 | 含义 | 当前状态 |
| --- | --- | --- |
| `contract-guarded` | API 参数、错误码、公开向量、host smoke 已有 CTest | 当前多数 active APIs 属于此级别 |
| `implementation-known` | 明确 runtime/root target 使用哪份 source，duplicate ownership 已有 source map | 部分完成；`src/` 与 module 目录仍需 source-map proposal |
| `provenance-reviewed` | 来源、许可证、引入方式、修改记录已人工审查并有记录 | pending |
| `security-reviewed` | 已有算法正确性、安全等级、placeholder/non-goal、侧信道边界审查记录 | pending |
| `hardware-validated` | 硬件 RNG/加速器/MCU target 证据或真实设备日志 | pending |

默认汇报只能声称 `contract-guarded`，除非后续有对应审查记录或真实硬件证据。

## 3. Algorithm review matrix（初始清单）

| 区域 | 活跃入口 / 测试 | 当前结论 | 后续审查重点 |
| --- | --- | --- | --- |
| CRC | `crypto_crc` | public vector / table / software / fallback contract guarded | 确认 duplicate `src/xy_crc.c` vs `xy_crc/xy_crc.c` ownership |
| Base64/Hex | `crypto_encode`, `crypto_smoke_example` | encoding helper contract guarded | 确认 `src/` copy 与 `xy_base`/`xy_hex` ownership；无安全等级声明 |
| Random/CSPRNG | `crypto_random`, `crypto_csprng` | API/lifecycle guarded | 明确 entropy source non-goal；host deterministic seed 不代表真实熵源安全 |
| MD5/SHA-256/HMAC | `crypto_hash`, `crypto_cipher_hmac` | vector/incremental guarded | MD5 只能作为 legacy/integrity utility；不得宣称 collision-resistant security |
| AES/SM3/SM4/ChaCha20 | `crypto_cipher_hmac` | vector/API guarded | 需要模式、padding、nonce/key-management non-goal；ChaCha/Poly1305 需单独安全审查 |
| SM2 | `crypto_sm2` | public guard path / placeholder-grade contract | 继续标记 placeholder-grade；不得用于 production signing/encryption |
| LWC/Ascon | `crypto_lwc` | host public contract guarded | 来源/算法版本与参数集需 provenance/security review |
| Curve25519 generic | `crypto_25519` | public API contract guarded | provenance、constant-time 与 test vector completeness review |
| Curve25519 Cortex-M0 fallback | `crypto_25519_m0` | host fallback/API smoke guarded | 上游 CC0 material、assembly TODO/placeholder 注释与 target-specific correctness review |

## 4. Non-goals / 禁止误用

1. 本计划不批准任何算法进入 production security use。
2. 本计划不改变 `COMPONENT_CRYPTO` 默认关闭策略。
3. 本计划不整理 duplicate source，也不移动 API/目录。
4. 本计划不把 host fake、deterministic seed 或 CTest output 提升为真实熵源/硬件安全证据。
5. 本计划不导入外部 crypto 库；若需要替换实现，应单独写 migration proposal。

## 5. 后续低风险 slice 顺序

1. **Review record template**：新增 `docs/validation/xinyi-crypto-security-provenance-review-record-template-YYYY-MM-DD.md`，固定人工审查证据格式。
2. **Source ownership map**：为 `src/` aggregate copy 与 module-directory source 写只读 source-map proposal，并用 focused build/CTest 证明当前使用路径。
3. **Manifest smoke**：已新增 `components/crypto/crypto_review_manifest.json` 与 `crypto_review_manifest` CTest，记录每个 algorithm 的 status、source path、review tier，并防止无记录地改成 `security-approved` / `provenance-reviewed`。
4. **Specific algorithm review**：每次只审一个区域（例如 SM2 placeholder 或 CSPRNG entropy boundary），补最小 docs/test guard。

## 6. 验证口径

本 proposal 是 docs-only slice，验证应至少运行：

```bash
make test-unit
git diff --check
```

后续若修改实现或 source ownership，应追加 focused CTest，例如：

```bash
cd build/tests/unit && ctest --output-on-failure -R '^crypto_'
cmake --build build/pc --target xy_tiny_crypto -j$(nproc)
```

## 7. 回滚方式

本轮 proposal 可通过以下方式单文件回滚：

```bash
git revert <commit>
```

或在未提交时删除：

```bash
git checkout -- docs/design/xinyi-crypto-security-provenance-review-plan-2026-08-12.md
```
