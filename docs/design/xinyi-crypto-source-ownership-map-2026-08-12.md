# XinYi Crypto source ownership map

**Date**: 2026-08-12
**Status**: design/source-map / no source moves
**Scope**: `components/crypto/src/*`, algorithm module directories, root `xy_tiny_crypto` target, and `tests/unit/crypto` focused CTest source wiring.

## 1. Purpose

Crypto currently has two historical source ownership styles:

1. **Root/runtime aggregate source** under `components/crypto/src/*.c`, collected by `components/crypto/CMakeLists.txt` into `xy_tiny_crypto`.
2. **Algorithm module source** under directories such as `xy_crc/`, `xy_rng/`, `xy_md/`, `xy_hmac/`, `xy_aes/`, `xy_sm*`, and `xy_25519/`, often linked directly by focused host CTests.

This map records the current build facts without moving or deleting any files. It is a guardrail for later cleanup: do not merge `src/` copies into module directories, rename the root target, or delete module copies until a separate implementation slice proves each public symbol source with focused tests and root builds.

## 2. Root target source facts

`components/crypto/CMakeLists.txt` currently owns the root static library as follows:

```cmake
file(GLOB CRYPTO_SOURCES "src/*.c")
add_library(xy_tiny_crypto STATIC ${CRYPTO_SOURCES})
target_link_libraries(xy_tiny_crypto PRIVATE xy_sm3 xy_sm4 xy_sm2)
```

Therefore the root `xy_tiny_crypto` runtime build uses:

| Area | Root/runtime source path | Notes |
| --- | --- | --- |
| CRC | `components/crypto/src/xy_crc.c` | Separate focused-test copy exists in `xy_crc/xy_crc.c`. |
| Base64 | `components/crypto/src/xy_base64.c` | Separate focused-test copy exists in `xy_base/xy_base64.c`. |
| Hex | `components/crypto/src/xy_hex.c` | Separate focused-test copy exists in `xy_hex/xy_hex.c`. |
| Random | `components/crypto/src/xy_random.c` | Separate focused-test copy exists in `xy_rng/xy_random.c`. |
| CSPRNG | `components/crypto/src/xy_csprng.c` | Separate focused-test copy exists in `xy_rng/xy_csprng.c`. |
| MD5 | `components/crypto/src/xy_md5.c` | Separate focused-test copy exists in `xy_md/xy_md5.c`. |
| SHA-256 | `components/crypto/src/xy_sha256.c` | Separate focused-test copy exists in `xy_hmac/xy_sha256.c`. |
| HMAC | `components/crypto/src/xy_hmac.c`, `components/crypto/src/xy_sha256_hmac.c` | Focused tests link `xy_hmac/xy_hmac.c` plus module SHA-256/MD5 sources. |
| AES | `components/crypto/src/xy_aes.c` | Separate focused-test copy exists in `xy_aes/xy_aes.c`. |
| ChaCha20/Poly1305 | `components/crypto/src/xy_chacha20poly1305.c` | Focused tests use `xy_chacha/xy_chacha20_poly1305.c`; basename differs. |
| BLAKE2 | `components/crypto/src/xy_blake2.c` | Module copy exists in `xy_blake/xy_blake2.c`; no current root focused CTest called out in the manifest. |
| ECDSA | `components/crypto/src/xy_ecdsa.c` | Root aggregate source only in current map; no focused CTest contract in current manifest. |
| SM3 | `components/crypto/xy_sm3/xy_sm3.c` | Linked via subdirectory target `xy_sm3`, not `src/*.c`. |
| SM4 | `components/crypto/xy_sm4/xy_sm4.c` | Linked via subdirectory target `xy_sm4`, not `src/*.c`. |
| SM2 | `components/crypto/xy_sm2/xy_sm2.c` | Linked via subdirectory target `xy_sm2`, single active source in manifest. |

## 3. Focused CTest source facts

The canonical host contract suite links algorithm sources directly from `tests/unit/CMakeLists.txt`:

