# XinYi Crypto ChaCha root/module reconciliation proposal

**Date**: 2026-08-13  
**Status**: proposal / no source moves  
**Scope**: `components/crypto/src/xy_chacha20poly1305.c`, `components/crypto/inc/xy_chacha20poly1305.h`, `components/crypto/xy_chacha/xy_chacha20_poly1305.c`, `components/crypto/xy_chacha/xy_chacha20_poly1305.h`, `components/crypto/CMakeLists.txt`, and `tests/unit/crypto` focused guards.

## 1. Why this proposal exists

The broad duplicate-source cleanup has moved Base64/Hex/CRC/BLAKE2/RNG/CSPRNG/MD5/HMAC/AES to module-owned single active sources. ChaCha20-Poly1305 remains a special case rather than a byte-identical duplicate:

- the root/runtime aggregate source is `components/crypto/src/xy_chacha20poly1305.c` and public header `components/crypto/inc/xy_chacha20poly1305.h`;
- the focused tested module source is `components/crypto/xy_chacha/xy_chacha20_poly1305.c` and header `components/crypto/xy_chacha/xy_chacha20_poly1305.h`;
- the basenames, public function names, return-code model, context layout, and AEAD API shape differ.

Because the module implementation already has RFC 8439-focused host coverage, the long-term direction should still be module-source ownership. However, this cannot be treated like the earlier byte-identical source pruning slices: root consumers currently include `xy_chacha20poly1305.h` and call the compact root API, while focused tests include the module header and call the explicit ciphertext/tag API.

## 2. Current facts

| Layer | Current source/header | Guard status |
| --- | --- | --- |
| Root/runtime aggregate | `src/xy_chacha20poly1305.c` + `inc/xy_chacha20poly1305.h` | Collected by `xy_tiny_crypto` through `src/*.c`; recorded in `crypto_review_manifest.json` as runtime source for `aes_sm3_sm4_chacha20`. |
| Focused module | `xy_chacha/xy_chacha20_poly1305.c` + `xy_chacha/xy_chacha20_poly1305.h` | Covered by `crypto_cipher_hmac`, including RFC 8439 Poly1305 and ChaCha20-Poly1305 AEAD vectors plus invalid-parameter/output-preservation guards. |
| Product/security evidence | Review record is `security-reviewed-limited` / provenance pending | Host CTest vectors are not security audit, source provenance approval, constant-time proof, hardware validation, or compliance evidence. |

## 3. Safe reconciliation direction

Use a compatibility-wrapper slice rather than direct deletion:

1. Keep the root public header name `xy_chacha20poly1305.h` for existing consumers.
2. Make the root aggregate source a thin compatibility wrapper around the module implementation, or replace it with a wrapper file that includes the module header and maps:
   - compact root encrypt API: output buffer contains `ciphertext || tag`, and `ct_len` is total length;
   - compact root decrypt API: input buffer contains `ciphertext || tag`, authentication failure preserves caller plaintext/length outputs;
   - root `0` / `-1` return contract to module `XY_CHACHA20_POLY1305_SUCCESS` / error codes.
3. Only after the wrapper is covered by a root-target CTest should `xy_tiny_crypto` stop compiling an independent ChaCha implementation body.
4. Keep the module implementation as the canonical arithmetic source; do not bulk-rename module symbols or rewrite all consumers in the same slice.

## 4. Proposed implementation sequence

### Slice A: root compatibility wrapper guard

- Add or extend a focused root-target CTest to link `xy_tiny_crypto` and include `xy_chacha20poly1305.h`.
- Cover at least:
  - encrypt success produces `ciphertext || tag` and sets total `ct_len`;
  - decrypt success returns original plaintext and `pt_len`;
  - NULL/short length guard returns root-style failure;
  - tampered tag returns failure and preserves caller plaintext/length outputs.
- Do not change source ownership yet unless the RED test exposes an isolated wrapper bug.

### Slice B: wrapper implementation and source ownership update

- Replace the root implementation body with a wrapper that delegates to `xy_chacha/xy_chacha20_poly1305.c`.
- Update `components/crypto/CMakeLists.txt` to compile the module source into `xy_tiny_crypto` and keep only the wrapper under `src/` if needed for legacy root symbols.
- Update `components/crypto/crypto_review_manifest.json` and `docs/design/xinyi-crypto-source-ownership-map-2026-08-12.md` to describe module arithmetic ownership plus root compatibility wrapper ownership.
- Update `tests/unit/crypto/check_crypto_review_manifest.py` so it checks the wrapper/module split instead of a full independent root implementation.

### Slice C: stale documentation/tooling cleanup

- After Slice B is verified, update or retire stale ad-hoc material such as `components/crypto/build.bat` only if it still claims removed `src/*.c` files as authoritative.
- Keep this separate from arithmetic/wrapper changes so Windows/demo tooling cleanup does not hide source-ownership regressions.

## 5. Required verification

For this proposal-only slice:

```bash
make test-unit
git diff --check
```

For Slice A/B implementation:

```bash
cmake --build build/pc --target xy_tiny_crypto -j$(nproc)
cd build/tests/unit && ctest --output-on-failure -R '^(crypto_cipher_hmac|crypto_root_target_smoke|crypto_review_manifest)$'
make test-unit
git diff --check
```

If `components/crypto/CMakeLists.txt` changes root runtime source ownership, also run:

```bash
make
make HAL_PLATFORM=STM32U5 -j$(nproc)
```

## 6. Non-goals

- No source movement in this proposal.
- No `xy_tiny_crypto` target rename.
- No default enablement of `COMPONENT_CRYPTO`.
- No external crypto library import.
- No security/provenance approval or hardware-validation claim.
- No bulk rewrite of FOTA or other consumers that include `xy_chacha20poly1305.h`.

## 7. Rollback

This proposal can be reverted with:

```bash
git revert <commit>
```

Before commit, restore this slice with:

```bash
git checkout -- docs/design/xinyi-crypto-chacha-root-module-reconciliation-proposal-2026-08-13.md
```
