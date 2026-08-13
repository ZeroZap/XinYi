# XinYi Crypto ECDSA placeholder security review

**组件**: `components/crypto`  
**审查对象**: `ecdsa_root_format_only` / `components/crypto/src/xy_ecdsa.c`  
**记录日期**: `2026-08-13`  
**审查人**: `Zero / scheduled component-quality loop`  
**状态**: `security-rejected`

> 重要：本记录只审查当前 root aggregate ECDSA 实现是否可作为安全实现使用。结论不得由 host CTest 通过、format guard、root-target smoke、fake input、或 STM32 compile-only 结果替代或升级。

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
  - `components/crypto/src/xy_ecdsa.c`
  - `components/crypto/inc/xy_ecdsa.h`
  - `tests/unit/crypto/test_ecdsa_root_contract.c`
  - `tests/unit/crypto/crypto_root_target_smoke_probe/main.c`
  - `components/crypto/crypto_review_manifest.json`
- Upstream/project origin:
  - Current repository-owned simplified placeholder material; no external reviewed upstream ECDSA implementation or audit evidence was identified in this slice.
- License evidence:
  - Repository component inherits project licensing context; no separate ECDSA upstream license or provenance record was linked in this slice.
- Local modifications:
  - No ECDSA implementation code changes in this review slice.
- Duplicate ownership notes:
  - ECDSA is currently root-aggregate-only in the source map. Unlike CRC/Base64/Hex/etc., there is no module-directory duplicate source to reconcile in this slice.

## 3. Security review

- Intended use:
  - Root aggregate API compatibility and defensive format guard only.
- Explicit non-goals:
  - No production signature verification, signing, secure boot, firmware authenticity, authentication, authorization, key exchange, certificate workflow, certified P-256 implementation, side-channel resistance, or hardware acceleration claim.
- Known placeholder/legacy/weak areas:
  - `xy_ecdsa_p256_verify()` validates null pointers, non-zero public key, public key byte range, and `r/s` byte ranges, then returns success for valid-looking fields.
  - It explicitly does not hash or bind the message, compute modular inverse, perform elliptic-curve scalar multiplication, perform point addition, or compare the calculated signature relation.
  - The focused `crypto_ecdsa_root_contract` CTest documents the current format-only success contract, including that a tampered/different message still returns success when the public key and signature bytes are valid-looking, so consumers do not mistake it for real ECDSA validation.
  - Historical FOTA/security docs may mention ECDSA/signature verification as desired or historical capability; those statements must not be read as current production security evidence for this placeholder source.
- Test evidence:
  - `crypto_ecdsa_root_contract` covers null guards, zero/out-of-range fields, simple-wrapper malformed input, and the documented format-only success path including message non-binding.
  - `crypto_root_target_smoke` links the real `xy_tiny_crypto` root target and exercises ECDSA only as a minimal format-only guard path.
  - `crypto_review_manifest` now links this review record so the rejected security status cannot be set silently without evidence.
- Missing evidence:
  - Real P-256 arithmetic, official ECDSA positive/negative/tamper vectors, deterministic hash/signature workflow, constant-time analysis, provenance/license record, hardware/accelerator evidence, secure-boot integration proof, and independent security audit.

## 4. Decision

- Decision status: `security-rejected`
- Allowed product usage:
  - Test-only / compatibility-only / format-guard-only placeholder. It must not be used for production signature verification, signing, secure boot, firmware authenticity checks, authentication, authorization, key exchange, or any security-sensitive product path.
- Required follow-up before stronger claims:
  - Replace or rework the implementation with reviewed P-256/ECDSA arithmetic or bind to a reviewed library/hardware backend.
  - Add provenance/license evidence for the implementation source.
  - Add official ECDSA vectors, tamper-negative cases, hash binding, and integration tests for any intended secure-boot/FOTA use.
  - Perform side-channel and hardware/backend review before moving to any limited production claim.

## 5. Verification commands

Recorded for this review-record/manifest slice:

```bash
cd build/tests/unit && ctest --output-on-failure -R '^(crypto_review_manifest|crypto_ecdsa_root_contract|crypto_root_target_smoke)$'
make test-unit
git diff --check
```

## 6. Attachments / evidence

- `components/crypto/src/xy_ecdsa.c` simplified implementation comments and current behavior.
- `tests/unit/crypto/test_ecdsa_root_contract.c` focused host contract coverage.
- `tests/unit/crypto/crypto_root_target_smoke_probe/main.c` root target smoke path.
- `components/crypto/crypto_review_manifest.json` `ecdsa_root_format_only` entry.
