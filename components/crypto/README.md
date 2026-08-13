# XinYi Crypto 组件

## 当前状态

Crypto 组件当前是 **host-guarded / API 契约已覆盖 / 安全等级待产品审查** 状态。

- 根构建会自动发现 `components/crypto/CMakeLists.txt`，当前产出 `xy_tiny_crypto` 静态库。
- root `Kconfig` 提供 `COMPONENT_CRYPTO` 入口，但默认仍为 `n`；当前核心算法源和 focused 单测可以 direct-opt-in 使用。
- 活跃 host Unity/CTest 位于 `tests/unit/crypto/`，覆盖 CRC、RNG、CSPRNG、Base64/Hex、MD5/SHA-256、AES/HMAC/SM3/SM4/ChaCha20、SM2 public contract、LWC/Ascon、Curve25519 generic 与 Cortex-M0 fallback。
- 旧 `ReadMe.md`/`crypto.md`/`xy_tiny_boot_crypto.md` 保留历史材料；已删除截断的 `xy_tiny_boot_crypto copy.md` stale duplicate；本文件是组件闭环入口与当前事实源。

> 安全边界：该组件用于 XinYi firmware 的轻量级/嵌入式软件契约护栏。现有 host CTest 证明 API 行为、向量与 guard path，不等同于正式密码学安全审计、侧信道评估、FIPS/国密合规认证或硬件加速验证。

## 构建入口

```bash
# 根 PC 构建中的 crypto 静态库
cmake --build build/pc --target xy_tiny_crypto -j$(nproc)

# 若 build/pc 不存在，先使用仓库默认 PC 配置
make configure
cmake --build build/pc --target xy_tiny_crypto -j$(nproc)
```

当前组件 CMake 仍沿用历史 target 名称 `xy_tiny_crypto`；如未来要改为 `xy_crypto`，需要单独 proposal，避免破坏已有 root auto-discovery 和外部引用。

## 单元测试入口

Focused CTest 名称：

| CTest | 覆盖范围 |
| --- | --- |
| `crypto_crc` | CRC public vectors、table/software/hw-fallback contract |
| `crypto_random` | simple RNG 参数、长度、输出形态 |
| `crypto_csprng` | CSPRNG lifecycle、buffering、reseed、integer helpers |
| `crypto_encode` | Base64/Hex vectors、invalid input、buffer-too-small |
| `crypto_hash` | MD5/SHA-256 vectors、incremental vs one-shot |
| `crypto_cipher_hmac` | AES/HMAC/SM3/SM4/ChaCha20/Poly1305/ChaCha20-Poly1305 AEAD API/vector contracts, including RFC 8439 host vectors and auth-failure output preservation |
| `crypto_blake2` | BLAKE2s public vectors, incremental/keyed behavior, invalid-parameter guards, and output-preservation contract; host contract only, not security/provenance review |
| `crypto_sm2` | SM2 public API guard paths and placeholder-grade contract |
| `crypto_lwc` | lightweight crypto/Ascon style public contracts |
| `crypto_smoke_example` | Host-safe public Base64/Hex/SHA-256/simple-RNG API smoke; links focused module sources and remains API-drift guard only, not a security/hardware proof |
| `crypto_alias_target` | CMake configure smoke that proves `xy_tiny_crypto` still exists and `xy_crypto` compatibility alias is exported |
| `crypto_root_target_smoke` | Standalone public consumer linked against the real `xy_tiny_crypto` root/runtime target; proves aggregate `src/` Base64/Hex/SHA-256, BLAKE2s, and ECDSA format-only guard paths for one small API flow |
| `crypto_review_manifest` | Policy smoke for security/provenance review status, source ownership map links, mapped root aggregate sources, root `src/*.c` collection shape, SM2/ECDSA security-rejected review-record linkage, and currently byte-identical CRC/Base64/Hex root/module duplicate pairs; not cryptographic validation |
| `crypto_25519` | Curve25519 generic public API contracts |
| `crypto_25519_m0` | Cortex-M0 fallback/API and field smoke contracts |

常用验证：

```bash
make test-unit
cd build/tests/unit && ctest --output-on-failure -R '^crypto_'
git diff --check
```

## 目录事实源

```text
components/crypto/
├── CMakeLists.txt              # root auto-discovered component target: xy_tiny_crypto
├── inc/                        # public config and umbrella headers
├── src/                        # historical aggregate source copies
├── xy_aes/ xy_base/ xy_crc/    # active focused-test source roots for many algorithms
├── xy_hmac/ xy_md/ xy_rng/
├── xy_sm2/ xy_sm3/ xy_sm4/
├── xy_25519/ xy_chacha/ xy_blake/
├── examples/                   # historical examples; not all are root-smoke guarded
└── curve25519-cortexm0-*/      # upstream/vendor-style Cortex-M0 material; excluded from unit inventory
```

Important ownership notes:

- Several algorithms still have duplicate historical source copies under both `src/` and module directories. Do not delete or merge them without a path-limited ownership proposal and focused tests proving each public symbol source.
- `tests/unit/crypto` intentionally links specific algorithm sources directly for deterministic host CTests; this does not by itself prove every historical example or aggregate copy is product-ready.
- The Cortex-M0 upstream helper tests under `curve25519-cortexm0-20150813/test/` remain excluded from the canonical Unity inventory.

