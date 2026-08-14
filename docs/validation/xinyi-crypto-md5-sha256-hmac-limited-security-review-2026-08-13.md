# XinYi Crypto MD5/SHA-256/HMAC limited security review

**组件**: `components/crypto`  
**审查对象**: `md5_sha256_hmac` / MD5, SHA-256, HMAC helper area  
**记录日期**: `2026-08-13`  
**审查人**: `Zero / scheduled component-quality loop`  
**状态**: `security-reviewed-limited`

> 重要：本记录只固定当前 MD5/SHA-256/HMAC 区域的有限安全边界。结论不得由 host CTest 通过、deterministic test vectors、fake entropy、或 STM32 compile-only 结果替代或升级为完整密码学安全审计。

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
  - `components/crypto/xy_md/xy_md5.c`
  - `components/crypto/xy_hmac/xy_hmac.c`
  - `components/crypto/xy_hmac/xy_sha256.c`
  - `components/crypto/src/xy_sha256_hmac.c` (historical duplicate, pruned after module-source ownership was proven)
  - `tests/unit/crypto/test_crypto_hash.c`
  - `tests/unit/crypto/test_cipher_hmac.c`
  - `components/crypto/crypto_review_manifest.json`
- Upstream/project origin:
  - Current repository-owned lightweight C implementations; no external upstream audit evidence was linked in this slice.
- License evidence:
  - Repository component inherits project licensing context; no separate upstream license/provenance record was linked for these files in this slice.
- Local modifications:
  - No MD5/SHA-256/HMAC implementation code changes in this review slice.
- Duplicate ownership notes:
  - MD5 and generic HMAC have been reconciled to module-directory single active source ownership: root `xy_tiny_crypto` and focused CTests both consume `components/crypto/xy_md/xy_md5.c` and `components/crypto/xy_hmac/xy_hmac.c`; the historical `components/crypto/src/xy_md5.c` and `components/crypto/src/xy_hmac.c` duplicates have been removed.
  - SHA-256/HMAC ownership has been reconciled to the module-directory source: root `xy_tiny_crypto` and focused CTests both consume `components/crypto/xy_hmac/xy_sha256.c`. The historical `components/crypto/src/xy_sha256_hmac.c` has been pruned as a byte-identical duplicate, and the stale `components/crypto/src/xy_sha256.c` remains excluded from the root aggregate target and is not approved by this record.

## 3. Security review

- Intended use:
  - MD5: legacy checksum/integrity compatibility only.
  - SHA-256/HMAC-SHA256: lightweight firmware helper contract with public vector coverage, pending provenance/side-channel/implementation audit.
- Explicit non-goals:
  - No collision-resistance claim for MD5.
  - No FIPS, 国密, formal verification, side-channel resistance, constant-time audit, hardware acceleration, entropy-source, or compliance certification claim for any file in this area.
  - No approval for `components/crypto/src/xy_sha256.c`; it remains excluded because it has an incompatible older API shape and duplicate symbol risk.
- Known placeholder/legacy/weak areas:
  - MD5 is cryptographically broken for collision-resistant/security-sensitive uses and must not be used for authentication, signatures, password hashing, or new security protocol design.
  - The SHA-256/HMAC helpers have host vector and incremental contract tests, but no independent audit or side-channel review.
  - MD5/SHA-256/HMAC source ownership has been reconciled, but SHA-256/HMAC provenance, side-channel, fuzzing, hardware, and compliance evidence remain outside this limited review record.
- Test evidence:
  - `crypto_hash` covers MD5/SHA-256 invalid params, empty/`abc` vectors, and incremental-vs-one-shot behavior.
  - `crypto_cipher_hmac` covers HMAC-MD5/HMAC-SHA256 vectors plus related cipher/HMAC API contracts.
  - `crypto_review_manifest` links this review record and keeps single-active-source/source-map policy checks active.
- Missing evidence:
  - External provenance/license record, independent security audit, side-channel review, fuzzing, and hardware acceleration evidence.

## 4. Decision

- Decision status: `security-reviewed-limited`
- Allowed product usage:
  - MD5: legacy/non-security checksum compatibility only; prohibited for collision-resistant or authentication-sensitive uses.
  - SHA-256/HMAC-SHA256: contract-guarded lightweight helper use is acceptable only where current implementation risk and missing audit evidence are explicitly accepted by the consuming product; not certified or hardware-validated.
- Required follow-up before stronger claims:
  - Add provenance/license evidence for each implementation source.
  - Keep source-map and manifest guards aligned with the MD5/SHA-256/HMAC module-directory single-active-source policy.
  - Add side-channel/constant-time review if used in security-sensitive product paths.
  - Add hardware acceleration/RNG/entropy evidence only through separate hardware validation records.

## 5. Verification commands

Recorded for this review-record/manifest slice:

```bash
cd build/tests/unit && ctest --output-on-failure -R '^(crypto_review_manifest|crypto_hash|crypto_cipher_hmac)$'
make test-unit
git diff --check
```

## 6. Attachments / evidence

- `tests/unit/crypto/test_crypto_hash.c` MD5/SHA-256 host vector coverage.
- `tests/unit/crypto/test_cipher_hmac.c` HMAC-MD5/HMAC-SHA256 host vector coverage.
- `docs/design/xinyi-crypto-source-ownership-map-2026-08-12.md` root/runtime vs focused-test source ownership map.
- `components/crypto/crypto_review_manifest.json` `md5_sha256_hmac` entry.
