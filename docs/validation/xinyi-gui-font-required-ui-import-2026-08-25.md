# XinYi GUI Required UI Glyph Import Record (2026-08-25)

status: host-guarded-active-subset
record_type: font-table-import
component: gui-fonts
scope: 15 manifest-required Chinese UI glyphs only

## Evidence boundary

This record proves that the 15 required UI codepoints were copied byte-for-byte from the manifest-pinned Noto Sans CJK SC Host candidate snapshot into the active 16x16 Chinese table and are protected by Host tests. It is not human visual approval, real display validation, performance evidence, or approval of the remaining legacy glyph table.

## Source identity

- Family: Noto Sans CJK SC Regular
- Upstream: <https://github.com/notofonts/noto-cjk>
- Source file: `NotoSansCJK-Regular.ttc`, collection index `2`
- Source SHA-256: `b76b0433203017ca80401b2ee0dd69350349871c4b19d504c34dbdd80541690a`
- License: OFL-1.1; checked-in copy at `components/gui/fonts/LICENSE-NotoSansCJK-OFL-1.1.txt`
- Deterministic Host snapshot: `components/gui/fonts/generated/noto_sans_cjk_sc_16_ui_snapshot.json`

## Active subset

The active table in `components/gui/fonts/xy_font_chinese_16x16.c` now uses distinct, nonblank 16x16 MSB-first bitmaps for:

`U+4E0A U+4E0B U+5DE6 U+53F3 U+786E U+8BA4 U+53D6 U+6D88 U+8BBE U+7F6E U+8FD8 U+56DE U+83DC U+5355 U+5B9A`

Legacy duplicate codepoint rows remain for compatibility but point to the same imported bitmap for that codepoint. All other legacy Chinese entries remain placeholder/review-pending and are not upgraded by this record.

## Verification

- RED: `gui_fonts` failed because required glyphs were blank/aliased (`Expected TRUE Was FALSE`).
- GREEN focused: `ctest --test-dir build/tests/unit -R '^gui_font' --output-on-failure` — 15/15 passed.
- Full Host and PC root build are recorded in the Sprint tracker for the closing commit.

## Remaining gates

- Human visual acceptance: pending.
- SSD1306/ST7789 real-display validation: pending/blocked on hardware.
- Remaining legacy Chinese placeholder replacement: pending.
- Legacy ASCII 8x16/16x24 provenance and visual acceptance: pending.
