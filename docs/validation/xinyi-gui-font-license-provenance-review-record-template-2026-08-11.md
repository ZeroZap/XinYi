# XinYi GUI Font License/Provenance Review Record Template (2026-08-11)

status: pending

## Scope

This template is the required evidence record before any current GUI bitmap font entry in
`components/gui/fonts/font_manifest.json` can move away from `project-review-pending`.

Current pending font ids:

- `ascii_8x16`
- `ascii_16x24`
- `chinese_16x16_ui_legacy`

## Review evidence to fill before approval

For each font id, record:

1. Source identification: author/import origin, source file(s), commit/import history if known, and whether bytes were modified after import.
2. License/ownership: SPDX license or internal ownership statement, plus redistribution/static-embedding/commercial-shipment compatibility.
3. Derivative scope: whether the conclusion covers only the legacy checked-in table or also a future regenerated asset set.
4. Placeholder/duplicate handling: explicit decision for duplicate Chinese codepoints and placeholder bitmap rows.
5. Reviewer/date: reviewer name, review date, remaining blockers, and final conclusion.

## Approval gate

Do not mark any font as approved in `font_manifest.json` until this record (or a dated replacement record under `docs/validation/`) contains completed evidence for that font id.

Host generator previews, checked-in legacy-passthrough generated files, and framebuffer snapshot checksums are not license/provenance evidence and must not be used as approval substitutes.
