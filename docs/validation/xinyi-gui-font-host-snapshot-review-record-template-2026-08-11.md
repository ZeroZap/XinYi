# XinYi GUI Font Host Snapshot Review Record Template

status: pending
record_type: host-snapshot-review
component: gui-fonts
scope: current manifest-declared legacy bitmap assets only

## Evidence boundary

This record is for human review of deterministic host framebuffer snapshots produced by `gui_font_snapshot` or a future equivalent host-only snapshot tool.

It is not a license/provenance approval, not a generated glyph-byte check-in approval, and not real LCD/OLED/LED-matrix hardware validation.

## Required review fields

- Reviewer:
- Review date:
- Repo commit:
- Snapshot source command:
  - `cd /home/eugene/zerozap/XinYi && cmake --build build/tests/unit --target test_gui_font_snapshot -j$(nproc)`
  - `cd /home/eugene/zerozap/XinYi/build/tests/unit && ctest --output-on-failure -R '^gui_font_snapshot$'`
- Reviewed samples:
  - ASCII 8x16:
  - ASCII 16x24:
  - Chinese 16x16 UI glyphs:
- Host snapshot metadata:
  - framebuffer dimensions: `32x16 RGB565`
  - checksum/hash: `0x8DD0D797` for the current deterministic `OK\n!~` fixture; clipping checksum `0x3ABE3861`
  - ASCII-art or image artifact path: `docs/validation/xinyi-gui-font-host-snapshot-artifact-2026-08-11.md`
- Visual conclusion:
  - pending / accepted-for-current-legacy-assets / rejected-needs-regeneration
- Remaining blockers:
  - license/provenance review:
  - generated glyph-byte output review:
  - real display hardware validation:

## Approval gate

Do not mark host snapshot review as accepted without storing the reviewed ASCII-art/image artifact and command output.
Do not use host-snapshot acceptance to change `font_manifest.json` license fields, import external fonts, or update any hardware validation record beyond host-snapshot-only.

## Current default status

This template intentionally remains `status: pending` until a reviewer records real snapshot artifacts and a conclusion for the current legacy bitmap assets.
