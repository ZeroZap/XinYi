from __future__ import annotations

import json
from dataclasses import dataclass, asdict
from pathlib import Path
from typing import Any, Mapping

SESSION_SCHEMA = "xinyi.serial.session.v1"


@dataclass(frozen=True)
class SerialTabSessionState:
    title: str
    port: str = ""
    baudrate: int = 115200
    filter_profile_path: str | None = None


@dataclass(frozen=True)
class SerialSessionState:
    active_index: int = 0
    tabs: tuple[SerialTabSessionState, ...] = ()


def session_state_to_mapping(state: SerialSessionState) -> dict[str, Any]:
    return {
        "schema": SESSION_SCHEMA,
        "active_index": state.active_index,
        "tabs": [asdict(tab) for tab in state.tabs],
    }


def session_state_from_mapping(data: Mapping[str, Any]) -> SerialSessionState:
    schema = data.get("schema")
    if schema != SESSION_SCHEMA:
        raise ValueError(f"unsupported serial session schema: {schema!r}")
    tabs = tuple(_tab_state_from_mapping(item) for item in data.get("tabs", ()))
    active_index = int(data.get("active_index", 0))
    if active_index < 0:
        active_index = 0
    if tabs and active_index >= len(tabs):
        active_index = len(tabs) - 1
    return SerialSessionState(active_index=active_index, tabs=tabs)


def save_session_state(path: str | Path, state: SerialSessionState) -> None:
    session_path = Path(path)
    session_path.parent.mkdir(parents=True, exist_ok=True)
    session_path.write_text(
        json.dumps(session_state_to_mapping(state), ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )


def load_session_state(path: str | Path) -> SerialSessionState:
    data = json.loads(Path(path).read_text(encoding="utf-8"))
    if not isinstance(data, dict):
        raise ValueError("serial session root must be a JSON object")
    return session_state_from_mapping(data)


def _tab_state_from_mapping(data: Mapping[str, Any]) -> SerialTabSessionState:
    return SerialTabSessionState(
        title=str(data.get("title", "Serial")),
        port=str(data.get("port", "")),
        baudrate=int(data.get("baudrate", 115200)),
        filter_profile_path=_optional_str(data.get("filter_profile_path")),
    )


def _optional_str(value: object) -> str | None:
    if value is None:
        return None
    text = str(value)
    return text or None
