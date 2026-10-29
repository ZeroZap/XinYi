#!/usr/bin/env python3
"""A HAL entry point may report success only if it says why doing nothing is correct.

Two shapes were in the tree. Legitimate: `xy_hal_adc_calibrate` on F4 - the part has no
calibration procedure, and the code says so. Illegitimate: `xy_hal_spi_set_cs` on U5 returned
XY_HAL_OK after discarding both parameters, so a caller that checked the status believed CS had
been driven while nothing was wired to it. The other backends refuse that call.

The difference is not visible in the code shape - both are `XY_UNUSED(x); return XY_HAL_OK;`. It
is a decision: is this call genuinely unnecessary here, or is it unimplemented? So each one is
declared with a reason, keyed by (path, function) so the same name elsewhere is not covered, and
every declaration is printed on each run. An entry that no longer describes the code is stale and
fails, which is what stops the list from quietly outliving its reasons.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))
from source_scan import read_code  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
HAL = ROOT / "components" / "hal"

SIGNATURE = re.compile(r"^(?:static\s+)?(?:xy_hal_error_t|xy_error_t)\s+(xy_hal_\w+)\s*\(", re.M)
XY_UNUSED = re.compile(r"XY_UNUSED\s*\(")
RETURNS_OK = re.compile(r"return\s+(?:XY_HAL_OK|XY_ERROR_OK)\s*;")

# (path relative to the repo, function) -> why doing nothing is correct here.
JUSTIFIED_NO_OPS = {
    ("components/hal/stm32/stm32f4/xy_hal_adc.c", "xy_hal_adc_calibrate"):
        "the F4 ADC has no calibration procedure; the call exists for API parity with ports "
        "whose part does have one",
    ("components/hal/stm32/stm32f4/xy_hal_exti.c", "xy_hal_exti_map_gpio"):
        "the GPIO-to-EXTI multiplexer is configured by HAL_GPIO_Init when the pin mode includes "
        "GPIO_MODE_IT_*, which every caller has already done",
    ("components/hal/stm32/stm32u5/xy_hal_exti.c", "xy_hal_exti_map_gpio"):
        "same as F4: the EXTI source multiplexer is set by HAL_GPIO_Init, so there is no separate "
        "mapping step on U5",
}


def function_body(source: str, start: int) -> str:
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


def is_success_stub(body: str) -> bool:
    """Every statement is a discarded parameter, and the function still reports success."""
    if not RETURNS_OK.search(body) or not XY_UNUSED.search(body):
        return False
    statements = [line.strip() for line in body.strip("{}").split("\n") if line.strip()]
    for statement in statements:
        if XY_UNUSED.search(statement) or RETURNS_OK.search(statement):
            continue
        return False
    return True


def stubs() -> list[tuple[str, str, str]]:
    """(path, function, body) for every HAL entry point that succeeds without doing anything."""
    found: list[tuple[str, str, str]] = []
    for path in sorted(HAL.rglob("*.c")):
        source = read_code(path)
        relative = path.relative_to(ROOT).as_posix()
        for match in SIGNATURE.finditer(source):
            body = function_body(source, match.start())
            if body and is_success_stub(body):
                found.append((relative, match.group(1), body))
    return found


def findings(found=None, registry=None) -> list[str]:
    found = stubs() if found is None else found
    registry = JUSTIFIED_NO_OPS if registry is None else registry
    problems: list[str] = []
    seen = set()
    for relative, function, body in found:
        key = (relative, function)
        seen.add(key)
        if key not in registry:
            problems.append(
                f"{relative}: {function} reports XY_HAL_OK while discarding its parameters, and "
                f"nothing says why that is correct here - implement it, refuse the call with "
                f"NOT_SUPPORTED, or declare the reason in JUSTIFIED_NO_OPS")
        elif not registry[key].strip():
            problems.append(f"{relative}: {function}: the justification is empty")
    for key in registry:
        if key not in seen:
            problems.append(
                f"{key[0]}: {key[1]} is declared a justified no-op but no longer is one; the "
                f"declaration is stale and its reason is no longer being checked")
    return problems


def main() -> int:
    problems = findings()
    for (relative, function), reason in sorted(JUSTIFIED_NO_OPS.items()):
        print(f"note: justified no-op {relative}: {function} - {reason}")
    if problems:
        print("hal_stub_returns failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    print("hal_stub_returns ok (no HAL entry point reports success without a reason)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
