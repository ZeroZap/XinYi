#!/usr/bin/env python3
"""Source-level contract for the STM32 F4 / L4 / U5 UART receive interrupt.

The public UART header gained xy_hal_uart_enable_rx_irq / xy_hal_uart_irq_handler /
xy_hal_uart_rx_byte when the WCH backend grew a receive interrupt. The STM32L4 backend -
the one Pandora runs - answered none of them, so the contract was true on one platform and
absent on the board that matters most.

This guard proves the shape of the implementation: the interrupt is armed through the
vendor receive-IT call, the byte is stashed before the callback runs, and the receive is
re-armed so the interrupt is not a one-shot. It does NOT prove the interrupt fires on
hardware - that is a runtime claim, and hardware validation for this board is paused. The
evidence here is source/compile level and is labelled as such.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
HEADER = ROOT / "components" / "hal" / "inc" / "xy_hal_uart.h"
TARGETS = {
    "stm32l4": ROOT / "components" / "hal" / "stm32" / "stm32l4" / "xy_hal_uart.c",
    "stm32u5": ROOT / "components" / "hal" / "stm32" / "stm32u5" / "xy_hal_uart.c",
    "stm32f4": ROOT / "components" / "hal" / "stm32" / "stm32f4" / "xy_hal_uart.c",
}


def body_of(source: str, signature: str) -> str:
    start = source.find(signature)
    if start < 0:
        return ""
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


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--target", default="stm32l4", choices=sorted(TARGETS))
    args = parser.parse_args()
    WRAPPER = TARGETS[args.target]
    label = f"{args.target}_uart_rx_irq_contract"

    failures: list[str] = []
    if not WRAPPER.is_file():
        print(f"{label} failed:\n- {WRAPPER} is missing")
        return 1

    source = WRAPPER.read_text(encoding="utf-8")
    header = HEADER.read_text(encoding="utf-8")

    for declaration in ("xy_hal_uart_enable_rx_irq", "xy_hal_uart_irq_handler",
                        "xy_hal_uart_rx_byte"):
        if declaration not in header:
            failures.append(f"the public header does not declare {declaration}")

    enable = body_of(source, "xy_hal_error_t xy_hal_uart_enable_rx_irq(")
    if not enable:
        failures.append(f"xy_hal_uart_enable_rx_irq is not implemented for {args.target}")
    else:
        if "HAL_UART_Receive_IT" not in enable:
            failures.append("enable_rx_irq does not arm the vendor receive interrupt")
        if "HAL_UART_AbortReceive_IT" not in enable:
            failures.append("enable_rx_irq does not disarm on the disable path")
        if "XY_HAL_ERROR_NOT_SUPPORTED" in enable:
            failures.append("enable_rx_irq reports NOT_SUPPORTED, which is the stub this "
                            "backend was supposed to grow out of")
        if "XY_HAL_ERROR_NOT_INIT" not in enable:
            failures.append("enable_rx_irq arms an unknown handle without reporting "
                            "NOT_INIT, so a missing init would look like a working interrupt")

    rx_complete = body_of(source, "void HAL_UART_RxCpltCallback(")
    if not rx_complete:
        failures.append("HAL_UART_RxCpltCallback is missing, so nothing can report a byte")
    else:
        if "pending_byte" not in rx_complete or "pending_valid" not in rx_complete:
            failures.append("the completion callback does not hand the byte over, so "
                            "xy_hal_uart_rx_byte would have nothing to return")
        if "irq_armed" not in rx_complete or "HAL_UART_Receive_IT" not in rx_complete:
            failures.append("the completion callback does not re-arm, which makes the "
                            "receive interrupt fire exactly once")
        # The two backends dispatch differently - one through a helper, one by calling the
        # stored pointer - so look for either rather than encoding one backend's shape.
        stash = rx_complete.find("pending_byte")
        dispatches = [index for index in (rx_complete.find("uart_callback_dispatch"),
                                          rx_complete.find("ctx->callback("),
                                          rx_complete.find("context->callback("))
                      if index >= 0]
        dispatch = min(dispatches) if dispatches else -1
        if stash < 0 or dispatch < 0 or stash > dispatch:
            failures.append("the byte is handed over after the callback runs, so the "
                            "callback would read a stale value or none at all")

    rx_byte = body_of(source, "int xy_hal_uart_rx_byte(")
    if not rx_byte:
        failures.append(f"xy_hal_uart_rx_byte is not implemented for {args.target}")
    elif "XY_HAL_ERROR_NOT_FOUND" not in rx_byte:
        failures.append("xy_hal_uart_rx_byte has no path for 'nothing pending', so callers "
                        "cannot tell an empty handover from a zero byte")

    handler = body_of(source, "void xy_hal_uart_irq_handler(")
    if not handler:
        failures.append(f"xy_hal_uart_irq_handler is not implemented for {args.target}")
    elif "HAL_UART_IRQHandler" not in handler:
        failures.append("xy_hal_uart_irq_handler does not forward to the vendor handler, "
                        "so a board routing its vector through the HAL would do nothing")

    # The half-complete callback must not claim RX_DONE: for a one byte receive it never
    # fires, and treating it as completion is how a backend looks implemented and is not.
    half = body_of(source, "void HAL_UART_RxHalfCpltCallback(")
    if half and "XY_HAL_UART_EVENT_RX_DONE" in half:
        failures.append("HAL_UART_RxHalfCpltCallback reports RX_DONE, but a one byte "
                        "receive completes without ever reaching half")

    if failures:
        print(f"{label} failed:")
        for failure in failures:
            print(f"- {failure}")
        return 1
    print(f"{label} ok (source/compile level; no runtime evidence)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
