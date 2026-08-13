# XinYi Crypto duplicate source reconciliation proposal

**Date**: 2026-08-13  
**Status**: proposal / no source moves  
**Scope**: `components/crypto/src/*`, algorithm module directories, `xy_tiny_crypto`, and `tests/unit/crypto` source ownership guards.

## 1. Why this proposal exists

Crypto is now host-guarded and has an explicit security/provenance review manifest, but several algorithms still exist in two byte-identical source copies:

- root/runtime aggregate copies under `components/crypto/src/*.c`, used by the `xy_tiny_crypto` target;
- module-directory copies under paths such as `xy_crc/`, `xy_base/`, `xy_rng/`, `xy_md/`, `xy_hmac/`, `xy_aes/`, and `xy_blake/`, used by focused host CTests.

The current `crypto_review_manifest` intentionally guards these duplicate pairs from silently diverging while their ownership remains `source-map-pending`. This proposal defines the low-risk reconciliation sequence for a later implementation slice. It does **not** move files, delete files, change `COMPONENT_CRYPTO`, rename `xy_tiny_crypto`, or upgrade any algorithm's security/provenance status.

## 2. Current duplicate/source-ownership pairs

The duplicate-source reconciliation is now mostly complete. Current module-owned pairs/status are:

| Area | Root/runtime source | Module/focused-test source | Current policy |
| --- | --- | --- | --- |
| CRC | removed | `components/crypto/xy_crc/xy_crc.c` | `single-active-source` |
| Base64 | removed | `components/crypto/xy_base/xy_base64.c` | `single-active-source` |
| Hex | removed | `components/crypto/xy_hex/xy_hex.c` | `single-active-source` |
| Random | removed | `components/crypto/xy_rng/xy_random.c` | `single-active-source` |
| CSPRNG | removed | `components/crypto/xy_rng/xy_csprng.c` | `single-active-source` |
| MD5 | removed | `components/crypto/xy_md/xy_md5.c` | `single-active-source` |
| HMAC | removed | `components/crypto/xy_hmac/xy_hmac.c` | `single-active-source` |
| AES | removed | `components/crypto/xy_aes/xy_aes.c` | `single-active-source` |
| BLAKE2 | removed | `components/crypto/xy_blake/xy_blake2.c` | `single-active-source` |

Related non-duplicate or special cases:

- `components/crypto/src/xy_sha256.c` remains excluded from `xy_tiny_crypto` because it exposes an older API shape and can collide with the aggregate SHA-256/HMAC implementation.
- `components/crypto/src/xy_sha256_hmac.c` is root-runtime-only for the public `xy_tiny_crypto.h` SHA-256/HMAC contract.
- `components/crypto/src/xy_ecdsa.c` is root-runtime-only and explicitly `security-rejected` as a format-only placeholder.
- SM2/SM3/SM4 are module targets linked into the root crypto library rather than duplicated in `src/*.c`.

## 3. Proposed reconciliation direction

For byte-identical pairs, prefer **module-directory source ownership** and make the root target consume module sources explicitly or through small module libraries. Rationale:

1. Focused CTests already guard the module-directory implementations.
2. Module directories preserve algorithm-local headers, examples, and review records.
3. Root `src/` aggregate copies are historical duplicates and create drift risk.
4. Reusing module sources in `xy_tiny_crypto` avoids changing public API names while reducing duplicate maintenance.

The final target shape should still keep `xy_tiny_crypto` as the public root/runtime target. A future `xy_crypto` full rename remains out of scope.

## 4. Safe implementation sequence

Do not reconcile all pairs in one patch. Use one small algorithm group per verified slice:

1. **Encoding group**: Base64 + Hex. **Done in first reconciliation slice; stale root duplicates pruned in follow-up closure.**
   - Switch root `xy_tiny_crypto` to compile `xy_base/xy_base64.c` and `xy_hex/xy_hex.c` instead of `src/xy_base64.c` and `src/xy_hex.c`.
   - Keep public root headers and focused `crypto_encode`/`crypto_smoke_example` unchanged.
   - Add or extend a root-target smoke assertion if root aggregate behavior is not already covered.
2. **Checksum/hash utility group**: CRC and BLAKE2. **Done in second reconciliation slice.**
   - Reuse module CRC/BLAKE2 sources in root target.
   - Run focused `crypto_crc`, `crypto_blake2`, `crypto_root_target_smoke`, and `crypto_review_manifest`.
3. **RNG group**: Random + CSPRNG. **Done in third reconciliation slice; stale root duplicates pruned in follow-up closure.**
   - Reuse module RNG/CSPRNG sources in root target.
   - Preserve the current warning that `xy_random_*` is non-security utility/demo only and `xy_csprng_*` depends on caller-owned entropy/seed quality.
4. **MD5/HMAC/AES group**: MD5, HMAC, AES. **Done in fourth reconciliation slice.**
   - Reuse module copies only after confirming root public `xy_tiny_crypto.h` signatures still bind correctly.
   - Keep `src/xy_sha256_hmac.c` ownership separate unless a dedicated SHA-256/HMAC root API reconciliation is designed.

After each remaining slice:

- update `docs/design/xinyi-crypto-source-ownership-map-2026-08-12.md` for only the reconciled pair(s);
- update `components/crypto/crypto_review_manifest.json` duplicate policy for only the reconciled pair(s);
- update `tests/unit/crypto/check_crypto_review_manifest.py` so it stops requiring byte-identical duplicate files only after the root target no longer depends on the duplicate copy;
- delete old root duplicate source files only in the same slice that proves root/focused tests still pass and the map no longer references them as required runtime sources.

## 5. Required verification for each implementation slice

Minimum gate for docs-only proposal changes:

```bash
make test-unit
git diff --check
```

Minimum gate for future source/CMake reconciliation changes:

```bash
cmake --build build/pc --target xy_tiny_crypto -j$(nproc)
cd build/tests/unit && ctest --output-on-failure -R '^(crypto_encode|crypto_crc|crypto_random|crypto_csprng|crypto_hash|crypto_cipher_hmac|crypto_blake2|crypto_root_target_smoke|crypto_review_manifest)$'
make test-unit
git diff --check
```

If CMake source ownership changes affect the root firmware library, also run:

```bash
make
make HAL_PLATFORM=STM32U5 -j$(nproc)
```

## 6. Non-goals and safety boundary

- No bulk `components/crypto/src/` deletion.
- No root target rename from `xy_tiny_crypto` to `xy_crypto`.
- No default enablement of `COMPONENT_CRYPTO`.
- No security/provenance approval or hardware-validation claim.
- No external crypto library import.
- No MCU/vendor/third-party tree edits.

## 7. Rollback

This proposal can be reverted with:

```bash
git revert <commit>
```

Before commit, restore this slice with:

```bash
git checkout -- docs/design/xinyi-crypto-duplicate-source-reconciliation-proposal-2026-08-13.md
```
