# XinYi bitmap font generator placeholder

The current XinYi GUI font assets are legacy checked-in bitmap tables. The
manifest smoke test validates their declared range/count/placeholder inventory,
but the deterministic generator is intentionally not implemented yet.

Future implementation requirements:

1. Read `components/gui/fonts/font_manifest.json`.
2. Validate source font provenance/license before generating outputs.
3. Emit deterministic `.c/.h` bitmap tables with explicit glyph order.
4. Reject duplicate codepoints unless the manifest allows and explains them.
5. Report placeholder glyph counts separately from generated real glyphs.

Do not bulk-import full CJK font data or claim hardware rendering validation as
part of the generator bootstrap slice.
