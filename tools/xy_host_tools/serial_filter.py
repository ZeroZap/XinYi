from __future__ import annotations

import re
from dataclasses import dataclass
from typing import Iterable, Tuple

from .serial_config import FilterRule


@dataclass(frozen=True)
class FilterResult:
    visible: bool = True
    foreground: str = "default"
    background: str = "default"
    matched_rules: Tuple[str, ...] = ()


def _normalize(text: str, case_sensitive: bool) -> str:
    return text if case_sensitive else text.lower()


def _rule_matches(line: str, rule: FilterRule) -> bool:
    if not rule.enabled or not rule.keywords:
        return False

    if rule.match == "regex":
        flags = 0 if rule.case_sensitive else re.IGNORECASE
        return any(re.search(pattern, line, flags) is not None for pattern in rule.keywords)

    haystack = _normalize(line, rule.case_sensitive)
    needles = tuple(_normalize(keyword, rule.case_sensitive) for keyword in rule.keywords)

    if rule.match == "any":
        return any(needle in haystack for needle in needles)
    if rule.match == "all":
        return all(needle in haystack for needle in needles)
    if rule.match == "sequence":
        pos = 0
        for needle in needles:
            found = haystack.find(needle, pos)
            if found < 0:
                return False
            pos = found + len(needle)
        return True

    raise ValueError(f"unsupported filter match mode: {rule.match}")


def apply_filters(line: str, rules: Iterable[FilterRule]) -> FilterResult:
    matched: list[FilterRule] = [rule for rule in rules if _rule_matches(line, rule)]
    matched_names = tuple(rule.name for rule in matched)

    if any(rule.action == "hide" for rule in matched):
        return FilterResult(visible=False, matched_rules=matched_names)

    highlight_rules = [rule for rule in matched if rule.action == "highlight"]
    if not highlight_rules:
        return FilterResult(matched_rules=matched_names)

    # Highest priority wins.  Python's stable max keeps the later rule on equal priority by using index.
    winner = max(enumerate(highlight_rules), key=lambda item: (item[1].priority, item[0]))[1]
    return FilterResult(
        visible=True,
        foreground=winner.foreground,
        background=winner.background,
        matched_rules=matched_names,
    )
