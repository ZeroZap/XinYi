from __future__ import annotations

import json
from dataclasses import asdict
from pathlib import Path
from typing import Any, Mapping

from .serial_config import ActionButton, FilterRule, SerialWindowProfile, SerialWorkspaceProfile

SCHEMA = "xinyi.serial.workspace.v1"


def _as_tuple(value: Any) -> tuple[Any, ...]:
    if value is None:
        return ()
    if isinstance(value, tuple):
        return value
    if isinstance(value, list):
        return tuple(value)
    raise ValueError(f"expected list value, got {type(value).__name__}")


def _filter_from_mapping(data: Mapping[str, Any]) -> FilterRule:
    payload = dict(data)
    payload["keywords"] = _as_tuple(payload.get("keywords"))
    return FilterRule(**payload)


def _button_from_mapping(data: Mapping[str, Any]) -> ActionButton:
    return ActionButton(**dict(data))


def _window_from_mapping(data: Mapping[str, Any]) -> SerialWindowProfile:
    payload = dict(data)
    payload["disabled_filter_names"] = _as_tuple(payload.get("disabled_filter_names"))
    payload["local_filters"] = tuple(_filter_from_mapping(item) for item in payload.get("local_filters", ()))
    payload["local_buttons"] = tuple(_button_from_mapping(item) for item in payload.get("local_buttons", ()))
    return SerialWindowProfile(**payload)


def workspace_to_mapping(
    workspace: SerialWorkspaceProfile,
    windows: tuple[SerialWindowProfile, ...] = (),
) -> dict[str, Any]:
    return {
        "schema": SCHEMA,
        "name": workspace.name,
        "global_filters": [asdict(rule) for rule in workspace.filters],
        "global_buttons": [asdict(button) for button in workspace.buttons],
        "windows": [asdict(window) for window in windows],
    }


def workspace_from_mapping(data: Mapping[str, Any]) -> tuple[SerialWorkspaceProfile, tuple[SerialWindowProfile, ...]]:
    schema = data.get("schema")
    if schema != SCHEMA:
        raise ValueError(f"unsupported serial profile schema: {schema!r}")

    workspace = SerialWorkspaceProfile(
        name=str(data.get("name", "XinYi Serial Workspace")),
        filters=tuple(_filter_from_mapping(item) for item in data.get("global_filters", ())),
        buttons=tuple(_button_from_mapping(item) for item in data.get("global_buttons", ())),
    )
    windows = tuple(_window_from_mapping(item) for item in data.get("windows", ()))
    return workspace, windows


def save_workspace_profile(
    path: str | Path,
    workspace: SerialWorkspaceProfile,
    windows: tuple[SerialWindowProfile, ...] = (),
) -> None:
    profile_path = Path(path)
    profile_path.write_text(
        json.dumps(workspace_to_mapping(workspace, windows), ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )


def load_workspace_profile(path: str | Path) -> tuple[SerialWorkspaceProfile, tuple[SerialWindowProfile, ...]]:
    data = json.loads(Path(path).read_text(encoding="utf-8"))
    if not isinstance(data, dict):
        raise ValueError("serial profile root must be a JSON object")
    return workspace_from_mapping(data)
