# XinYi GUI Font License/Provenance Review Plan (2026-08-11)

## Slice

Define the evidence boundary for reviewing the currently bundled GUI bitmap font assets before any generated bitmap tables or larger CJK imports are allowed.

This is a documentation/control-plane slice only. It does not change runtime font lookup, does not import external fonts, does not rewrite bitmap bytes, and does not claim visual or hardware validation.

## Current baseline

- `components/gui/fonts/font_manifest.json` records three legacy project asset groups: ASCII 8x16, ASCII 16x24, and Chinese 16x16 UI bitmap tables.
- The manifest intentionally keeps `license` / `source_license` as `project-review-pending` for all current assets.
- `components/gui/fonts/tools/generate_bitmap_font.py` enforces that pending status in its manifest/glyph metadata self-tests, so generated preview work cannot silently convert pending provenance into approved provenance.
- `components/gui/fonts/generated/*` is only a legacy-passthrough generated preview tier; it references existing public handles and is not new licensed glyph art.
- `gui_font_snapshot` provides deterministic host framebuffer/checksum evidence, but it is not a legal/provenance review, art approval, or real display evidence.

## Review checklist before changing license status

For each font entry in `font_manifest.json`, keep `project-review-pending` until all applicable items are recorded in a follow-up review note:

1. **Source identification**
   - Identify whether the bitmap bytes were hand-authored, imported from an upstream font, generated from a known source font, or inherited from an earlier internal project snapshot.
   - Record the exact source file(s), commit/import origin if known, and whether the table was modified after import.
2. **License compatibility**
   - Record the upstream license or internal ownership statement.
   - Confirm the license allows firmware redistribution, generated derivatives, static linking/embedding, and commercial product shipment if applicable.
3. **Coverage and derivative scope**
   - Distinguish reviewed legacy ASCII/Chinese UI tables from any future regenerated bitmap table or larger CJK import.
   - Do not allow an approved legacy table to automatically approve new generated glyph art from a different source font.
4. **Placeholder/duplicate handling**
   - Keep the current Chinese duplicate-codepoint and placeholder-bitmap flags visible until a licensed replacement set is generated and reviewed.
   - Treat placeholder rows as asset quality debt, not a licensing shortcut.
5. **Evidence storage**
   - Store review notes under `docs/validation/` or `docs/design/` with date, reviewer, reviewed font ids, license conclusion, and remaining blockers.
   - Only after that evidence exists may a later slice update `font_manifest.json` from `project-review-pending` to a concrete SPDX-style status or internal ownership tag.

## Allowed next implementation slices

Low-risk follow-ups that keep the current safety boundary:

1. Add a machine-readable `review_status` block to `font_manifest.json` while keeping all licenses pending.
2. Add a `gui_font_license_manifest` host smoke test that fails if pending licenses are accidentally rewritten without a review note reference.
3. Add a dated validation record template under `docs/validation/` for manual asset review.
4. Add host framebuffer snapshot review docs that compare rendered ASCII-art/checksum output; keep it separate from license approval.

## Disallowed in this phase

- Do not bulk-import a full CJK font or external font package.
- Do not rewrite legacy bitmap tables from a rasterizer output.
- Do not change runtime font lookup to generated preview files.
- Do not mark any font as license-approved without review evidence.
- Do not update hardware validation records beyond pending/host-snapshot-only without real display photos/logs.

## Verification anchors

For this proposal/documentation slice:

```bash
make test-unit

git diff --check
```

For a later manifest/test slice:

```bash
cmake -B build/tests/unit -S tests/unit
cd build/tests/unit && ctest --output-on-failure -R '^gui_font_(manifest|generator_glyph_metadata|generator_checked_in_preview)$'
make test-unit
git diff --check
```
