#!/usr/bin/env python3
"""Fail-closed contract for the STM32F4 SPI wrapper.

The wrapper was deferred from the STM32F4 build for the same class of defects as
the UART and I2C wrappers, plus three silent configuration rewrites that the
compiler could not see:

  * every entry point was declared ``int`` instead of ``xy_hal_error_t``
    (eight conflicting-type errors at the platform gate),
  * ``xy_hal_spi_stop`` / ``xy_hal_spi_error`` / ``xy_hal_spi_set_error_cb``
    were never implemented at all,
  * ``XY_HAL_SPI_DIR_2LINES_RXONLY`` collapsed into ``SPI_DIRECTION_1LINE``,
  * ``XY_HAL_SPI_NSS_HARD_INPUT`` collapsed into ``SPI_NSS_HARD_OUTPUT``,
  * a non-8-bit ``datasize`` became 16-bit unconditionally and an arbitrary
    ``baudrate_prescaler`` was written into ``CR1.BR`` unchecked.
  * CPOL/CPHA were re-derived from a returned ``SPI_POLARITY_* | SPI_PHASE_*``
    bit mask instead of being mapped per mode.

This is a source-level guard.  It proves the wrapper shape, not the electrical
behaviour: the compile evidence is the STM32F4 platform gate and it remains a
source/compile-level claim, never a board result.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))
from source_scan import read_code, read_hash_code, read_raw  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
WRAPPER = ROOT / "components" / "hal" / "stm32" / "stm32f4" / "xy_hal_spi.c"
HEADER = ROOT / "components" / "hal" / "inc" / "xy_hal_spi.h"
HAL_CMAKE = ROOT / "components" / "hal" / "CMakeLists.txt"

# Public entry points the old wrapper either typed as `int` or never defined.
REQUIRED_ENTRY_POINTS = (
    "xy_hal_spi_init",
    "xy_hal_spi_deinit",
    "xy_hal_spi_transmit",
    "xy_hal_spi_receive",
    "xy_hal_spi_transmit_receive",
    "xy_hal_spi_transmit_dma",
    "xy_hal_spi_receive_dma",
    "xy_hal_spi_transmit_receive_dma",
    "xy_hal_spi_stop",
    "xy_hal_spi_register_callback",
    "xy_hal_spi_set_cs",
    "xy_hal_spi_error",
    "xy_hal_spi_set_error_cb",
)

# Every transfer entry point must resolve its length through the frame helper.
TRANSFER_ENTRY_POINTS = (
    "xy_hal_spi_transmit",
    "xy_hal_spi_receive",
    "xy_hal_spi_transmit_receive",
    "xy_hal_spi_transmit_dma",
    "xy_hal_spi_receive_dma",
    "xy_hal_spi_transmit_receive_dma",
)

VENDOR_CALLBACKS = (
    ("HAL_SPI_TxCpltCallback", "XY_HAL_SPI_EVENT_TX_DONE"),
    ("HAL_SPI_RxCpltCallback", "XY_HAL_SPI_EVENT_RX_DONE"),
    ("HAL_SPI_TxRxCpltCallback", "XY_HAL_SPI_EVENT_TX_RX_DONE"),
    ("HAL_SPI_ErrorCallback", "XY_HAL_SPI_EVENT_ERROR"),
)

# The eight representable CR1.BR divider values.
PRESCALERS = (
    "SPI_BAUDRATEPRESCALER_2",
    "SPI_BAUDRATEPRESCALER_4",
    "SPI_BAUDRATEPRESCALER_8",
    "SPI_BAUDRATEPRESCALER_16",
    "SPI_BAUDRATEPRESCALER_32",
    "SPI_BAUDRATEPRESCALER_64",
    "SPI_BAUDRATEPRESCALER_128",
    "SPI_BAUDRATEPRESCALER_256",
)


def function_body(source: str, signature: str) -> str:
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

    # 1. Built by the platform gate, behind the guard.
    if "#ifdef STM32_HAL_ENABLED" not in raw:
        errors.append("STM32F4 SPI wrapper must be wrapped in #ifdef STM32_HAL_ENABLED")
    deferred = re.search(r"set\(XY_HAL_F4_DEFERRED_SOURCES(.*?)\)",
                         read_hash_code(HAL_CMAKE), re.S)
    if deferred and "xy_hal_spi.c" in deferred.group(1):
        errors.append(
            "STM32F4 branch still lists xy_hal_spi.c as contract-deferred; the migrated wrapper "
            "must be built by the platform gate"
        )

    # 2. The whole public surface is implemented with the public error type.
    for name in sorted(set(re.findall(r"xy_hal_error_t\s+(xy_hal_spi_\w+)\s*\(", header))):
        if not re.search(r"xy_hal_error_t\s+" + re.escape(name) + r"\s*\(", source):
            errors.append(
                f"{name} is declared xy_hal_error_t but the STM32F4 wrapper does not implement it "
                "with the public error type"
            )
    for name in REQUIRED_ENTRY_POINTS:
        body = function_body(source, f"{name}(")
        if not body:
            errors.append(f"STM32F4 SPI wrapper must define {name}()")
            continue
        for literal in ("return 0;", "return -1;"):
            if literal in body:
                errors.append(
                    f"{name} still returns a bare literal ({literal.strip()}) instead of the "
                    "public xy_hal_error_t taxonomy"
                )

    # 3. No enum may collapse onto another: both downgrades were silent.  Each
    #    helper must publish an exact case -> vendor-constant mapping, so a
    #    substring match (XY_HAL_SPI_NSS_HARD_INPUT contains SPI_NSS_HARD_INPUT)
    #    can no longer satisfy the guard.
    exact_maps = {
        "spi_direction(": {
            "XY_HAL_SPI_DIR_2LINES": "SPI_DIRECTION_2LINES",
            "XY_HAL_SPI_DIR_2LINES_RXONLY": "SPI_DIRECTION_2LINES_RXONLY",
            "XY_HAL_SPI_DIR_1LINE": "SPI_DIRECTION_1LINE",
        },
        "spi_nss(": {
            "XY_HAL_SPI_NSS_SOFT": "SPI_NSS_SOFT",
            "XY_HAL_SPI_NSS_HARD_INPUT": "SPI_NSS_HARD_INPUT",
            "XY_HAL_SPI_NSS_HARD_OUTPUT": "SPI_NSS_HARD_OUTPUT",
        },
        "spi_datasize(": {
            "XY_HAL_SPI_DATASIZE_8BIT": "SPI_DATASIZE_8BIT",
            "XY_HAL_SPI_DATASIZE_16BIT": "SPI_DATASIZE_16BIT",
        },
        "spi_firstbit(": {
            "XY_HAL_SPI_FIRSTBIT_MSB": "SPI_FIRSTBIT_MSB",
            "XY_HAL_SPI_FIRSTBIT_LSB": "SPI_FIRSTBIT_LSB",
        },
        "spi_polarity(": {
            "XY_HAL_SPI_MODE_0": "SPI_POLARITY_LOW",
            "XY_HAL_SPI_MODE_1": "SPI_POLARITY_LOW",
            "XY_HAL_SPI_MODE_2": "SPI_POLARITY_HIGH",
            "XY_HAL_SPI_MODE_3": "SPI_POLARITY_HIGH",
        },
        "spi_phase(": {
            "XY_HAL_SPI_MODE_0": "SPI_PHASE_1EDGE",
            "XY_HAL_SPI_MODE_1": "SPI_PHASE_2EDGE",
            "XY_HAL_SPI_MODE_2": "SPI_PHASE_1EDGE",
            "XY_HAL_SPI_MODE_3": "SPI_PHASE_2EDGE",
        },
    }
    for signature, expected in exact_maps.items():
        body = function_body(source, signature)
        if not body:
            errors.append(f"STM32F4 SPI wrapper must own one {signature[:-1]}) mapping helper")
            continue
        observed = dict(re.findall(r"case\s+(XY_HAL_SPI_\w+)\s*:\s*return\s+(SPI_\w+)\s*;", body))
        for public_name, vendor_name in expected.items():
            if observed.get(public_name) != vendor_name:
                errors.append(
                    f"{signature[:-1]}() must map {public_name} -> {vendor_name}; observed "
                    f"{observed.get(public_name, 'nothing')}"
                )
        for public_name in observed:
            if public_name not in expected:
                errors.append(
                    f"{signature[:-1]}() maps unexpected public value {public_name}"
                )

    init_body = function_body(source, "xy_hal_spi_init(")
    for assignment in (
        "handle->Init.CLKPolarity = spi_polarity(",
        "handle->Init.CLKPhase = spi_phase(",
        "handle->Init.Direction = spi_direction(",
        "handle->Init.NSS = spi_nss(",
        "handle->Init.DataSize = spi_datasize(",
        "handle->Init.FirstBit = spi_firstbit(",
    ):
        if assignment not in init_body:
            errors.append(
                "xy_hal_spi_init must take every mapped field from its per-enum helper; missing: "
                f"{assignment}"
            )
    if re.search(r"CLKPolarity\s*=.*&\s*SPI_POLARITY_HIGH", source):
        errors.append(
            "CPOL must not be re-derived by testing a combined polarity|phase mask against a "
            "vendor constant"
        )

    # 5. Every config field is validated before the first vendor write, and the
    #    prescaler set is exhaustive because CR1.BR has only three bits.
    prescaler_body = function_body(source, "spi_prescaler_valid(")
    if not prescaler_body:
        errors.append(
            "STM32F4 SPI wrapper must validate baudrate_prescaler against the representable "
            "CR1.BR divider values"
        )
    else:
        for constant in PRESCALERS:
            if constant not in prescaler_body:
                errors.append(
                    f"spi_prescaler_valid() must accept the vendor constant {constant}"
                )
    config_body = function_body(source, "spi_config_check(")
    if not config_body:
        errors.append("STM32F4 SPI wrapper must own one configuration-domain check")
    else:
        for field in ("is_master", "mode", "direction", "datasize", "firstbit", "nss",
                      "baudrate_prescaler"):
            if field not in config_body:
                errors.append(
                    f"spi_config_check() must reject an out-of-domain {field} before the first "
                    "vendor field write"
                )
    if "spi_config_check(config)" not in init_body:
        errors.append("xy_hal_spi_init must validate the configuration before writing Init")
    elif "spi_config_check(config)" in init_body:
        first_init_write = init_body.find("handle->Init.")
        check_at = init_body.find("spi_config_check(config)")
        if first_init_write < 0 or check_at > first_init_write:
            errors.append(
                "xy_hal_spi_init must call spi_config_check() before the first handle->Init write"
            )

    # 7. The byte length is converted to vendor frames, never passed through.
    frame_body = function_body(source, "spi_frame_count(")
    if not frame_body:
        errors.append(
            "STM32F4 SPI wrapper must convert the public byte length into HAL frame counts"
        )
    else:
        for required in ("datasize", "XY_HAL_SPI_DATASIZE_16BIT", "& 1U", "UINT16_MAX"):
            if required not in frame_body:
                errors.append(
                    "spi_frame_count() must halve 16-bit byte lengths, reject an odd length there, "
                    f"and bound the frame count; missing: {required}"
                )
    for name in TRANSFER_ENTRY_POINTS:
        body = function_body(source, f"{name}(")
        if not body:
            continue
        if "spi_validate_transfer(" not in body:
            errors.append(
                f"{name} must resolve its context and frame count through the shared helper"
            )
        if re.search(r"HAL_SPI_\w+\([^;]*\b(len)\b", body):
            errors.append(
                f"{name} passes the raw byte length to the vendor call; HAL_SPI_* takes frames"
            )

    # 8. Callback registration records the context; vendor completions dispatch.
    register_body = function_body(source, "xy_hal_spi_register_callback(")
    for required in ("context->callback = callback;", "context->arg = arg;"):
        if required not in register_body:
            errors.append(
                f"xy_hal_spi_register_callback must record the callback context; "
                f"missing: {required}"
            )
    for vendor_name, event in VENDOR_CALLBACKS:
        body = function_body(source, f"void {vendor_name}(")
        if not body:
            errors.append(f"STM32F4 SPI wrapper must define {vendor_name}()")
            continue
        if event not in body:
            errors.append(f"{vendor_name}() must dispatch {event}")

    # 9. The two software-pin entries have no HAL backing: claiming success was
    #    the old lie (set_cs returned 0 and drove nothing).
    for name in ("xy_hal_spi_set_cs", "xy_hal_spi_error"):
        body = function_body(source, f"{name}(")
        if not body:
            continue
        if "XY_HAL_OK" in body:
            errors.append(
                f"{name}() must not report success: no vendor call backs it, only the driver's "
                "own GPIO can drive that pin"
            )
        if "XY_HAL_ERROR_NOT_SUPPORTED" not in body:
            errors.append(
                f"{name}() must report XY_HAL_ERROR_NOT_SUPPORTED instead of pretending to act"
            )

    # 10. Context state is committed only after the vendor accepts the handle.
    init_call = init_body.find("HAL_SPI_Init(")
    commit = init_body.find("context->initialized = 1U;")
    if init_call < 0:
        errors.append("xy_hal_spi_init must call HAL_SPI_Init()")
    elif commit < 0 or commit < init_call:
        errors.append(
            "xy_hal_spi_init must commit the context only after HAL_SPI_Init() succeeds"
        )
    if "XY_HAL_ERROR_FAIL" not in init_body:
        errors.append("xy_hal_spi_init must report a vendor init failure as XY_HAL_ERROR_FAIL")
    deinit_body = function_body(source, "xy_hal_spi_deinit(")
    if "memset(context, 0, sizeof(*context));" not in deinit_body:
        errors.append("xy_hal_spi_deinit must clear the context after a successful deinit")

    # 10. The stale helper names must not come back.
    for stale in ("xy_to_stm32_spi_mode", "xy_to_stm32_datasize", "xy_to_stm32_firstbit"):
        if stale in source:
            errors.append(f"STM32F4 SPI wrapper still defines the stale helper {stale}()")

    if errors:
        print("stm32f4_spi_wrapper_contract failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("stm32f4_spi_wrapper_contract=passed")
    print("  surface=13-error-typed-entries")
    print("  mapping=explicit-enums")
    print("  prescaler=representable-set-only")
    print("  length=bytes-to-frames")
    print("  callback=recorded+dispatched")
    print("  evidence=source-level only")
    return 0


if __name__ == "__main__":
    sys.exit(main())
