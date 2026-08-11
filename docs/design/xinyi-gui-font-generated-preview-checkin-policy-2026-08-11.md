# XinYi GUI Font Generated Preview Check-in Policy (2026-08-11)

## Slice

Define the review boundary for eventually checking generated GUI font preview artifacts into the repository.

This is a policy/proposal slice only. It does not generate or commit `components/gui/fonts/generated/*`, does not import a complete CJK font set, and does not change runtime GUI/font code.

## Current verified baseline

- `components/gui/fonts/font_manifest.json` records the current legacy ASCII 8x16, ASCII 16x24, and Chinese 16x16 UI legacy assets plus explicit output paths and provenance fields.
- `components/gui/fonts/tools/generate_bitmap_font.py` can validate the manifest, emit/write deterministic manifest headers, emit/write deterministic legacy-passthrough glyph preview `.h/.c` pairs, and compile those previews in a temporary tree.
- `gui_font_generator_glyph_compile` proves temporary generated previews are C99-buildable, but those generated files are not yet source-of-truth artifacts.
- `gui_font_snapshot` proves deterministic host framebuffer metadata for a small runtime sample; it is not a visual/art approval or hardware validation.

## Generated artifact tiers

| Tier | Artifact | May be checked in? | Required evidence |
| --- | --- | --- | --- |
| Manifest inventory | `generated/xy_gui_font_manifest_generated.h` | Optional after review | `font_manifest.json`, `gui_font_generator_output`, `gui_font_generator_write` |
| Legacy-passthrough preview | `generated/xy_font_*_generated.{h,c}` that references current public legacy handles | Optional after this policy is accepted | `gui_font_generator_glyph_preview`, `gui_font_generator_glyph_write`, `gui_font_generator_glyph_compile` |
| Regenerated bitmap tables | `.c/.h` containing new glyph byte arrays | Not yet | reviewed source font, license/provenance, deterministic rasterizer, snapshot diff, focused compile |
| Full CJK import | large generated CJK bitmap table | Not yet | explicit product/font-scope decision, size budget, license review, snapshot/hardware evidence |

## Check-in rules for the next implementation slice

If the project decides to check in generated preview artifacts, keep the first slice limited to the legacy-passthrough tier:

1. Generate into `components/gui/fonts/generated/` only; do not overwrite the legacy tables.
2. Add a short generated README or header comment stating that the files are `legacy-passthrough preview` artifacts, not new font art.
3. Extend the generator self-test to compare checked-in generated files against freshly emitted bytes; checked-in generated files must be reproducible byte-for-byte.
4. Compile the checked-in generated `.c` files, not only temporary copies.
5. Keep the runtime font engine using the existing reviewed public handles until a separate migration slice explicitly switches consumers.
6. Do not update the GUI hardware validation record beyond `host-snapshot-only` or `compile-only` unless real display evidence exists.

## Recommended path limit for the future code slice

```text
components/gui/fonts/font_manifest.json
components/gui/fonts/tools/generate_bitmap_font.py
components/gui/fonts/generated/README.md
components/gui/fonts/generated/xy_font_*_generated.{h,c}
tests/unit/CMakeLists.txt
docs/design/xinyi-gui-font-generated-preview-checkin-policy-2026-08-11.md
docs/design/unit-test-inventory.md
```

Do not include `MCU/`, `third_party/`, display HAL/vendor drivers, z-serial GUI files, or unrelated GUI widgets/effects.

## Verification anchors

For this proposal-only slice:

```bash
cd build/tests/unit && ctest --output-on-failure -R '^gui_font_generator_(glyph_preview|glyph_write|glyph_compile)$'
make test-unit
git diff --check
```

For a future generated-file check-in slice:

```bash
cmake -B build/tests/unit -S tests/unit
cd build/tests/unit && ctest --output-on-failure -R '^gui_font_generator_(manifest|output|write|glyph_metadata|glyph_preview|glyph_write|glyph_compile)$'
gcc -std=c99 -Wall -Wextra -Werror -Icomponents/gui/fonts -Icomponents/gui/fonts/generated -c components/gui/fonts/generated/<generated>.c -o /tmp/<generated>.o
make test-unit
git diff --check
```

## Non-goals

- No generated glyph-byte source is committed by this proposal.
- No external font package or complete CJK asset is imported.
- No runtime font lookup order changes.
- No Display driver, HAL, MCU, or hardware validation status changes.
