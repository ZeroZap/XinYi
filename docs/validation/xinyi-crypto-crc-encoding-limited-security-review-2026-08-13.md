# XinYi Crypto CRC/Base64/Hex limited security review

**组件**: `components/crypto`  
**审查对象**: `crc` and `base64_hex` helper areas  
**记录日期**: `2026-08-13`  
**审查人**: `Zero / scheduled component-quality loop`  
**状态**: `security-reviewed-limited`

> 重要：本记录只固定 CRC/Base64/Hex 区域的有限安全边界。结论不得由 host CTest 通过、deterministic vectors、fake entropy、或 STM32 compile-only 结果替代或升级为完整密码学安全审计。

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
  - `components/crypto/src/xy_crc.c`
  - `components/crypto/xy_crc/xy_crc.c`
  - `components/crypto/src/xy_base64.c`
  - `components/crypto/xy_base/xy_base64.c`
  - `components/crypto/src/xy_hex.c`
  - `components/crypto/xy_hex/xy_hex.c`
  - `tests/unit/crypto/test_crypto_crc.c`
  - `tests/unit/crypto/test_crypto_encode.c`
  - `tests/unit/crypto/test_crypto_smoke_example.c`
  - `components/crypto/crypto_review_manifest.json`
- Upstream/project origin:
  - Current repository-owned lightweight C helper implementations; no external upstream audit evidence was linked in this slice.
- License evidence:
  - Repository component inherits project licensing context; no separate upstream license/provenance record was linked for these files in this slice.
- Local modifications:
  - No CRC/Base64/Hex implementation code changes in this review slice.
- Duplicate ownership notes:
  - CRC, Base64, and Hex still have root/runtime copies under `components/crypto/src/` and focused-test module copies under `xy_crc/`, `xy_base/`, and `xy_hex/`; `crypto_review_manifest` keeps the byte-identical duplicate-copy guard active.

## 3. Security review

- Intended use:
  - CRC: non-security integrity/checksum utility only.
  - Base64/Hex: encoding/decoding utility only.
- Explicit non-goals:
  - No authentication, signature, confidentiality, anti-tamper, password hashing, secure boot, protocol security, certification, side-channel, constant-time, hardware-acceleration, or compliance claim.
  - No provenance/license approval beyond current repository context.
  - No duplicate-source ownership reconciliation or source deletion.
- Known placeholder/legacy/weak areas:
  - CRC is linear and not collision-resistant; it must not be used as a MAC, signature substitute, tamper-proof checksum, or authorization decision.
  - Base64/Hex only encode bytes for transport/debug/storage. They provide no encryption, integrity, authentication, or validation of semantic content.
  - Duplicate root/module source ownership remains `source-map-pending`; this record does not choose a canonical copy.
- Test evidence:
  - `crypto_crc` covers CRC public vectors plus software/table/HW-fallback contract.
  - `crypto_encode` covers Base64/Hex vectors, invalid inputs, buffer-too-small handling, and public length helpers.
  - `crypto_smoke_example` exercises host-safe public Base64/Hex APIs as API-drift smoke only.
  - `crypto_review_manifest` links this review record and keeps duplicate-copy/source-map policy checks active.
- Missing evidence:
  - External provenance/license record, independent audit, fuzzing, hardware acceleration evidence, and a source-ownership reconciliation decision.

## 4. Decision

- Decision status: `security-reviewed-limited`
- Allowed product usage:
  - CRC may be used only as a non-security checksum/integrity helper where accidental-error detection is sufficient.
  - Base64/Hex may be used only as encoding/decoding helpers with no security claim.
- Required follow-up before stronger claims:
  - Add provenance/license evidence for each implementation source.
  - Reconcile or intentionally preserve root/module duplicate source ownership with focused and root-target tests.
  - Add fuzz/boundary evidence if these helpers become public input parsers in a product surface.
  - Do not promote CRC/Base64/Hex to security controls; use reviewed cryptographic MAC/signature/encryption primitives instead.

## 5. Verification commands

Recorded for this review-record/manifest slice:

```bash
cd build/tests/unit && ctest --output-on-failure -R '^(crypto_review_manifest|crypto_crc|crypto_encode|crypto_smoke_example)$'
make test-unit
git diff --check
```

## 6. Attachments / evidence

- `tests/unit/crypto/test_crypto_crc.c` CRC host vector/guard coverage.
- `tests/unit/crypto/test_crypto_encode.c` Base64/Hex host vector/guard coverage.
- `tests/unit/crypto/test_crypto_smoke_example.c` host-safe public API smoke.
- `docs/design/xinyi-crypto-source-ownership-map-2026-08-12.md` root/runtime vs focused-test source ownership map.
- `components/crypto/crypto_review_manifest.json` `crc` and `base64_hex` entries.
