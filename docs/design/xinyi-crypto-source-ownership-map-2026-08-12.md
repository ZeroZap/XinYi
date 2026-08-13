# XinYi Crypto source ownership map

**Date**: 2026-08-12
**Status**: source-map / reconciled module-source ownership plus root compatibility wrappers
**Scope**: `components/crypto/src/*`, algorithm module directories, root `xy_tiny_crypto` target, and `tests/unit/crypto` focused CTest source wiring.

## 1. Purpose

Crypto currently has two historical source ownership styles:

1. **Root/runtime aggregate source** under `components/crypto/src/*.c`, collected by `components/crypto/CMakeLists.txt` into `xy_tiny_crypto`.
2. **Algorithm module source** under directories such as `xy_crc/`, `xy_rng/`, `xy_md/`, `xy_hmac/`, `xy_aes/`, `xy_sm*`, and `xy_25519/`, often linked directly by focused host CTests.

This map records the current build facts. It is a guardrail for staged cleanup: do not bulk-merge `src/` copies into module directories, rename the root target, or delete remaining module/root copies until a separate implementation slice proves each public symbol source with focused tests and root builds.

## 2. Root target source facts

`components/crypto/CMakeLists.txt` currently owns the root static library with a hybrid source list:

```cmake
file(GLOB CRYPTO_SOURCES "src/*.c")
list(FILTER CRYPTO_SOURCES EXCLUDE REGEX ".*/src/xy_sha256\\.c$")
list(FILTER CRYPTO_SOURCES EXCLUDE REGEX ".*/src/xy_crc\\.c$")
list(FILTER CRYPTO_SOURCES EXCLUDE REGEX ".*/src/xy_base64\\.c$")
list(FILTER CRYPTO_SOURCES EXCLUDE REGEX ".*/src/xy_hex\\.c$")
list(FILTER CRYPTO_SOURCES EXCLUDE REGEX ".*/src/xy_blake2\\.c$")
list(FILTER CRYPTO_SOURCES EXCLUDE REGEX ".*/src/xy_random\\.c$")
list(FILTER CRYPTO_SOURCES EXCLUDE REGEX ".*/src/xy_csprng\\.c$")
list(FILTER CRYPTO_SOURCES EXCLUDE REGEX ".*/src/xy_md5\\.c$")
list(FILTER CRYPTO_SOURCES EXCLUDE REGEX ".*/src/xy_hmac\\.c$")
list(FILTER CRYPTO_SOURCES EXCLUDE REGEX ".*/src/xy_aes\\.c$")
list(APPEND CRYPTO_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/xy_crc/xy_crc.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/xy_base/xy_base64.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/xy_hex/xy_hex.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/xy_blake/xy_blake2.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/xy_rng/xy_random.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/xy_rng/xy_csprng.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/xy_md/xy_md5.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/xy_hmac/xy_hmac.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/xy_aes/xy_aes.c"
)
add_library(xy_tiny_crypto STATIC ${CRYPTO_SOURCES})
target_link_libraries(xy_tiny_crypto PRIVATE xy_sm3 xy_sm4 xy_sm2)
```

Therefore the root `xy_tiny_crypto` runtime build uses:

