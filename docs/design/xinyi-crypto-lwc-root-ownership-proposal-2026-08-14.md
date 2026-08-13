# XinYi Crypto LWC root ownership proposal

**Date**: 2026-08-14  
**Status**: proposal / no runtime enablement  
**Scope**: `components/crypto/xy_ascon/`, `xy_tinyjambu/`, `xy_photon_beetle/`, `xy_tiny_crypto`, `tests/unit/crypto`, and `crypto_review_manifest` ownership wording.

## 1. Why this proposal exists

The Crypto source-ownership map now records Base64/Hex/CRC/BLAKE2/Random/CSPRNG/MD5/HMAC/AES as reconciled module-owned runtime sources, while lightweight crypto remains explicitly focused-test-only:

- `components/crypto/xy_ascon/xy_ascon.c`
- `components/crypto/xy_tinyjambu/xy_tinyjambu.c`
- `components/crypto/xy_photon_beetle/xy_photon_beetle.c`

At the same time, the historical umbrella header `components/crypto/xy_tiny_crypto.h` includes the LWC public headers. That header inclusion makes APIs discoverable, but it does **not** mean the root `xy_tiny_crypto` runtime target currently links those implementations or that the algorithms are production/security approved.

This proposal fixes the next-step decision boundary before any CMake/source change is made.

## 2. Current facts

Current host evidence:

- `crypto_lwc` is the focused contract CTest for Ascon/TinyJAMBU/Photon-Beetle public API behavior.
- `crypto_review_manifest` records LWC as `focused-test-only-until-root-ownership-decided`.
- `docs/validation/xinyi-crypto-lwc-limited-security-review-2026-08-13.md` records only a limited/security-boundary review; provenance, authoritative KAT breadth, side-channel behavior, and target evidence remain pending.

Current runtime boundary:

- `xy_tiny_crypto` does not append the LWC module `.c` files.
- Existing root/runtime smoke tests do not link LWC through the aggregate root library.
- Header discoverability alone must not be treated as runtime ownership or product readiness.

## 3. Recommended staged path

If a real consumer needs LWC through `xy_tiny_crypto`, use this path-limited sequence:

1. **Root-link smoke first**
   - Add one small root-target smoke CTest that links against `xy_tiny_crypto` and calls a minimal LWC API flow.
   - Keep the test scoped to link/API availability and guard behavior; do not add new cryptographic claims.
2. **CMake ownership slice**
   - Append the exact LWC module sources to `components/crypto/CMakeLists.txt` only after the smoke test proves the missing runtime ownership.
   - Update `docs/design/xinyi-crypto-source-ownership-map-2026-08-12.md` and `components/crypto/crypto_review_manifest.json` from `focused-test-only-until-root-ownership-decided` to a narrower runtime policy only in that same verified slice.
3. **Security/provenance remains separate**
   - Do not change `security_status` or `provenance_status` because root linking is not a security review.
   - Do not enable `COMPONENT_CRYPTO` by default.
   - Do not use host CTest vectors as certification, side-channel evidence, compliance evidence, or hardware acceleration validation.

## 4. Non-goals

- No source movement or deletion.
- No bulk import of external LWC vectors or libraries.
- No default enablement of Crypto or LWC algorithms.
- No root target rename from `xy_tiny_crypto`.
- No security/provenance approval.
- No MCU/vendor/third-party edits.

## 5. Verification for this proposal slice

Docs-only proposal validation:

```bash
make test-unit
cd build/tests/unit && ctest --output-on-failure -R '^crypto_review_manifest$'
git diff --check
```

Future CMake/runtime ownership implementation must additionally run:

```bash
cmake --build build/pc --target xy_tiny_crypto -j$(nproc)
cd build/tests/unit && ctest --output-on-failure -R '^(crypto_lwc|crypto_root_target_smoke|crypto_review_manifest)$'
make test-unit
git diff --check
```

If LWC becomes linked into the root firmware target, also run the default PC build and STM32U5 compile gate before commit.

## 6. Rollback

This proposal can be reverted with:

```bash
git revert <commit>
```

Before commit, restore only this file with:

```bash
git checkout -- docs/design/xinyi-crypto-lwc-root-ownership-proposal-2026-08-14.md
```
