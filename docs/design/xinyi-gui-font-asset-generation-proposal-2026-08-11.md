# XinYi GUI Font Asset Scope and Generation Proposal (2026-08-11)

## Slice

Close the design gap that remained after `gui_fonts` and `gui_font_engine` became host-guarded: define what the bundled bitmap fonts currently prove, what they intentionally do not prove, and the minimum evidence required before XinYi can claim complete Chinese/font rendering support.

This is a design-stage proposal only. It does not add glyph data, replace the runtime font engine, or claim real screen rendering validation.

## Current facts

- `components/gui/fonts/xy_font_8x16.c/.h` exports a 95-character ASCII 8x16 bitmap table for `0x20..0x7E`.
- `components/gui/fonts/xy_font_16x24.c/.h` exports a 95-character ASCII 16x24 bitmap table for `0x20..0x7E`.
- `components/gui/fonts/xy_font_chinese_16x16.c/.h` exports 168 Chinese table entries. The current table includes common UI characters plus placeholder/duplicate bitmap patterns; it is a host-guarded contract table, not a complete font product.
- `tests/unit/gui/test_gui_fonts.c` (`gui_fonts`) verifies exported font handles, ASCII boundaries, Chinese lookup boundaries, and measurement contracts.
- `tests/unit/gui/test_gui_font_engine.c` (`gui_font_engine`) verifies runtime glyph lookup, measurement, framebuffer writes, cache lifecycle/LRU behavior, load-stub guards, and wide-glyph row bounds.
- There is no generated-font manifest, source-font license record, glyph-art review snapshot, or real display framebuffer/screen capture record.

## Proposed target states

### 1. Contract-guarded font assets (current state)

This state is already achieved and should remain the default wording until more evidence exists:

```text
GUI font assets are host-guarded for public API contracts and table boundaries.
They are not complete Chinese coverage, final glyph art, or hardware rendering validation.
```

Evidence:

- `gui_fonts` CTest passes.
- `gui_font_engine` CTest passes.
- `components/gui/fonts/README.md` documents the exported tables and limitations.
- `gui_font_manifest` CTest validates manifest/table inventory for the current
  legacy assets.
- `gui_font_generator_manifest` CTest validates the generator bootstrap against
  manifest schema/range/source-file contracts; it is not generated glyph output
  yet.

### 2. Generated/reviewable font asset set (next software-only milestone)

XinYi may claim a generated/reviewable font asset set only after a small, reproducible pipeline exists:

1. A tracked manifest such as `components/gui/fonts/font_manifest.json` lists:
   - font family/source file name;
   - license/SPDX or project-owned provenance;
   - pixel size;
   - supported Unicode ranges or explicit codepoint list;
   - output `.c/.h` files;
   - generator version/options.
2. A repo-local generator script produces deterministic `.c/.h` tables from the manifest.
3. A host CTest validates manifest/table consistency and generator output preview:
   - ASCII table count and range;
   - Chinese/UI codepoint list presence;
   - every manifest codepoint has a bitmap entry;
   - duplicate codepoints are rejected unless explicitly allowed with a reason;
   - placeholder bitmap patterns are counted and reported;
   - generated manifest-inventory header preview is deterministic and contains
     asset-count, glyph-count, dimensions, byte-size, and ASCII codepoint bounds.
4. The generator can run without GUI or hardware dependencies.

This milestone still does not prove visual quality; it only proves reproducibility and declared coverage.

### 3. Snapshot-reviewed rendering (visual software milestone)

XinYi may claim software-rendered font visual review only after deterministic host snapshots exist:

1. A host smoke renders representative text into a memory framebuffer through `xy_font.c`:
   - ASCII: `Hello XinYi 123`;
   - UI Chinese: `确认 取消 设置 返回 菜单`;
   - mixed string: `XinYi 设置 OK`;
   - boundary/unknown glyph cases.
2. The smoke exports PBM/PGM/PNG or an equivalent text-safe bitmap artifact.
3. The artifact is compared against a checked-in golden snapshot or stored as a review record with explicit update policy.
4. The result is labeled as host framebuffer rendering only, not hardware validation.

### 4. Hardware-rendered font validation (board milestone)

XinYi may claim real font display validation only after a board-specific record exists:

- board/display model and resolution;
- display driver/backend used (`SSD1306`, LCD, LED matrix, etc.);
- firmware commit hash;
- test text rendered;
- photo/frame capture or logic/framebuffer trace;
- pass/fail notes for legibility, clipping, alignment, contrast, and refresh behavior.

Host CTests, generated tables, and compile-only builds must not be promoted to this milestone.

## Proposed next implementation slice

The next low-risk implementation slice should be software-only and path-limited:

```text
components/gui/fonts/font_manifest.json
components/gui/fonts/tools/generate_bitmap_font.py
components/gui/fonts/README.md
tests/unit/gui/test_gui_font_manifest.c or a small manifest smoke target
```

Scope rules:

- Start with current exported assets; do not bulk-import a full Chinese font in the same slice.
- Treat current Chinese placeholder/duplicate behavior as inventory data first, not an immediate rewrite.
- Keep generated outputs deterministic and reviewable.
- If licensing/provenance is unknown, record `provenance: legacy-project-asset` and do not claim third-party license compatibility until reviewed.

## Non-goals for this proposal

- No GUI framework migration.
- No vendor/HAL/display-driver changes.
- No new complete CJK font import.
- No claim that `gui_fonts` or `gui_font_engine` proves final typography quality.
- No hardware validation without real board/display evidence.

## Verification anchors

Any future font asset/generator slice should run at least:

```bash
cmake -B build/tests/unit -S tests/unit
cmake --build build/tests/unit --target test_gui_fonts test_gui_font_engine test_gui_font_manifest -j$(nproc)
cd build/tests/unit && ctest --output-on-failure -R '^gui_(fonts|font_engine|font_manifest|font_generator_manifest|font_generator_output)$'
make test-unit
git diff --check
```

If a snapshot smoke is added later, it should be included in `make test-unit` and its artifact update policy must be documented before committing new golden files.
