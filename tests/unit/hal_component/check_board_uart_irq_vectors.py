#!/usr/bin/env python3
"""Every board interrupt vector for a UART must forward to the HAL, not swallow it.

The HAL can arm a receive interrupt correctly - peripheral: yes, NVIC: yes, byte stashed
and re-armed - and still collect nothing, because the vector that the NVIC actually calls
lives in the board. A board that writes

    void USART1_IRQHandler(void) { /* does its own thing */ }

takes the interrupt away from the HAL silently. Nothing fails to compile, the interrupt
fires, and the HAL's callback never runs.

So two rules, checked against real sources:

  1. Every UART vector defined anywhere must forward to the HAL (HAL_UART_IRQHandler or
     xy_hal_uart_irq_handler).
  2. A board that arms the receive interrupt must have a vector reachable - either one it
     defines, or the backend's opt-in vectors via XY_HAL_UART_PROVIDE_IRQ_VECTORS.

The judgement is kept in pure functions so tests/unit/hal_component/test_board_uart_irq_vectors.py
can drive them with synthetic boards, including the failing shapes. A guard that can only be
run against the repository can only be observed passing.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]

UART_VECTOR = re.compile(
    r"void\s+(USART\d+|UART\d+|LPUART\d+)_IRQHandler\s*\(\s*void\s*\)", re.M)
FORWARD_MARKERS = ("HAL_UART_IRQHandler", "xy_hal_uart_irq_handler")
ARM_PATTERN = re.compile(r"xy_hal_uart_enable_rx_irq\s*\([^;]*?,\s*([^)]*)\)", re.S)


def body_after(source: str, match: re.Match) -> str:
    brace = source.find("{", match.end())
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


def vectors_in(source: str) -> dict[str, str]:
    """Vector name → body, for each UART vector defined in this source."""
    found: dict[str, str] = {}
    for match in UART_VECTOR.finditer(source):
        name = f"{match.group(1)}_IRQHandler"
        found[name] = body_after(source, match)
    return found


def vector_findings(vectors: dict[str, str], where: str) -> list[str]:
    findings: list[str] = []
    for name, body in sorted(vectors.items()):
        if not body:
            findings.append(f"{where}: {name} is declared but has no body to inspect")
        elif not any(marker in body for marker in FORWARD_MARKERS):
            findings.append(
                f"{where}: {name} does not forward to the HAL, so the interrupt is taken "
                f"away from the HAL silently - it still fires, the HAL callback never runs")
    return findings


def arming_findings(sources: dict[str, str], vectors_present: set[str],
                    provides_macro: bool, where: str) -> list[str]:
    """A board that arms the receive interrupt needs a vector that reaches the HAL."""
    arms = False
    for source in sources.values():
        for argument in ARM_PATTERN.findall(source):
            if argument.strip() not in ("0", ""):
                arms = True
    if not arms:
        return []
    if vectors_present or provides_macro:
        return []
    return [f"{where}: arms the UART receive interrupt but no UART vector exists in this "
            f"board and XY_HAL_UART_PROVIDE_IRQ_VECTORS is not defined - the NVIC line "
            f"fires into the startup file's default handler and no byte is collected"]


def scan_repository(root: Path = ROOT) -> list[str]:
    findings: list[str] = []
    boards_dir = root / "boards"
    for board in sorted(path for path in boards_dir.iterdir() if path.is_dir()):
        sources = {path.name: path.read_text(encoding="utf-8", errors="replace")
                   for path in board.glob("*.c")}
        vectors: dict[str, str] = {}
        for name, source in sources.items():
            local = vectors_in(source)
            findings.extend(vector_findings(local, f"{board.name}/{name}"))
            vectors.update(local)

        cmake = board / "CMakeLists.txt"
        cmake_text = cmake.read_text(encoding="utf-8") if cmake.is_file() else ""
        findings.extend(arming_findings(sources, set(vectors),
                                        "XY_HAL_UART_PROVIDE_IRQ_VECTORS" in cmake_text,
                                        board.name))
    return findings


def main() -> int:
    findings = scan_repository()
    if findings:
        print("board_uart_irq_vectors failed:")
        for finding in findings:
            print(f"- {finding}")
        return 1
    print("board_uart_irq_vectors ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