## Remaining backlog

1. **Security/provenance review**: initial review policy now lives in `docs/design/xinyi-crypto-security-provenance-review-plan-2026-08-12.md`, with record template `docs/validation/xinyi-crypto-security-provenance-review-record-template-2026-08-12.md`; `crypto_review_manifest` guards `components/crypto/crypto_review_manifest.json` so algorithms stay review-pending unless a real review record is linked. CRC/Base64/Hex now have a limited boundary record (`docs/validation/xinyi-crypto-crc-encoding-limited-security-review-2026-08-13.md`) documenting that CRC is non-security checksum/integrity only and Base64/Hex are encoding-only helpers with no encryption, authentication, compliance, or production security claim. SM2 now has an explicit `security-rejected` record (`docs/validation/xinyi-crypto-sm2-placeholder-security-review-2026-08-13.md`) documenting that it is test-only/compatibility-only placeholder code and must not be used for production signing, verification, encryption, key exchange, authentication, or other security-sensitive paths. Root aggregate ECDSA now has an explicit `security-rejected` record (`docs/validation/xinyi-crypto-ecdsa-placeholder-security-review-2026-08-13.md`) documenting that `xy_ecdsa_p256_verify()` is only a format guard returning success for valid-looking fields and must not be used for production signature verification, secure boot, firmware authenticity, authentication, authorization, or other security-sensitive paths. Random/CSPRNG now has a limited boundary record (`docs/validation/xinyi-crypto-random-csprng-limited-security-review-2026-08-13.md`) documenting that `xy_random_*` is non-security utility/test/demo only and `xy_csprng_*` still requires caller-owned seed/entropy quality, reseed policy, provenance, side-channel review, and hardware RNG evidence before stronger claims. MD5/SHA-256/HMAC now has a limited boundary record (`docs/validation/xinyi-crypto-md5-sha256-hmac-limited-security-review-2026-08-13.md`) that keeps MD5 legacy/non-security-only while SHA-256/HMAC-SHA256 remain contract-guarded lightweight helpers pending provenance and side-channel review. AES/SM3/SM4/ChaCha20/Poly1305 now has a limited boundary record (`docs/validation/xinyi-crypto-aes-sm3-sm4-chacha-limited-security-review-2026-08-13.md`) documenting that these helpers are only contract-guarded lightweight implementations and still require caller-owned key/nonce/padding policy plus provenance/side-channel review before stronger claims. BLAKE2 now has its own limited boundary record (`docs/validation/xinyi-crypto-blake2-limited-security-review-2026-08-13.md`) covering both root/runtime and module copies as host-contract-only BLAKE2s helpers while preserving source-map-pending duplicate ownership. LWC/Ascon/TinyJambu/Photon-Beetle now has a limited boundary record (`docs/validation/xinyi-crypto-lwc-limited-security-review-2026-08-13.md`) documenting focused-test-only ownership, missing upstream provenance/license evidence, incomplete authoritative full KAT coverage, and no production/certification/hardware claim. The manifest checks that `components/crypto/CMakeLists.txt` still uses the mapped `src/*.c` aggregate-source collection shape, keeps currently byte-identical CRC/Base64/Hex/Random/CSPRNG/MD5/HMAC/AES/BLAKE2 root/module duplicate source pairs from silently diverging before an explicit ownership slice, and no longer carries mapped-but-unreviewed BLAKE2 after the limited review record was linked. The root smoke exercises ECDSA only as the documented format-only guard contract; root ECDSA no longer pulls the duplicate SHA-256 implementation into `xy_tiny_crypto`, and it is still not production signature validation.
2. **Root target compatibility alias**: historical `xy_tiny_crypto` remains the real runtime/install target, while `components/crypto/CMakeLists.txt` now provides an `xy_crypto` ALIAS for examples/consumers that already use the component-style name; any full rename still needs a separate proposal.
3. **Duplicate source ownership**: current root/runtime vs focused-test source ownership is mapped in `docs/design/xinyi-crypto-source-ownership-map-2026-08-12.md`; reconcile `src/` copies vs module-directory copies only after a proposal and focused source-map verification.
4. **Examples**: add small host-safe smoke examples only for active public APIs; do not revive stale broad demos in one batch.
5. **Hardware acceleration**: keep HAL/SDK acceleration default-off until a focused host seam plus MCU compile/hardware evidence exists.

## Verification snapshot

Last status-sync slice validated:

```bash
cmake --build build/pc --target xy_tiny_crypto -j$(nproc)
make test-unit
cd build/tests/unit && ctest --output-on-failure -R '^crypto_'
git diff --check
```

The `xy_tiny_crypto` build currently succeeds but may still emit pre-existing warning classes in placeholder/aggregate sources. Treat warning cleanup as a separate code-quality slice with focused regression tests, not as part of this README/status sync. The focused module-source `crypto_cipher_hmac` CTest now guards RFC 8439 Poly1305/AEAD vectors, and `crypto_blake2` guards focused BLAKE2s vector/guard contracts while `crypto_review_manifest` keeps the duplicate root/module BLAKE2 copies synchronized. These are still host contract evidence only and must not be promoted to security/provenance approval.
