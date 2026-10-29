#!/usr/bin/env python3
"""Every interrupt vector must hand the interrupt to the HAL, or say why it does not.

The HAL can arm a peripheral correctly and still collect nothing, because the vector the
NVIC actually calls lives in the board. A vector that handles things itself takes the
interrupt away from the HAL silently: it compiles, the interrupt fires, the HAL callback
never runs.

Three ways to be correct, in order of how little explanation they need:

  * the vector lives in the HAL implementation file for its own peripheral - nothing is
    being bypassed, so no claim has to be written down;
  * it forwards to the HAL, directly or through a dispatcher defined in the same file;
  * it is listed in BOUNDARY_VECTORS, keyed by file and name, with a reason.

Anything else fails. Comments are stripped before scanning, so a vector that only appears
inside a doc comment in an example is not mistaken for code - there is one of those in the
tree, and it would have been reported as a board that swallows its interrupt.

Kept as pure functions with synthetic cases in test_board_irq_vectors.py: run only against
the repository, a guard like this can only ever be observed passing.
"""

from __future__ import annotations

import functools
import os
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))
from source_scan import code_only, read_code, read_hash_code  # noqa: E402

VECTOR = re.compile(r"void\s+([A-Z][A-Z0-9_]*_IRQHandler)\s*\(\s*void\s*\)", re.M)

# Vector family → the calls that count as handing the interrupt on.
FAMILIES = (
    (re.compile(r"^(USART|UART|LPUART)\d+_IRQHandler$"),
     ("HAL_UART_IRQHandler", "xy_hal_uart_irq_handler")),
    (re.compile(r"^TIM\d+(_DAC)?_IRQHandler$"), ("xy_hal_timer_irq_handler",)),
    (re.compile(r"^DMA\d+_Channel\d+_IRQHandler$"), ("xy_hal_dma_irq_handler",)),
    (re.compile(r"^EXTI\d+(_\d+)?_IRQHandler$"), ("xy_hal_gpio_irq_handler",)),
)

# A HAL implementation file's stem, and the peripheral names its own vectors use.
PERIPHERAL_ALIASES = {
    "timer": ("TIM",), "rtc": ("RTC",), "wdg": ("WWDG", "IWDG"), "gpio": ("EXTI",),
    "uart": ("UART", "USART"), "i2c": ("I2C",), "spi": ("SPI",), "adc": ("ADC",),
    "dac": ("DAC",), "dma": ("DMA",), "rng": ("RNG",), "flash": ("FLASH",),
}

# Board-owned interrupts. Keyed by (path relative to the repository, vector name): a name
# alone would exempt every board that happens to define the same vector, including ones
# where the same name means something else.
BOUNDARY_VECTORS = {
    ("boards/opench_ch32v307/freertos_sdk_port_main.c", "TIM2_IRQHandler"):
        "port demo: TIM2 is this board's own tick source and the ISR is what is being "
        "demonstrated",
    ("boards/opench_ch32v307/osal_freertos_sdk_main.c", "TIM2_IRQHandler"):
        "port demo: TIM2 is this board's own tick source and the ISR is what is being "
        "demonstrated",
    ("boards/opench_ch32v307/osal_freertos_isr_stress_main.c", "TIM2_IRQHandler"):
        "ISR stress demo: the interrupt body is the subject of the test",
}

ARM_PATTERN = re.compile(r"xy_hal_uart_enable_rx_irq\s*\([^;]*?,\s*([^)]*)\)", re.S)


