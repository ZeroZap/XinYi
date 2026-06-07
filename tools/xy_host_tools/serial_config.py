from __future__ import annotations

from dataclasses import dataclass, field
from typing import Literal, Tuple

FilterMatchMode = Literal["any", "all", "sequence", "regex"]
FilterAction = Literal["highlight", "hide", "pin", "count"]
ButtonMode = Literal["text", "hex", "script"]


@dataclass(frozen=True)
class FilterRule:
    name: str
    keywords: Tuple[str, ...]
    match: FilterMatchMode = "any"
    case_sensitive: bool = False
    foreground: str = "default"
    background: str = "default"
    action: FilterAction = "highlight"
    priority: int = 0
    enabled: bool = True


@dataclass(frozen=True)
class ActionButton:
    name: str
    label: str
    mode: ButtonMode
    payload: str
    append_newline: bool = False
    confirm: bool = False


@dataclass(frozen=True)
class SerialWindowProfile:
    window_id: str
    title: str
    port: str = ""
    baudrate: int = 115200
    inherit_global_filters: bool = True
    inherit_global_buttons: bool = True
    disabled_filter_names: Tuple[str, ...] = field(default_factory=tuple)
    local_filters: Tuple[FilterRule, ...] = field(default_factory=tuple)
    local_buttons: Tuple[ActionButton, ...] = field(default_factory=tuple)


@dataclass(frozen=True)
class SerialWorkspaceProfile:
    name: str
    filters: Tuple[FilterRule, ...] = field(default_factory=tuple)
    buttons: Tuple[ActionButton, ...] = field(default_factory=tuple)

    def effective_filters_for(self, window: SerialWindowProfile) -> Tuple[FilterRule, ...]:
        effective: dict[str, FilterRule] = {}
        disabled = set(window.disabled_filter_names)

        if window.inherit_global_filters:
            for rule in self.filters:
                if rule.name not in disabled:
                    effective[rule.name] = rule

        for rule in window.local_filters:
            if rule.name not in disabled:
                effective[rule.name] = rule

        return tuple(effective.values())

    def effective_buttons_for(self, window: SerialWindowProfile) -> Tuple[ActionButton, ...]:
        buttons: list[ActionButton] = []
        if window.inherit_global_buttons:
            buttons.extend(self.buttons)
        buttons.extend(window.local_buttons)
        return tuple(buttons)
