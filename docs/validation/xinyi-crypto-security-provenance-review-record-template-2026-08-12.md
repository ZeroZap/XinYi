# XinYi Crypto 安全/来源审查记录模板

**组件**: `components/crypto`  
**审查对象**: `<algorithm or source area>`  
**记录日期**: `<YYYY-MM-DD>`  
**审查人**: `<name / role>`  
**状态**: `pending`（允许值见下方）

> 重要：本模板用于记录真实人工审查和证据。不得用 host CTest 通过、deterministic test vectors、fake entropy、或 STM32 compile-only 结果替代安全/来源审查结论。

## 1. 状态枚举

| 状态 | 含义 |
| --- | --- |
| `pending` | 尚未完成来源/安全审查；只能声称 contract-guarded |
| `provenance-reviewed` | 来源、许可证、修改记录已审查，但不代表安全可用于 production |
| `security-reviewed-limited` | 已明确安全等级、non-goals、placeholder/legacy 边界；适合限定用途 |
| `security-rejected` | 不适合作为安全实现；只能保留测试/兼容/历史用途 |
| `hardware-validated` | 另有真实硬件 RNG/加速器/板级证据；仍需说明范围 |

## 2. Source / provenance

- Source paths reviewed:
  - `components/crypto/...`
- Upstream/project origin:
  - `<URL, commit, vendor drop, original author, or project-owned>`
- License evidence:
  - `<license file / header / SPDX / unknown>`
- Local modifications:
  - `<summary or git range>`
- Duplicate ownership notes:
  - `<src aggregate vs module-directory copy mapping>`

## 3. Security review

- Intended use:
  - `<hash/integrity/encoding/RNG/signature/etc.>`
- Explicit non-goals:
  - `<e.g. no FIPS, no side-channel proof, no certified SM2>`
- Known placeholder/legacy/weak areas:
  - `<e.g. MD5 collision resistance, SM2 placeholder EC math>`
- Test evidence:
  - `<focused CTest names and vector source>`
- Missing evidence:
  - `<side-channel, fuzzing, formal vectors, hardware entropy, etc.>`

## 4. Decision

- Decision status: `pending`
- Allowed product usage:
  - `<none / test-only / non-security utility / limited production with constraints>`
- Required follow-up before stronger claims:
  - `<actions>`

## 5. Verification commands

Record real outputs here when a review changes docs/tests/code:

```bash
make test-unit
git diff --check
```

If implementation/source ownership changes, also record focused commands, for example:

```bash
cd build/tests/unit && ctest --output-on-failure -R '^crypto_'
cmake --build build/pc --target xy_tiny_crypto -j$(nproc)
```

## 6. Attachments / evidence

- `<links to licenses, upstream tarball hash, audit notes, hardware logs>`