| Area | Root/runtime source path | Notes |
| --- | --- | --- |
| CRC | `components/crypto/xy_crc/xy_crc.c` | Reconciled checksum/hash utility slice: root `xy_tiny_crypto` now consumes the same module source as `crypto_crc`; stale duplicate `src/xy_crc.c` was removed. |
| Base64 | `components/crypto/xy_base/xy_base64.c` | Reconciled first encoding slice: root `xy_tiny_crypto` now consumes the same module source as `crypto_encode`; stale duplicate `src/xy_base64.c` has been removed after root/focused/manifest gates proved module ownership. |
| Hex | `components/crypto/xy_hex/xy_hex.c` | Reconciled first encoding slice: root `xy_tiny_crypto` now consumes the same module source as `crypto_encode`; stale duplicate `src/xy_hex.c` has been removed after root/focused/manifest gates proved module ownership. |
| Random | `components/crypto/xy_rng/xy_random.c` | Reconciled RNG slice: root `xy_tiny_crypto` now consumes the same module source as `crypto_random`; stale duplicate `src/xy_random.c` has been removed after root/focused/manifest gates proved module ownership. |
| CSPRNG | `components/crypto/xy_rng/xy_csprng.c` | Reconciled RNG slice: root `xy_tiny_crypto` now consumes the same module source as `crypto_csprng`; stale duplicate `src/xy_csprng.c` has been removed after root/focused/manifest gates proved module ownership. |
| MD5 | `components/crypto/xy_md/xy_md5.c` | Reconciled MD5/HMAC/AES slice: root `xy_tiny_crypto` now consumes the same module source as `crypto_hash`/`crypto_cipher_hmac`; stale duplicate `src/xy_md5.c` was removed. |
| SHA-256 | `components/crypto/src/xy_sha256_hmac.c` | Root public `xy_tiny_crypto.h` SHA-256/HMAC contract is implemented here; `components/crypto/src/xy_sha256.c` is excluded from `xy_tiny_crypto` because it exposes the older `xy_sha256.h` API shape and duplicate `xy_sha256_*` symbols. Separate focused-test copy exists in `xy_hmac/xy_sha256.c`. |
| HMAC | `components/crypto/xy_hmac/xy_hmac.c`, `components/crypto/src/xy_sha256_hmac.c` | Reconciled MD5/HMAC/AES slice: root `xy_tiny_crypto` now consumes the same HMAC module source as `crypto_cipher_hmac`, while `src/xy_sha256_hmac.c` remains root-runtime-only for the public `xy_tiny_crypto.h` SHA-256 implementation. Root-target HMAC smoke must link the aggregate library so the stale `src/xy_sha256.c` duplicate cannot silently re-enter. |
| AES | `components/crypto/xy_aes/xy_aes.c` | Reconciled MD5/HMAC/AES slice: root `xy_tiny_crypto` now consumes the same module source as `crypto_cipher_hmac`; stale duplicate `src/xy_aes.c` was removed. |
| ChaCha20/Poly1305 | `components/crypto/src/xy_chacha20poly1305.c` | Root compact compatibility wrapper over the module-owned RFC 8439 implementation in `xy_chacha/xy_chacha20_poly1305.c`; `crypto_root_target_smoke` covers root `ciphertext || tag` behavior and auth-failure output preservation. |
| BLAKE2 | `components/crypto/xy_blake/xy_blake2.c` | Reconciled checksum/hash utility slice: root `xy_tiny_crypto` now consumes the same module source as `crypto_blake2`; stale duplicate `src/xy_blake2.c` was removed. |
| ECDSA | `components/crypto/src/xy_ecdsa.c` | Root aggregate source only in current map; explicit format-only placeholder contract is covered by `crypto_ecdsa_root_contract` and linked from `crypto_review_manifest` as `security-rejected`. |
| SM3 | `components/crypto/xy_sm3/xy_sm3.c` | Linked via subdirectory target `xy_sm3`, not `src/*.c`. |
| SM4 | `components/crypto/xy_sm4/xy_sm4.c` | Linked via subdirectory target `xy_sm4`, not `src/*.c`. |
| SM2 | `components/crypto/xy_sm2/xy_sm2.c` | Linked via subdirectory target `xy_sm2`, single active source in manifest. |

## 3. Focused CTest source facts

The canonical host contract suite links algorithm sources directly from `tests/unit/CMakeLists.txt`:

