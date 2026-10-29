#!/usr/bin/env python3
"""Fail-closed contract for the STM32F4 I2C wrapper.

The wrapper was deferred from the STM32F4 build because it did not compile at
all: it referenced an ``xy_hal_i2c_handle_t`` type the public HAL never declares
and returned ``XY_HAL_I2C_ADDRESSINGMODE_*`` symbols instead of the vendor
``I2C_ADDRESSINGMODE_*`` ones.  Beyond the compile break it declared every entry
point ``int`` instead of ``xy_hal_error_t``, shifted ``dev_addr`` without a range
check (an 8-bit address silently addressed a different slave), forced the
register address to eight bits with no validation, recorded nothing in
``register_callback``, and never implemented ``xy_hal_i2c_error`` /
``xy_hal_i2c_set_error_cb``.

This is a source-level guard.  It proves the wrapper shape, not the electrical
behaviour: the compile evidence is the STM32F4 platform gate and it remains a
source/compile-level claim, never a board result.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))
from source_scan import read_code, read_raw  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
WRAPPER = ROOT / "components" / "hal" / "stm32" / "stm32f4" / "xy_hal_i2c.c"
HEADER = ROOT / "components" / "hal" / "inc" / "xy_hal_i2c.h"
HAL_CMAKE = ROOT / "components" / "hal" / "CMakeLists.txt"

# The three transfer entry points that shift the 7-bit device address.
ADDRESSED_ENTRY_POINTS = (
    "xy_hal_i2c_master_transmit",
    "xy_hal_i2c_master_receive",
    "xy_hal_i2c_master_transmit_dma",
    "xy_hal_i2c_master_receive_dma",
    "xy_hal_i2c_mem_write",
    "xy_hal_i2c_mem_read",
    "xy_hal_i2c_is_device_ready",
)

VENDOR_CALLBACKS = (
    ("HAL_I2C_MasterTxCpltCallback", "XY_HAL_I2C_EVENT_TX_DONE"),
    ("HAL_I2C_MasterRxCpltCallback", "XY_HAL_I2C_EVENT_RX_DONE"),
    ("HAL_I2C_ErrorCallback", "XY_HAL_I2C_EVENT_ERROR"),
)

ERROR_ENTRY_POINTS = (
    "xy_hal_i2c_init",
    "xy_hal_i2c_deinit",
    "xy_hal_i2c_master_transmit",
    "xy_hal_i2c_master_receive",
    "xy_hal_i2c_mem_write",
    "xy_hal_i2c_mem_read",
    "xy_hal_i2c_master_transmit_dma",
    "xy_hal_i2c_master_receive_dma",
    "xy_hal_i2c_register_callback",
    "xy_hal_i2c_is_device_ready",
    "xy_hal_i2c_error",
    "xy_hal_i2c_set_error_cb",
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
        errors.append("STM32F4 I2C wrapper must be wrapped in #ifdef STM32_HAL_ENABLED")
    deferred = re.search(r"set\(XY_HAL_F4_DEFERRED_SOURCES(.*?)\)",
                         read_raw(HAL_CMAKE), re.S)
    if deferred and "xy_hal_i2c.c" in deferred.group(1):
        errors.append(
            "STM32F4 branch still lists xy_hal_i2c.c as contract-deferred; the migrated wrapper "
            "must be built by the platform gate"
        )

    # 2. The two symbols that stopped it compiling in the first place.
    for stale in ("xy_hal_i2c_handle_t", "XY_HAL_I2C_ADDRESSINGMODE"):
        if stale in source:
            errors.append(
                f"STM32F4 I2C wrapper references undeclared symbol {stale}; the vendor handle "
                "type and address-mode constants must be used"
            )
    init_body = function_body(source, "xy_hal_i2c_init(")
    for required in (
        "I2C_ADDRESSINGMODE_10BIT",
        "I2C_ADDRESSINGMODE_7BIT",
        "I2C_DUTYCYCLE_2",
        "I2C_DUTYCYCLE_16_9",
        "handle->Init.ClockSpeed",
        "handle->Init.DutyCycle",
    ):
        if required not in init_body:
            errors.append(
                "xy_hal_i2c_init must program the F4-specific clock speed / duty cycle and map "
                f"the address mode to vendor constants; missing: {required}"
            )

    # 3. The vendor I2C extension header is self-guarded, so it may only be
    #    included, never assumed present.
    if "stm32f4xx_hal_i2c_ex.h" in raw and "I2C_FLTR_ANOFF" not in raw:
        errors.append(
            "the I2C analog/digital filter extension exists only on F4 parts exposing the FLTR "
            "register; guard the HAL_I2CEx_* calls with I2C_FLTR_ANOFF"
        )

    # 4. Every transfer must validate the device address before shifting it.
    for name in ADDRESSED_ENTRY_POINTS:
        body = function_body(source, f"{name}(")
        if not body:
            errors.append(f"STM32F4 I2C wrapper must define {name}()")
            continue
        if "i2c_validate_transfer(" not in body and "dev_addr >" not in body:
            errors.append(
                f"{name} must validate dev_addr before shifting it into the vendor address word"
            )
        if "dev_addr << 1" not in body:
            errors.append(f"{name} must build the vendor address word from the 7-bit address")
    if "XY_HAL_I2C_ADDR_7BIT_MAX" not in source:
        errors.append(
            "STM32F4 I2C wrapper must define the 7-bit address ceiling it validates against"
        )

    # 5. The register address width must follow the value, not be forced to 8 bits.
    helper_body = function_body(source, "i2c_mem_address_size(")
    if not helper_body:
        errors.append(
            "STM32F4 I2C wrapper must own one register-address-width decision instead of repeating "
            "it per entry point"
        )
    else:
        for required in ("I2C_MEMADD_SIZE_8BIT", "I2C_MEMADD_SIZE_16BIT", "0xFF"):
            if required not in helper_body:
                errors.append(
                    "i2c_mem_address_size() must pick the vendor width from the register address; "
                    f"missing: {required}"
                )
    for name in ("xy_hal_i2c_mem_write", "xy_hal_i2c_mem_read"):
        body = function_body(source, f"{name}(")
        if not body:
            errors.append(f"STM32F4 I2C wrapper must define {name}()")
            continue
        if "I2C_MEMADD_SIZE_8BIT" in body:
            errors.append(
                f"{name} must not force I2C_MEMADD_SIZE_8BIT; a >0xFF register address would be "
                "truncated onto the wrong register"
            )
        if "i2c_mem_address_size(reg_addr)" not in body and "I2C_MEMADD_SIZE_16BIT" not in body:
            errors.append(
                f"{name} must pass a register-address width derived from the address"
            )

    # 6. Callback registration must record the context; vendor completions dispatch.
    register_body = function_body(source, "xy_hal_i2c_register_callback(")
    for required in ("context->callback = callback;", "context->arg = arg;"):
        if required not in register_body:
            errors.append(
                f"xy_hal_i2c_register_callback must record the callback context; missing: {required}"
            )
    for vendor_name, event in VENDOR_CALLBACKS:
        body = function_body(source, f"void {vendor_name}(")
        if not body:
            errors.append(f"STM32F4 I2C wrapper must define {vendor_name}()")
            continue
        if event not in body:
            errors.append(f"{vendor_name}() must dispatch {event}")

    # 7. The whole public error-typed surface, including the two entry points
    #    the old wrapper omitted, must use the public error type.
    for name in sorted(set(re.findall(r"xy_hal_error_t\s+(xy_hal_i2c_\w+)\s*\(", header))):
        if not re.search(r"xy_hal_error_t\s+" + re.escape(name) + r"\s*\(", source):
            errors.append(
                f"{name} is declared xy_hal_error_t but the STM32F4 wrapper does not implement it "
                "with the public error type"
            )
    for name in ERROR_ENTRY_POINTS:
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
        print("stm32f4_i2c_wrapper_contract failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("stm32f4_i2c_wrapper_contract=passed")
    print("  mapping=vendor-i2c-constants")
    print("  address=validated-7bit")
    print("  memadd=value-derived")
    print("  callback=recorded+dispatched")
    print("  evidence=source-level only")
    return 0


if __name__ == "__main__":
    sys.exit(main())
