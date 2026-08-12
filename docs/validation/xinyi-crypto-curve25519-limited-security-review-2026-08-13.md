# XinYi Crypto Curve25519/Ed25519 limited security review

**组件**: `components/crypto`  
**审查对象**: `curve25519_generic` / `curve25519_cortex_m0` helper areas  
**记录日期**: `2026-08-13`  
**审查人**: `Zero / scheduled component-quality loop`  
**状态**: `security-reviewed-limited`

> 重要：本记录只固定当前 Curve25519/Ed25519 helper 区域的有限安全边界。结论不得由 host CTest 通过、deterministic test stubs、Cortex-M0 field smoke、或 compile-only 结果替代或升级为完整密码学安全审计。

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
  - `components/crypto/xy_25519/xy_25519.c`
  - `components/crypto/xy_25519/xy_25519.h`
  - `components/crypto/xy_25519/xy_25519_m0.c`
  - `components/crypto/xy_25519/fe25519_m0.c`
  - `components/crypto/xy_25519/fe25519_m0.h`
  - `components/crypto/xy_25519/asm/`
  - `components/crypto/xy_25519/README.md`
  - `components/crypto/xy_25519/README_M0.md`
  - `tests/unit/crypto/test_25519.c`
  - `tests/unit/crypto/test_25519_m0.c`
  - `components/crypto/crypto_review_manifest.json`
- Upstream/project origin:
  - The generic and Cortex-M0 sources are historical in-tree helpers. The M0 README/assembly material documents upstream-style Cortex-M0 optimization context, but this slice did not link a reviewed upstream URL/tag/tarball hash/import diff for the full generic and M0 implementation set.
- License evidence:
  - Current files do not provide enough SPDX/upstream license evidence to mark provenance as reviewed. `provenance_status` therefore remains `review-pending`.
- Local modifications:
  - No Curve25519/Ed25519 implementation code changes in this review slice.
- Ownership notes:
  - Both entries remain focused-test-only in `crypto_review_manifest.json`; neither implementation is part of the root/runtime `components/crypto/src/*.c` aggregate target.
  - The Cortex-M0 entry remains `focused-test-only-upstream-material`: host tests use C fallbacks for assembly hooks and cannot prove target assembly correctness or timing behavior.
  - This record does not add these implementations to `xy_tiny_crypto`, change root `COMPONENT_CRYPTO` default-off policy, or reconcile algorithm ownership.

## 3. Security review

- Intended use:
  - Contract-guarded X25519/Ed25519 helper experimentation for explicitly opted-in firmware consumers that accept current provenance, constant-time, RNG, and target-specific evidence gaps.
- Explicit non-goals:
  - No approval for production key exchange, signatures, authentication, firmware update trust, secure boot, identity, pairing, certificate, or password/key-storage workflows without a separate product security review.
  - No constant-time/side-channel audit, formal verification, fuzzing, independent cryptographic audit, misuse-resistance review, hardware acceleration evidence, or target assembly validation.
  - No approval to export these helpers through the default root runtime target or enable `COMPONENT_CRYPTO` by default.
- Known placeholder/legacy/weak areas:
  - `crypto_25519` uses deterministic `xy_random_bytes` / `xy_sha512_hash` fakes in host tests. That proves API dependency wiring and guard behavior only; it is not entropy-source or hash provenance evidence.
  - The generic tests currently cover null guards, weak public-key rejection, RNG failure propagation, keypair/public-key derivation consistency, and Ed25519 hash dependency shape. They do not prove RFC/official vector conformance, constant-time scalar multiplication, signature security, or key-management policy.
  - `crypto_25519_m0` covers public null guards, a weak-key boundary, field-operation smoke, and pack/unpack low-value behavior through host C fallbacks. It does not prove Cortex-M0 assembly correctness, cycle/timing behavior, or equivalence across all field elements.
  - The M0 assembly files require separate target build/run evidence before any MCU-specific performance or security claim.
- Test evidence:
  - `crypto_25519` covers generic X25519/Ed25519 public API contract and dependency seams.
  - `crypto_25519_m0` covers Cortex-M0 public API and field smoke contract with host fallback assembly shims.
  - `crypto_review_manifest` links this review record and keeps both entries out of provenance-approved or hardware-validated status.
- Missing evidence:
  - Upstream URL/tag/hash, license/provenance review, authoritative RFC/official vectors, independent security audit, side-channel/constant-time review, fuzzing, RNG/hash provenance, target assembly equivalence tests, MCU hardware validation, and a root-runtime ownership decision.

## 4. Decision

- Decision status: `security-reviewed-limited`
- Allowed product usage:
  - Contract-guarded experimentation only where missing provenance, missing constant-time evidence, deterministic host fakes, focused-test-only ownership, and target-specific assembly gaps are explicitly accepted.
  - These helpers remain prohibited for production security-sensitive key exchange, signatures, authentication, secure boot, identity, pairing, or certificate workflows until separate provenance/security/product review records exist.
- Required follow-up before stronger claims:
  - Link upstream source/version/license evidence and record any local import modifications.
  - Add authoritative vectors for each public algorithm/path intended for use.
  - Add constant-time/side-channel and RNG/hash dependency review before security-sensitive use.
  - Add Cortex-M0 target assembly equivalence/performance/timing validation before MCU-specific claims.
  - Reconcile focused-test-only vs root/runtime ownership before exposing through `xy_tiny_crypto`.
  - Add hardware validation only through separate board/MCU evidence records.

## 5. Verification commands

Recorded for this review-record/manifest slice:

```bash
cd build/tests/unit && ctest --output-on-failure -R '^(crypto_review_manifest|crypto_25519|crypto_25519_m0)$'
make test-unit
git diff --check
```

## 6. Attachments / evidence

- `tests/unit/crypto/test_25519.c` generic 25519 host contract coverage.
- `tests/unit/crypto/test_25519_m0.c` Cortex-M0 public API and field smoke coverage.
- `docs/design/xinyi-crypto-source-ownership-map-2026-08-12.md` root/runtime vs focused-test source ownership map.
- `components/crypto/crypto_review_manifest.json` `curve25519_generic` and `curve25519_cortex_m0` entries.
