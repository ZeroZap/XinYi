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

The scan is a character walk rather than a regular expression on purpose. A regex cannot
tell a `//` inside a string literal from a comment, and this helper had exactly that bug:
`const char *s = "a // b"` lost its value. Dropping text from a literal is a false
negative in the direction that matters, so the states are tracked explicitly.
"""

from __future__ import annotations


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
