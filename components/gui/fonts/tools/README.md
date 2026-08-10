# XinYi bitmap font generator bootstrap

The current XinYi GUI font assets are legacy checked-in bitmap tables. The
`generate_bitmap_font.py` bootstrap validates `font_manifest.json`, source-file
presence, range/count consistency, known duplicate/placeholder inventory flags,
and deterministic manifest summaries. It intentionally does not import external
font files or rewrite the legacy C bitmap tables yet.

Current host guard:

```bash
python3 components/gui/fonts/tools/generate_bitmap_font.py --check
python3 components/gui/fonts/tools/generate_bitmap_font.py --summary
```

`tests/unit/CMakeLists.txt` registers the first command as
`gui_font_generator_manifest`, so `make test-unit` catches manifest/schema drift.

Future implementation requirements:

1. Read `components/gui/fonts/font_manifest.json`.
2. Validate source font provenance/license before generating outputs.
3. Emit deterministic `.c/.h` bitmap tables with explicit glyph order.
4. Reject duplicate codepoints unless the manifest allows and explains them.
5. Report placeholder glyph counts separately from generated real glyphs.
6. Keep manifest validation passing before and after generated table updates.

Do not bulk-import full CJK font data or claim hardware rendering validation as
part of the generator bootstrap slice.
