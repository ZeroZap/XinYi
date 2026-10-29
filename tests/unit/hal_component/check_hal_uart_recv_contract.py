#!/usr/bin/env python3
"""xy_hal_uart_recv means the same thing on every backend, and the header says what.

Two divergences were live at once:

  * STM32 handed `timeout` to the vendor receive call, which runs one deadline for the whole
    transfer. WCH waited `timeout` per byte, so the same call with len=4 and a 10 ms timeout
    could block for 40 ms there and about 10 ms elsewhere. Nothing in the header said which
    one callers should expect, so neither backend could be called wrong.
  * `len == 0` was rejected by the STM32 backends and accepted by WCH, which turned a caller
    mistake into a silent success on one platform only.

The contract is now written down in xy_hal_uart.h - timeout is the budget for the whole
transfer, zero length is INVALID_PARAM - and this checks the backends against it.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
HEADER = ROOT / "components" / "hal" / "inc" / "xy_hal_uart.h"

BACKENDS = {
    "stm32l4": ROOT / "components/hal/stm32/stm32l4/xy_hal_uart.c",
    "stm32u5": ROOT / "components/hal/stm32/stm32u5/xy_hal_uart.c",
    "stm32f4": ROOT / "components/hal/stm32/stm32f4/xy_hal_uart.c",
    "wch": ROOT / "components/hal/wch/ch32x/src/xy_hal_uart.c",
}

# A per-byte wait hands the bare timeout to each iteration: the defect this guard exists for.
PER_BYTE_WAIT = re.compile(r"\w*wait\w*\s*\([^;]*?\btimeout\b[^;]*\)", re.S)
LOOP = re.compile(r"\b(for|while)\s*\(")
# Evidence of a single deadline: the vendor takes the timeout once, or the body derives a
# remaining budget from a start tick.
VENDOR_TAKES_TIMEOUT = re.compile(r"(?:HAL_UART_Receive|xy_hal_uart_recv_vendor)\s*\([^;]*\btimeout\b",
                                  re.S)
DERIVED_BUDGET = re.compile(r"\b(remaining|left|budget|deadline|elapsed)\b")


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


def body_findings(body: str, where: str) -> list[str]:
    problems: list[str] = []
    if not body:
        return [f"{where}: xy_hal_uart_recv is missing"]

    if not re.search(r"\blen\s*==\s*0U?\b", body):
        problems.append(f"{where}: zero length is not rejected; a zero-length receive is a "
                        f"caller mistake, not a success")
    elif "XY_HAL_ERROR_INVALID_PARAM" not in body:
        problems.append(f"{where}: zero length is checked but not reported as INVALID_PARAM")

    loops = LOOP.search(body)
    per_byte = PER_BYTE_WAIT.search(body)
    if per_byte and loops:
        # Ordering is what matters: a wait helper inside the loop that receives the caller's
        # timeout restarts the clock for every byte.
        problems.append(
            f"{where}: waits on {per_byte.group(0).split('(')[0].strip()} with the caller's "
            f"timeout inside a loop, so every byte gets the full budget and one call can block "
            f"len times longer than the documented contract")

    if not (VENDOR_TAKES_TIMEOUT.search(body) or DERIVED_BUDGET.search(body)):
        problems.append(f"{where}: the timeout is neither handed to the vendor once nor "
                        f"converted into a remaining budget; the transfer has no single deadline")
    return problems


def header_findings(header: str) -> list[str]:
    problems: list[str] = []
    if "budget for the whole transfer" not in header:
        problems.append("xy_hal_uart.h: the timeout semantics are not stated (expected the "
                        "budget-for-the-whole-transfer wording next to xy_hal_uart_recv)")
    if "per byte" not in header:
        problems.append("xy_hal_uart.h: the header does not say that per-byte waiting is wrong")
    if not re.search(r"len must be non-zero|zero-length", header):
        problems.append("xy_hal_uart.h: zero length behaviour is not stated")
    return problems


def findings(backends: dict[str, Path] | None = None, header: Path | None = None) -> list[str]:
    backends = BACKENDS if backends is None else backends
    header = HEADER if header is None else header
    problems = header_findings(header.read_text(encoding="utf-8"))
    for name, path in sorted(backends.items()):
        text = path.read_text(encoding="utf-8")
        problems.extend(body_findings(function_body(text, "xy_hal_uart_recv("), name))
    return problems


def main() -> int:
    problems = findings()
    if problems:
        print("hal_uart_recv_contract failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    print(f"hal_uart_recv_contract ok ({len(BACKENDS)} backends, one deadline each)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
