# XinYi Crypto lightweight crypto limited security review

**组件**: `components/crypto`  
**审查对象**: `lwc_ascon_tinyjambu_photon_beetle` / Ascon, TinyJambu, Photon-Beetle helper area  
**记录日期**: `2026-08-13`  
**审查人**: `Zero / scheduled component-quality loop`  
**状态**: `security-reviewed-limited`

> 重要：本记录只固定当前 lightweight crypto 区域的有限安全边界。结论不得由 host CTest 通过、deterministic test vectors、fake entropy、或 STM32 compile-only 结果替代或升级为完整密码学安全审计。

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
  - `components/crypto/xy_ascon/xy_ascon.c`
  - `components/crypto/xy_ascon/xy_ascon.h`
  - `components/crypto/xy_tinyjambu/xy_tinyjambu.c`
  - `components/crypto/xy_tinyjambu/xy_tinyjambu.h`
  - `components/crypto/xy_photon_beetle/xy_photon_beetle.c`
  - `components/crypto/xy_photon_beetle/xy_photon_beetle.h`
  - `tests/unit/crypto/test_lwc.c`
  - `components/crypto/crypto_review_manifest.json`
- Upstream/project origin:
  - The implementation comments say the code is based on the Ascon, TinyJambu, and Photon reference implementations, but this slice did not link upstream URL, version/tag, tarball hash, or import diff evidence.
- License evidence:
  - No SPDX header or separate upstream license/provenance record was found in the reviewed LWC source files during this slice; provenance therefore remains `review-pending`.
- Local modifications:
  - No Ascon/TinyJambu/Photon-Beetle implementation code changes in this review slice.
- Duplicate ownership notes:
  - These sources are currently `focused-test-only-until-root-ownership-decided` in `crypto_review_manifest.json`; they are not part of the root/runtime `components/crypto/src/*.c` aggregate target.
  - This record does not add the LWC implementations to `xy_tiny_crypto`, change root `COMPONENT_CRYPTO` default-off policy, or reconcile algorithm ownership.

## 3. Security review

- Intended use:
  - Contract-guarded lightweight crypto helper experiments for explicitly opted-in firmware consumers that accept current implementation risk and missing provenance/side-channel evidence.
- Explicit non-goals:
  - No CAESAR/NIST LWC compliance claim, FIPS/国密 certification, formal verification, side-channel resistance, constant-time audit, hardware acceleration, key-management, nonce-management, AEAD misuse-resistance proof, or production enablement approval.
  - No approval to export these helpers through the default root runtime target or enable `COMPONENT_CRYPTO` by default.
- Known placeholder/legacy/weak areas:
  - `crypto_lwc` currently proves host API behavior and some roundtrip/auth-failure contracts, but not authoritative upstream KAT conformance for every variant/parameter set.
  - The Ascon decrypt test documents current authentication-failure behavior for vectors produced by the local encrypt helper rather than proving a normal encrypt/decrypt success roundtrip.
  - Correct security-sensitive use would require caller-owned key storage, nonce uniqueness, associated-data policy, algorithm/version selection, error handling, and reviewed upstream provenance.
- Test evidence:
  - `crypto_lwc` covers Ascon encrypt variants/hash, current Ascon decrypt authentication-failure behavior, TinyJambu encrypt/roundtrip/bad-tag behavior, and Photon-Beetle roundtrip/tag/hash/bad-tag behavior.
  - `crypto_review_manifest` links this review record and keeps the LWC area out of provenance-approved or hardware-validated status.
- Missing evidence:
  - Upstream URL/tag/hash, license/provenance review, authoritative KAT corpus, independent security audit, side-channel/constant-time review, fuzzing, misuse-resistance review, MCU/hardware acceleration evidence, and a root-runtime ownership decision.

## 4. Decision

- Decision status: `security-reviewed-limited`
- Allowed product usage:
  - Contract-guarded lightweight helper experimentation only where current implementation risk, focused-test-only ownership, missing upstream provenance, and missing side-channel evidence are explicitly accepted.
  - These helpers remain prohibited as certified, compliance-sensitive, hardware-validated, or side-channel-reviewed implementations until separate evidence exists.
- Required follow-up before stronger claims:
  - Link upstream source/version/license evidence and record any local import modifications.
  - Add authoritative KAT coverage for each algorithm variant/parameter set that a product intends to use.
  - Reconcile focused-test-only vs root/runtime ownership before exposing through `xy_tiny_crypto`.
  - Add side-channel/constant-time and misuse-resistance review for any security-sensitive product path.
  - Add hardware acceleration or MCU-specific validation only through separate hardware validation records.

## 5. Verification commands

Recorded for this review-record/manifest slice:

```bash
cd build/tests/unit && ctest --output-on-failure -R '^(crypto_review_manifest|crypto_lwc)$'
make test-unit
git diff --check
```

## 6. Attachments / evidence

- `tests/unit/crypto/test_lwc.c` lightweight crypto host contract coverage.
- `docs/design/xinyi-crypto-source-ownership-map-2026-08-12.md` root/runtime vs focused-test source ownership map.
- `components/crypto/crypto_review_manifest.json` `lwc_ascon_tinyjambu_photon_beetle` entry.
