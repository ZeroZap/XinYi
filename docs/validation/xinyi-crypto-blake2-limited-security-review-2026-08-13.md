# XinYi Crypto BLAKE2 limited security review

**组件**: `components/crypto`  
**审查对象**: `blake2` / BLAKE2s module-owned root runtime source
**记录日期**: `2026-08-13`  
**审查人**: `Zero / scheduled component-quality loop`  
**状态**: `security-reviewed-limited`

> 重要：本记录只固定当前 BLAKE2 区域的有限安全边界。结论不得由 host CTest 通过、deterministic test vectors、fake entropy、或 STM32 compile-only 结果替代或升级为完整密码学安全审计。

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
  - `components/crypto/xy_blake/xy_blake2.c`
  - `components/crypto/xy_blake/xy_blake2.h`
  - `tests/unit/crypto/test_blake2.c`
  - `components/crypto/crypto_review_manifest.json`
- Upstream/project origin:
  - The public header references RFC 7693 semantics and the implementation has host vector coverage, but this slice did not link upstream URL, version/tag, tarball hash, or import-diff evidence.
- License evidence:
  - No separate SPDX/upstream license/provenance record was linked for the BLAKE2 implementation files in this slice; provenance therefore remains `review-pending`.
- Local modifications:
  - No BLAKE2 implementation code changes in this review slice.
- Duplicate ownership notes:
  - BLAKE2 has been reconciled to the module-directory single active source `components/crypto/xy_blake/xy_blake2.c`; both the root `xy_tiny_crypto` target and focused `crypto_blake2` CTest consume that implementation.
  - The historical root/runtime duplicate `components/crypto/src/xy_blake2.c` has been removed. This record still does not upgrade provenance, side-channel, fuzzing, hardware, or compliance evidence.

## 3. Security review

- Intended use:
  - Contract-guarded BLAKE2s helper use for explicitly opted-in firmware consumers that accept current implementation risk, missing provenance evidence, and missing side-channel review.
- Explicit non-goals:
  - No FIPS, certification, formal verification, side-channel resistance, constant-time audit, hardware acceleration, keyed-MAC misuse-resistance approval, or production enablement approval.
  - No approval for enabling `COMPONENT_CRYPTO` by default or treating BLAKE2 as a certified or hardware-validated primitive.
- Known placeholder/legacy/weak areas:
  - Current focused coverage is BLAKE2s-only; BLAKE2b is compile-configurable but not part of the active host contract target in this slice.
  - Keyed BLAKE2s vector coverage proves local API behavior only; production use still needs caller-owned key management, domain separation, personalization/salt policy, and misuse review.
  - Source ownership has been reconciled to the module-directory single active source, but missing provenance and side-channel evidence still limit allowed usage.
- Test evidence:
  - `crypto_blake2` covers BLAKE2s empty/`abc` vectors, incremental-vs-one-shot behavior, keyed vector, invalid-parameter guards, and output preservation.
  - `crypto_root_target_smoke` exercises one root `xy_tiny_crypto` BLAKE2s vector through the aggregate target.
  - `crypto_review_manifest` links this review record and keeps single-active-source/source-map policy checks active.
- Missing evidence:
  - Upstream URL/tag/hash, license/provenance review, independent security audit, side-channel/constant-time review, fuzzing, broader RFC 7693 KAT corpus, BLAKE2b active-contract coverage if enabled, and hardware acceleration evidence.

## 4. Decision

- Decision status: `security-reviewed-limited`
- Allowed product usage:
  - Contract-guarded BLAKE2s helper use is acceptable only where current implementation risk, missing provenance, and missing side-channel evidence are explicitly accepted by the consuming product.
  - BLAKE2 remains prohibited as certified, compliance-sensitive, hardware-validated, or side-channel-reviewed implementation until separate evidence exists.
- Required follow-up before stronger claims:
  - Link upstream source/version/license evidence and record any local import modifications.
  - Keep source-map and manifest guards aligned with the module-directory single-active-source policy.
  - Add side-channel/constant-time and keyed-use misuse-resistance review for any security-sensitive product path.
  - Add broader authoritative KAT coverage, and BLAKE2b coverage if a product enables `XY_BLAKE2_ENABLE_BLAKE2B`.
  - Add hardware acceleration or MCU-specific validation only through separate hardware validation records.

## 5. Verification commands

Recorded for this review-record/manifest slice:

```bash
cd build/tests/unit && ctest --output-on-failure -R '^(crypto_review_manifest|crypto_blake2|crypto_root_target_smoke)$'
make test-unit
git diff --check
```

## 6. Attachments / evidence

- `tests/unit/crypto/test_blake2.c` BLAKE2s host vector and guard coverage.
- `tests/unit/crypto/crypto_root_target_smoke_probe/main.c` root-target BLAKE2s smoke path.
- `docs/design/xinyi-crypto-source-ownership-map-2026-08-12.md` root/runtime vs focused-test source ownership map.
- `components/crypto/crypto_review_manifest.json` `blake2` entry.
