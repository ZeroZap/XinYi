# XinYi GUI Font Host Snapshot Artifact (2026-08-11)

status: host-snapshot-only
record_type: deterministic-host-framebuffer-artifact
component: gui-fonts
scope: current test-local snapshot font fixture rendered through `xy_font_draw_string()` / `xy_font_draw_char()`

## Evidence boundary

This artifact records the deterministic host framebuffer metadata currently guarded by `gui_font_snapshot`.

It is not a visual-quality approval, not a license/provenance approval, not generated glyph-byte approval, and not real LCD/OLED/LED-matrix hardware validation.

## Snapshot source

- Source test: `tests/unit/gui/test_gui_font_snapshot.c`
- CTest: `gui_font_snapshot`
- Render sample: `OK\n!~`
  - `O`, `K`, and `!` are known fixture glyphs.
  - `~` is intentionally unsupported and must preserve the framebuffer on direct `xy_font_draw_char()` failure.
- Framebuffer: 32x16 RGB565
- Foreground color: `0xFFFF`
- Hash: FNV-1a 32-bit over little-endian RGB565 pixels

## Deterministic metadata

- framebuffer: 32x16 RGB565
- lit_pixels: 19
- checksum_fnv1a32: 0x8DD0D797
- clipping_checksum_fnv1a32: 0x3ABE3861
- unknown_glyph_contract: `xy_font_draw_char('~') == -1` and framebuffer checksum unchanged

## ASCII-art preview

`#` means a non-zero RGB565 pixel; `.` means zero.

```text
.##..#..#.......................
#..#.#.#........................
#..#.##.........................
.##..#.#........................
................................
#...............................
#...............................
................................
#...............................
................................
................................
................................
................................
................................
................................
................................
```

## Review status

This artifact is machine-verifiable host evidence only. It remains pending for human visual review, license/provenance review, generated glyph-byte replacement review, and real display hardware validation.

Do not use this artifact to mark `font_manifest.json` as license-approved or hardware-passed.
