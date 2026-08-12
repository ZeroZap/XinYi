# XinYi Crypto ChaCha20-Poly1305 RFC vector hardening proposal

**Date**: 2026-08-12
**Status**: proposal / no code change
**Scope**: `components/crypto/xy_chacha/xy_chacha20_poly1305.c`, `tests/unit/crypto/test_cipher_hmac.c`, `crypto_cipher_hmac` CTest

## 1. Background

The current Crypto component is contract-guarded but not security/provenance reviewed. `crypto_cipher_hmac` already covers AES, HMAC, SM3, SM4, and the ChaCha20 stream cipher vector/roundtrip. It does **not** currently cover the Poly1305 MAC or ChaCha20-Poly1305 AEAD public APIs with RFC vectors.

During the component loop, a narrow RED probe was attempted by adding RFC-style Poly1305 and AEAD vectors to `tests/unit/crypto/test_cipher_hmac.c`. The focused test built, but the Poly1305 tag did not match the RFC 8439 known vector:

```text
/home/eugene/zerozap/XinYi/tests/unit/crypto/test_cipher_hmac.c:306:test_poly1305_and_aead_vectors:FAIL: Memory Mismatch. Byte 0 Expected 0xA8 Was 0x09
```

The probe was reverted before this proposal was written; this document records the discovered hardening target without committing an incomplete test or risky crypto implementation patch.

## 2. Current evidence boundary

- Existing `crypto_cipher_hmac` passing means the currently tested cipher/HMAC API contracts still hold.
- The failed RED probe indicates Poly1305/AEAD RFC-vector compatibility is not yet proven and may expose implementation drift.
- This proposal is not a cryptographic review and does not approve ChaCha20-Poly1305 for security-critical use.
- Do not treat host CTest output as side-channel, nonce-management, key-management, hardware acceleration, or compliance evidence.

## 3. Recommended implementation slice

A safe follow-up should be path-limited to:

```text
tests/unit/crypto/test_cipher_hmac.c
components/crypto/xy_chacha/xy_chacha20_poly1305.c
components/crypto/README.md
components/crypto/crypto_review_manifest.json      # only if evidence wording/status needs sync
docs/design/xinyi-crypto-source-ownership-map-2026-08-12.md  # only if ownership wording changes
```

Suggested TDD sequence:

1. Re-add the RED test to `test_cipher_hmac.c` with:
   - RFC 8439 Poly1305 test vector: key `85 d6 be 78 ... f5 1b`, message `Cryptographic Forum Research Group`, expected tag `a8 06 1d c1 30 51 36 c6 c2 2b 8b af 0c 01 27 a9`.
   - RFC 8439 ChaCha20-Poly1305 AEAD encryption/decryption vector with AAD, ciphertext, and tag.
   - Invalid-parameter guards for `xy_poly1305_*` and `xy_chacha20_poly1305_*`.
   - Authentication failure output-preservation check for decrypt with a tampered tag.
2. Run only the focused target first:

   ```bash
   cmake --build build/tests/unit --target test_cipher_hmac -j$(nproc)
   cd build/tests/unit && ctest --output-on-failure -R '^crypto_cipher_hmac$'
   ```

3. Fix `xy_chacha20_poly1305.c` only after the RED vector is committed locally in the working tree, keeping changes narrowly focused on Poly1305/AEAD arithmetic or final-block handling.
4. Do not broaden this into duplicate-source reconciliation, root target renaming, security approval, or external crypto library import.
5. After focused GREEN, run:

   ```bash
   make test-unit
   cd build/tests/unit && ctest --output-on-failure -R '^crypto_'
   git diff --check
   ```

## 4. Review checkpoints before production claims

Even after RFC vectors pass, keep Crypto status at `contract-guarded` / `review-pending` until separate review artifacts exist:

- provenance/source review for `xy_chacha/xy_chacha20_poly1305.c` and any root aggregate copy;
- side-channel and constant-time review;
- nonce/key-management API guidance;
- hardware acceleration evidence if a HAL/MCU backend is introduced;
- product decision on `COMPONENT_CRYPTO` default-off policy.

## 5. Rollback

This proposal is docs-only and can be reverted with:

```bash
git revert <commit>
```

Before commit, it can be removed with:

```bash
git checkout -- docs/design/xinyi-crypto-chacha-poly1305-rfc-vector-hardening-proposal-2026-08-12.md
```
