#!/usr/bin/env python3
"""Board interrupt vectors must hand the interrupt to the HAL, or say why not.

The HAL can arm a peripheral correctly and still collect nothing, because the vector the
NVIC actually calls lives in the board. A board writing its own handler takes the interrupt
away from the HAL silently: it compiles, the interrupt fires, the HAL callback never runs.

Two ways to be correct:

  * forward to the HAL - `xy_hal_*_irq_handler` or the vendor handler the HAL wraps;
  * be declared in BOUNDARY_VECTORS with a reason, for interrupts the board genuinely owns.

Anything else fails. The declaration exists so that the decision is visible in one place
instead of being inferred from whichever file happens to define a vector.

Kept as pure functions with synthetic cases in test_board_irq_vectors.py: run only against
the repository, a guard like this can only ever be observed passing.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]

VECTOR = re.compile(r"void\s+([A-Z][A-Z0-9_]*_IRQHandler)\s*\(\s*void\s*\)", re.M)

# Vector family → the calls that count as handing the interrupt on.
FAMILIES = (
    (re.compile(r"^(USART|UART|LPUART)\d+_IRQHandler$"),
     ("HAL_UART_IRQHandler", "xy_hal_uart_irq_handler")),
    (re.compile(r"^TIM\d+(_DAC)?_IRQHandler$"), ("xy_hal_timer_irq_handler",)),
    (re.compile(r"^DMA\d+_Channel\d+_IRQHandler$"), ("xy_hal_dma_irq_handler",)),
    (re.compile(r"^EXTI\d+(_\d+)?_IRQHandler$"), ("xy_hal_gpio_irq_handler",)),
)

# Interrupts the board owns outright. Each entry has to say why, because "the board owns it"
# is the claim being made and it should be readable, not assumed.
BOUNDARY_VECTORS = {
    "TIM2_IRQHandler": (
        "the OSAL port demos use TIM2 as their own tick source and drive it through vendor "
        "registers; the ISR is the thing being demonstrated on those boards, not a component "
        "reaching around the HAL"),
}

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
    """Vector name → body, for each interrupt vector defined in this source."""
    found: dict[str, str] = {}
    for match in VECTOR.finditer(source):
        found[match.group(1)] = body_after(source, match)
    return found


def required_markers(name: str) -> tuple[str, ...] | None:
    for pattern, markers in FAMILIES:
        if pattern.match(name):
            return markers
    return None


def vector_findings(vectors: dict[str, str], where: str,
                    boundary: dict[str, str] | None = None) -> list[str]:
    boundary = BOUNDARY_VECTORS if boundary is None else boundary
    findings: list[str] = []
    for name, body in sorted(vectors.items()):
        if not body:
            findings.append(f"{where}: {name} is declared but has no body to inspect")
            continue
        if name in boundary:
            if not boundary[name].strip():
                findings.append(f"{where}: {name} is declared a boundary vector without a "
                                f"reason, which is the same as not declaring it")
            continue
        markers = required_markers(name)
        if markers is None:
            # Unknown family: any HAL delegation is accepted, silence is not.
            if "xy_hal_" not in body and "HAL_" not in body:
                findings.append(
                    f"{where}: {name} neither forwards to the HAL nor declares a boundary "
                    f"reason; unrecognised vector families must still hand the interrupt on")
            continue
        if not any(marker in body for marker in markers):
            findings.append(
                f"{where}: {name} does not hand the interrupt to the HAL (expected one of "
                f"{', '.join(markers)}), so it is taken away from the HAL silently")
    return findings


def arming_findings(sources: dict[str, str], vectors_present: set[str],
                    provides_macro: bool, where: str) -> list[str]:
    """A board that arms the UART receive interrupt needs a vector that reaches the HAL."""
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
    for board in sorted(path for path in (root / "boards").iterdir() if path.is_dir()):
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


def boundary_report(root: Path = ROOT) -> list[str]:
    """Print the declared boundary vectors so the list stays visible, not just permitted."""
    report: list[str] = []
    for board in sorted(path for path in (root / "boards").iterdir() if path.is_dir()):
        for path in board.glob("*.c"):
            for name in vectors_in(path.read_text(encoding="utf-8", errors="replace")):
                if name in BOUNDARY_VECTORS:
                    report.append(f"{board.name}/{path.name}: {name} (boundary)")
    return report


def main() -> int:
    findings = scan_repository()
    if findings:
        print("board_irq_vectors failed:")
        for finding in findings:
            print(f"- {finding}")
        return 1
    for line in boundary_report():
        print(f"note: {line}")
    print(f"board_irq_vectors ok ({len(BOUNDARY_VECTORS)} declared boundary vector(s))")
    return 0


if __name__ == "__main__":
    sys.exit(main())
