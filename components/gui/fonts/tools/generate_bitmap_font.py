#!/usr/bin/env python3
"""Deterministic XinYi bitmap-font manifest validator/generator bootstrap.

This bootstrap validates the current checked-in font manifest and can emit the
manifest-inventory generated header.  It does not import external font files or
rewrite the legacy C bitmap tables.  Future glyph-table modes should preserve
the same strict manifest checks before emitting regenerated bitmap data.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
import sys
from pathlib import Path
from typing import Any, cast


REQUIRED_FONT_FIELDS = {
    "id",
    "family",
    "provenance",
    "license",
    "pixel_size",
    "encoding",
    "range",
    "bytes_per_glyph",
    "source_files",
    "source_sha256",
    "output_header",
    "output_source",
    "source_font",
    "source_license",
    "generator_mode",
    "glyph_order",
    "public_handle",
}

SUPPORTED_GENERATOR_MODES = {"legacy-inventory", "legacy-passthrough", "font-rasterize"}
SUPPORTED_GLYPH_ORDERS = {"ascii-range", "explicit-codepoints"}
REPLACEMENT_CANDIDATE_FIELDS = {
    "id", "family", "style", "source_collection_index", "source_filename",
    "source_sha256", "source_url", "license", "license_file", "pixel_size",
    "render_mode", "output_snapshot", "status",
}


class ManifestError(ValueError):
    """Raised when the font manifest is structurally invalid."""


def _parse_hex(value: str, field_name: str) -> int:
    if not isinstance(value, str) or not value.startswith("0x"):
        raise ManifestError(f"{field_name} must be a hex string such as 0x20")
    try:
        return int(value, 16)
    except ValueError as exc:
        raise ManifestError(f"{field_name} is not valid hexadecimal: {value}") from exc


def _require(condition: bool, message: str) -> None:
    if not condition:
        raise ManifestError(message)


def _validate_pixel_size(font: dict[str, Any]) -> None:
    pixel_size = font.get("pixel_size")
    _require(isinstance(pixel_size, dict), f"{font.get('id')}: pixel_size must be an object")
    pixel_size = cast(dict[str, Any], pixel_size)
    for key in ("width", "height"):
        value = pixel_size.get(key)
        _require(isinstance(value, int) and value > 0, f"{font.get('id')}: pixel_size.{key} must be a positive integer")

    bytes_per_glyph = font.get("bytes_per_glyph")
    expected_min = (pixel_size["width"] * pixel_size["height"] + 7) // 8
    _require(isinstance(bytes_per_glyph, int) and bytes_per_glyph >= expected_min,
             f"{font.get('id')}: bytes_per_glyph must cover width*height bits")


def _validate_range(font: dict[str, Any]) -> None:
    font_id = font.get("id")
    range_spec = font.get("range")
    _require(isinstance(range_spec, dict), f"{font_id}: range must be an object")
    range_spec = cast(dict[str, Any], range_spec)

    if "first" in range_spec or "last" in range_spec:
        first = _parse_hex(range_spec.get("first"), f"{font_id}: range.first")
        last = _parse_hex(range_spec.get("last"), f"{font_id}: range.last")
        count = range_spec.get("count")
        _require(isinstance(count, int) and count > 0, f"{font_id}: range.count must be positive")
        _require(last >= first, f"{font_id}: range.last must be >= range.first")
        _require(last - first + 1 == count,
                 f"{font_id}: range count {count} does not match {first:#x}..{last:#x}")
    elif "explicit_codepoints" in range_spec:
        count = range_spec.get("count")
        _require(isinstance(range_spec.get("explicit_codepoints"), str) and range_spec["explicit_codepoints"],
                 f"{font_id}: explicit_codepoints must name a public table accessor")
        _require(isinstance(count, int) and count > 0, f"{font_id}: range.count must be positive")
    else:
        raise ManifestError(f"{font_id}: range must declare first/last or explicit_codepoints")


def _validate_required_codepoints(font: dict[str, Any]) -> None:
    codepoints = font.get("required_ui_codepoints", [])
    _require(isinstance(codepoints, list), f"{font.get('id')}: required_ui_codepoints must be a list")
    seen: set[int] = set()
    for index, item in enumerate(codepoints):
        value = _parse_hex(item, f"{font.get('id')}: required_ui_codepoints[{index}]")
        _require(value not in seen, f"{font.get('id')}: duplicate required UI codepoint {item}")
        seen.add(value)


def _validate_source_files(fonts_root: Path, font: dict[str, Any]) -> None:
    source_files = font.get("source_files")
    _require(isinstance(source_files, list) and len(source_files) > 0,
             f"{font.get('id')}: source_files must be a non-empty list")
    source_files = cast(list[Any], source_files)
    source_sha256 = font.get("source_sha256")
    _require(isinstance(source_sha256, dict),
             f"{font.get('id')}: source_sha256 must map every source file to a digest")
    source_sha256 = cast(dict[str, Any], source_sha256)
    _require(set(source_sha256) == set(source_files),
             f"{font.get('id')}: source_sha256 keys must match source_files")
    for item in source_files:
        _require(isinstance(item, str) and len(item) > 0,
                 f"{font.get('id')}: source_files entries must be strings")
        source_path = fonts_root / item
        _require(source_path.exists(), f"{font.get('id')}: missing source file {source_path}")
        expected_digest = source_sha256[item]
        _require(isinstance(expected_digest, str) and re.fullmatch(r"[0-9a-f]{64}", expected_digest) is not None,
                 f"{font.get('id')}: source_sha256[{item}] must be a lowercase SHA-256 digest")
        actual_digest = hashlib.sha256(source_path.read_bytes()).hexdigest()
        _require(actual_digest == expected_digest,
                 f"{font.get('id')}: source file digest drifted for {item}")


def _validate_generation_metadata(font: dict[str, Any]) -> None:
    font_id = font.get("id")

    for key in ("output_header", "output_source"):
        value = font.get(key)
        _require(isinstance(value, str) and value.endswith((".h", ".c")),
                 f"{font_id}: {key} must be a relative generated .h/.c path")
        value_str = cast(str, value)
        _require(not Path(value_str).is_absolute(), f"{font_id}: {key} must be relative")
        _require(".." not in Path(value_str).parts, f"{font_id}: {key} must not escape the fonts directory")
        _require(value_str.startswith("generated/"), f"{font_id}: {key} must be under generated/")

    _require(cast(str, font["output_header"]).endswith(".h"),
             f"{font_id}: output_header must end with .h")
    _require(cast(str, font["output_source"]).endswith(".c"),
             f"{font_id}: output_source must end with .c")
    _require(cast(str, font["output_header"]) != cast(str, font["output_source"]),
             f"{font_id}: output_header and output_source must differ")

    source_font = font.get("source_font")
    source_license = font.get("source_license")
    generator_mode = font.get("generator_mode")
    glyph_order = font.get("glyph_order")
    _require(isinstance(source_font, str) and bool(source_font),
             f"{font_id}: source_font must declare provenance or legacy-table")
    _require(isinstance(source_license, str) and bool(source_license),
             f"{font_id}: source_license must declare SPDX/provenance status")
    _require(generator_mode in SUPPORTED_GENERATOR_MODES,
             f"{font_id}: unsupported generator_mode {generator_mode!r}")
    _require(glyph_order in SUPPORTED_GLYPH_ORDERS,
             f"{font_id}: unsupported glyph_order {glyph_order!r}")
    if generator_mode == "legacy-passthrough":
        _require(source_font == "legacy-table",
                 f"{font_id}: legacy-passthrough requires source_font=legacy-table")
    range_spec = cast(dict[str, Any], font["range"])
    if glyph_order == "ascii-range":
        _require("first" in range_spec, f"{font_id}: ascii-range glyph_order requires first/last range")
    if glyph_order == "explicit-codepoints":
        _require("explicit_codepoints" in range_spec,
                 f"{font_id}: explicit-codepoints glyph_order requires explicit_codepoints range")


def validate_manifest(manifest_path: Path) -> dict[str, Any]:
    manifest_path = manifest_path.resolve()
    fonts_root = manifest_path.parent
    data = json.loads(manifest_path.read_text(encoding="utf-8"))

    _require(data.get("schema_version") == 1, "schema_version must be 1")
    _require(data.get("component") == "xinyi-gui-fonts", "component must be xinyi-gui-fonts")

    review_status = data.get("review_status")
    _require(isinstance(review_status, dict), "review_status must be an object")
    review_status = cast(dict[str, Any], review_status)
    _require(review_status.get("state") == "project-review-pending",
             "review_status.state must remain project-review-pending until review evidence exists")
    review_note = review_status.get("review_note")
    _require(isinstance(review_note, str) and review_note.startswith("docs/validation/")
             and review_note.endswith(".md"),
             "review_status.review_note must point to a docs/validation/*.md record")
    _require(isinstance(review_status.get("policy"), str) and bool(review_status["policy"]),
             "review_status.policy must describe the license/provenance review gate")
    _require(isinstance(review_status.get("reviewed_font_ids"), list),
             "review_status.reviewed_font_ids must be a list")
    _require(isinstance(review_status.get("pending_font_ids"), list),
             "review_status.pending_font_ids must be a list")

    fonts = data.get("fonts")
    _require(isinstance(fonts, list) and fonts, "fonts must be a non-empty list")

    seen_ids: set[str] = set()
    for font in fonts:
        _require(isinstance(font, dict), "each font entry must be an object")
        missing = REQUIRED_FONT_FIELDS - set(font)
        _require(not missing, f"{font.get('id', '<missing-id>')}: missing fields: {sorted(missing)}")
        font_id = font["id"]
        _require(isinstance(font_id, str) and len(font_id) > 0,
                 "font id must be a non-empty string")
        font_id = cast(str, font_id)
        _require(font_id not in seen_ids, f"duplicate font id: {font_id}")
        seen_ids.add(font_id)

        _validate_pixel_size(font)
        _validate_range(font)
        _validate_required_codepoints(font)
        _validate_source_files(fonts_root, font)
        _validate_generation_metadata(font)

        if "known_inventory_flags" in font:
            flags = font["known_inventory_flags"]
            _require(isinstance(flags, dict), f"{font_id}: known_inventory_flags must be an object")
            for key, value in flags.items():
                if key.endswith("_allowed"):
                    _require(isinstance(value, bool), f"{font_id}: {key} must be boolean")
                if key.endswith("_reason"):
                    _require(isinstance(value, str) and value, f"{font_id}: {key} must be non-empty")

    pending_ids = review_status["pending_font_ids"]
    reviewed_ids = review_status["reviewed_font_ids"]
    _require(all(isinstance(item, str) for item in pending_ids),
             "review_status.pending_font_ids entries must be strings")
    _require(all(isinstance(item, str) for item in reviewed_ids),
             "review_status.reviewed_font_ids entries must be strings")
    _require(set(pending_ids) == seen_ids,
             "review_status.pending_font_ids must list every current font while review is pending")
    _require(not reviewed_ids,
             "review_status.reviewed_font_ids must stay empty until a review record approves fonts")

    snapshot_status = data.get("snapshot_review_status")
    _require(isinstance(snapshot_status, dict), "snapshot_review_status must be an object")
    snapshot_status = cast(dict[str, Any], snapshot_status)
    _require(snapshot_status.get("state") == "host-snapshot-review-pending",
             "snapshot_review_status.state must remain host-snapshot-review-pending until reviewed artifacts exist")
    snapshot_note = snapshot_status.get("review_note")
    _require(isinstance(snapshot_note, str) and snapshot_note.startswith("docs/validation/")
             and snapshot_note.endswith(".md"),
             "snapshot_review_status.review_note must point to a docs/validation/*.md record")
    _require(isinstance(snapshot_status.get("policy"), str) and bool(snapshot_status["policy"]),
             "snapshot_review_status.policy must describe the host snapshot review gate")
    _require(snapshot_status.get("evidence_level") == "host-snapshot-only-pending",
             "snapshot_review_status.evidence_level must remain host-snapshot-only-pending")
    snapshot_artifacts = snapshot_status.get("host_snapshot_artifacts")
    _require(isinstance(snapshot_artifacts, list) and len(snapshot_artifacts) > 0,
             "snapshot_review_status.host_snapshot_artifacts must list host snapshot artifacts")
    snapshot_artifacts = cast(list[Any], snapshot_artifacts)
    for artifact in snapshot_artifacts:
        _require(isinstance(artifact, str) and artifact.startswith("docs/validation/")
                 and artifact.endswith(".md"),
                 "snapshot_review_status.host_snapshot_artifacts entries must be docs/validation/*.md")
        artifact_path = (manifest_path.parents[3] / cast(str, artifact)).resolve()
        _require(artifact_path.exists(), f"font host snapshot artifact is missing: {artifact_path}")
    technical_reviews = snapshot_status.get("technical_review_records", [])
    _require(isinstance(technical_reviews, list),
             "snapshot_review_status.technical_review_records must be a list")
    for review in cast(list[Any], technical_reviews):
        _require(isinstance(review, str) and review.startswith("docs/validation/")
                 and review.endswith(".md"),
                 "snapshot_review_status.technical_review_records entries must be docs/validation/*.md")
        review_path = (manifest_path.parents[3] / cast(str, review)).resolve()
        _require(review_path.exists(), f"font host technical review record is missing: {review_path}")
    snapshot_pending_ids = snapshot_status.get("pending_font_ids")
    snapshot_reviewed_ids = snapshot_status.get("reviewed_font_ids")
    _require(isinstance(snapshot_pending_ids, list),
             "snapshot_review_status.pending_font_ids must be a list")
    _require(isinstance(snapshot_reviewed_ids, list),
             "snapshot_review_status.reviewed_font_ids must be a list")
    snapshot_pending_ids = cast(list[Any], snapshot_pending_ids)
    snapshot_reviewed_ids = cast(list[Any], snapshot_reviewed_ids)
    _require(all(isinstance(item, str) for item in snapshot_pending_ids),
             "snapshot_review_status.pending_font_ids entries must be strings")
    _require(all(isinstance(item, str) for item in snapshot_reviewed_ids),
             "snapshot_review_status.reviewed_font_ids entries must be strings")
    _require(set(snapshot_pending_ids) == seen_ids,
             "snapshot_review_status.pending_font_ids must list every current font while review is pending")
    _require(not snapshot_reviewed_ids,
             "snapshot_review_status.reviewed_font_ids must stay empty until snapshot review evidence exists")

    generation_plan = data.get("generation_plan")
    _require(isinstance(generation_plan, dict), "generation_plan must be an object")
    generator_rel = generation_plan.get("generator")
    _require(isinstance(generator_rel, str) and generator_rel.endswith("generate_bitmap_font.py"),
             "generation_plan.generator must point to generate_bitmap_font.py")
    generator_rel = cast(str, generator_rel)
    generator_path = (manifest_path.parents[3] / generator_rel).resolve()
    _require(generator_path.exists(), f"generation_plan.generator path does not exist: {generator_path}")

    candidate = data.get("replacement_candidate")
    _require(isinstance(candidate, dict), "replacement_candidate must be an object")
    candidate = cast(dict[str, Any], candidate)
    _require(REPLACEMENT_CANDIDATE_FIELDS <= set(candidate),
             "replacement_candidate is missing required provenance fields")
    _require(candidate["license"] == "OFL-1.1",
             "replacement_candidate.license must be OFL-1.1")
    _require(candidate["status"] == "generated-host-candidate-not-active",
             "replacement_candidate must remain explicitly non-active")
    _require(candidate["render_mode"] == "Pillow-1bit-centered-bbox",
             "replacement_candidate.render_mode is unsupported")
    _require(isinstance(candidate["source_collection_index"], int)
             and candidate["source_collection_index"] >= 0,
             "replacement_candidate.source_collection_index must be a non-negative integer")
    _require(isinstance(candidate["pixel_size"], int) and candidate["pixel_size"] > 0,
             "replacement_candidate.pixel_size must be a positive integer")
    _require(re.fullmatch(r"[0-9a-f]{64}", candidate["source_sha256"]) is not None,
             "replacement_candidate.source_sha256 must be a lowercase SHA-256 digest")
    license_path = fonts_root / cast(str, candidate["license_file"])
    _require(license_path.exists(), f"replacement candidate license is missing: {license_path}")
    _require("SIL OPEN FONT LICENSE Version 1.1" in license_path.read_text(encoding="utf-8"),
             "replacement candidate license file does not contain OFL-1.1")
    snapshot_rel = Path(cast(str, candidate["output_snapshot"]))
    _require(not snapshot_rel.is_absolute() and ".." not in snapshot_rel.parts
             and cast(str, candidate["output_snapshot"]).startswith("generated/"),
             "replacement_candidate.output_snapshot must stay under generated/")

    return data


def build_summary(data: dict[str, Any]) -> str:
    lines = [
        f"component={data['component']}",
        f"schema_version={data['schema_version']}",
        f"font_count={len(data['fonts'])}",
    ]
    for font in data["fonts"]:
        range_spec = font["range"]
        if "first" in range_spec:
            coverage = f"{range_spec['first']}..{range_spec['last']} ({range_spec['count']})"
        else:
            coverage = f"{range_spec['explicit_codepoints']} ({range_spec['count']})"
        lines.append(
            f"font={font['id']} size={font['pixel_size']['width']}x{font['pixel_size']['height']} "
            f"bytes_per_glyph={font['bytes_per_glyph']} coverage={coverage}"
        )
    lines.append(f"generation_state={data['generation_plan'].get('state', '<missing>')}")
    return "\n".join(lines)


def _macro_name(font_id: str) -> str:
    macro = re.sub(r"[^A-Za-z0-9]+", "_", font_id).strip("_").upper()
    if not macro:
        raise ManifestError("font id does not produce a usable macro name")
    if macro[0].isdigit():
        macro = f"FONT_{macro}"
    return macro


def _find_font(data: dict[str, Any], font_id: str) -> dict[str, Any]:
    for font in data["fonts"]:
        if font["id"] == font_id:
            return cast(dict[str, Any], font)
    raise ManifestError(f"unknown font id: {font_id}")


def build_manifest_header(data: dict[str, Any]) -> str:
    """Build deterministic generated-header preview from the manifest.

    This output intentionally contains manifest inventory constants only. It is
    safe for host review and tests because it does not rewrite checked-in bitmap
    tables or claim generated glyph art exists yet.
    """

    lines = [
        "/* Auto-generated from components/gui/fonts/font_manifest.json; do not edit. */",
        "#ifndef XY_GUI_FONT_MANIFEST_GENERATED_H",
        "#define XY_GUI_FONT_MANIFEST_GENERATED_H",
        "",
        f"#define XY_GUI_FONT_MANIFEST_SCHEMA_VERSION {data['schema_version']}U",
        f"#define XY_GUI_FONT_ASSET_COUNT {len(data['fonts'])}U",
        "",
    ]

    for index, font in enumerate(data["fonts"]):
        macro = _macro_name(cast(str, font["id"]))
        pixel_size = cast(dict[str, Any], font["pixel_size"])
        range_spec = cast(dict[str, Any], font["range"])
        lines.extend([
            f"#define XY_GUI_FONT_{macro}_INDEX {index}U",
            f"#define XY_GUI_FONT_{macro}_WIDTH {pixel_size['width']}U",
            f"#define XY_GUI_FONT_{macro}_HEIGHT {pixel_size['height']}U",
            f"#define XY_GUI_FONT_{macro}_BYTES_PER_GLYPH {font['bytes_per_glyph']}U",
            f"#define XY_GUI_FONT_{macro}_GLYPH_COUNT {range_spec['count']}U",
        ])
        if "first" in range_spec:
            first = _parse_hex(cast(str, range_spec["first"]), f"{font['id']}: range.first")
            last = _parse_hex(cast(str, range_spec["last"]), f"{font['id']}: range.last")
            lines.extend([
                f"#define XY_GUI_FONT_{macro}_FIRST_CODEPOINT 0x{first:X}U",
                f"#define XY_GUI_FONT_{macro}_LAST_CODEPOINT 0x{last:X}U",
            ])
        if "required_ui_codepoints" in font:
            codepoints = cast(list[Any], font.get("required_ui_codepoints", []))
            lines.append(f"#define XY_GUI_FONT_{macro}_REQUIRED_UI_CODEPOINT_COUNT {len(codepoints)}U")

        lines.append("")

    lines.extend([
        "#endif /* XY_GUI_FONT_MANIFEST_GENERATED_H */",
        "",
    ])
    return "\n".join(lines)


def build_glyph_header(data: dict[str, Any], font_id: str) -> str:
    """Build a deterministic legacy-passthrough glyph header preview.

    This output is still a preview: it references the current reviewed public
    legacy font handles and constants instead of emitting regenerated bitmap
    bytes. That keeps the generator path reviewable before any generated glyph
    table is checked in.
    """

    font = _find_font(data, font_id)
    _require(font["generator_mode"] == "legacy-passthrough",
             f"{font_id}: glyph header preview only supports legacy-passthrough")
    macro = _macro_name(font_id)
    pixel_size = cast(dict[str, Any], font["pixel_size"])
    range_spec = cast(dict[str, Any], font["range"])
    include_name = Path(cast(list[Any], font["source_files"])[-1]).name
    public_handle = cast(str, font["public_handle"])

    lines = [
        f"/* Auto-generated preview for {font_id}; do not edit. */",
        f"#ifndef XY_GUI_FONT_{macro}_GENERATED_H",
        f"#define XY_GUI_FONT_{macro}_GENERATED_H",
        "",
        f"#include \"{include_name}\"",
        "",
        f"#define XY_GUI_FONT_{macro}_GENERATED_WIDTH {pixel_size['width']}U",
        f"#define XY_GUI_FONT_{macro}_GENERATED_HEIGHT {pixel_size['height']}U",
        f"#define XY_GUI_FONT_{macro}_GENERATED_BYTES_PER_GLYPH {font['bytes_per_glyph']}U",
        f"#define XY_GUI_FONT_{macro}_GENERATED_GLYPH_COUNT {range_spec['count']}U",
    ]
    if "first" in range_spec:
        first = _parse_hex(cast(str, range_spec["first"]), f"{font_id}: range.first")
        last = _parse_hex(cast(str, range_spec["last"]), f"{font_id}: range.last")
        lines.extend([
            f"#define XY_GUI_FONT_{macro}_GENERATED_FIRST_CODEPOINT 0x{first:X}U",
            f"#define XY_GUI_FONT_{macro}_GENERATED_LAST_CODEPOINT 0x{last:X}U",
        ])

    lines.extend([
        "",
        f"#define XY_GUI_FONT_{macro}_GENERATED_HANDLE() {public_handle}()",
    ])
    if font["glyph_order"] == "explicit-codepoints":
        lines.append(f"#define XY_GUI_FONT_{macro}_GENERATED_CODEPOINTS() {range_spec['explicit_codepoints']}()")

    lines.extend([
        "",
        f"#endif /* XY_GUI_FONT_{macro}_GENERATED_H */",
        "",
    ])
    return "\n".join(lines)


def build_glyph_source(data: dict[str, Any], font_id: str, output_header_name: str | None = None) -> str:
    """Build a deterministic legacy-passthrough glyph source preview."""

    font = _find_font(data, font_id)
    _require(font["generator_mode"] == "legacy-passthrough",
             f"{font_id}: glyph source preview only supports legacy-passthrough")
    macro = _macro_name(font_id)
    output_header = output_header_name or Path(cast(str, font["output_header"])).name
    public_handle = cast(str, font["public_handle"])
    data_symbol = f"g_xy_gui_font_{macro.lower()}_generated_preview"

    lines = [
        f"/* Auto-generated preview for {font_id}; do not edit. */",
        f"#include \"{output_header}\"",
        "",
        "#include <stdint.h>",
        "",
        f"const uint8_t {data_symbol}[] = {{",
        f"    XY_GUI_FONT_{macro}_GENERATED_WIDTH,",
        f"    XY_GUI_FONT_{macro}_GENERATED_HEIGHT,",
        f"    XY_GUI_FONT_{macro}_GENERATED_BYTES_PER_GLYPH,",
        f"    XY_GUI_FONT_{macro}_GENERATED_GLYPH_COUNT,",
        "};",
        "",
        f"const void *xy_gui_font_{macro.lower()}_generated_legacy_handle(void)",
        "{",
        f"    return (const void *){public_handle}();",
        "}",
        "",
    ]
    return "\n".join(lines)


def self_test_generated_output(data: dict[str, Any]) -> None:
    header_once = build_manifest_header(data)
    header_twice = build_manifest_header(data)
    _require(header_once == header_twice, "generated manifest header is not deterministic")
    _require("#define XY_GUI_FONT_ASSET_COUNT 3U" in header_once,
             "generated manifest header is missing font asset count")
    _require("#define XY_GUI_FONT_ASCII_8X16_WIDTH 8U" in header_once,
             "generated manifest header is missing ASCII 8x16 width")
    _require("#define XY_GUI_FONT_ASCII_16X24_BYTES_PER_GLYPH 48U" in header_once,
             "generated manifest header is missing ASCII 16x24 byte count")
    _require("#define XY_GUI_FONT_CHINESE_16X16_UI_LEGACY_GLYPH_COUNT 168U" in header_once,
             "generated manifest header is missing Chinese legacy glyph count")
    _require("#define XY_GUI_FONT_CHINESE_16X16_UI_LEGACY_REQUIRED_UI_CODEPOINT_COUNT 15U"
             in header_once,
             "generated manifest header is missing required UI codepoint count")
    _require("\t" not in header_once, "generated manifest header must not contain tabs")


def write_manifest_header(data: dict[str, Any], output_path: Path) -> None:
    """Write the deterministic manifest-inventory header to ``output_path``."""

    output_path = output_path.resolve()
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(build_manifest_header(data), encoding="utf-8")


def write_glyph_preview(data: dict[str, Any], font_id: str, header_path: Path, source_path: Path) -> None:
    """Write deterministic legacy-passthrough glyph header/source previews."""

    header_output = header_path.resolve()
    source_output = source_path.resolve()
    _require(header_output != source_output, "glyph header/source output paths must differ")
    header_output.parent.mkdir(parents=True, exist_ok=True)
    source_output.parent.mkdir(parents=True, exist_ok=True)
    header_output.write_text(build_glyph_header(data, font_id), encoding="utf-8")
    source_output.write_text(build_glyph_source(data, font_id, header_output.name), encoding="utf-8")


def self_test_written_output(data: dict[str, Any]) -> None:
    """Validate that the write path emits the same deterministic header preview."""

    import tempfile

    with tempfile.TemporaryDirectory(prefix="xinyi-font-manifest-") as tmpdir:
        output_path = Path(tmpdir) / "generated" / "xy_gui_font_manifest_generated.h"
        write_manifest_header(data, output_path)
        written_header = output_path.read_text(encoding="utf-8")
    _require(written_header == build_manifest_header(data),
             "written manifest header differs from emitted preview")


def self_test_glyph_write(data: dict[str, Any]) -> None:
    """Validate that glyph preview write mode emits the same bytes as preview mode."""

    import tempfile

    with tempfile.TemporaryDirectory(prefix="xinyi-font-glyph-") as tmpdir:
        output_root = Path(tmpdir) / "generated"
        for font in data["fonts"]:
            font_id = cast(str, font["id"])
            header_path = output_root / f"{font_id}.h"
            source_path = output_root / f"{font_id}.c"
            write_glyph_preview(data, font_id, header_path, source_path)
            _require(header_path.read_text(encoding="utf-8") == build_glyph_header(data, font_id),
                     f"{font_id}: written glyph header differs from preview")
            written_source = source_path.read_text(encoding="utf-8")
            _require(written_source == build_glyph_source(data, font_id, header_path.name),
                     f"{font_id}: written glyph source differs from preview")
            _require(f'#include "{header_path.name}"' in written_source,
                     f"{font_id}: glyph source does not include the requested header basename")

    try:
        with tempfile.TemporaryDirectory(prefix="xinyi-font-glyph-negative-") as tmpdir:
            same_path = Path(tmpdir) / "same_output.h"
            write_glyph_preview(data, "ascii_8x16", same_path, same_path)
    except ManifestError as exc:
        _require("must differ" in str(exc), "same-path glyph write probe failed for the wrong reason")
    else:
        raise ManifestError("same-path glyph write probe was accepted")


def self_test_glyph_compile(data: dict[str, Any], manifest_path: Path) -> None:
    """Validate that written glyph previews are C99-compilable host artifacts."""

    import tempfile

    fonts_root = manifest_path.resolve().parent
    with tempfile.TemporaryDirectory(prefix="xinyi-font-glyph-compile-") as tmpdir:
        output_root = Path(tmpdir) / "generated"
        for font in data["fonts"]:
            font_id = cast(str, font["id"])
            header_path = Path(tmpdir) / cast(str, font["output_header"])
            source_path = Path(tmpdir) / cast(str, font["output_source"])
            write_glyph_preview(data, font_id, header_path, source_path)
            result = subprocess.run(
                [
                    "gcc",
                    "-std=c99",
                    "-Wall",
                    "-Wextra",
                    "-Werror",
                    "-I",
                    str(output_root),
                    "-I",
                    str(fonts_root),
                    "-c",
                    str(source_path),
                    "-o",
                    str(output_root / f"{font_id}.o"),
                ],
                check=False,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
            )
            _require(result.returncode == 0,
                     f"{font_id}: glyph preview compile failed: {result.stderr.strip()}")


def self_test_checked_in_preview(data: dict[str, Any], manifest_path: Path) -> None:
    """Validate that checked-in generated previews are reproducible and buildable."""

    fonts_root = manifest_path.resolve().parent
    generated_root = fonts_root / "generated"
    readme_path = generated_root / "README.md"
    _require(readme_path.exists(), "generated preview README is missing")
    readme_text = readme_path.read_text(encoding="utf-8")
    _require("legacy-passthrough preview" in readme_text,
             "generated README must declare the legacy-passthrough preview tier")
    _require("do not prove display hardware rendering" in readme_text,
             "generated README must keep hardware validation out of scope")

    manifest_header_path = generated_root / "xy_gui_font_manifest_generated.h"
    _require(manifest_header_path.exists(), "checked-in manifest generated header is missing")
    _require(manifest_header_path.read_text(encoding="utf-8") == build_manifest_header(data),
             "checked-in manifest generated header differs from generator output")

    for font in data["fonts"]:
        font_id = cast(str, font["id"])
        header_path = fonts_root / cast(str, font["output_header"])
        source_path = fonts_root / cast(str, font["output_source"])
        _require(header_path.exists(), f"{font_id}: checked-in generated header is missing")
        _require(source_path.exists(), f"{font_id}: checked-in generated source is missing")
        _require(header_path.read_text(encoding="utf-8") == build_glyph_header(data, font_id),
                 f"{font_id}: checked-in generated header differs from generator output")
        expected_source = build_glyph_source(data, font_id, header_path.name)
        _require(source_path.read_text(encoding="utf-8") == expected_source,
                 f"{font_id}: checked-in generated source differs from generator output")

        result = subprocess.run(
            [
                "gcc",
                "-std=c99",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-I",
                str(generated_root),
                "-I",
                str(fonts_root),
                "-c",
                str(source_path),
                "-o",
                "/tmp/xinyi-font-checked-in-preview.o",
            ],
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        _require(result.returncode == 0,
                 f"{font_id}: checked-in generated source compile failed: {result.stderr.strip()}")


def self_test_license_manifest(data: dict[str, Any], manifest_path: Path) -> None:
    """Validate that font provenance remains pending without review evidence."""

    review_status = cast(dict[str, Any], data["review_status"])
    review_note = cast(str, review_status["review_note"])
    review_note_path = (manifest_path.resolve().parents[3] / review_note).resolve()
    _require(review_note_path.exists(), f"font license/provenance review note is missing: {review_note_path}")
    review_note_text = review_note_path.read_text(encoding="utf-8")
    _require("status: pending" in review_note_text,
             "font license/provenance review note must keep status pending")
    _require("Do not mark any font as approved" in review_note_text,
             "font license/provenance review note must preserve the approval gate")

    pending_ids = set(cast(list[str], review_status["pending_font_ids"]))
    _require(len(pending_ids) == len(data["fonts"]),
             "font license review pending list must cover every current font")
    for font in data["fonts"]:
        font_id = cast(str, font["id"])
        _require(font_id in pending_ids, f"{font_id}: missing from pending font review list")
        _require(font["license"] == "project-review-pending",
                 f"{font_id}: font license changed before review evidence was recorded")
        _require(font["source_license"] == "project-review-pending",
                 f"{font_id}: source license changed before review evidence was recorded")
        _require(font["provenance"] == "legacy-project-asset",
                 f"{font_id}: provenance changed before source review was recorded")


def self_test_snapshot_review_manifest(data: dict[str, Any], manifest_path: Path) -> None:
    """Validate that host snapshot review remains a separate pending evidence tier."""

    snapshot_status = cast(dict[str, Any], data["snapshot_review_status"])
    review_note = cast(str, snapshot_status["review_note"])
    review_note_path = (manifest_path.resolve().parents[3] / review_note).resolve()
    _require(review_note_path.exists(), f"font host snapshot review note is missing: {review_note_path}")
    review_note_text = review_note_path.read_text(encoding="utf-8")
    _require("status: pending" in review_note_text,
             "font host snapshot review note must keep status pending")
    _require("host-snapshot-only" in review_note_text,
             "font host snapshot review note must declare host-snapshot-only scope")
    _require("not a license/provenance approval" in review_note_text,
             "font host snapshot review note must not approve license/provenance")
    _require("not real LCD/OLED/LED-matrix hardware validation" in review_note_text,
             "font host snapshot review note must not approve hardware validation")
    artifacts = cast(list[str], snapshot_status["host_snapshot_artifacts"])
    _require(len(artifacts) == 1,
             "font host snapshot review must reference exactly one current host artifact record")
    artifact_path = (manifest_path.resolve().parents[3] / artifacts[0]).resolve()
    artifact_text = artifact_path.read_text(encoding="utf-8")
    _require("status: host-snapshot-only" in artifact_text,
             "font host snapshot artifact must declare host-snapshot-only status")
    _require("framebuffer: 32x16 RGB565" in artifact_text,
             "font host snapshot artifact must record framebuffer dimensions")
    _require("lit_pixels: 19" in artifact_text,
             "font host snapshot artifact must record deterministic lit pixel count")
    _require("checksum_fnv1a32: 0x8DD0D797" in artifact_text,
             "font host snapshot artifact must record deterministic checksum")
    _require("not a visual-quality approval" in artifact_text,
             "font host snapshot artifact must not claim visual approval")

    technical_reviews = cast(list[str], snapshot_status.get("technical_review_records", []))
    _require(len(technical_reviews) > 0,
             "font host snapshot review must reference at least one technical review record")
    for review in technical_reviews:
        review_path = (manifest_path.resolve().parents[3] / review).resolve()
        review_text = review_path.read_text(encoding="utf-8")
        _require("status: rejected-needs-regeneration" in review_text,
                 "font host technical review must record the current rejection status")
        _require("not a human art approval" in review_text,
                 "font host technical review must preserve the human-review boundary")
        _require("License/provenance remains `project-review-pending`" in review_text,
                 "font host technical review must preserve the license/provenance boundary")
    _require("not real LCD/OLED/LED-matrix hardware validation" in artifact_text,
             "font host snapshot artifact must keep hardware validation out of scope")

    pending_ids = set(cast(list[str], snapshot_status["pending_font_ids"]))
    _require(snapshot_status["state"] == "host-snapshot-review-pending",
             "font host snapshot review state changed before reviewed artifacts were recorded")
    _require(snapshot_status["evidence_level"] == "host-snapshot-only-pending",
             "font host snapshot evidence level must remain pending")
    _require(len(pending_ids) == len(data["fonts"]),
             "font host snapshot pending list must cover every current font")
    _require(not cast(list[str], snapshot_status["reviewed_font_ids"]),
             "font host snapshot reviewed list must stay empty until a real review record exists")
    for font in data["fonts"]:
        font_id = cast(str, font["id"])
        _require(font_id in pending_ids, f"{font_id}: missing from pending host snapshot list")


def self_test_glyph_metadata(data: dict[str, Any]) -> None:
    """Validate glyph-generation metadata before real glyph table output exists."""

    seen_outputs: set[str] = set()
    expected = {
        "ascii_8x16": ("generated/xy_font_8x16_generated.h", "generated/xy_font_8x16_generated.c", "ascii-range"),
        "ascii_16x24": ("generated/xy_font_16x24_generated.h", "generated/xy_font_16x24_generated.c", "ascii-range"),
        "chinese_16x16_ui_legacy": (
            "generated/xy_font_chinese_16x16_generated.h",
            "generated/xy_font_chinese_16x16_generated.c",
            "explicit-codepoints",
        ),
    }

    for font in data["fonts"]:
        font_id = cast(str, font["id"])
        output_header = cast(str, font["output_header"])
        output_source = cast(str, font["output_source"])
        glyph_order = cast(str, font["glyph_order"])
        _require(font_id in expected, f"unexpected font id in glyph metadata self-test: {font_id}")
        _require((output_header, output_source, glyph_order) == expected[font_id],
                 f"{font_id}: glyph metadata does not match reviewed output contract")
        _require(output_header not in seen_outputs, f"duplicate generated output path: {output_header}")
        _require(output_source not in seen_outputs, f"duplicate generated output path: {output_source}")
        seen_outputs.add(output_header)
        seen_outputs.add(output_source)
        _require(font["generator_mode"] == "legacy-passthrough",
                 f"{font_id}: current slice only permits legacy-passthrough metadata")
        _require(font["source_license"] == "project-review-pending",
                 f"{font_id}: license provenance must remain explicit until reviewed")

    broken = dict(cast(dict[str, Any], data["fonts"][0]))
    broken["id"] = "metadata_negative_probe"
    broken["generator_mode"] = "unsupported-mode"
    try:
        _validate_generation_metadata(broken)
    except ManifestError as exc:
        _require("unsupported generator_mode" in str(exc),
                 "negative metadata probe failed for the wrong reason")
    else:
        raise ManifestError("unsupported generator_mode was accepted")


def self_test_glyph_preview(data: dict[str, Any]) -> None:
    """Validate deterministic legacy-passthrough glyph header/source previews."""

    for font in data["fonts"]:
        font_id = cast(str, font["id"])
        header_once = build_glyph_header(data, font_id)
        header_twice = build_glyph_header(data, font_id)
        source_once = build_glyph_source(data, font_id)
        source_twice = build_glyph_source(data, font_id)
        macro = _macro_name(font_id)

        _require(header_once == header_twice, f"{font_id}: glyph header preview is not deterministic")
        _require(source_once == source_twice, f"{font_id}: glyph source preview is not deterministic")
        _require(f"XY_GUI_FONT_{macro}_GENERATED_WIDTH" in header_once,
                 f"{font_id}: glyph header preview is missing width macro")
        _require(f"XY_GUI_FONT_{macro}_GENERATED_HANDLE()" in header_once,
                 f"{font_id}: glyph header preview is missing legacy handle macro")
        _require(cast(str, font["public_handle"]) in source_once,
                 f"{font_id}: glyph source preview is missing legacy handle reference")
        _require("\t" not in header_once and "\t" not in source_once,
                 f"{font_id}: glyph preview output must not contain tabs")

    try:
        build_glyph_header(data, "missing_font")
    except ManifestError as exc:
        _require("unknown font id" in str(exc), "missing-font probe failed for the wrong reason")
    else:
        raise ManifestError("missing font id was accepted")


def build_licensed_candidate_snapshot(data: dict[str, Any], source_path: Path) -> str:
    """Rasterize the required UI glyph set from the reviewed replacement source."""

    try:
        from PIL import Image, ImageDraw, ImageFont
    except ImportError as exc:
        raise ManifestError("Pillow is required to rasterize the licensed font candidate") from exc

    candidate = cast(dict[str, Any], data["replacement_candidate"])
    source_bytes = source_path.read_bytes()
    _require(hashlib.sha256(source_bytes).hexdigest() == candidate["source_sha256"],
             "replacement candidate source digest does not match the manifest")
    chinese = _find_font(data, "chinese_16x16_ui_legacy")
    codepoints = [_parse_hex(item, "required_ui_codepoints")
                  for item in cast(list[str], chinese["required_ui_codepoints"])]
    size = cast(int, candidate["pixel_size"])
    font = ImageFont.truetype(str(source_path), size,
                              index=cast(int, candidate["source_collection_index"]))
    _require(font.getname() == (candidate["family"], candidate["style"]),
             f"replacement candidate face mismatch: {font.getname()!r}")

    glyphs = []
    for codepoint in codepoints:
        image = Image.new("1", (size, size))
        draw = ImageDraw.Draw(image)
        bbox = draw.textbbox((0, 0), chr(codepoint), font=font)
        x = (size - (bbox[2] - bbox[0])) // 2 - bbox[0]
        y = (size - (bbox[3] - bbox[1])) // 2 - bbox[1]
        draw.text((x, y), chr(codepoint), font=font, fill=1)
        rows = []
        for row in range(size):
            value = 0
            for column in range(size):
                value = (value << 1) | int(image.getpixel((column, row)) != 0)
            rows.append(f"{value:04x}")
        _require(any(row != "0000" for row in rows),
                 f"replacement candidate rendered blank glyph U+{codepoint:04X}")
        glyphs.append({"codepoint": f"U+{codepoint:04X}", "rows_be16": rows})

    snapshot = {
        "schema_version": 1,
        "status": candidate["status"],
        "font_id": candidate["id"],
        "source_filename": candidate["source_filename"],
        "source_sha256": candidate["source_sha256"],
        "source_collection_index": candidate["source_collection_index"],
        "license": candidate["license"],
        "pixel_size": size,
        "render_mode": candidate["render_mode"],
        "glyphs": glyphs,
        "evidence_boundary": "Host-generated candidate only; not active firmware, visual approval, or hardware evidence.",
    }
    return json.dumps(snapshot, ensure_ascii=False, indent=2, sort_keys=True) + "\n"


def write_licensed_candidate_snapshot(data: dict[str, Any], source_path: Path, output_path: Path) -> None:
    output_path = output_path.resolve()
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(build_licensed_candidate_snapshot(data, source_path), encoding="utf-8")


def self_test_licensed_candidate_snapshot(data: dict[str, Any], manifest_path: Path) -> None:
    """Verify the checked-in candidate without requiring Pillow or the 19 MB source font in CI."""

    candidate = cast(dict[str, Any], data["replacement_candidate"])
    snapshot_path = manifest_path.resolve().parent / cast(str, candidate["output_snapshot"])
    _require(snapshot_path.exists(), f"licensed replacement snapshot is missing: {snapshot_path}")
    snapshot = json.loads(snapshot_path.read_text(encoding="utf-8"))
    _require(snapshot["status"] == "generated-host-candidate-not-active",
             "licensed snapshot must remain explicitly non-active")
    expected_metadata = {
        "font_id": candidate["id"],
        "source_filename": candidate["source_filename"],
        "source_sha256": candidate["source_sha256"],
        "source_collection_index": candidate["source_collection_index"],
        "license": candidate["license"],
        "pixel_size": candidate["pixel_size"],
        "render_mode": candidate["render_mode"],
    }
    for field, expected_value in expected_metadata.items():
        _require(snapshot[field] == expected_value,
                 f"licensed snapshot metadata drifted: {field}")
    chinese = _find_font(data, "chinese_16x16_ui_legacy")
    expected = [f"U+{_parse_hex(item, 'required_ui_codepoints'):04X}"
                for item in cast(list[str], chinese["required_ui_codepoints"])]
    glyphs = cast(list[dict[str, Any]], snapshot["glyphs"])
    _require([glyph["codepoint"] for glyph in glyphs] == expected,
             "licensed snapshot glyph order/coverage does not match required UI codepoints")
    _require(len({tuple(glyph["rows_be16"]) for glyph in glyphs}) == len(glyphs),
             "licensed snapshot contains duplicate required UI glyph bitmaps")
    for glyph in glyphs:
        rows = glyph["rows_be16"]
        _require(isinstance(rows, list) and len(rows) == candidate["pixel_size"],
                 f"{glyph['codepoint']}: licensed snapshot row count is invalid")
        _require(all(re.fullmatch(r"[0-9a-f]{4}", row) is not None for row in rows),
                 f"{glyph['codepoint']}: licensed snapshot rows must be 16-bit lowercase hex")
        _require(any(row != "0000" for row in rows),
                 f"{glyph['codepoint']}: licensed snapshot glyph is blank")
    _require("not active firmware" in snapshot["evidence_boundary"],
             "licensed snapshot must preserve its non-product evidence boundary")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", default="components/gui/fonts/font_manifest.json",
                        help="Path to the font manifest JSON")
    parser.add_argument("--check", action="store_true", help="Validate the manifest and exit")
    parser.add_argument("--summary", action="store_true", help="Print a deterministic manifest summary")
    parser.add_argument("--emit-manifest-header", action="store_true",
                        help="Print deterministic generated-header preview for manifest inventory")
    parser.add_argument("--write-manifest-header", metavar="PATH",
                        help="Write deterministic generated-header output for manifest inventory")
    parser.add_argument("--self-test-output", action="store_true",
                        help="Validate deterministic generated-output contracts and exit")
    parser.add_argument("--self-test-write", action="store_true",
                        help="Validate that written output matches the generated-header preview")
    parser.add_argument("--self-test-glyph-metadata", action="store_true",
                        help="Validate glyph-output metadata contracts without writing glyph tables")
    parser.add_argument("--emit-glyph-header", metavar="FONT_ID",
                        help="Print deterministic legacy-passthrough glyph header preview for FONT_ID")
    parser.add_argument("--emit-glyph-source", metavar="FONT_ID",
                        help="Print deterministic legacy-passthrough glyph source preview for FONT_ID")
    parser.add_argument("--self-test-glyph-preview", action="store_true",
                        help="Validate deterministic glyph header/source preview contracts")
    parser.add_argument("--write-glyph-preview", nargs=3, metavar=("FONT_ID", "HEADER", "SOURCE"),
                        help="Write deterministic legacy-passthrough glyph header/source previews")
    parser.add_argument("--self-test-glyph-write", action="store_true",
                        help="Validate that glyph preview write mode matches preview output")
    parser.add_argument("--self-test-glyph-compile", action="store_true",
                        help="Validate that written glyph previews compile as C99 host artifacts")
    parser.add_argument("--self-test-checked-in-preview", action="store_true",
                        help="Validate checked-in generated previews against the generator and compile them")
    parser.add_argument("--self-test-license-manifest", action="store_true",
                        help="Validate pending font license/provenance review manifest contracts")
    parser.add_argument("--self-test-snapshot-review-manifest", action="store_true",
                        help="Validate pending host snapshot review manifest contracts")
    parser.add_argument("--write-licensed-candidate-snapshot", nargs=2,
                        metavar=("SOURCE_FONT", "OUTPUT"),
                        help="Rasterize the reviewed licensed source into a deterministic UI-glyph snapshot")
    parser.add_argument("--self-test-licensed-candidate-snapshot", action="store_true",
                        help="Validate the checked-in licensed replacement snapshot without the source font")
    args = parser.parse_args(argv)

    try:
        data = validate_manifest(Path(args.manifest))
    except (ManifestError, json.JSONDecodeError, OSError) as exc:
        print(f"font manifest validation failed: {exc}", file=sys.stderr)
        return 1

    if args.self_test_output:
        try:
            self_test_generated_output(data)
        except ManifestError as exc:
            print(f"font generated-output self-test failed: {exc}", file=sys.stderr)
            return 1
        print("font generated-output self-test passed")
    elif args.self_test_write:
        try:
            self_test_written_output(data)
        except ManifestError as exc:
            print(f"font written-output self-test failed: {exc}", file=sys.stderr)
            return 1
        print("font written-output self-test passed")
    elif args.self_test_glyph_metadata:
        try:
            self_test_glyph_metadata(data)
        except ManifestError as exc:
            print(f"font glyph-metadata self-test failed: {exc}", file=sys.stderr)
            return 1
        print("font glyph-metadata self-test passed")
    elif args.self_test_glyph_preview:
        try:
            self_test_glyph_preview(data)
        except ManifestError as exc:
            print(f"font glyph-preview self-test failed: {exc}", file=sys.stderr)
            return 1
        print("font glyph-preview self-test passed")
    elif args.self_test_glyph_write:
        try:
            self_test_glyph_write(data)
        except (ManifestError, OSError) as exc:
            print(f"font glyph-write self-test failed: {exc}", file=sys.stderr)
            return 1
        print("font glyph-write self-test passed")
    elif args.self_test_glyph_compile:
        try:
            self_test_glyph_compile(data, Path(args.manifest))
        except (ManifestError, OSError) as exc:
            print(f"font glyph-compile self-test failed: {exc}", file=sys.stderr)
            return 1
        print("font glyph-compile self-test passed")
    elif args.self_test_checked_in_preview:
        try:
            self_test_checked_in_preview(data, Path(args.manifest))
        except (ManifestError, OSError) as exc:
            print(f"font checked-in-preview self-test failed: {exc}", file=sys.stderr)
            return 1
        print("font checked-in-preview self-test passed")
    elif args.self_test_license_manifest:
        try:
            self_test_license_manifest(data, Path(args.manifest))
        except (ManifestError, OSError) as exc:
            print(f"font license-manifest self-test failed: {exc}", file=sys.stderr)
            return 1
        print("font license-manifest self-test passed")
    elif args.self_test_snapshot_review_manifest:
        try:
            self_test_snapshot_review_manifest(data, Path(args.manifest))
        except (ManifestError, OSError) as exc:
            print(f"font snapshot-review-manifest self-test failed: {exc}", file=sys.stderr)
            return 1
        print("font snapshot-review-manifest self-test passed")
    elif args.self_test_licensed_candidate_snapshot:
        try:
            self_test_licensed_candidate_snapshot(data, Path(args.manifest))
        except (ManifestError, json.JSONDecodeError, OSError) as exc:
            print(f"font licensed-candidate snapshot self-test failed: {exc}", file=sys.stderr)
            return 1
        print("font licensed-candidate snapshot self-test passed")
    elif args.write_licensed_candidate_snapshot:
        source_path, output_path = args.write_licensed_candidate_snapshot
        try:
            write_licensed_candidate_snapshot(data, Path(source_path), Path(output_path))
        except (ManifestError, OSError) as exc:
            print(f"font licensed-candidate snapshot write failed: {exc}", file=sys.stderr)
            return 1
        print(f"font licensed-candidate snapshot written: {output_path}")
    elif args.emit_glyph_header:
        try:
            print(build_glyph_header(data, args.emit_glyph_header), end="")
        except ManifestError as exc:
            print(f"font glyph-header preview failed: {exc}", file=sys.stderr)
            return 1
    elif args.emit_glyph_source:
        try:
            print(build_glyph_source(data, args.emit_glyph_source), end="")
        except ManifestError as exc:
            print(f"font glyph-source preview failed: {exc}", file=sys.stderr)
            return 1
    elif args.write_glyph_preview:
        font_id, header_path, source_path = args.write_glyph_preview
        try:
            write_glyph_preview(data, font_id, Path(header_path), Path(source_path))
        except (ManifestError, OSError) as exc:
            print(f"font glyph-preview write failed: {exc}", file=sys.stderr)
            return 1
        print(f"font glyph preview written: {header_path} {source_path}")
    elif args.write_manifest_header:
        try:
            write_manifest_header(data, Path(args.write_manifest_header))
        except OSError as exc:
            print(f"font manifest header write failed: {exc}", file=sys.stderr)
            return 1
        print(f"font manifest header written: {args.write_manifest_header}")
    elif args.emit_manifest_header:
        print(build_manifest_header(data), end="")
    elif args.summary or not args.check:
        print(build_summary(data))
    else:
        print("font manifest validation passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
