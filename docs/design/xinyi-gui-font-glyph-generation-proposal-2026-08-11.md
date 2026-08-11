# XinYi GUI Font Glyph Generation Proposal (2026-08-11)

## Slice

Define the next implementation boundary after the manifest/header bootstrap: a deterministic `.c/.h` glyph table generation path that remains software-only, reviewable, and separate from full CJK import or hardware validation.

This proposal does **not** generate new font art, import third-party font files, or replace the checked-in legacy bitmap tables in this slice.

## Current facts

- `components/gui/fonts/font_manifest.json` records the current legacy ASCII 8x16, ASCII 16x24, and Chinese 16x16 UI asset inventory.
- `components/gui/fonts/tools/generate_bitmap_font.py` currently validates the manifest and can emit/write a manifest-inventory generated header preview.
- `gui_font_generator_manifest`, `gui_font_generator_output`, `gui_font_generator_write`, and `gui_font_generator_glyph_metadata` guard schema, deterministic preview, explicit write-path behavior, and reviewed glyph-output metadata.
- The current generator still does not emit glyph bitmap `.c/.h` tables and does not have a source-font/license input contract.

## Proposed generator boundary

Add glyph generation as an explicit opt-in mode rather than changing the existing legacy tables implicitly:

```bash
python3 components/gui/fonts/tools/generate_bitmap_font.py \
  --manifest components/gui/fonts/font_manifest.json \
  --emit-glyph-header <font-id>

python3 components/gui/fonts/tools/generate_bitmap_font.py \
  --manifest components/gui/fonts/font_manifest.json \
  --emit-glyph-source <font-id>
```

Initial scope should be limited to **manifest-declared current assets** and deterministic generated previews. If the generator cannot yet derive glyph bytes from a licensed source font, it should emit a clear unsupported error instead of copying placeholder data silently.

## Manifest additions before real glyph output

Before `.c/.h` glyph generation is considered complete, each generated font entry should declare:

- `output_header`: relative path of the generated public header.
- `output_source`: relative path of the generated source table.
- `source_font`: source asset path or `legacy-table` for legacy passthrough.
- `source_license`: SPDX identifier or `project-review-pending`.
- `generator_mode`: one of `legacy-inventory`, `legacy-passthrough`, or `font-rasterize`.
- `glyph_order`: `ascii-range` or explicit codepoint table.

The generator should reject entries that request glyph output without a declared mode and license/provenance.

## First implementation slice

A safe first code slice has been implemented:

1. Extend `font_manifest.json` with `output_header`, `output_source`, `source_font`, `source_license`, `generator_mode`, and `glyph_order` for the three current assets.
2. Extend `generate_bitmap_font.py --check` to validate those fields.
3. Add `--self-test-glyph-metadata` to verify deterministic output filenames, mode validation, and unsupported-mode errors without writing generated glyph tables.
4. Register the Python smoke as `gui_font_generator_glyph_metadata`.

The next safe slice is still **not** a full CJK import. It should either add reviewed generated-header/source previews for `legacy-passthrough` only, or add a host framebuffer snapshot-review proposal before generated glyph files are committed.

Recommended path limit:

```text
components/gui/fonts/font_manifest.json
components/gui/fonts/tools/generate_bitmap_font.py
tests/unit/CMakeLists.txt
docs/design/xinyi-gui-font-glyph-generation-proposal-2026-08-11.md
```

## Non-goals

- No full Chinese/CJK font import in the metadata slice.
- No generated `.c/.h` files committed until output policy and review are stable.
- No GUI core, display driver, HAL, MCU, or vendor tree changes.
- No claim that generated metadata equals visual typography quality.
- No hardware validation without real board/display evidence.

## Verification anchors

Future implementation should run at least:

```bash
cmake -B build/tests/unit -S tests/unit
cd build/tests/unit && ctest --output-on-failure -R '^gui_font_generator_(manifest|output|write|glyph_metadata)$'
make test-unit
git diff --check
```

If real generated `.c/.h` output is added later, add a focused compile target that includes the generated header and links the generated source before committing generated files.
