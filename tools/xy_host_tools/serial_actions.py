from __future__ import annotations

import ast
from datetime import datetime
from typing import Any, Mapping

from .serial_config import ActionButton

_FORBIDDEN_SCRIPT_NAMES = {
    "__import__",
    "eval",
    "exec",
    "open",
    "compile",
    "globals",
    "locals",
    "vars",
    "dir",
    "getattr",
    "setattr",
    "delattr",
    "input",
    "help",
}

_ALLOWED_BUILTINS = {
    "str": str,
    "bytes": bytes,
    "len": len,
    "int": int,
    "hex": hex,
    "min": min,
    "max": max,
}


def _text_to_bytes(text: str) -> bytes:
    return text.encode("utf-8")


def _hex_to_bytes(text: str) -> bytes:
    compact = "".join(text.split())
    if len(compact) % 2 != 0:
        raise ValueError("hex payload must contain an even number of digits")
    try:
        return bytes.fromhex(compact)
    except ValueError as exc:
        raise ValueError("hex payload contains non-hex characters") from exc


def _assert_safe_script(tree: ast.AST) -> None:
    for node in ast.walk(tree):
        if isinstance(node, (ast.Import, ast.ImportFrom, ast.Global, ast.Nonlocal, ast.Lambda, ast.ClassDef)):
            raise ValueError("script payload contains unsupported syntax")
        if isinstance(node, ast.FunctionDef) and node.name != "__xy_button_script__":
            raise ValueError("script payload contains unsupported syntax")
        if isinstance(node, ast.Name) and node.id in _FORBIDDEN_SCRIPT_NAMES:
            raise ValueError(f"script payload uses forbidden name: {node.id}")
        if isinstance(node, ast.Attribute) and node.attr.startswith("__"):
            raise ValueError("script payload cannot access dunder attributes")


def _script_source(payload: str) -> str:
    stripped = payload.strip()
    if stripped.startswith("return "):
        return "def __xy_button_script__():\n    " + stripped
    return "def __xy_button_script__():\n    return " + stripped


def _script_to_bytes(payload: str, context: Mapping[str, Any] | None) -> bytes:
    source = _script_source(payload)
    tree = ast.parse(source, mode="exec")
    _assert_safe_script(tree)

    safe_context: dict[str, Any] = {
        "port": "",
        "window_id": "",
        "now": datetime.now().isoformat(timespec="seconds"),
        "last_rx": b"",
    }
    if context:
        safe_context.update(context)

    namespace: dict[str, Any] = {"__builtins__": _ALLOWED_BUILTINS, **safe_context}
    exec(compile(tree, "<xy-button-script>", "exec"), namespace, namespace)
    result = namespace["__xy_button_script__"]()

    if isinstance(result, bytes):
        return result
    if isinstance(result, str):
        return result.encode("utf-8")
    raise ValueError("script payload must return str or bytes")


def render_button_payload(button: ActionButton, context: Mapping[str, Any] | None = None) -> bytes:
    if button.mode == "text":
        payload = button.payload + ("\n" if button.append_newline else "")
        return _text_to_bytes(payload)
    if button.mode == "hex":
        return _hex_to_bytes(button.payload)
    if button.mode == "script":
        rendered = _script_to_bytes(button.payload, context)
        if button.append_newline and not rendered.endswith(b"\n"):
            rendered += b"\n"
        return rendered
    raise ValueError(f"unsupported button mode: {button.mode}")
