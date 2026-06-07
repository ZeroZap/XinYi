from __future__ import annotations

from html import escape

from .z_serial_view_model import RenderedLine

_DEFAULT_FOREGROUND = "#d4d4d4"
_DEFAULT_BACKGROUND = "transparent"
_COLOR_MAP = {
    "black": "#000000",
    "red": "#d70000",
    "green": "#008700",
    "yellow": "#af8700",
    "blue": "#005faf",
    "magenta": "#af00af",
    "cyan": "#0087af",
    "white": "#ffffff",
    "gray": "#808080",
    "grey": "#808080",
    "default": "transparent",
}


def color_to_css(color: str, default: str = _DEFAULT_FOREGROUND) -> str:
    normalized = color.strip().lower()
    if not normalized:
        return default
    if normalized.startswith("#") and len(normalized) in (4, 7):
        return normalized
    return _COLOR_MAP.get(normalized, default)


def line_to_html(line: RenderedLine) -> str:
    foreground = color_to_css(line.foreground, _DEFAULT_FOREGROUND)
    background = color_to_css(line.background, _DEFAULT_BACKGROUND)
    rules = escape(",".join(line.matched_rules) or "-")
    text = escape(line.text)
    return (
        '<span style="'
        f"color: {foreground}; background-color: {background};"
        ' font-family: monospace; white-space: pre-wrap;"'
        f' data-rules="{rules}">{text}</span>'
    )


def lines_to_html(lines: tuple[RenderedLine, ...] | list[RenderedLine]) -> str:
    return "<br/>".join(line_to_html(line) for line in lines)
