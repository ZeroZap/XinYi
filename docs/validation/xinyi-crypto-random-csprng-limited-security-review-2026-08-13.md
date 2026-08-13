# XinYi Crypto Random/CSPRNG limited security review

**组件**: `components/crypto`  
**审查对象**: `random_csprng` / simple RNG and ChaCha20-based CSPRNG helper area  
**记录日期**: `2026-08-13`  
**审查人**: `Zero / scheduled component-quality loop`  
**状态**: `security-reviewed-limited`

> 重要：本记录只固定当前 RNG/CSPRNG 区域的有限安全边界。结论不得由 host CTest 通过、deterministic output shape、fake entropy、或 STM32 compile-only 结果替代或升级为正式熵源审查、CSPRNG 安全审计或硬件 RNG 验证。

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
  - `components/crypto/xy_rng/xy_random.c`
  - `components/crypto/xy_rng/xy_csprng.c`
  - `tests/unit/crypto/test_random.c`
  - `tests/unit/crypto/test_csprng.c`
  - `components/crypto/crypto_review_manifest.json`
- Upstream/project origin:
  - Current repository-owned lightweight implementations. This slice did not link a reviewed upstream URL/tag/tarball hash/import diff for either the simple RNG or CSPRNG code.
- License evidence:
  - Repository component inherits project licensing context; no separate upstream license/provenance record was linked for these files in this slice.
- Local modifications:
  - No RNG/CSPRNG implementation code changes in this review slice.
- Duplicate ownership notes:
  - Root/runtime `xy_tiny_crypto` now consumes `xy_rng/xy_random.c` and `xy_rng/xy_csprng.c`, the same sources as the focused `crypto_random` and `crypto_csprng` CTests.
  - Historical `components/crypto/src/xy_random.c` and `components/crypto/src/xy_csprng.c` duplicates have been removed; `crypto_review_manifest` records the active runtime policy as `single-active-source` for the module-owned RNG sources.
  - This record does not expose new APIs, enable `COMPONENT_CRYPTO` by default, or claim hardware entropy availability.

## 3. Security review

- Intended use:
  - `xy_random_*`: simple/randomness-shaped helper for API-contract smoke and non-security use only.
  - `xy_csprng_*`: contract-guarded CSPRNG helper API that may only be considered where the consumer explicitly provides and validates seed/entropy policy outside this component.
- Explicit non-goals:
  - No approval that `xy_random_bytes()` is cryptographically secure, unpredictable, or suitable for keys/nonces/tokens.
  - No approval for seed generation, entropy collection, entropy health tests, TRNG/HWRNG integration, FIPS/DRBG compliance, side-channel resistance, fork/process safety, persistent reseed policy, or hardware validation.
  - No claim that host deterministic tests prove entropy quality or production randomness.
- Known placeholder/legacy/weak areas:
  - `crypto_random` intentionally validates API shape, zero-length behavior, and non-zero output shape only; it does not assert unpredictability.
  - `crypto_csprng` uses deterministic seed/entropy fixtures to check lifecycle, buffering, reseed, and integer helpers; those fixtures are not entropy-source evidence.
  - CSPRNG security depends on caller-owned seed/entropy source quality and reseed policy, neither of which is reviewed or implemented as a product-level hardware source in this slice.
  - Historical root duplicate files have been removed in a later verified source-ownership slice; this record still does not upgrade provenance/security claims.
- Test evidence:
  - `crypto_random` covers invalid params, zero-length no-op behavior, requested-length writes, and repeated `xy_random_uint32()` availability.
  - `crypto_csprng` covers init/cleanup guards, deterministic split-vs-full buffering, reseed output change, and integer/uniform helper boundaries.
  - `crypto_review_manifest` links this review record and keeps provenance pending plus single-active-source/source-map policy checks active.
- Missing evidence:
  - External provenance/license record, independent DRBG/CSPRNG security audit, authoritative DRBG vectors, entropy-source design, entropy health tests, side-channel review, fuzzing, and hardware RNG evidence.

## 4. Decision

- Decision status: `security-reviewed-limited`
- Allowed product usage:
  - `xy_random_*`: non-security utility/test/demo use only; prohibited for key generation, nonce generation, authentication tokens, session IDs, pairing, secure boot, firmware-update trust, or any security-sensitive randomness.
  - `xy_csprng_*`: contract-guarded helper use only where a consuming product explicitly owns seed/entropy quality, reseed policy, and missing audit evidence; not certified, not hardware-validated, and not an entropy-source implementation.
- Required follow-up before stronger claims:
  - Link provenance/license evidence for each RNG/CSPRNG implementation source.
  - Define and validate a product entropy-source/HWRNG/seed-injection policy.
  - Add DRBG/CSPRNG review, authoritative vectors, health tests, and side-channel considerations for security-sensitive use.
  - Keep source-map and manifest guards aligned with the RNG/CSPRNG module-directory single-active-source policy.
  - Add hardware RNG or board entropy evidence only through separate hardware validation records.

## 5. Verification commands

Recorded for this review-record/manifest slice:

```bash
cd build/tests/unit && ctest --output-on-failure -R '^(crypto_review_manifest|crypto_random|crypto_csprng)$'
make test-unit
git diff --check
```

## 6. Attachments / evidence

- `tests/unit/crypto/test_random.c` simple RNG host API-shape coverage.
- `tests/unit/crypto/test_csprng.c` deterministic CSPRNG lifecycle/buffering coverage.
- `docs/design/xinyi-crypto-source-ownership-map-2026-08-12.md` root/runtime vs focused-test source ownership map.
- `components/crypto/crypto_review_manifest.json` `random_csprng` entry.