| CTest | Linked implementation source roots | Ownership implication |
| --- | --- | --- |
| `crypto_crc` | `components/crypto/xy_crc/xy_crc.c` | Guards the canonical CRC source now shared by focused tests and the root runtime target. |
| `crypto_csprng` | `components/crypto/xy_rng/xy_csprng.c` | Guards the canonical CSPRNG source now shared by focused tests and the root runtime target. |
| `crypto_random` | `components/crypto/xy_rng/xy_random.c` | Guards the canonical simple RNG source now shared by focused tests and the root runtime target. |
| `crypto_encode` | `components/crypto/xy_base/xy_base64.c`, `components/crypto/xy_hex/xy_hex.c` | Guards the canonical Base64/Hex sources now shared by focused tests and the root runtime target. |
| `crypto_hash` | `components/crypto/xy_md/xy_md5.c`, `components/crypto/xy_hmac/xy_sha256.c` | Guards module MD5/SHA-256 copies. |
| `crypto_cipher_hmac` | `components/crypto/xy_aes/xy_aes.c`, `components/crypto/xy_hmac/xy_hmac.c`, `components/crypto/xy_md/xy_md5.c`, `components/crypto/xy_hmac/xy_sha256.c`, `components/crypto/xy_sm3/xy_sm3.c`, `components/crypto/xy_sm4/xy_sm4.c`, `components/crypto/xy_chacha/xy_chacha20_poly1305.c` | Guards module cipher/HMAC/SM copies and the canonical ChaCha20/Poly1305 arithmetic implementation. |
| `crypto_blake2` | `components/crypto/xy_blake/xy_blake2.c` | Focused BLAKE2s host vectors, incremental/keyed behavior, invalid-parameter output preservation, and the canonical source now shared by focused tests and the root runtime target; still not security/provenance review. |
| `crypto_ecdsa_root_contract` | `components/crypto/src/xy_ecdsa.c` | Focused root-only ECDSA format-guard placeholder contract: null/malformed/range guards plus explicit message-non-binding success behavior; security status remains `security-rejected`. |
| `crypto_sm2` | `xy_sm2/`, `xy_sm3/`, `xy_sm4/`, `xy_rng/` | Guards SM2 public placeholder-grade contract plus helper modules. |
| `crypto_lwc` | `components/crypto/xy_ascon/xy_ascon.c`, `components/crypto/xy_tinyjambu/xy_tinyjambu.c`, `components/crypto/xy_photon_beetle/xy_photon_beetle.c` | Focused-test-only until root ownership is intentionally decided. |
| `crypto_25519` | `components/crypto/xy_25519/xy_25519.c` | Focused-test-only until root ownership is intentionally decided. |
| `crypto_25519_m0` | `components/crypto/xy_25519/xy_25519_m0.c`, `components/crypto/xy_25519/fe25519_m0.c` | Focused-test-only/upstream-material boundary. |
| `crypto_smoke_example` | module Base64/Hex/SHA-256/RNG sources | Host-safe API smoke only; not a root aggregate source proof. |
| `crypto_root_target_smoke` | links `xy_tiny_crypto` root target, therefore uses reconciled `components/crypto/xy_crc/xy_crc.c`, reconciled `components/crypto/xy_base/xy_base64.c`, reconciled `components/crypto/xy_hex/xy_hex.c`, reconciled `components/crypto/xy_blake/xy_blake2.c`, reconciled `components/crypto/xy_rng/xy_random.c`, reconciled `components/crypto/xy_rng/xy_csprng.c`, reconciled `components/crypto/xy_md/xy_md5.c`, reconciled `components/crypto/xy_hmac/xy_hmac.c`, reconciled `components/crypto/xy_aes/xy_aes.c`, plus `components/crypto/src/xy_sha256_hmac.c`, `components/crypto/src/xy_chacha20poly1305.c`, and `components/crypto/src/xy_ecdsa.c` through the aggregate library; `components/crypto/src/xy_sha256.c` is intentionally excluded | Minimal root/runtime public consumer proof for Base64/Hex/SHA-256/HMAC-SHA256, one BLAKE2s vector, ChaCha20-Poly1305 compact wrapper behavior, and ECDSA format-only guard paths; not broad duplicate-source reconciliation or security validation. |
| `crypto_review_manifest` | `components/crypto/crypto_review_manifest.json` plus `components/crypto/CMakeLists.txt` root-source glob and reconciled module append shape | Policy guard only; not cryptographic validation. It now fails if the root target stops using the mapped source ownership shape without a matching map update, records Base64/Hex/CRC/BLAKE2/Random/CSPRNG/MD5/HMAC/AES as module-owned `single-active-source` runtime paths, and links the root ECDSA placeholder to an explicit `security-rejected` review record. |

Additional root aggregate sources currently mapped but intentionally not represented as reviewed algorithm entries:

