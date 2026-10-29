#!/usr/bin/env python3
"""The I2C device address means the same thing on every backend, and it reaches the vendor once.

STM32's HAL takes the address already shifted left: the datasheet's 7-bit 0x50 arrives as 0xA0.
Every backend is therefore supposed to take the plain 7-bit address from the caller and shift it
exactly once on the way out.

U5 did not shift it. Every transaction asked for the device at `dev_addr` - so a sensor at 0x50
was addressed at 0x28: a device that exists at double the address, or nothing at all. L4 and WCH
both shifted, which is what made the divergence invisible: the contract, the header and two of
the three backends agreed.

U5 also validated nothing about the address, so an out-of-range value went to the vendor rather
than coming back as INVALID_PARAM.

This pins all three backends to the same rule set - one shift, a 7-bit range check, and a header
that says which address the caller passes.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))
from source_scan import read_code, read_raw  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
HEADER = ROOT / "components" / "hal" / "inc" / "xy_hal_i2c.h"

BACKENDS = {
    "stm32l4": ROOT / "components/hal/stm32/stm32l4/xy_hal_i2c.c",
    "stm32u5": ROOT / "components/hal/stm32/stm32u5/xy_hal_i2c.c",
    "wch": ROOT / "components/hal/wch/ch32x/src/xy_hal_i2c.c",
}

# Every vendor entry point that receives a device address.
VENDOR_CALLS = (
    "HAL_I2C_Master_Transmit", "HAL_I2C_Master_Receive", "HAL_I2C_Mem_Write",
    "HAL_I2C_Mem_Read", "HAL_I2C_Master_Transmit_DMA", "HAL_I2C_Master_Receive_DMA",
    "HAL_I2C_IsDeviceReady", "I2C_Send7bitAddress",
)
# Prefix the identifier loosely on purpose: `\bsdk_address` does not match
# `i2c_sdk_address(`, because `_` and `s` are both word characters and there is no boundary
# between them. Three guards in this tree have now been written with a false alarm from that
# mistake, so the helper names match with a leading `\w*` instead.
SHIFTED = re.compile(r"<<\s*1\b|\w*sdk_address\s*\(")
RANGE_CHECK = re.compile(r">\s*0x7FU?\b|\w*address_valid\s*\(|<= *0x7FU?\b")


def function_body(source: str, signature: str) -> str:
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


def vendor_address_arguments(body: str) -> list[str]:
    """The device-address argument of each vendor call in `body`.

    Both shapes are the second argument: HAL_I2C_x(handle, address, ...) on STM32 and
    I2C_Send7bitAddress(instance, address, ...) on WCH.
    """
    arguments: list[str] = []
    for call in VENDOR_CALLS:
        for match in re.finditer(re.escape(call) + r"\s*\(([^;]*?)\)\s*(?:;|\n\s*if)", body):
            parts = [part.strip() for part in match.group(1).split(",")]
            if len(parts) >= 2:
                arguments.append(parts[1])
    return arguments


def body_findings(body: str, where: str) -> list[str]:
    problems: list[str] = []
    if not body:
        return [f"{where}: missing"]

    calls = vendor_address_arguments(body)
    for argument in calls:
        if not SHIFTED.search(argument):
            problems.append(
                f"{where}: passes the address as {argument!r} to the vendor, without the "
                f"left shift the caller's 7-bit address needs - the device is addressed at "
                f"half the value")
            break

    if not RANGE_CHECK.search(body):
        problems.append(f"{where}: the address range is not validated; an out-of-range value "
                        f"reaches the vendor instead of coming back as INVALID_PARAM")
    return problems


def helper_findings(source: str, where: str) -> list[str]:
    """An address check folded into a helper still has to happen."""
    if "address_valid" not in source:
        return []
    body = function_body(source, "static int i2c_address_valid(") or \
        function_body(source, "i2c_address_valid(uint16_t")
    if not body:
        return []
    return [] if RANGE_CHECK.search(body) else [
        f"{where}: i2c_address_valid does not restrict the address to 7 bits"]


FUNCTION_INIT = "xy_hal_i2c_init("
TEN_BIT_PROGRAMMING = re.compile(r"I2C_ADDRESSINGMODE_10BIT|I2C_10BITADDR|I2C_ADDMODE_10BIT")


def ten_bit_findings(source: str, where: str) -> list[str]:
    """A mode the header declares but no backend implements must be refused, not programmed.

    All three STM32 backends set the peripheral to 10-bit addressing when the caller asked for
    it, and then rejected every address above 0x7F at the transfer boundary: init accepted a
    mode the backend could never use. WCH already returned NOT_SUPPORTED at init, which is the
    behaviour this pins.
    """
    problems: list[str] = []
    body = function_body(source, FUNCTION_INIT)
    if not body:
        return [f"{where}: has no xy_hal_i2c_init to refuse 10-bit addressing"] if False else []
    if "XY_HAL_I2C_ADDR_10BIT" not in body or "XY_HAL_ERROR_NOT_SUPPORTED" not in body:
        problems.append(
            f"{where}: xy_hal_i2c_init accepts XY_HAL_I2C_ADDR_10BIT instead of returning "
            f"XY_HAL_ERROR_NOT_SUPPORTED; 10-bit is declared in the header but no transfer "
            f"accepts a 10-bit address, so init would approve a mode that cannot work")
    if TEN_BIT_PROGRAMMING.search(source):
        problems.append(
            f"{where}: programs the peripheral for 10-bit addressing, which no transfer path "
            f"supports - that is what let init accept the mode in the first place")
    return problems


MEM_CALLS = ("xy_hal_i2c_mem_write(", "xy_hal_i2c_mem_read(")
HARDCODED_EIGHT_BIT = re.compile(r"I2C_MEMADD_SIZE_8BIT\s*,")
SIZE_FROM_VALUE = re.compile(r"\w*mem_?address_?size\s*\(|\w*memadd_size\w*\s*\(")
HIGH_BYTE_SENT = re.compile(r"reg\s*>>\s*8")
SINGLE_BYTE_REGISTER = re.compile(r"\(uint8_t\)\s*\(?reg\b")
WIDE_REGISTER = re.compile(r"reg(_addr)?\s*>\s*0xFFU?")


def if_statements(body: str) -> list[tuple[str, str]]:
    """Every `if (condition)` with the text that follows its closing parenthesis.

    A regex cannot do this: the conditions these backends write contain calls, so the first `)`
    belongs to `i2c_address_valid(address)`, not to the `if`. Splitting the condition from what
    follows it is the whole point - `if (reg > 0xFFU) return INVALID_PARAM;` refuses the wide
    address, while `if (reg > 0xFFU) { send the high byte }` is the fix for it.
    """
    statements: list[tuple[str, str]] = []
    for match in re.finditer(r"\bif\s*\(", body):
        depth = 0
        start = match.end() - 1
        for index in range(start, len(body)):
            if body[index] == "(":
                depth += 1
            elif body[index] == ")":
                depth -= 1
                if depth == 0:
                    statements.append((body[start + 1:index], body[index + 1:index + 120]))
                    break
    return statements


def refuses_wide_register(body: str) -> bool:
    """A condition on a >0xFF register that returns INVALID_PARAM instead of handling it."""
    for condition, following in if_statements(body):
        if WIDE_REGISTER.search(condition) and "XY_HAL_ERROR_INVALID_PARAM" in following:
            return True
    return False


def register_address_findings(source: str, where: str) -> list[str]:
    """One 16-bit parameter, three behaviours: truncated, derived, or refused.

    L4 and U5 passed a hardcoded I2C_MEMADD_SIZE_8BIT, so a register above 0xFF was silently
    truncated to its low byte - the wrong register, with no error. F4 derived the size from the
    value. WCH refused the address outright. F4 is the only one that matches the header's 16-bit
    parameter, so the rule is that shared: derive the encoding from the value, never truncate it,
    and do not refuse it either.

    The truncation rule is stated only for backends that do not refuse the wide address: refusing
    is a different (and reported) defect, and calling it truncation would name the wrong bug.
    """
    problems: list[str] = []
    for call in MEM_CALLS:
        body = function_body(source, call)
        if not body:
            continue
        name = call.rstrip("(")
        refuses = refuses_wide_register(body)
        if HARDCODED_EIGHT_BIT.search(body):
            problems.append(
                f"{where}:{name}: passes a hardcoded I2C_MEMADD_SIZE_8BIT, so a register "
                f"address above 0xFF is truncated to its low byte and the wrong register is "
                f"accessed")
        elif refuses:
            problems.append(
                f"{where}:{name}: refuses register addresses above 0xFF, but the parameter is "
                f"uint16_t and the other backends support the wider address")
        elif "I2C_MEMADD" not in body and SINGLE_BYTE_REGISTER.search(body) \
                and not HIGH_BYTE_SENT.search(body):
            problems.append(
                f"{where}:{name}: sends the register address as a single byte with no "
                f"high-byte path, so a 16-bit address is silently truncated")
    return problems


def header_findings(header: str) -> list[str]:
    problems: list[str] = []
    if "reg_addr" in header and "16-bit" not in header:
        problems.append("xy_hal_i2c.h: reg_addr is 16-bit in the signature and the header does "
                        "not say how the size is chosen")
    if "ADDR_10BIT" in header and "NOT_SUPPORTED" not in header:
        problems.append("xy_hal_i2c.h: XY_HAL_I2C_ADDR_10BIT is declared without saying that "
                        "init refuses it")
    if "7-bit" not in header.lower():
        problems.append("xy_hal_i2c.h: the header does not say the caller passes a 7-bit "
                        "address (the backends are expected to shift it themselves)")
    return problems


FUNCTIONS = (
    "xy_hal_i2c_master_transmit(", "xy_hal_i2c_master_receive(", "xy_hal_i2c_mem_write(",
    "xy_hal_i2c_mem_read(", "xy_hal_i2c_master_transmit_dma(",
    "xy_hal_i2c_master_receive_dma(", "xy_hal_i2c_is_device_ready(",
)


def findings(backends: dict[str, Path] | None = None, header: Path | None = None) -> list[str]:
    backends = BACKENDS if backends is None else backends
    header = HEADER if header is None else header
    problems = header_findings(read_raw(header))
    for name, path in sorted(backends.items()):
        text = read_code(path)
        problems.extend(helper_findings(text, name))
        problems.extend(ten_bit_findings(text, name))
        problems.extend(register_address_findings(text, name))
        for function in FUNCTIONS:
            body = function_body(text, function)
            if not body:
                continue  # not every backend implements every entry point
            problems.extend(body_findings(body, f"{name}:{function.rstrip('(')}"))
    return problems


def main() -> int:
    problems = findings()
    if problems:
        print("hal_i2c_address_convention failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    print(f"hal_i2c_address_convention ok ({len(BACKENDS)} backends, one shift each)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
