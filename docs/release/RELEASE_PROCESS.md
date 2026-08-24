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

5. Commit and push the verified release preparation to `main`.
6. Create and push an annotated tag whose name exactly matches `v$(cat VERSION)`, without moving or
   rewriting an existing tag.

The `release.yml` workflow validates the tag against `VERSION`, reruns the canonical Host/PC gates,
and creates a GitHub Release using this changelog plus Known Limitations. It does not publish firmware
artifacts until a later release-qualified build pipeline exists.