| Source | Current guard status | Notes |
| --- | --- | --- |
| `components/crypto/xy_blake/xy_blake2.c` | `security-reviewed-limited` in the `blake2` manifest entry; root smoke path is exercised by `crypto_root_target_smoke` and focused contract by `crypto_blake2` | Single active source after the checksum/hash utility reconciliation slice; this is still not provenance approval, security audit, hardware validation, or compliance evidence. |
| `components/crypto/xy_rng/xy_random.c`, `components/crypto/xy_rng/xy_csprng.c` | `security-reviewed-limited` in the `random_csprng` manifest entry; focused contracts are exercised by `crypto_random` and `crypto_csprng`, and public smoke by `crypto_smoke_example` | Single active root/runtime sources after the RNG reconciliation slice; this is still not entropy-source approval, formal DRBG audit, hardware RNG validation, or compliance evidence. |
| `components/crypto/src/xy_ecdsa.c` | `security-rejected` algorithm entry `ecdsa_root_format_only` in `crypto_review_manifest.json`; format-only guard paths are exercised by `crypto_root_target_smoke` and focused `crypto_ecdsa_root_contract` | Simplified verifier returns success after format checks; do not treat it as production signature validation, secure boot evidence, or firmware authenticity validation. |

Excluded historical root source:

| Source | Current guard status | Notes |
| --- | --- | --- |
| `components/crypto/src/xy_sha256.c` | excluded from `xy_tiny_crypto` by `components/crypto/CMakeLists.txt`; policy-guarded by `crypto_review_manifest` | This older `xy_sha256.h` implementation defines `xy_sha256_init/update` with a different API shape from the public `xy_tiny_crypto.h` SHA-256/HMAC contract. Keeping it in the aggregate archive caused root HMAC consumers to fail with duplicate `xy_sha256_*` symbols once `xy_hmac_sha256()` was referenced. Do not re-enable it without a separate SHA-256 ownership reconciliation slice. |

## 4. Cleanup policy

Allowed low-risk follow-ups:

1. Keep machine checks that this map and `crypto_review_manifest.json` stay in sync, including the current root `file(GLOB CRYPTO_SOURCES "src/*.c")` collection shape plus explicit Base64/Hex/CRC/BLAKE2 module-source append.
2. Keep source-ownership guards for the current `single-active-source` algorithm set and the ChaCha root-wrapper boundary; no root/module byte-identical duplicate pairs remain guarded after MD5/HMAC/AES moved to module ownership. Base64/Hex/CRC/BLAKE2/Random/CSPRNG/MD5/HMAC/AES have left the old pending set because the root target now consumes the module copies, and stale root duplicates have been pruned.
3. Add root-target smoke or focused root-copy coverage for one algorithm at a time if a consumer needs `xy_tiny_crypto` behavior specifically; `crypto_ecdsa_root_contract` is the current example for the root-only placeholder-grade ECDSA copy.
4. For a single algorithm, compare aggregate and module copies, decide canonical ownership, then update CMake/tests/docs in one path-limited verified slice.
5. Keep ad-hoc build helpers from naming removed historical `src/*.c` duplicates: `components/crypto/build.bat` now delegates to the canonical CMake `xy_tiny_crypto` target instead of compiling stale root copies directly.
6. Prune stale historical documentation duplicates only when they are tracked, unreferenced, and demonstrably superseded by the current component README/source map. The truncated `components/crypto/xy_tiny_boot_crypto copy.md` stale duplicate was removed; `xy_tiny_boot_crypto.md` remains as historical material.

Explicit non-goals for this slice:

- No source movement or deletion.
- No root target rename from `xy_tiny_crypto` to `xy_crypto`.
- No security/provenance approval.
- No production enablement of `COMPONENT_CRYPTO` by default.
- No external crypto library import.

## 5. Verification

This map should be validated with:

```bash
cd build/tests/unit && ctest --output-on-failure -R '^crypto_review_manifest$'
make test-unit
git diff --check
```

If later source ownership changes touch code or CMake, additionally run:

```bash
cmake --build build/pc --target xy_tiny_crypto -j$(nproc)
cd build/tests/unit && ctest --output-on-failure -R '^crypto_'
```

## 6. Rollback

This docs-only source map and manifest link can be reverted with:

```bash
git revert <commit>
```

Before commit, restore the touched paths with:

```bash
git checkout -- docs/design/xinyi-crypto-source-ownership-map-2026-08-12.md components/crypto/crypto_review_manifest.json components/crypto/README.md tests/unit/crypto/check_crypto_review_manifest.py
```
