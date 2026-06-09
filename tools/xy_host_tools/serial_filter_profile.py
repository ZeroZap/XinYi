from __future__ import annotations

import json
from dataclasses import asdict
from pathlib import Path
from typing import Any, Mapping

from .serial_config import FilterRule
from .serial_profile import _filter_from_mapping

FILTER_SCHEMA = "xinyi.serial.filters.v1"


def filter_profile_to_mapping(name: str, filters: tuple[FilterRule, ...]) -> dict[str, Any]:
    return {
        "schema": FILTER_SCHEMA,
        "name": name,
        "filters": [asdict(rule) for rule in filters],
    }


def filter_profile_from_mapping(data: Mapping[str, Any]) -> tuple[str, tuple[FilterRule, ...]]:
    schema = data.get("schema")
    if schema != FILTER_SCHEMA:
        raise ValueError(f"unsupported filter profile schema: {schema!r}")
    name = str(data.get("name", "XinYi Serial Filters"))
    filters = tuple(_filter_from_mapping(item) for item in data.get("filters", ()))
    return name, filters


def save_filter_profile(path: str | Path, name: str, filters: tuple[FilterRule, ...]) -> None:
    profile_path = Path(path)
    profile_path.write_text(
        json.dumps(filter_profile_to_mapping(name, filters), ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )


def load_filter_profile(path: str | Path) -> tuple[str, tuple[FilterRule, ...]]:
    data = json.loads(Path(path).read_text(encoding="utf-8"))
    if not isinstance(data, dict):
        raise ValueError("filter profile root must be a JSON object")
    return filter_profile_from_mapping(data)
