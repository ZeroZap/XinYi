# Generated GUI font preview artifacts

This directory contains deterministic generated-preview artifacts produced from
`components/gui/fonts/font_manifest.json` by
`components/gui/fonts/tools/generate_bitmap_font.py`.

Current status:

- **Tier**: legacy-passthrough preview.
- **Source of truth**: the reviewed legacy bitmap tables in `components/gui/fonts/`.
- **Runtime behavior**: unchanged; the GUI font engine still uses the existing public legacy font handles.
- **Non-goals**: these files do not import a complete CJK font set, do not contain new rasterized glyph art, and do not prove display hardware rendering.

Regenerate/check with:

```bash
python3 components/gui/fonts/tools/generate_bitmap_font.py \
    --write-manifest-header components/gui/fonts/generated/xy_gui_font_manifest_generated.h
python3 components/gui/fonts/tools/generate_bitmap_font.py \
    --write-glyph-preview ascii_8x16 \
    components/gui/fonts/generated/xy_font_8x16_generated.h \
    components/gui/fonts/generated/xy_font_8x16_generated.c
python3 components/gui/fonts/tools/generate_bitmap_font.py \
    --write-glyph-preview ascii_16x24 \
    components/gui/fonts/generated/xy_font_16x24_generated.h \
    components/gui/fonts/generated/xy_font_16x24_generated.c
python3 components/gui/fonts/tools/generate_bitmap_font.py \
    --write-glyph-preview chinese_16x16_ui_legacy \
    components/gui/fonts/generated/xy_font_chinese_16x16_generated.h \
    components/gui/fonts/generated/xy_font_chinese_16x16_generated.c
```

The `gui_font_generator_checked_in_preview` CTest verifies that these checked-in
files remain byte-for-byte reproducible from the generator and manifest.
