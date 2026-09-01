# XinYi Known Limitations

XinYi is currently a **host-guarded development baseline**, not a release-qualified or broadly
production-ready framework.

- Host CTests and PC builds validate software contracts only; they are not real-board evidence.
- Cross-compilation proves source/toolchain reachability only; it is not runtime or hardware evidence.
- QEMU results include simulated behavior and are not interchangeable with board validation.
- STM32U5 GPIO/UART/I2C/SPI/IRQ/DMA still lack a unified B1/B2 hardware record.
- Crypto correctness tests do not establish provenance, side-channel resistance, or security approval;
  SM2/ECDSA product use remains rejected and Secure FOTA remains blocked without an approved provider.
- GUI rendering is Host-guarded; real-display quality, frame time, RAM peak, and recovery are pending.
- Sensor, Fuel Gauge, DM, PM, Net, and storage have unresolved ownership, durability, concurrency, or
  real-hardware evidence gaps described in `docs/validation/component-evidence-matrix.md`.
- One bounded PC static-library artifact (`libxy_device.a`) is rebuilt reproducibly from
  `git archive HEAD` and archived with a checksum, CycloneDX JSON 1.6 SBOM, ephemeral CI-gate
  Ed25519 signature, Apache-2.0 license text, and bounded license/NOTICE records. This is not a
  complete release artifact set: legal review remains `LEGAL_REVIEW_PENDING`, the signing key has
  no release identity or publication authority, and no complete PC/MCU SBOM or release-candidate
  HIL gate exists.

The component evidence matrix is the authority for current evidence levels. A release tag must not be
interpreted as upgrading any component beyond that matrix.
