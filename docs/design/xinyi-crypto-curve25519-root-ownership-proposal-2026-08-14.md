# XinYi Crypto Curve25519/Ed25519 root ownership proposal

**Date**: 2026-08-14  
**Status**: proposal / no runtime ownership change  
**Scope**: `components/crypto/inc/xy_tiny_crypto.h`, `components/crypto/xy_25519/*`, root `xy_tiny_crypto` target, `crypto_25519`, `crypto_25519_m0`, `crypto_review_manifest`, and `crypto_root_target_smoke`.

## 1. Why this proposal exists

The current crypto source map has closed several duplicate-source groups by moving the root `xy_tiny_crypto` target to module-owned implementations one group at a time. LWC was the latest root-link slice: Ascon/TinyJAMBU/Photon-Beetle now link into the root aggregate target with a minimal root smoke guard, while security/provenance status remains limited.

Curve25519/Ed25519 is different and should not be copied into that path automatically:

- `components/crypto/inc/xy_tiny_crypto.h` declares X25519/Ed25519 APIs when `XY_CRYPTO_ENABLE_CURVE25519` is enabled.
- `components/crypto/xy_25519/xy_25519.c` is currently guarded by focused CTests only (`crypto_25519`).
- `components/crypto/xy_25519/xy_25519_m0.c` and `fe25519_m0.c` are also focused-test-only/upstream-material (`crypto_25519_m0`).
- The root `xy_tiny_crypto` target does **not** link `xy_25519.c`, so a root consumer that calls the declared X25519/Ed25519 symbols would currently need an explicit module-source link or a future ownership slice.
- The generic implementation depends on external `xy_random_bytes()` and `xy_sha512_hash()` seams, and the current host tests use deterministic fakes. That is contract coverage, not RNG/hash provenance evidence.
- `xy_ed25519_verify()` is not a production verifier; the existing limited review record keeps Curve25519/Ed25519 prohibited for production security-sensitive use until separate evidence exists.

This proposal records the boundary so the next cron slice does not silently add Curve25519 to the root runtime target just because header declarations exist.

## 2. Current evidence

| Area | Current guard | What it proves | What it does not prove |
| --- | --- | --- | --- |
| Generic X25519/Ed25519 | `crypto_25519` | Public null guards, weak X25519 public-key rejection, RNG failure propagation, deterministic key derivation shape, SHA-512 dependency seam | RFC vector conformance, entropy quality, SHA-512 provenance, constant-time scalar multiplication, signature security |
| Cortex-M0 25519 material | `crypto_25519_m0` | Host fallback public guard and field smoke contracts | Target assembly equivalence, target timing/cycle behavior, side-channel behavior |
| Review manifest | `crypto_review_manifest` | Both entries remain `review-pending` provenance and `security-reviewed-limited`, with focused-test-only ownership | Security approval, provenance approval, root runtime link availability |
| Root target | `crypto_root_target_smoke` | Other reconciled module/root paths link through `xy_tiny_crypto` | X25519/Ed25519 root link availability |

## 3. Proposed ownership options

### Option A — keep focused-test-only until a real consumer exists (recommended now)

Keep `curve25519_generic` and `curve25519_cortex_m0` out of `xy_tiny_crypto`.

Required maintenance:

1. Preserve the source-map wording that `xy_tiny_crypto.h` declarations do not imply root-link availability.
2. Keep `crypto_25519`, `crypto_25519_m0`, and `crypto_review_manifest` as the active guard set.
3. If a root consumer appears, require a separate root-link proposal and smoke before changing CMake.
4. Do not upgrade `provenance_status`, `security_status`, or `allowed_usage` without a real review record.

This avoids promoting a complex signature/key-exchange implementation with deterministic test seams into root runtime by accident.

### Option B — add generic `xy_25519.c` to root runtime as limited experimental API

Only do this if a consumer explicitly needs root `xy_tiny_crypto` linkage.

Minimum implementation slice:

1. Append `components/crypto/xy_25519/xy_25519.c` to `CRYPTO_SOURCES` in `components/crypto/CMakeLists.txt`.
2. Provide or link root-runtime implementations for `xy_random_bytes()` and `xy_sha512_hash()` that match the public crypto ownership policy; do not reuse test-only fakes.
3. Extend `crypto_root_target_smoke` with X25519/Ed25519 link-only and output-preservation guards that avoid claiming security validation.
4. Update `docs/design/xinyi-crypto-source-ownership-map-2026-08-12.md` and `components/crypto/crypto_review_manifest.json` to a new policy such as `root-runtime-module-source-limited`.
5. Run focused `crypto_25519`, root smoke, review manifest, full `make test-unit`, PC root target build, STM32U5 build if toolchain is available, and `git diff --check`.

This option still would not approve production key exchange/signatures, because provenance, side-channel, RNG/hash, vector, and hardware evidence would remain missing.

### Option C — root compatibility wrapper with explicit unsupported/error behavior

If the product wants the root header declarations to be link-safe but not operational, add a small root wrapper that returns explicit error codes for X25519/Ed25519 until full ownership is approved.

Minimum implementation slice:

1. Add a root-only wrapper source under `components/crypto/src/` with no crypto arithmetic and no test fakes.
2. Make every X25519/Ed25519 root API return the documented invalid/unsupported error without mutating outputs except where the public contract explicitly says otherwise.
3. Keep module `xy_25519.c` focused-test-only.
4. Add root smoke assertions for link-safety and unsupported/error behavior.
5. Update headers/docs so consumers see that root `xy_tiny_crypto` does not provide operational Curve25519/Ed25519 yet.

This is safer than Option B if consumers only need ABI/link stability, but it creates a second behavior surface and therefore needs very explicit docs.

## 4. Recommendation

Use **Option A** until a real root consumer or product decision exists.

Next low-risk implementation, if needed, should be a policy-smoke slice rather than a runtime change:

- Add a `crypto_curve25519_root_policy` smoke that verifies the source map and manifest keep Curve25519 focused-test-only while root `xy_tiny_crypto` does not claim X25519/Ed25519 root runtime ownership. **Implemented:** `tests/unit/crypto/check_crypto_curve25519_root_policy.py` is registered as the `crypto_curve25519_root_policy` CTest.
- Keep it documentation/policy-only; do not link `xy_25519.c` into the root target in the same slice.

## 5. Non-goals

- No source movement or deletion.
- No addition of `xy_25519.c` / Cortex-M0 assembly to the root runtime target in this proposal slice.
- No replacement of deterministic host test seams with production RNG/hash implementations.
- No security/provenance approval.
- No default enablement of `COMPONENT_CRYPTO`.
- No claim that host CTest output proves production key exchange, signatures, secure boot, authentication, pairing, identity, or certificate workflows.

## 6. Verification for this proposal slice

This proposal is docs-only and should be guarded with:

```bash
cd build/tests/unit && ctest --output-on-failure -R '^(crypto_review_manifest|crypto_25519|crypto_25519_m0|crypto_root_target_smoke)$'
make test-unit
git diff --check
```

If a later slice chooses Option B or C, also run:

```bash
cmake --build build/pc --target xy_tiny_crypto -j$(nproc)
make HAL_PLATFORM=STM32U5 -j$(nproc)
```

## 7. Rollback

Before commit:

```bash
git checkout -- docs/design/xinyi-crypto-curve25519-root-ownership-proposal-2026-08-14.md
```

After commit:

```bash
git revert <commit>
```
