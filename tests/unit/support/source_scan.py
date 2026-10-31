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

The same is true one syntax over. A guard that decides something from Kconfig or CMake - does
this selection have a dispatch branch, can the root build set it - reads prose if it matches
lines in the raw text: a commented-out `elseif(OSAL_BACKEND STREQUAL "freertos")` satisfied the
"which backends are reachable" set just as well as a live one, and a commented-out
`set(OSAL_BACKEND ...)` still counted as selectable. `code_only_hash()` is that half.

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


def code_only_hash(text: str) -> str:
    """Return Kconfig/CMake/shell text with its `#` comments removed.

    Same reason as `code_only`, different syntax. A guard that decides something by
    matching a line of CMake - which backends have a dispatch branch, which selection the
    root build can set - is answered by a commented-out line just as readily as by a live
    one, and only one of those builds. The mirror image bites too: a `# CONFIG_X` trailing
    comment on an `if()` line was counted as a tested symbol.

    Quoted text is preserved: a `#` inside `"..."` is data, not a comment. CMake's `#[[ ]]`
    bracket comments are dropped with their newlines kept, so line numbers in a guard's
    message still point at the right line. An unterminated quote is ended at the newline
    rather than swallowing the rest of the file - neither language allows a plain string
    literal to span lines, and guessing otherwise would delete real code.
    """
    out: list[str] = []
    index = 0
    length = len(text)

    while index < length:
        char = text[index]

        if char == "#":
            if text.startswith("#[[", index):
                end = text.find("]]", index + 3)
                stop = length if end < 0 else end + 2
                out.extend("\n" for char in text[index:stop] if char == "\n")
                index = stop
                continue
            while index < length and text[index] != "\n":
                index += 1
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
                last = text[index]
                index += 1
                if last == quote or last == "\n":
                    break
            continue

        out.append(char)
        index += 1

    return "".join(out)


def read_raw(path: str | Path) -> str:
    """Read a file whose prose is the subject: Markdown, JSON, or a doc comment.

    `code_only` only knows C comment syntax and `code_only_hash` only knows `#`, so neither
    is a safe default for "some other file": applying the wrong one leaves every comment in
    place, a half strip that reads as though it had been done. Choosing here means a guard
    has to say which it means. Use `read_hash_code` for Kconfig and CMake, where a guard
    decides what the build does - and this one only where a comment is what is being asked
    about (the `@param` documentation checks) or where the format has no comments at all.
    """
    return Path(path).read_text(encoding="utf-8", errors="replace")


def read_hash_code(path: str | Path) -> str:
    """Read a Kconfig/CMake file with its `#` comments already removed.

    The safe read is the short one, as with `read_code`: a guard that reads the raw text has
    to remember to strip, and the one that forgets is satisfied by a comment.
    """
    return code_only_hash(Path(path).read_text(encoding="utf-8", errors="replace"))


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


# The one fenced-block reader, shared so two guards cannot disagree about what a code block is.
#
# A single regex over the whole document - ```[^\n]*\n(.*?)``` - cannot tell a nested fence from a
# closing one, knows nothing about `~~~`, and silently accepts a block whose terminator is missing.
# That made a documentation guard punish a doc for writing a rule down inside a block and miss the
# same shape written with tildes. The states are walked line by line instead.

FENCE_OPEN = re.compile(r"^ {0,3}(`{3,}|~{3,})(.*)$")


def fenced_blocks(text: str) -> list[tuple[str, int, str]]:
    """Every fenced block in `text` as `(info string, first content line, body)`.

    The info string is what follows the fence (`c`, `bash`, or empty) so a caller can judge only
    the blocks that make the claim it cares about, and the line number lets a finding point at the
    block rather than at the document. A fence closes on the same character, at least as long, with
    nothing else on the line; CommonMark allows up to three spaces of indent. A block whose
    terminator never arrives runs to the end of the document, because a file that lost its closing
    fence still shows what it was describing.
    """
    blocks: list[tuple[str, int, str]] = []
    current: list[str] | None = None
    marker = ""
    info = ""
    start = 0

    for number, line in enumerate(text.splitlines(), start=1):
        opener = FENCE_OPEN.match(line)
        if current is None:
            if opener is not None:
                marker = opener.group(1)
                info = opener.group(2).strip()
                current = []
                start = number + 1
            continue
        if opener is not None and opener.group(1)[0] == marker[0] and \
                len(opener.group(1)) >= len(marker) and not opener.group(2).strip():
            blocks.append((info, start, "\n".join(current)))
            current = None
            marker = ""
            info = ""
            continue
        current.append(line)
    if current is not None:
        blocks.append((info, start, "\n".join(current)))
    return blocks
