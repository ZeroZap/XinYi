# XinYi GUI Font Framebuffer Snapshot Review Proposal

Status: proposal / host-only visual-review boundary
Date: 2026-08-11

## Why this exists

The GUI font path now has host guards for legacy font tables, manifest metadata,
generator preview/write paths, and C99-compilable generated-preview artifacts.
Those tests prove API and generator contracts, but they still do not answer the
next product question: what do representative glyphs actually look like when the
runtime font engine writes pixels into a framebuffer?

This proposal defines a small, deterministic host snapshot-review slice before
any generated glyph-byte tables or larger CJK assets are committed.

## Current verified baseline

- `gui_fonts` guards legacy ASCII 8x16, ASCII 16x24, and Chinese 16x16 bitmap
  lookup/measurement contracts.
- `gui_font_engine` guards runtime glyph lookup, drawing, cache, unsupported
  cases, and wide-glyph row bounds.
- `gui_font_manifest` plus `gui_font_generator_*` Python CTests guard manifest
  schema, deterministic summary/header output, glyph metadata, legacy-passthrough
  preview/write paths, and C99 compile of generated previews.
- None of the above is a visual-quality review, complete Chinese font import, or
  real display hardware validation.

## Proposed next slice

Add a host-only font framebuffer snapshot smoke that renders a small reviewed
sample set into a deterministic in-memory monochrome/RGB565 framebuffer and emits
stable text/metadata artifacts for review.

Suggested path-limited implementation:

```text
tests/unit/gui/test_gui_font_snapshot.c
components/gui/fonts/tools/generate_bitmap_font.py   # only if a Python snapshot helper is preferred
docs/design/unit-test-inventory.md
docs/design/xinyi-component-quality-loop.md
```

Recommended CTest name:

```text
gui_font_snapshot
```

## Snapshot contract

The host smoke should render only the current manifest-declared assets:

1. ASCII 8x16: printable boundary and common UI text, e.g. `" !~OK"`.
2. ASCII 16x24: numeric/status sample, e.g. `"12:34"`.
3. Chinese 16x16 legacy UI glyphs: only already-required manifest codepoints
   such as `上/下/左/右/确认/取消/菜单`.

The test should assert deterministic software facts, not subjective art quality:

- framebuffer dimensions and stride are fixed;
- rendered pixel count is non-zero for known glyphs;
- unknown glyphs preserve the current fallback/unsupported contract;
- line wrapping or clipping uses the same bounds as `gui_font_engine`;
- a stable checksum/hash or ASCII-art preview changes only when glyph output
  actually changes;
- generated snapshot metadata labels the result as `host-snapshot`, not
  `hardware-passed`.

## Non-goals

- Do not import a complete CJK font set in this slice.
- Do not commit generated glyph-byte `.c/.h` tables yet.
- Do not modify `MCU/`, vendor display drivers, or board pinmux/HAL bindings.
- Do not claim real LCD/OLED/LED-matrix rendering has passed from host snapshots.
- Do not mix z-serial/GUI application dirty files into this component slice.

## Evidence ladder after this proposal

1. **Current:** contract-guarded legacy font tables and generator previews.
2. **Next:** host framebuffer snapshot smoke for representative glyph samples.
3. **Later:** reviewed generated glyph-byte tables from licensed/source-tracked
   inputs.
4. **Final:** real screen hardware photo/log record tied to a board/display
   configuration.

## Verification for the proposal slice

For a proposal-only update, lightweight validation is sufficient:

```bash
git diff --check
```

For the future implementation slice, run:

```bash
make test-unit
cd build/tests/unit && ctest --output-on-failure -R '^gui_font_(snapshot|fonts|font_engine|font_manifest|generator_.*)$'
git diff --check
```
