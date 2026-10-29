#!/usr/bin/env python3
"""Fail-closed contract for the STM32F4 DMA wrapper.

The wrapper was deferred from the STM32F4 build for far more than signature
drift: it published ``XY_HAL_DMA_*`` enum values where the vendor HAL expects
its own ``DMA_*`` constants, returned undeclared ``XY_HAL_DMA_PDATAALIGN_*``
names, used one alignment helper for both the peripheral and memory fields,
reused ``DMA_PINC_*`` for the memory increment, and reported success from
``register_callback`` without registering anything.

This is a source-level guard.  It proves the mapping shape, not the electrical
behaviour: the compile evidence for the wrapper is the STM32F4 platform gate and
it remains a source/compile-level claim, never a board result.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))
from source_scan import read_code, read_raw  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
WRAPPER = ROOT / "components" / "hal" / "stm32" / "stm32f4" / "xy_hal_dma.c"
HEADER = ROOT / "components" / "hal" / "inc" / "xy_hal_dma.h"
HAL_CMAKE = ROOT / "components" / "hal" / "CMakeLists.txt"

# helper -> every (adapter enum, vendor constant) pair that helper must translate.
# A helper may use a switch or a ternary; either shape is accepted as long as
# both symbols appear in that helper.
REQUIRED_MAPPINGS = {
    "to_direction": (
        ("XY_HAL_DMA_DIR_PERIPH_TO_MEM", "DMA_PERIPH_TO_MEMORY"),
        ("XY_HAL_DMA_DIR_MEM_TO_PERIPH", "DMA_MEMORY_TO_PERIPH"),
        ("XY_HAL_DMA_DIR_MEM_TO_MEM", "DMA_MEMORY_TO_MEMORY"),
    ),
    "to_mode": (
        ("XY_HAL_DMA_MODE_CIRCULAR", "DMA_CIRCULAR"),
        ("XY_HAL_DMA_MODE_CIRCULAR", "DMA_NORMAL"),
    ),
    "to_priority": (
        ("XY_HAL_DMA_PRIORITY_LOW", "DMA_PRIORITY_LOW"),
        ("XY_HAL_DMA_PRIORITY_MEDIUM", "DMA_PRIORITY_MEDIUM"),
        ("XY_HAL_DMA_PRIORITY_HIGH", "DMA_PRIORITY_HIGH"),
        ("XY_HAL_DMA_PRIORITY_VERY_HIGH", "DMA_PRIORITY_VERY_HIGH"),
    ),
    "to_periph_alignment": (
        ("XY_HAL_DMA_WIDTH_BYTE", "DMA_PDATAALIGN_BYTE"),
        ("XY_HAL_DMA_WIDTH_HALFWORD", "DMA_PDATAALIGN_HALFWORD"),
        ("XY_HAL_DMA_WIDTH_WORD", "DMA_PDATAALIGN_WORD"),
    ),
    "to_mem_alignment": (
        ("XY_HAL_DMA_WIDTH_BYTE", "DMA_MDATAALIGN_BYTE"),
        ("XY_HAL_DMA_WIDTH_HALFWORD", "DMA_MDATAALIGN_HALFWORD"),
        ("XY_HAL_DMA_WIDTH_WORD", "DMA_MDATAALIGN_WORD"),
    ),
    "to_periph_incr": (("XY_HAL_DMA_INCR_ENABLE", "DMA_PINC_ENABLE"),),
    "to_mem_incr": (("XY_HAL_DMA_INCR_ENABLE", "DMA_MINC_ENABLE"),),
}

PUBLIC_ERROR_ENTRY_POINTS = (
    "xy_hal_dma_init",
    "xy_hal_dma_deinit",
    "xy_hal_dma_start",
    "xy_hal_dma_stop",
    "xy_hal_dma_register_callback",
    "xy_hal_dma_poll_complete",
)


def function_body(source: str, signature: str) -> str:
    """Body of the function whose definition starts with `signature`."""
    start = source.find(signature)
    if start < 0:
        return ""
    brace = source.find("{", start)
    if brace < 0:
        return ""
    depth = 0
    for index in range(brace, len(source)):
        char = source[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return source[brace:index + 1]
    return ""


def main() -> int:
    raw = read_raw(WRAPPER)  # the #ifdef check is about presence, not code
    source = read_code(WRAPPER)
    header = read_code(HEADER)
    errors: list[str] = []

    # 1. The wrapper must be compiled by the platform gate, behind the guard.
    if "#ifdef STM32_HAL_ENABLED" not in raw:
        errors.append("STM32F4 DMA wrapper must be wrapped in #ifdef STM32_HAL_ENABLED")
    deferred = re.search(r"set\(XY_HAL_F4_DEFERRED_SOURCES(.*?)\)",
                         read_raw(HAL_CMAKE), re.S)
    if deferred and "xy_hal_dma.c" in deferred.group(1):
        errors.append(
            "STM32F4 branch still lists xy_hal_dma.c as contract-deferred; the migrated wrapper "
            "must be built by the platform gate"
        )

    # 2. Every adapter enum must be translated into a vendor DMA_* constant.
    bodies = {}
    for helper, pairs in REQUIRED_MAPPINGS.items():
        body = function_body(source, f"static uint32_t {helper}(")
        bodies[helper] = body
        if not body:
            errors.append(f"STM32F4 DMA wrapper must define {helper}()")
            continue
        for xy_symbol, dma_symbol in pairs:
            if xy_symbol not in body:
                errors.append(f"{helper}() must switch on the adapter enum {xy_symbol}")
            if dma_symbol not in body:
                errors.append(
                    f"{helper}() must return the vendor constant {dma_symbol} for {xy_symbol}"
                )
        for returned in re.findall(r"return\s+(XY_HAL_DMA_\w+)\s*;", body):
            errors.append(
                f"{helper}() returns the adapter enum {returned} where a vendor DMA_* constant "
                "is required"
            )

    # 3. Peripheral and memory sides are distinct register fields.
    for stale in ("XY_HAL_DMA_PDATAALIGN", "XY_HAL_DMA_MDATAALIGN"):
        if stale in source:
            errors.append(
                f"STM32F4 DMA wrapper still references the undeclared symbol {stale}"
            )
    init_body = function_body(source, "xy_hal_dma_init(")
    if not init_body:
        errors.append("STM32F4 DMA wrapper must define xy_hal_dma_init()")
    else:
        for required in (
            "to_periph_alignment(config->periph_width)",
            "to_mem_alignment(config->mem_width)",
            "to_periph_incr(config->periph_incr)",
            "to_mem_incr(config->mem_incr)",
        ):
            if required not in init_body:
                errors.append(
                    f"xy_hal_dma_init must map each side through its own helper: missing {required}"
                )
        # 4. An out-of-range enum is rejected before the first vendor access.
        validate = init_body.find("dma_config_valid(config)")
        first_write = init_body.find("hdma->Init.")
        if validate < 0:
            errors.append(
                "xy_hal_dma_init must validate the public config domain instead of masking an "
                "invalid enum into a legal register field"
            )
        elif first_write >= 0 and validate > first_write:
            errors.append(
                "xy_hal_dma_init must reject an out-of-domain configuration before programming "
                "any hdma->Init field"
            )

    # 5. Callback registration must actually register.
    register_body = function_body(source, "xy_hal_dma_register_callback(")
    for required in (
        "HAL_DMA_RegisterCallback(",
        "context->callbacks[index] = callback;",
        "context->args[index] = arg;",
        "index >= DMA_EVENT_COUNT",
    ):
        if required not in register_body:
            errors.append(
                "xy_hal_dma_register_callback must register with the vendor HAL and record the "
                f"callback context; missing: {required}"
            )

    # 6. The public error domain must be honoured and the IRQ hook implemented.
    for name in sorted(set(re.findall(r"xy_hal_error_t\s+(xy_hal_dma_\w+)\s*\(", header))):
        if not re.search(r"xy_hal_error_t\s+" + re.escape(name) + r"\s*\(", source):
            errors.append(
                f"{name} is declared xy_hal_error_t but the STM32F4 wrapper does not implement it "
                "with the public error type"
            )
    if "HAL_DMA_IRQHandler(" not in source:
        errors.append("STM32F4 DMA wrapper must implement xy_hal_dma_irq_handler()")
    for name in PUBLIC_ERROR_ENTRY_POINTS:
        body = function_body(source, f"{name}(")
        if not body:
            continue
        for literal in ("return 0;", "return -1;"):
            if literal in body:
                errors.append(
                    f"{name} still returns a bare literal ({literal.strip()}) instead of the "
                    "public xy_hal_error_t taxonomy"
                )

    if errors:
        print("stm32f4_dma_wrapper_contract failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("stm32f4_dma_wrapper_contract=passed")
    print("  mapping=xy-enums-to-vendor-dma-constants")
    print("  alignment=periph+mem")
    print("  increment=periph+mem")
    print("  callback=registered-with-context")
    print("  evidence=source-level only")
    return 0


if __name__ == "__main__":
    sys.exit(main())
