#!/usr/bin/env python3
"""Fail-closed contract for the STM32F4 UART wrapper.

The wrapper was deferred from the STM32F4 build because every entry point was
declared ``int`` instead of ``xy_hal_error_t`` (so re-adding it does not even
compile), ``send``/``recv`` published the transfer length where the contract
expects an error code, ``flush`` spun forever on ``gState`` with no timeout, the
three vendor completion callbacks were ``void`` functions that tried to return
an error value, ``register_callback`` recorded nothing, ``available`` was a
constant, and ``xy_hal_uart_error``/``xy_hal_uart_set_error_cb`` were never
implemented.

This is a source-level guard.  It proves the wrapper shape, not the electrical
behaviour: the compile evidence is the STM32F4 platform gate and it remains a
source/compile-level claim, never a board result.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))
from source_scan import code_only  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
WRAPPER = ROOT / "components" / "hal" / "stm32" / "stm32f4" / "xy_hal_uart.c"
HEADER = ROOT / "components" / "hal" / "inc" / "xy_hal_uart.h"
HAL_CMAKE = ROOT / "components" / "hal" / "CMakeLists.txt"

# helper -> every (adapter enum, vendor constant) pair that helper must translate.
REQUIRED_MAPPINGS = {
    "uart_word_length": (("XY_HAL_UART_WORDLEN_9B", "UART_WORDLENGTH_9B"),),
    "uart_stop_bits": (("XY_HAL_UART_STOPBITS_2", "UART_STOPBITS_2"),),
    "uart_parity": (
        ("XY_HAL_UART_PARITY_EVEN", "UART_PARITY_EVEN"),
        ("XY_HAL_UART_PARITY_ODD", "UART_PARITY_ODD"),
        ("XY_HAL_UART_PARITY_NONE", "UART_PARITY_NONE"),
    ),
    "uart_flow_control": (
        ("XY_HAL_UART_FLOWCTRL_RTS", "UART_HWCONTROL_RTS"),
        ("XY_HAL_UART_FLOWCTRL_CTS", "UART_HWCONTROL_CTS"),
        ("XY_HAL_UART_FLOWCTRL_RTS_CTS", "UART_HWCONTROL_RTS_CTS"),
        ("XY_HAL_UART_FLOWCTRL_NONE", "UART_HWCONTROL_NONE"),
    ),
    "uart_mode": (
        ("XY_HAL_UART_MODE_TX", "UART_MODE_TX"),
        ("XY_HAL_UART_MODE_RX", "UART_MODE_RX"),
        ("XY_HAL_UART_MODE_TX_RX", "UART_MODE_TX_RX"),
    ),
}

# Entry points that hand the caller a transfer result must not smuggle a length
# through the error-typed return.
LENGTH_RETURN_ENTRY_POINTS = (
    "xy_hal_uart_send",
    "xy_hal_uart_recv",
    "xy_hal_uart_send_dma",
    "xy_hal_uart_recv_dma",
)

VENDOR_CALLBACKS = (
    ("HAL_UART_TxCpltCallback", "XY_HAL_UART_EVENT_TX_DONE"),
    ("HAL_UART_RxCpltCallback", "XY_HAL_UART_EVENT_RX_DONE"),
    ("HAL_UART_ErrorCallback", "XY_HAL_UART_EVENT_ERROR"),
)


def strip_comments(text: str) -> str:
    """Stripped by the shared character walk; a `//` in a string is not a comment."""
    return code_only(text)


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
    raw = WRAPPER.read_text(encoding="utf-8")
    source = strip_comments(raw)
    header = HEADER.read_text(encoding="utf-8")
    errors: list[str] = []

    # 1. Built by the platform gate, behind the guard.
    if "#ifdef STM32_HAL_ENABLED" not in raw:
        errors.append("STM32F4 UART wrapper must be wrapped in #ifdef STM32_HAL_ENABLED")
    deferred = re.search(r"set\(XY_HAL_F4_DEFERRED_SOURCES(.*?)\)",
                         HAL_CMAKE.read_text(encoding="utf-8"), re.S)
    if deferred and "xy_hal_uart.c" in deferred.group(1):
        errors.append(
            "STM32F4 branch still lists xy_hal_uart.c as contract-deferred; the migrated wrapper "
            "must be built by the platform gate"
        )

    # 2. Every adapter enum must be translated into a vendor UART_* constant.
    for helper, pairs in REQUIRED_MAPPINGS.items():
        body = function_body(source, f"static uint32_t {helper}(")
        if not body:
            errors.append(f"STM32F4 UART wrapper must define {helper}()")
            continue
        for xy_symbol, vendor_symbol in pairs:
            if xy_symbol not in body:
                errors.append(f"{helper}() must switch on the adapter enum {xy_symbol}")
            if vendor_symbol not in body:
                errors.append(
                    f"{helper}() must return the vendor constant {vendor_symbol} for {xy_symbol}"
                )
        for returned in re.findall(r"return\s+(XY_HAL_UART_\w+)\s*;", body):
            errors.append(
                f"{helper}() returns the adapter enum {returned} where a vendor UART_* constant "
                "is required"
            )

    # 3. F4 cannot express 7-bit words or 1.5 stop bits; they must fail closed.
    check_body = function_body(source, "uart_config_check(")
    if not check_body:
        errors.append(
            "STM32F4 UART wrapper must validate the public configuration domain before the first "
            "vendor access"
        )
    else:
        for required in (
            "XY_HAL_UART_WORDLEN_7B",
            "XY_HAL_UART_STOPBITS_1_5",
            "XY_HAL_ERROR_NOT_SUPPORTED",
            "config->baudrate == 0U",
        ):
            if required not in check_body:
                errors.append(
                    "uart_config_check() must reject an unrepresentable or missing configuration; "
                    f"missing: {required}"
                )
        init_body = function_body(source, "xy_hal_uart_init(")
        validate = init_body.find("uart_config_check(config)")
        first_write = init_body.find("->Init.")
        if validate < 0 or (first_write >= 0 and validate > first_write):
            errors.append(
                "xy_hal_uart_init must reject an out-of-domain configuration before programming "
                "any Init field"
            )

    # 4. An error-typed entry point may not return a length or a bare literal.
    for name in LENGTH_RETURN_ENTRY_POINTS:
        body = function_body(source, f"{name}(")
        if not body:
            errors.append(f"STM32F4 UART wrapper must define {name}()")
            continue
        for bad in ("return (int)len;", "return 0;", "return -1;"):
            if bad in body:
                errors.append(
                    f"{name} must return the public xy_hal_error_t taxonomy, not `{bad.strip()}`"
                )

    # 5. flush must not spin forever on the vendor state machine.
    flush_body = function_body(source, "xy_hal_uart_flush(")
    if not flush_body:
        errors.append("STM32F4 UART wrapper must define xy_hal_uart_flush()")
    else:
        if re.search(r"while\s*\(", flush_body):
            errors.append(
                "xy_hal_uart_flush must not busy-wait on the vendor state machine without a "
                "bounded timeout"
            )
        if "HAL_UART_STATE_READY" not in flush_body:
            errors.append("xy_hal_uart_flush must report the vendor ready/busy state")

    # 6. Callback registration must record the context, and the vendor
    #    completions must dispatch the public events.
    register_body = function_body(source, "xy_hal_uart_register_callback(")
    for required in (
        "context->callback = callback;",
        "context->callback_arg = arg;",
    ):
        if required not in register_body:
            errors.append(
                "xy_hal_uart_register_callback must record the callback context; "
                f"missing: {required}"
            )
    for vendor_name, event in VENDOR_CALLBACKS:
        body = function_body(source, f"void {vendor_name}(")
        if not body:
            errors.append(f"STM32F4 UART wrapper must define {vendor_name}()")
            continue
        if event not in body:
            errors.append(f"{vendor_name}() must dispatch {event}")
        if "return" in strip_returns_void(body):
            errors.append(
                f"{vendor_name}() is a void vendor callback and must not return a value"
            )

    # 7. The whole public error-typed surface must be implemented with the
    #    public type, including the two entry points the old wrapper omitted.
    for name in sorted(set(re.findall(r"xy_hal_error_t\s+(xy_hal_uart_\w+)\s*\(", header))):
        if not re.search(r"xy_hal_error_t\s+" + re.escape(name) + r"\s*\(", source):
            errors.append(
                f"{name} is declared xy_hal_error_t but the STM32F4 wrapper does not implement it "
                "with the public error type"
            )

    if errors:
        print("stm32f4_uart_wrapper_contract failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("stm32f4_uart_wrapper_contract=passed")
    print("  mapping=xy-enums-to-vendor-uart-constants")
    print("  domain=fail-closed")
    print("  callback=recorded+dispatched")
    print("  evidence=source-level only")
    return 0


def strip_returns_void(body: str) -> str:
    """Drop `return;` so only value-returning statements are inspected."""
    return re.sub(r"return\s*;", "", body)


if __name__ == "__main__":
    sys.exit(main())
