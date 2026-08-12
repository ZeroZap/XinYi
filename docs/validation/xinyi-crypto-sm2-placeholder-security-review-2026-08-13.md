# XinYi Crypto SM2 placeholder security review

**组件**: `components/crypto`  
**审查对象**: `sm2` / `components/crypto/xy_sm2/xy_sm2.c`  
**记录日期**: `2026-08-13`  
**审查人**: `Zero / scheduled component-quality loop`  
**状态**: `security-rejected`

> 重要：本记录只审查当前 SM2 实现是否可作为安全实现使用。结论不得由 host CTest 通过、deterministic test vectors、fake entropy、或 STM32 compile-only 结果替代或升级。

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
  - `components/crypto/xy_sm2/xy_sm2.c`
  - `components/crypto/xy_sm2/xy_sm2.h`
  - `tests/unit/crypto/test_sm2.c`
  - `components/crypto/crypto_review_manifest.json`
- Upstream/project origin:
  - Current repository-owned placeholder material; no external upstream audit evidence was identified in this slice.
- License evidence:
  - Repository component inherits project licensing context; no separate SM2 upstream license or provenance record was linked in this slice.
- Local modifications:
  - No SM2 implementation code changes in this review slice.
- Duplicate ownership notes:
  - SM2 is currently listed as `single-active-source` in `crypto_review_manifest.json`: runtime and focused tests both reference `components/crypto/xy_sm2/xy_sm2.c`.

## 3. Security review

- Intended use:
  - SM2 public API compatibility and guard-path coverage only.
- Explicit non-goals:
  - No production signing, signature verification, key exchange, encryption, certified 国密 implementation, FIPS-style assurance, side-channel resistance, or hardware acceleration claim.
- Known placeholder/legacy/weak areas:
  - `xy_sm2.c` explicitly documents simplified placeholder arithmetic in the elliptic-curve helper path.
  - The active focused CTest guards API shape, invalid inputs, and placeholder-grade behavior; it does not prove SM2 cryptographic correctness.
  - No reviewed big-integer/elliptic-curve arithmetic, official SM2 vector suite, constant-time review, RNG/nonce review, or independent audit record is present.
- Test evidence:
  - `crypto_sm2` host CTest covers the current placeholder-grade public contract only.
  - `crypto_review_manifest` now links this review record so the rejected security status cannot be set silently without evidence.
- Missing evidence:
  - Official SM2 vectors, constant-time analysis, nonce/RNG review, provenance/license record, hardware/accelerator evidence, and independent security audit.

## 4. Decision

- Decision status: `security-rejected`
- Allowed product usage:
  - Test-only / compatibility-only placeholder. It must not be used for production signing, verification, encryption, key exchange, authentication, or any security-sensitive product path.
- Required follow-up before stronger claims:
  - Replace or rework the SM2 implementation with reviewed arithmetic and official vectors.
  - Add provenance/license evidence for the implementation source.
  - Add focused CTests for official SM2 vectors and negative/tamper cases.
  - Perform side-channel/RNG/nonce review before moving to any limited production claim.

## 5. Verification commands

Recorded for this review-record/manifest slice:

```bash
cd build/tests/unit && ctest --output-on-failure -R '^(crypto_review_manifest|crypto_sm2)$'
make test-unit
git diff --check
```

## 6. Attachments / evidence

- `components/crypto/xy_sm2/xy_sm2.c` placeholder comments and current implementation behavior.
- `tests/unit/crypto/test_sm2.c` focused host contract coverage.
- `components/crypto/crypto_review_manifest.json` `sm2` entry.
