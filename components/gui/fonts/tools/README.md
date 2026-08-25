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
python3 components/gui/fonts/tools/generate_bitmap_font.py --emit-manifest-header
python3 components/gui/fonts/tools/generate_bitmap_font.py --write-manifest-header /tmp/xy_gui_font_manifest_generated.h
python3 components/gui/fonts/tools/generate_bitmap_font.py --self-test-output
python3 components/gui/fonts/tools/generate_bitmap_font.py --self-test-write
```

`--emit-manifest-header` prints a deterministic generated-header preview with
manifest inventory constants only (asset count, per-font dimensions, glyph
counts, byte sizes, and ASCII codepoint bounds). It deliberately does not write
files or regenerate bitmap glyph tables yet. `--write-manifest-header` writes
the same deterministic inventory header to an explicit path for review or
packaging smoke tests. `--self-test-output` validates that preview output is
deterministic and contains the current asset contract markers; `--self-test-write`
validates that the write path matches the preview without checking generated files
into the repo.

`tests/unit/CMakeLists.txt` registers `--check` as `gui_font_generator_manifest`,
`--self-test-output` as `gui_font_generator_output`, and `--self-test-write` as
`gui_font_generator_write`, so `make test-unit` catches manifest/schema drift,
deterministic output-preview drift, and write-path drift.

Future implementation requirements:

1. Read `components/gui/fonts/font_manifest.json`.
2. Validate source font provenance/license before generating outputs.
3. Emit deterministic `.c/.h` bitmap tables with explicit glyph order.
4. Reject duplicate codepoints unless the manifest allows and explains them.
5. Report placeholder glyph counts separately from generated real glyphs.
6. Keep manifest validation passing before and after generated table updates.

Do not bulk-import full CJK font data or claim hardware rendering validation as
part of the generator bootstrap slice.

## Licensed replacement candidate

The manifest also pins a software-only replacement candidate: the Simplified
Chinese face (TTC index 2) of `NotoSansCJK-Regular.ttc`, licensed under OFL-1.1.
The 19 MB source font is not vendored. To reproduce the checked-in required-UI
glyph snapshot, provide the exact source whose SHA-256 is recorded in the
manifest and run:

```bash
python3 components/gui/fonts/tools/generate_bitmap_font.py \
  --write-licensed-candidate-snapshot /path/to/NotoSansCJK-Regular.ttc \
  components/gui/fonts/generated/noto_sans_cjk_sc_16_ui_snapshot.json
python3 components/gui/fonts/tools/generate_bitmap_font.py \
  --self-test-licensed-candidate-snapshot
```

CI validates the checked-in metadata and 15 required UI glyphs without Pillow
or the source TTC. The snapshot is not wired into firmware and is not a visual
or hardware approval; activating it requires a separate reviewed slice.
