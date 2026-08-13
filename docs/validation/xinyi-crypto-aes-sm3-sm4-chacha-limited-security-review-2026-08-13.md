# XinYi Crypto AES/SM3/SM4/ChaCha20 limited security review

**组件**: `components/crypto`  
**审查对象**: `aes_sm3_sm4_chacha20` / AES, SM3, SM4, ChaCha20, Poly1305, and ChaCha20-Poly1305 helper area  
**记录日期**: `2026-08-13`  
**审查人**: `Zero / scheduled component-quality loop`  
**状态**: `security-reviewed-limited`

> 重要：本记录只固定当前 AES/SM3/SM4/ChaCha20/Poly1305 区域的有限安全边界。结论不得由 host CTest 通过、deterministic test vectors、fake entropy、或 STM32 compile-only 结果替代或升级为完整密码学安全审计。

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
  - `components/crypto/xy_aes/xy_aes.c`
  - `components/crypto/src/xy_chacha20poly1305.c`
  - `components/crypto/xy_sm3/xy_sm3.c`
  - `components/crypto/xy_sm4/xy_sm4.c`
  - `components/crypto/xy_chacha/xy_chacha20_poly1305.c`
  - `tests/unit/crypto/test_cipher_hmac.c`
  - `components/crypto/crypto_review_manifest.json`
- Upstream/project origin:
  - Current repository-owned lightweight C implementations; no external upstream audit evidence was linked in this slice.
- License evidence:
  - Repository component inherits project licensing context; no separate upstream license/provenance record was linked for these files in this slice.
- Local modifications:
  - No AES/SM3/SM4/ChaCha20/Poly1305 implementation code changes in this review slice.
- Duplicate ownership notes:
  - AES has been reconciled to module-directory single active source ownership: root `xy_tiny_crypto` and focused `crypto_cipher_hmac` both consume `components/crypto/xy_aes/xy_aes.c`; the historical duplicate `components/crypto/src/xy_aes.c` has been removed.
  - `components/crypto/src/xy_chacha20poly1305.c` remains a compact root compatibility wrapper over the module-owned RFC 8439 arithmetic implementation in `components/crypto/xy_chacha/xy_chacha20_poly1305.c`, guarded by `crypto_root_target_smoke` plus `crypto_cipher_hmac`.
  - SM3 and SM4 are currently focused-test module sources only in the manifest; this record does not add them to the root aggregate target.

## 3. Security review

- Intended use:
  - AES/SM3/SM4/ChaCha20/Poly1305: host contract-guarded lightweight crypto helpers for explicitly opted-in firmware consumers that accept current implementation risk and missing audit evidence.
- Explicit non-goals:
  - No FIPS, 国密 certification, formal verification, side-channel resistance, constant-time audit, hardware acceleration, key-management, nonce-management, padding-scheme approval, AEAD misuse-resistance proof, or compliance certification claim for any file in this area.
  - No approval for enabling `COMPONENT_CRYPTO` by default or exporting these helpers as production security primitives without product-level review.
- Known placeholder/legacy/weak areas:
  - AES/SM3/SM4/ChaCha20/Poly1305 have vector/API host tests, but no independent implementation audit or side-channel review.
  - Correct production use depends on caller-owned key storage, nonce uniqueness, mode selection, padding policy, and error handling; these are outside the current component contract tests.
  - AES source ownership and the ChaCha root-wrapper boundary are now reconciled in the source map, but this record still does not provide provenance, side-channel, fuzzing, hardware, or compliance evidence.
- Test evidence:
  - `crypto_cipher_hmac` covers AES-128, SM3, SM4, ChaCha20, Poly1305, ChaCha20-Poly1305 AEAD vectors, invalid-parameter guards, and tampered-tag output preservation.
  - `crypto_review_manifest` links this review record and keeps single-active-source / root-compatibility-wrapper source-map policy checks active.
- Missing evidence:
  - External provenance/license record, independent security audit, side-channel/constant-time review, fuzzing, misuse-resistance review, and hardware acceleration evidence.

## 4. Decision

- Decision status: `security-reviewed-limited`
- Allowed product usage:
  - Contract-guarded lightweight helper use is acceptable only where current implementation risk, caller-owned key/nonce/padding policy, and missing audit evidence are explicitly accepted by the consuming product.
  - These helpers remain prohibited as certified, compliance-sensitive, hardware-validated, or side-channel-reviewed implementations until separate evidence exists.
- Required follow-up before stronger claims:
  - Add provenance/license evidence for each implementation source.
  - Keep source-map and manifest guards aligned with the AES single-active-source policy and ChaCha root compatibility-wrapper boundary.
  - Add side-channel/constant-time and misuse-resistance review for any security-sensitive product path.
  - Add hardware acceleration or MCU-specific validation only through separate hardware validation records.

## 5. Verification commands

Recorded for this review-record/manifest slice:

```bash
cd build/tests/unit && ctest --output-on-failure -R '^(crypto_review_manifest|crypto_cipher_hmac)$'
make test-unit
git diff --check
```

## 6. Attachments / evidence

- `tests/unit/crypto/test_cipher_hmac.c` AES/HMAC/SM3/SM4/ChaCha20/Poly1305/AEAD host vector and guard coverage.
- `docs/design/xinyi-crypto-source-ownership-map-2026-08-12.md` root/runtime vs focused-test source ownership map.
- `components/crypto/crypto_review_manifest.json` `aes_sm3_sm4_chacha20` entry.
