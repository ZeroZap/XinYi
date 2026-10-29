#!/usr/bin/env python3
"""The blocking UART calls mean the same thing on every backend, and the header says what.

`xy_hal_uart_send` and `xy_hal_uart_recv` each had two meanings at once:

  * STM32 handed `timeout` to the vendor call, which runs one deadline for the whole transfer.
    WCH restarted the caller's timeout on every byte - and for send, again for the final
    transmit-complete wait. The same call could block len times longer on one backend.
  * `len == 0` was rejected by the STM32 backends and accepted by WCH, turning a caller mistake
    into a silent success on one platform only.

The contract is now in xy_hal_uart.h: the timeout is the budget for the whole transfer (zero
means wait indefinitely), and zero length or a length beyond UINT16_MAX is INVALID_PARAM.

One trap belongs to the deadline form and gets its own rule. A backend that derives a
"remaining" budget and hands it to a wait helper turns an exhausted budget into a zero - and
zero is how these helpers spell "wait forever". Running out of time becomes waiting for a byte
that may never arrive. A derived budget has to be checked and reported as XY_HAL_ERROR_TIMEOUT
before it is passed on.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))
from source_scan import read_code, read_raw, with_delegation  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
HEADER = ROOT / "components" / "hal" / "inc" / "xy_hal_uart.h"

BACKENDS = {
    "stm32l4": ROOT / "components/hal/stm32/stm32l4/xy_hal_uart.c",
    "stm32u5": ROOT / "components/hal/stm32/stm32u5/xy_hal_uart.c",
    "stm32f4": ROOT / "components/hal/stm32/stm32f4/xy_hal_uart.c",
    "wch": ROOT / "components/hal/wch/ch32x/src/xy_hal_uart.c",
    # The host backend models the same contract; it must be held to it too.
    "pc": ROOT / "components/hal/PC/xy_hal_uart_pc.c",
}

FUNCTIONS = ("xy_hal_uart_send(", "xy_hal_uart_recv(")

# The defect is passing the caller's timeout *itself*, not an expression that mentions it:
# uart_wait(instance, FLAG, uart_budget_remaining(start, timeout)) is the fix, not the bug.
PER_BYTE_WAIT = re.compile(r"\b\w*wait\w*\s*\([^;()]*,\s*timeout\s*\)", re.S)
LOOP = re.compile(r"\b(for|while)\s*\(")
VENDOR_TAKES_TIMEOUT = re.compile(
    r"(?:HAL_UART_(?:Transmit|Receive)|HAL_UART_IRQHandler)\s*\([^;]*\btimeout\b", re.S)
# No \b around these fragments: `\bbudget\b` does not match `uart_budget_remaining`,
# because `_` and `b` are both word characters. This has now cost four false alarms in this
# tree, twice in guards written the same evening as the note about it.
DERIVED = re.compile(r"(remaining|budget|deadline|elapsed)")


def function_body(source: str, signature: str) -> str:
    start = source.find(signature)
    if start < 0:
        return ""
    brace = source.find("{", start)
    if brace < 0:
        return ""
    depth = 0
    for index in range(brace, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[brace:index + 1]
    return ""


def loop_body(body: str) -> str:
    """The brace-balanced body of the first for/while loop in `body`."""
    match = LOOP.search(body)
    if not match:
        return ""
    brace = body.find("{", match.end())
    if brace < 0:
        return ""
    depth = 0
    for index in range(brace, len(body)):
        if body[index] == "{":
            depth += 1
        elif body[index] == "}":
            depth -= 1
            if depth == 0:
                return body[brace:index + 1]
    return ""


# A backend that never blocks has no deadline to split, and the honest way to say so is to
# discard the value where it would be used and write down why. The reason has to come from the raw
# text: read_code strips comments, so a comment-based rule cannot use the code view.
NEVER_BLOCKS = re.compile(r"never blocks")


def body_findings(body: str, where: str, raw: str | None = None) -> list[str]:
    problems: list[str] = []
    if not body:
        return [f"{where}: missing"]

    if not re.search(r"\blen\s*==\s*0U?\b", body):
        problems.append(f"{where}: zero length is not rejected; it is a caller mistake, not a "
                        f"success")
    elif "XY_HAL_ERROR_INVALID_PARAM" not in body:
        problems.append(f"{where}: zero length is checked but not reported as INVALID_PARAM")

    if "UINT16_MAX" not in body:
        problems.append(f"{where}: the documented UINT16_MAX length limit is not enforced")

    inner = loop_body(body)
    per_byte = PER_BYTE_WAIT.search(inner) if inner else None
    if per_byte:
        problems.append(
            f"{where}: hands the caller's timeout to {per_byte.group(0).split('(')[0].strip()} "
            f"inside a loop, so every byte restarts the clock and one call can block len times "
            f"longer than the documented budget")

    if not (VENDOR_TAKES_TIMEOUT.search(body) or DERIVED.search(body)):
        blocked = "(void)timeout" in body and raw is not None and NEVER_BLOCKS.search(raw)
        if not blocked:
            problems.append(
                f"{where}: the timeout is neither handed to the vendor once nor turned into a "
                f"remaining budget, and nothing says which - a backend that never blocks has to "
                f"discard the value and say why, with read_code stripping the note")

    if inner and DERIVED.search(inner):
        wait = re.search(r"\b\w*wait\w*\s*\(", inner)
        if "XY_HAL_ERROR_TIMEOUT" not in inner:
            problems.append(
                f"{where}: derives a remaining budget inside the loop but never reports an "
                f"exhausted one; a zero budget reads as 'wait forever'")
        elif wait is not None and inner.index("XY_HAL_ERROR_TIMEOUT") > wait.start():
            problems.append(
                f"{where}: the derived budget is passed to the wait before the exhausted case "
                f"is reported as TIMEOUT; a zero budget reads as 'wait forever'")
    return problems


def header_findings(header: str) -> list[str]:
    problems: list[str] = []
    if header.count("budget for the whole transfer") < 2:
        problems.append("xy_hal_uart.h: both blocking calls must state that the timeout is the "
                        "budget for the whole transfer")
    if "per byte" not in header:
        problems.append("xy_hal_uart.h: the header does not say that per-byte waiting is wrong")
    if not re.search(r"len must be non-zero|zero-length", header):
        problems.append("xy_hal_uart.h: zero length behaviour is not stated")
    if "UINT16_MAX" not in header:
        problems.append("xy_hal_uart.h: the length limit is not stated")
    return problems


def findings(backends: dict[str, Path] | None = None, header: Path | None = None) -> list[str]:
    backends = BACKENDS if backends is None else backends
    header = HEADER if header is None else header
    problems = header_findings(read_raw(header))
    for name, path in sorted(backends.items()):
        text = read_code(path)
        raw = read_raw(path)
        for function in FUNCTIONS:
            body = with_delegation(text, function_body(text, function))
            problems.extend(body_findings(body, f"{name}:{function.rstrip('(')}", raw))
    return problems


def main() -> int:
    problems = findings()
    if problems:
        print("hal_uart_blocking_contract failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    print(f"hal_uart_blocking_contract ok ({len(BACKENDS)} backends x {len(FUNCTIONS)} calls, "
          f"one deadline each)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
