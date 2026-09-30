# XinYi Known Limitations

XinYi is currently a **host-guarded development baseline**, not a release-qualified or broadly
production-ready framework.

- Host CTests and PC builds validate software contracts only; they are not real-board evidence.
- Cross-compilation proves source/toolchain reachability only; it is not runtime or hardware evidence.
- QEMU results include simulated behavior and are not interchangeable with board validation.
- STM32U5 remains enhancement compile-only and has no unified B1/B2 hardware qualification.
- Pandora STM32L475VE has multiple bounded B1/B2 records, but this is not complete board qualification; ICM20608 dynamic response and the deferred hardware/endurance backlog remain pending.
- Crypto correctness tests do not establish provenance, side-channel resistance, or security approval;
  SM2/ECDSA product use remains rejected and Secure FOTA remains blocked without an approved provider.
- GUI core/rendering remains Host-guarded; Pandora ST7789 fixed-panel color/pattern/rotation has bounded visual evidence, while font approval, input, frame time, RAM peak, recovery, and product visual quality remain pending.
- Sensor, Fuel Gauge, DM, PM, Net, and storage have unresolved ownership, durability, concurrency, or
  real-hardware evidence gaps described in `docs/validation/component-evidence-matrix.md`.
- One bounded PC static-library artifact (`libxy_device.a`) is rebuilt reproducibly from
  `git archive HEAD` and archived with a checksum, CycloneDX JSON 1.6 SBOM, ephemeral CI-gate
  Ed25519 signature, Apache-2.0 license text, and bounded license/NOTICE records. This is not a
  complete release artifact set: legal review remains `LEGAL_REVIEW_PENDING`, the signing key has
  no release identity or publication authority, and no complete PC/MCU SBOM or release-candidate
  HIL gate exists.
- The Pandora pre-RC scope selects `pandora_stm32l475_rtos.bin` as the sole MCU build-gate artifact
  and excludes every inventoried example/project from release support. A default-off gate rebuilds that
  BIN twice from `git archive HEAD` plus the pinned STM32CubeL4 gitlink checkout, compares SHA-256/size,
  archives the BIN/checksum pair, and independently verifies it. This is not an MCU SBOM, signature,
  CI publication, install/runtime qualification, publication authority, RC approval, or R1 qualification.
- The [Pandora MCU SBOM policy](../validation/pandora-release-sbom-policy.json) defines the next
  CycloneDX 1.6 generation inputs and exact artifact scope, but remains `GENERATION_PENDING`; no MCU
  SBOM, complete dependency provenance, or legal/license approval is claimed.

The component evidence matrix is the authority for current evidence levels. A release tag must not be
interpreted as upgrading any component beyond that matrix.