| CTest | Linked implementation source roots | Ownership implication |
| --- | --- | --- |
| `crypto_crc` | `components/crypto/xy_crc/xy_crc.c` | Guards module CRC copy, not the root aggregate `src/xy_crc.c` directly. |
| `crypto_csprng` | `components/crypto/xy_rng/xy_csprng.c` | Guards module CSPRNG copy. |
| `crypto_random` | `components/crypto/xy_rng/xy_random.c` | Guards module RNG copy. |
| `crypto_encode` | `components/crypto/xy_base/xy_base64.c`, `components/crypto/xy_hex/xy_hex.c` | Guards module Base64/Hex copies. |
| `crypto_hash` | `components/crypto/xy_md/xy_md5.c`, `components/crypto/xy_hmac/xy_sha256.c` | Guards module MD5/SHA-256 copies. |
| `crypto_cipher_hmac` | `components/crypto/xy_aes/xy_aes.c`, `components/crypto/xy_hmac/xy_hmac.c`, `components/crypto/xy_md/xy_md5.c`, `components/crypto/xy_hmac/xy_sha256.c`, `components/crypto/xy_sm3/xy_sm3.c`, `components/crypto/xy_sm4/xy_sm4.c`, `components/crypto/xy_chacha/xy_chacha20_poly1305.c` | Guards module cipher/HMAC/SM copies. |
| `crypto_sm2` | `xy_sm2/`, `xy_sm3/`, `xy_sm4/`, `xy_rng/` | Guards SM2 public placeholder-grade contract plus helper modules. |
| `crypto_lwc` | `components/crypto/xy_ascon/xy_ascon.c`, `components/crypto/xy_tinyjambu/xy_tinyjambu.c`, `components/crypto/xy_photon_beetle/xy_photon_beetle.c` | Focused-test-only until root ownership is intentionally decided. |
| `crypto_25519` | `components/crypto/xy_25519/xy_25519.c` | Focused-test-only until root ownership is intentionally decided. |
| `crypto_25519_m0` | `components/crypto/xy_25519/xy_25519_m0.c`, `components/crypto/xy_25519/fe25519_m0.c` | Focused-test-only/upstream-material boundary. |
| `crypto_smoke_example` | module Base64/Hex/SHA-256/RNG sources | Host-safe API smoke only; not a root aggregate source proof. |
| `crypto_root_target_smoke` | links `xy_tiny_crypto` root target, therefore uses `components/crypto/src/xy_base64.c`, `components/crypto/src/xy_hex.c`, `components/crypto/src/xy_sha256.c`, `components/crypto/src/xy_blake2.c`, and `components/crypto/src/xy_ecdsa.c` through the aggregate library | Minimal root/runtime public consumer proof for Base64/Hex/SHA-256, one BLAKE2s vector, and ECDSA format-only guard paths; not broad duplicate-source reconciliation or security validation. |
| `crypto_review_manifest` | `components/crypto/crypto_review_manifest.json` plus `components/crypto/CMakeLists.txt` root-source glob shape | Policy guard only; not cryptographic validation. It now also records root aggregate copies that are mapped but still unreviewed, fails if the root target stops using the mapped `file(GLOB CRYPTO_SOURCES "src/*.c")` ownership shape without a matching map update, and guards currently byte-identical CRC/Base64/Hex root/module duplicate copies from silently diverging before an explicit ownership slice. |

Additional root aggregate sources currently mapped but intentionally not represented as reviewed algorithm entries:

| Source | Current guard status | Notes |
| --- | --- | --- |
| `components/crypto/src/xy_blake2.c` | `root-source-unreviewed` in `crypto_review_manifest.json`; root smoke path is exercised by `crypto_root_target_smoke` | Root aggregate copy has root-target smoke coverage, but no active focused CTest or review record currently promotes it beyond mapped/unreviewed status. |
| `components/crypto/src/xy_ecdsa.c` | `root-source-unreviewed` in `crypto_review_manifest.json`; format-only guard path is exercised by `crypto_root_target_smoke` | Simplified verifier returns success after format checks; do not treat it as production signature validation without a focused CTest plus real security/provenance review. |

## 4. Cleanup policy

Allowed low-risk follow-ups:

1. Keep machine checks that this map and `crypto_review_manifest.json` stay in sync, including the current root `file(GLOB CRYPTO_SOURCES "src/*.c")` collection shape.
2. Keep byte-identical duplicate-copy guards for pairs already proven identical and still marked `source-map-pending`; the current guarded pairs are CRC (`src/xy_crc.c` vs `xy_crc/xy_crc.c`), Base64 (`src/xy_base64.c` vs `xy_base/xy_base64.c`), and Hex (`src/xy_hex.c` vs `xy_hex/xy_hex.c`). If one of these pairs intentionally diverges, update this map plus focused/root tests in the same explicit ownership slice.
3. Add root-target smoke coverage for one algorithm at a time if a consumer needs `xy_tiny_crypto` behavior specifically.
4. For a single algorithm, compare aggregate and module copies, decide canonical ownership, then update CMake/tests/docs in one path-limited verified slice.

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
