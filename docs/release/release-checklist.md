# XinYi Release Readiness Checklist

**Status:** `BLOCKED`

**Release decision:** `NO-GO`

**Authority:** [component evidence matrix](../validation/component-evidence-matrix.md),
[Sprint tracker](../plans/SPRINT_TRACKER.md), and the tagged source tree.

This checklist is a fail-closed release gate, not release evidence. An unchecked item remains a blocker.
Host/PC/QEMU/compile-only evidence cannot satisfy Board, Security, Performance, or Release qualification.
A release tag does not upgrade any evidence level.

## 1. Source and version facts

- [ ] Release Candidate approved by the named release owner with date and commit SHA.
- [ ] `VERSION`, tag `vMAJOR.MINOR.PATCH`, and the matching `CHANGELOG.md` entry agree.
- [ ] Release scope promotes reviewed entries from the
  [release input inventory](../validation/release-input-inventory.json) and explicitly lists unsupported paths.
- [ ] The tagged commit is pushed, immutable for the release decision, and the worktree/submodules are clean.
- [ ] Known Limitations are reviewed against the current evidence matrix and included in release notes.

## 2. Software gates

- [ ] Canonical Host suite passes from the tagged clean checkout with the exact discovered/pass count recorded.
- [ ] PC root build passes from the tagged clean checkout.
- [ ] Supported target compile matrix passes from the tagged clean checkout with toolchain, chip, config, and submodule revisions recorded.
- [ ] Clean-checkout rebuild succeeds in the documented release environment without untracked inputs.
- [ ] Canonical CI for the tagged commit is green with no empty-test or unexplained allow-failure path.

## 3. Hardware and recovery gates

- [ ] Reference-board HIL passes for every supported target; records include board, wiring, firmware SHA, logs, and captures.
- [ ] Required B2 negative paths pass: timeout/NACK, reset/re-init, interrupted update/write, and recovery.
- [ ] Rollback and recovery evidence covers boot failure, metadata corruption, interrupted update, and return to a known-good image.
- [ ] Performance claims have fixed hardware, clocks, compiler flags, samples, and statistics; otherwise no performance claim is published.

## 4. Security and supply-chain gates

- [ ] Security review and Secure FOTA provider are approved for the release scope; rejected/placeholder implementations cannot be selected.
- [ ] Key provisioning, signature verification, anti-rollback, and bootloader integration have review and target evidence.
- [ ] SBOM and third-party license review cover all release sources, submodules, generated assets, and redistributed binaries.
- [ ] Dependency/source provenance and pinned revisions are archived with the release record.

## 5. Artifacts and publication gates

- [ ] Reproducible release artifacts are generated only from the tagged clean checkout using the documented environment.
- [ ] Checksums and signatures are produced, independently verified, and published with the artifacts.
- [ ] Release notes contain the canonical changelog section, Known Limitations, supported matrix, and evidence boundaries.
- [ ] Artifact install/flash and rollback smoke tests pass on each supported reference board.
- [ ] Final release record links CI, target build, HIL, security, SBOM/license, artifact, checksum/signature, and approval evidence.
- [ ] R1 release qualified is recorded in the component evidence matrix only after every applicable item above passes.

## Current blocking facts (2026-08-31)

- Reference-board HIL/B1/B2 remains unavailable.
- Secure FOTA has no approved production signature provider or real bootloader/board integration evidence.
- The machine-guarded [source dependency inventory](../validation/source-dependency-inventory.json)
  records tracked vendored inputs and top-level gitlinks, but remains `REVIEW_PENDING`; it is not an
  artifact SBOM or license approval. SBOM, reproducible artifacts, and signed checksum publication
  are not established.
- The machine-guarded [release input inventory](../validation/release-input-inventory.json) classifies
  every tracked top-level `examples/` and `projects/` entry. All remain `excluded-pending-review`;
  Host CTest or compile-only evidence does not select an entry for release support.
- The [PC release build environment](../validation/pc-release-build-environment.json) records a fixed
  Ubuntu 24.04 runner contract and emits actual CMake/CC/AR/Python identity. The bounded
  [PC release artifact manifest](../validation/pc-release-artifact-manifest.json) selects only
  `xy_device` / `libxy_device.a` for the current reproducibility gate. This is not a complete release
  artifact set; no immutable container digest or complete dependency lock exists.
- Therefore Sprint 6 and R1 remain blocked; this checklist does not authorize a release.
