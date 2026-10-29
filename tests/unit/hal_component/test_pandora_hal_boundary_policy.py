#!/usr/bin/env python3
"""Fail-closed policy for direct STM32 HAL calls in Pandora board sources."""

import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))
from source_scan import read_code, read_raw  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
BOARD = ROOT / "boards" / "pandora_stm32l475"
OWNER = BOARD / "pandora_platform_startup.c"
DOC = ROOT / "docs" / "design" / "pandora-board-hal-boundary.md"
UNIT_HAL = ROOT / "tests" / "unit" / "hal_component"
UNIT_CMAKE = ROOT / "tests" / "unit" / "CMakeLists.txt"
FAKE_VENDOR = UNIT_HAL / "fake_vendor" / "stm32l4xx_hal.h"
HOST_CONTRACT = UNIT_HAL / "test_pandora_platform_startup.c"

HAL_CALL = re.compile(r"(?<![A-Za-z0-9_])(HAL_[A-Za-z0-9_]+)\s*\(")
ALLOWED_OWNER_CALLS = {
    "HAL_PWREx_ControlVoltageScaling",
    "HAL_RCC_OscConfig",
    "HAL_RCC_ClockConfig",
}


def assert_host_clock_contract() -> None:
    """The startup owner's only host evidence must stay on the fake vendor seam."""
    for path in (FAKE_VENDOR, HOST_CONTRACT):
        assert path.is_file(), f"missing {path.relative_to(ROOT)}"

    fake = read_code(FAKE_VENDOR)
    for symbol in (
        *sorted(ALLOWED_OWNER_CALLS),
        "__HAL_RCC_PWR_CLK_ENABLE",
        "PWR_REGULATOR_VOLTAGE_SCALE1",
        "FLASH_LATENCY_4",
        "RCC_PLLSOURCE_HSE",
        "RCC_ClkInitTypeDef",
    ):
        assert symbol in fake, f"{FAKE_VENDOR.relative_to(ROOT)} missing {symbol!r}"

    contract = read_code(HOST_CONTRACT)
    for token in (
        "pandora_platform_startup()",
        "PWR_REGULATOR_VOLTAGE_SCALE1",
        "RCC_PLLSOURCE_HSE",
        "FLASH_LATENCY_4",
        "PANDORA_HSE_HZ 8000000U",
    ):
        assert token in contract, f"{HOST_CONTRACT.relative_to(ROOT)} missing {token!r}"

    cmake = read_raw(UNIT_CMAKE)
    anchor = "xy_add_unit_test(test_pandora_platform_startup pandora_platform_startup UNITY"
    start = cmake.find(anchor)
    assert start >= 0, "tests/unit/CMakeLists.txt does not register the startup host contract"
    end = cmake.find("\n# ---", start)
    block = cmake[start : end if end > start else len(cmake)]
    assert "pandora_platform_startup.c" in block, (
        "startup host contract no longer compiles the board startup owner"
    )
    seam_include = (
        "target_include_directories(test_pandora_platform_startup BEFORE PRIVATE\n"
        "    ${UNIT_HAL}/fake_vendor\n"
        ")"
    )
    board_include = (
        "target_include_directories(test_pandora_platform_startup PRIVATE\n"
        "    ${ROOT}/boards/pandora_stm32l475"
    )
    assert seam_include in block, (
        "startup host contract no longer puts the fake vendor seam on the BEFORE include path"
    )
    assert board_include in block, (
        "startup host contract no longer includes the board directory"
    )
    assert block.index(seam_include) < block.index(board_include), (
        "the fake vendor seam must precede the board include so the SDK header never wins"
    )


def main() -> None:
    assert BOARD.is_dir(), f"missing board directory: {BOARD.relative_to(ROOT)}"

    seen_owner_calls: set[str] = set()
    for source in sorted(BOARD.rglob("*.c")):
        calls = set(HAL_CALL.findall(read_code(source)))
        if source == OWNER:
            unexpected = calls - ALLOWED_OWNER_CALLS
            assert not unexpected, (
                f"{source.relative_to(ROOT)} has non-whitelisted direct HAL calls: "
                f"{sorted(unexpected)}"
            )
            seen_owner_calls = calls
        else:
            assert not calls, (
                f"{source.relative_to(ROOT)} bypasses xy_hal with direct HAL calls: {sorted(calls)}"
            )

    assert seen_owner_calls == ALLOWED_OWNER_CALLS, (
        "Pandora startup HAL whitelist drift: "
        f"expected={sorted(ALLOWED_OWNER_CALLS)} actual={sorted(seen_owner_calls)}"
    )

    assert_host_clock_contract()

    assert DOC.is_file(), f"missing architecture boundary document: {DOC.relative_to(ROOT)}"
    doc = read_raw(DOC)
    for token in (
        "pandora_platform_startup.c",
        "HAL_PWREx_ControlVoltageScaling",
        "HAL_RCC_OscConfig",
        "HAL_RCC_ClockConfig",
        "application → component → xy_hal_* → platform backend",
        "唯一的 board-local vendor clock owner",
        "不构成新的 B1/B2",
    ):
        assert token in doc, f"{DOC.relative_to(ROOT)} missing {token!r}"


if __name__ == "__main__":
    main()
