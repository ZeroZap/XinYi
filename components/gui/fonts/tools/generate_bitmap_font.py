#!/usr/bin/env python3
"""Deterministic XinYi bitmap-font manifest validator/generator bootstrap.

This first bootstrap slice intentionally validates the current checked-in font
manifest only.  It does not import external font files or rewrite the legacy C
bitmap tables.  Future generator modes should preserve the same strict manifest
checks before emitting regenerated tables.
"""

from __future__ import annotations

import argparse
import json
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
    "public_handle",
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
    for item in source_files:
        _require(isinstance(item, str) and len(item) > 0,
                 f"{font.get('id')}: source_files entries must be strings")
        source_path = fonts_root / item
        _require(source_path.exists(), f"{font.get('id')}: missing source file {source_path}")


def validate_manifest(manifest_path: Path) -> dict[str, Any]:
    manifest_path = manifest_path.resolve()
    fonts_root = manifest_path.parent
    data = json.loads(manifest_path.read_text(encoding="utf-8"))

    _require(data.get("schema_version") == 1, "schema_version must be 1")
    _require(data.get("component") == "xinyi-gui-fonts", "component must be xinyi-gui-fonts")

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

        if "known_inventory_flags" in font:
            flags = font["known_inventory_flags"]
            _require(isinstance(flags, dict), f"{font_id}: known_inventory_flags must be an object")
            for key, value in flags.items():
                if key.endswith("_allowed"):
                    _require(isinstance(value, bool), f"{font_id}: {key} must be boolean")
                if key.endswith("_reason"):
                    _require(isinstance(value, str) and value, f"{font_id}: {key} must be non-empty")

    generation_plan = data.get("generation_plan")
    _require(isinstance(generation_plan, dict), "generation_plan must be an object")
    generator_rel = generation_plan.get("generator")
    _require(isinstance(generator_rel, str) and generator_rel.endswith("generate_bitmap_font.py"),
             "generation_plan.generator must point to generate_bitmap_font.py")
    generator_rel = cast(str, generator_rel)
    generator_path = (manifest_path.parents[3] / generator_rel).resolve()
    _require(generator_path.exists(), f"generation_plan.generator path does not exist: {generator_path}")

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


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", default="components/gui/fonts/font_manifest.json",
                        help="Path to the font manifest JSON")
    parser.add_argument("--check", action="store_true", help="Validate the manifest and exit")
    parser.add_argument("--summary", action="store_true", help="Print a deterministic manifest summary")
    args = parser.parse_args(argv)

    try:
        data = validate_manifest(Path(args.manifest))
    except (ManifestError, json.JSONDecodeError, OSError) as exc:
        print(f"font manifest validation failed: {exc}", file=sys.stderr)
        return 1

    if args.summary or not args.check:
        print(build_summary(data))
    else:
        print("font manifest validation passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
