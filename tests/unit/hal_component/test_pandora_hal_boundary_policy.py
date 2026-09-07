#!/usr/bin/env python3
"""Fail-closed policy for direct STM32 HAL calls in Pandora board sources."""

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
BOARD = ROOT / "boards" / "pandora_stm32l475"
OWNER = BOARD / "pandora_platform_startup.c"
DOC = ROOT / "docs" / "design" / "pandora-board-hal-boundary.md"

HAL_CALL = re.compile(r"(?<![A-Za-z0-9_])(HAL_[A-Za-z0-9_]+)\s*\(")
ALLOWED_OWNER_CALLS = {
    "HAL_PWREx_ControlVoltageScaling",
    "HAL_RCC_OscConfig",
    "HAL_RCC_ClockConfig",
}


def main() -> None:
    assert BOARD.is_dir(), f"missing board directory: {BOARD.relative_to(ROOT)}"

    seen_owner_calls: set[str] = set()
    for source in sorted(BOARD.rglob("*.c")):
        calls = set(HAL_CALL.findall(source.read_text(encoding="utf-8")))
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

    assert DOC.is_file(), f"missing architecture boundary document: {DOC.relative_to(ROOT)}"
    doc = DOC.read_text(encoding="utf-8")
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
