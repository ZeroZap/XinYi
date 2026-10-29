#!/usr/bin/env python3
"""Helpers for guards that scan source text for forbidden symbols.

A boundary guard asks a question about code: does this file call the vendor API directly
instead of going through the HAL? Answering that over the raw file text goes wrong in both
directions once comments exist:

- A comment that names the forbidden call - usually to explain why it is *not* used -
  trips the guard. The code is right and the documentation is the evidence, so the guard
  punishes writing things down.
- The mirror image: text inside a comment cannot be a violation, so scanning it can only
  ever produce false positives, never extra coverage.

Stripping comments first makes the guard answer the question it was written to ask.

There is one exception, and guards must say when they are taking it: a check that requires a
rationale to have been written down *is* a check about a comment. `xy_os_thread_detach` in the
RT-Thread backend and the task-handle inventory in the FreeRTOS backend both require a specific
explanatory sentence to be present at the code that makes the decision. Stripping comments would
delete exactly what is being asked for, so those checks read the raw text - deliberately, with a
comment saying so. Everything else reads code_only(): if a guard can be satisfied by writing a
comment, it is checking prose, not behaviour.

The scan is a character walk rather than a regular expression on purpose. A regex cannot
tell a `//` inside a string literal from a comment, and this helper had exactly that bug:
`const char *s = "a // b"` lost its value. Dropping text from a literal is a false
negative in the direction that matters, so the states are tracked explicitly.
"""

from __future__ import annotations

import re

from pathlib import Path


def code_only(text: str) -> str:
    """Return source text with C and C++ comments removed.

    String and character literals are preserved, including any comment syntax inside
    them: a symbol someone typed into a string is still there to be found.
    """
    out: list[str] = []
    index = 0
    length = len(text)

    while index < length:
        char = text[index]

        if char == "/" and index + 1 < length and text[index + 1] == "/":
            index += 2
            while index < length and text[index] != "\n":
                index += 1
            continue

        if char == "/" and index + 1 < length and text[index + 1] == "*":
            index += 2
            while index < length and not (text[index] == "*" and index + 1 < length
                                          and text[index + 1] == "/"):
                out.append("\n" if text[index] == "\n" else "")
                index += 1
            index += 2  # the closing */
            continue

        if char in ("'", '"'):
            quote = char
            out.append(char)
            index += 1
            while index < length:
                if text[index] == "\\" and index + 1 < length:
                    out.append(text[index:index + 2])
                    index += 2
                    continue
                out.append(text[index])
                if text[index] == quote:
                    index += 1
                    break
                index += 1
            continue

        out.append(char)
        index += 1

    return "".join(out)


def read_code(path: str | Path) -> str:
    """Read a C-family source file with its comments already removed.

    Guards ask a question about code. Answering it from `path.read_text()` means each call
    site decides for itself whether to strip, and the ones that forget can be satisfied by
    a comment that merely names the symbol. Going through this helper makes the safe read
    the short one.
    """
    return code_only(Path(path).read_text(encoding="utf-8", errors="replace"))


def read_raw(path: str | Path) -> str:
    """Read a file that is not C: Kconfig, CMake, Markdown, JSON.

    `code_only` only knows C comment syntax. Applying it to a Kconfig file would leave
    every `#` comment in place - a half strip that reads as though it had been done - so
    the non-code choice is spelled out here rather than being the silent default. A future
    `#`-comment stripper belongs next to this function, not inside the guards.
    """
    return Path(path).read_text(encoding="utf-8", errors="replace")


# One hop of delegation, shared so every guard means the same thing by it.
#
# A guard that only reads an entry point's own lines marks a correct backend as broken the moment the
# check moves into a helper - `pc_i2c_check_transfer()` validated the address, and the guard still
# said "the address range is not validated". Each guard grew its own copy of this logic once already;
# the second copy is where the two answers start to differ.

_LOCAL_STATIC = re.compile(r"^static\s+[^;{=]*?\b(\w+)\s*\(", re.M)
_LOCAL_CALL = re.compile(r"\b(\w+)\s*\(")


def braced_body(source: str, budget: int = 40000) -> str:
    """The braced block starting at the first `{` in `source` (which begins at a signature)."""
    brace = source.find("{")
    if brace < 0:
        return ""
    depth = 0
    for index in range(brace, min(len(source), brace + budget)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[brace:index + 1]
    return ""


def local_definitions(source: str) -> dict[str, str]:
    """Every file-local function in `source`, keyed by name, as braced bodies."""
    definitions: dict[str, str] = {}
    for match in _LOCAL_STATIC.finditer(source):
        body = braced_body(source[match.start():])
        if body:
            definitions[match.group(1)] = body
    return definitions


def with_delegation(source: str, body: str, hops: int = 1) -> str:
    """`body` plus the bodies of file-local helpers it calls, `hops` deep.

    The result answers "does this entry point, directly or through one helper, do X" - which is the
    question the guards actually mean. Returning `body` unchanged when the source has no local
    definitions keeps callers from having to branch.
    """
    definitions = local_definitions(source)
    if not definitions:
        return body
    seen: set[str] = set()
    for _ in range(max(0, hops)):
        added = False
        for name in _LOCAL_CALL.findall(body):
            if name in definitions and name not in seen:
                seen.add(name)
                body = body + "\n" + definitions[name]
                added = True
        if not added:
            break
    return body
