# XinYi GUI Legacy Font Host Snapshot Review (2026-08-25)

status: rejected-needs-regeneration
record_type: host-snapshot-review
component: gui-fonts
scope: current manifest-declared legacy bitmap assets only

## Evidence boundary

This is a Host-only technical rejection record. It is not a human art approval, license/provenance approval, generated glyph-byte check-in approval, or real display validation. Rejection does not upgrade any evidence tier.

## Review identity

- Reviewer: Zero (automated technical review; not a human visual approver)
- Review date: 2026-08-25
- Repo commit reviewed: `9801ef9c10568f0cd86d7f132076d0ab8a03ad67`
- Manifest: `components/gui/fonts/font_manifest.json`
- Source asset digests: guarded by `gui_font_generator_manifest` and `gui_font_generator_glyph_metadata`

## Commands and samples

Focused source-render probe:

```bash
cc -std=c99 -Wall -Wextra -Werror \
  -Icomponents/gui/fonts \
  /tmp/xinyi_gui_font_asset_probe.c \
  components/gui/fonts/xy_font_8x16.c \
  components/gui/fonts/xy_font_16x24.c \
  components/gui/fonts/xy_font_chinese_16x16.c \
  -o /tmp/xinyi_gui_font_asset_probe
/tmp/xinyi_gui_font_asset_probe
```

The probe rendered source-table bytes directly, MSB first, for:

- ASCII 8x16 `A`: FNV-1a checksum `A7BA9665`
- ASCII 16x24 `A`: FNV-1a checksum `75461FCD`
- Chinese 16x16 required UI glyph `U+4E0A` (`上`): FNV-1a checksum `0B2AE445`
- Chinese 16x16 placeholder glyph `U+4E00` (`一`): FNV-1a checksum `229D0D26`

## Findings

| Asset | Finding | Conclusion |
|---|---|---|
| `ascii_8x16` | `A` renders a recognizable but visibly skewed legacy bitmap | Not accepted as final product art; replacement/review required |
| `ascii_16x24` | `A` source bytes do not form a coherent 16x24 glyph under the documented two-bytes-per-row, MSB-first layout | Rejected; regenerate from a licensed source and verify layout |
| `chinese_16x16_ui_legacy` | Required UI glyph `U+4E0A` is entirely blank; many other rows intentionally share placeholders | Rejected; required UI glyphs need real distinct bitmaps |

The existing test-local `gui_font_snapshot` artifact remains deterministic and useful for renderer/clipping contracts, but it does not render these three production asset tables and therefore cannot overturn this rejection.

## Conclusion

- Visual conclusion: `rejected-needs-regeneration`
- Manifest snapshot state remains `host-snapshot-review-pending`; no font id moves to reviewed/accepted.
- License/provenance remains `project-review-pending` for all three font ids because no author/import origin, SPDX license, or internal ownership statement was found in this slice.
- Real display validation remains pending and blocked on hardware.

## Required remediation

1. Select a redistributable font source and record upstream URL/version/hash/license.
2. Generate deterministic ASCII 8x16, ASCII 16x24, and the required Chinese UI glyph set.
3. Add source-asset snapshots (not only the test-local renderer fixture) to the focused Host gate.
4. Obtain human visual acceptance before changing `snapshot_review_status` to accepted.
5. Keep hardware validation separate and pending until a real display record exists.
