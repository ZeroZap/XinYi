# XinYi Release Process

## Facts and evidence boundary

- `VERSION` is the only framework-version fact source.
- Root CMake reads `VERSION`; `Kconfig` and `components/xy_version.h` are checked mirrors for embedded
  consumers and generated configuration.
- `docs/release/CHANGELOG.md` is the canonical release-note path.
- `docs/release/known-limitations.md` is mandatory release context.
- Files under `docs/history/` are archival and do not define current release readiness.
- A tag or GitHub Release does not upgrade Host, compile-only, QEMU, hardware, performance, or
  security evidence. Those claims require records in `docs/validation/`.

## Preparing a release

1. Update `VERSION` with a strict `MAJOR.MINOR.PATCH` value.
2. Synchronize the checked mirrors in `Kconfig` and `components/xy_version.h`.
3. Move relevant entries from `Unreleased` into a matching version section in
   `docs/release/CHANGELOG.md` and update Known Limitations.
4. Run:

   ```bash
   python3 tools/scripts/check_release_facts.py
   make test-unit
   cmake -S . -B build/pc -DHAL_PLATFORM=PC -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF
   cmake --build build/pc -j$(nproc)
   git diff --check
   ```

5. Complete every release checklist gate, record the exact marker **R1 status:** `QUALIFIED` in the
   evidence matrix, then change the checklist status/decision to `READY` / `GO` in the reviewed
   release-preparation commit. Do not add that marker before the R1 evidence package is approved.
6. Verify the publication authorization gate explicitly:

   ```bash
   python3 tools/scripts/check_release_facts.py --require-release-authorization
   ```

7. Commit and push the verified release preparation to `main`.
8. Create and push an annotated tag whose name exactly matches `v$(cat VERSION)`, without moving or
   rewriting an existing tag.

The `release.yml` workflow validates the tag against `VERSION` and requires the same explicit release
authorization before running Host/PC gates or creating a GitHub Release. With the current checklist at
`BLOCKED` / `NO-GO`, a pushed version tag fails closed and cannot reach publication. Future authorization
must not be inferred from a tag: every checklist item and the `R1` evidence record must exist first.