def body_after(source: str, brace_index: int) -> str:
    if brace_index < 0:
        return ""
    depth = 0
    for index in range(brace_index, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[brace_index:index + 1]
    return ""


def vectors_in(source: str) -> dict[str, str]:
    """Vector name → body, for each interrupt vector defined in this source."""
    found: dict[str, str] = {}
    for match in VECTOR.finditer(source):
        found[match.group(1)] = body_after(source, source.find("{", match.end()))
    return found


def function_body(source: str, name: str) -> str:
    match = re.search(rf"\b{name}\s*\([^;{{]*\)\s*\{{", source)
    return body_after(source, match.end() - 1) if match else ""


def required_markers(name: str) -> tuple[str, ...] | None:
    for pattern, markers in FAMILIES:
        if pattern.match(name):
            return markers
    return None


def owned_by_its_hal(relative_path: str, name: str) -> bool:
    """True when the vector sits in the HAL implementation file of its own peripheral."""
    parts = Path(relative_path).parts
    if "components" not in parts or "hal" not in parts:
        return False
    stem = Path(relative_path).stem
    token = stem[len("xy_hal_"):] if stem.startswith("xy_hal_") else stem
    peripherals = PERIPHERAL_ALIASES.get(token, (token.upper(),))
    return any(peripheral in name for peripheral in peripherals)


def forwarding_text(source: str, body: str) -> str:
    """The vector body plus one hop into local helpers it calls.

    A vector that calls a dispatcher in the same file is forwarding, and the vendor handler
    lives in the dispatcher. One hop, because more than that is obfuscation.
    """
    text = body
    for candidate in set(re.findall(r"\b([a-z_][a-z0-9_]*)\s*\(", body)):
        helper = function_body(source, candidate)
        if helper and helper != body:
            text += helper
    return text


def vector_findings(source: str, where: str, relative_path: str,
                    boundary: dict[tuple[str, str], str] | None = None) -> list[str]:
    boundary = BOUNDARY_VECTORS if boundary is None else boundary
    code = code_only(source)
    findings: list[str] = []
    for name, body in sorted(vectors_in(code).items()):
        if not body:
            findings.append(f"{where}: {name} is declared but has no body to inspect")
            continue
        if owned_by_its_hal(relative_path, name):
            continue
        if (relative_path, name) in boundary:
            if not boundary[(relative_path, name)].strip():
                findings.append(f"{where}: {name} is listed as a boundary vector without a "
                                f"reason, which is the same as not listing it")
            continue
        markers = required_markers(name)
        text = forwarding_text(code, body)
        if markers is None:
            if "xy_hal_" not in text and "HAL_" not in text:
                findings.append(f"{where}: {name} neither forwards to the HAL nor declares a "
                                f"boundary reason; an unrecognised family must still hand the "
                                f"interrupt on")
            continue
        if not any(marker in text for marker in markers):
            findings.append(
                f"{where}: {name} does not hand the interrupt to the HAL (expected one of "
                f"{', '.join(markers)}), so it is taken away from the HAL silently")
    return findings


def arming_findings(sources: dict[str, str], vectors_present: set[str],
                    provides_macro: bool, where: str) -> list[str]:
    """A board that arms the UART receive interrupt needs a vector that reaches the HAL."""
    arms = False
    for source in sources.values():
        for argument in ARM_PATTERN.findall(code_only(source)):
            if argument.strip() not in ("0", ""):
                arms = True
    if not arms or vectors_present or provides_macro:
        return []
    return [f"{where}: arms the UART receive interrupt but no UART vector exists in this "
            f"board and XY_HAL_UART_PROVIDE_IRQ_VECTORS is not defined - the NVIC line "
            f"fires into the startup file's default handler and no byte is collected"]


SKIP_DIRS = {".git", "build", "cmake-build-debug", "__pycache__", ".cache"}


def walk_sources(root: Path):
    """Yield (path, text) for build files and C sources, skipping whole work directories.

    Two full rglob passes over a tree this size cost seconds per run, and a slow guard is a
    guard that gets taken out of the suite.
    """
    cmakes: dict[str, str] = {}
    sources: list[tuple[Path, str]] = []
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [name for name in dirnames
                       if name not in SKIP_DIRS and not name.startswith("build-")]
        for name in filenames:
            path = Path(dirpath) / name
            if name == "CMakeLists.txt" or name.endswith(".cmake"):
                try:
                    cmakes[str(path)] = read_hash_code(path)
                except OSError:
                    continue
            elif name.endswith(".c"):
                sources.append((path, ""))
    return cmakes, sources


@functools.lru_cache(maxsize=4)
def named_sources(root: Path = ROOT) -> set[str]:
    """Basenames of sources that some CMakeLists lists by name.

    A file nobody names may still be built through file(GLOB ...), and this tree has many
    of those. So "not named" is not proof of "not built" - such files are reported as
    unchecked rather than quietly passing or quietly failing.
    """
    names: set[str] = set()
    cmakes, _sources = walk_sources(root)
    for text in cmakes.values():
        names.update(re.findall(r"[A-Za-z0-9_.+-]+\.c\b", text))
    return names


@functools.lru_cache(maxsize=4)
def _sources_to_scan(root: Path) -> tuple[tuple[Path, ...], tuple[Path, ...]]:
    """(checked, unchecked) - board sources are always checked, components need a reference."""
    named = named_sources(root)
    checked: list[Path] = []
    unchecked: list[Path] = []
    _cmakes, sources = walk_sources(root)
    for path, _text in sorted(sources):
        parts = path.relative_to(root).parts
        if parts[0] == "boards":
            checked.append(path)
        elif parts[0] == "components" and "third_party" not in parts:
            (checked if path.name in named else unchecked).append(path)
    return tuple(checked), tuple(unchecked)


def sources_to_scan(root: Path = ROOT) -> tuple[list[Path], list[Path]]:
    checked, unchecked = _sources_to_scan(root)
    return list(checked), list(unchecked)


def scan_repository(root: Path = ROOT) -> list[str]:
    findings: list[str] = []
    checked, _unchecked = sources_to_scan(root)
    for path in checked:
        relative = path.relative_to(root).as_posix()
        findings.extend(vector_findings(read_code(path),
                                        relative, relative))

    for board in sorted(path for path in (root / "boards").iterdir() if path.is_dir()):
        sources = {path.name: read_code(path)
                   for path in board.glob("*.c")}
        vectors: set[str] = set()
        for source in sources.values():
            vectors.update(vectors_in(code_only(source)))
        cmake = board / "CMakeLists.txt"
        cmake_text = read_hash_code(cmake) if cmake.is_file() else ""
        findings.extend(arming_findings(sources, vectors,
                                        "XY_HAL_UART_PROVIDE_IRQ_VECTORS" in cmake_text,
                                        board.name))
    return findings


def boundary_report(root: Path = ROOT) -> list[str]:
    """Print the declared boundaries, including any that no longer match a real vector."""
    report: list[str] = []
    for (relative, name), reason in sorted(BOUNDARY_VECTORS.items()):
        path = root / relative
        present = path.is_file() and name in vectors_in(
            read_code(path))
        if present:
            report.append(f"{relative}: {name} (boundary: {reason.split(':')[0]})")
        else:
            report.append(f"{relative}: {name} (declared, but no such vector found - the "
                          f"exemption is stale)")
    return report


def unchecked_vectors(root: Path = ROOT) -> list[str]:
    """Vectors in sources no CMakeLists names, so they are neither passed nor failed."""
    notes: list[str] = []
    _checked, unchecked = sources_to_scan(root)
    for path in unchecked:
        relative = path.relative_to(root).as_posix()
        for name in sorted(vectors_in(code_only(
                read_code(path)))):
            notes.append(f"not checked (no CMakeLists names this file): {relative}: {name}")
    return notes


def main() -> int:
    findings = scan_repository()
    for line in unchecked_vectors():
        print(f"note: {line}")
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
