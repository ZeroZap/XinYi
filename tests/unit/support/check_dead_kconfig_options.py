#!/usr/bin/env python3
"""A Kconfig option nothing reads is a switch that does not switch.

This is the same defect as an argparse option nothing reads and a CMake option nothing reads, one
layer further down: `Kconfig` declares 81 symbols, the parser turns them into `CONFIG_*` values in
`config.cmake` and `autoconf.h`, and the build reads 23 of them.

The other 58 are not harmless. Ten of them are the platform menu - `PLATFORM_STM32L4`,
`PLATFORM_STM32U5`, `HAL_PC`, ... - and the platform a build is actually for is chosen by
`HAL_PLATFORM` on the make command line, which the parser consumes as input (`--platform`) rather
than publishing as a decision. So the menu and the build are two names for one thing and only the
second one reaches the compiler: setting `PLATFORM_STM32L4=y` in a `.config` builds PC, silently.
`autoconf.h` is generated into a directory that `include_directories()` does put on the include path,
so the macros are reachable - and no source includes the header, so not one of them is consulted in
C either.

A reader of `Kconfig` therefore cannot tell which of its 81 switches do anything. That is worse than
having no menu: the menu is the thing that says what is configurable.

The corpus a reference may legitimately live in is wide on purpose, for the reason
`check_dead_cmake_options.py` states - a symbol can be read by a workflow, a preset or a helper
script and still be doing its job. What it cannot be is decoration. Text is read through
`read_code()` / `read_hash_code()` so that a symbol named in a comment does not count as a consumer;
that direction matters here, because a comment is exactly where a dead switch gets discussed.
"""

from __future__ import annotations

import os
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from source_scan import read_code, read_hash_code  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
KCONFIG = ROOT / "Kconfig"

#: Declarations are `config NAME` / `menuconfig NAME` at the start of a line, which is the whole
#: grammar this needs: the repository declares its symbols in one flat file.
DECLARATION = re.compile(r"^\s*(?:menu)?config\s+([A-Za-z_][A-Za-z0-9_]*)\s*$", re.M)

#: A consumer is a `CONFIG_<symbol>` token in a file the build or a tool actually reads.
def consumers_of(symbol: str, text: str) -> bool:
    return re.search(rf"\bCONFIG_{re.escape(symbol)}\b", text) is not None

#: Directories never descended into: the build tree, the vendors, and the vendored MCU trees.
PRUNE_DIRS = {"build", ".git", ".venv", "MCU", "third_party", "node_modules", "__pycache__", "docs"}

C_SUFFIXES = {".c", ".h"}
HASH_SUFFIXES = {".cmake", ".txt", ".py", ".mk", ".yml", ".yaml", ".json"}
HASH_NAMES = {"Makefile"}

#: Declared and read nowhere, each with why it is still in the menu. Fifty-eight entries is the
#: finding, not an inconvenience: this table is the inventory of what `Kconfig` promises and the
#: build does not honour. A symbol here may be deleted from `Kconfig`, or wired up and dropped from
#: this table - what it may not do is quietly become a fifty-ninth.
BY_PLATFORM = (
    "the platform is chosen by HAL_PLATFORM on the make command line (-DHAL_PLATFORM=); the parser "
    "reads it as an input and writes this symbol out, and nothing reads it back")
BY_COMPONENT = (
    "component sources are compiled unconditionally by their own CMakeLists; nothing gates on this")
BY_DRIVER = (
    "driver sources are compiled by their own CMakeLists; nothing gates on this")
BY_PROTO = (
    "protocol sources are compiled by their own CMakeLists; nothing gates on this")

DECLARED_BUT_UNREAD: dict[str, str] = {
    # --- the platform menu: the single most misleading group on this list ---
    "PLATFORM_STM32U5": BY_PLATFORM,
    "PLATFORM_STM32F4": BY_PLATFORM,
    "PLATFORM_STM32L4": BY_PLATFORM,
    "PLATFORM_STM32F1": BY_PLATFORM,
    "PLATFORM_WCH": BY_PLATFORM,
    "PLATFORM_HC32": BY_PLATFORM,
    "HAL_PC": BY_PLATFORM,
    "HAL_STM32": BY_PLATFORM,
    "HAL_WCH": BY_PLATFORM,
    "HAL_HC32": BY_PLATFORM,
    "HAL_USE_STANDARD_API": BY_PLATFORM,
    # --- components ---
    "COMPONENT_DEVICE": BY_COMPONENT,
    "COMPONENT_CHARGER": BY_COMPONENT,
    "COMPONENT_CRYPTO": BY_COMPONENT,
    "COMPONENT_DM": BY_COMPONENT,
    "COMPONENT_SENSOR": BY_COMPONENT,
    "COMPONENT_ACTUATOR": BY_COMPONENT,
    "COMPONENT_PID": BY_COMPONENT,
    "COMPONENT_MUX": BY_COMPONENT,
    "COMPONENT_SYS": BY_COMPONENT,
    "COMPONENT_FUEL_GAUGE": BY_COMPONENT,
    "COMPONENT_BMS": BY_COMPONENT,
    "KERNEL_OSAL": (
        "the OSAL backend is selected by OSAL_BACKEND, whose per-backend symbols this one is the "
        "parent of; nothing reads the parent"),
    # --- drivers ---
    "DRIVER_SENSOR": BY_DRIVER,
    "DRIVER_DISPLAY": BY_DRIVER,
    "DRIVER_DISPLAY_OLED": BY_DRIVER,
    "DRIVER_DISPLAY_SSD1306": BY_DRIVER,
    "DRIVER_DISPLAY_LCD": BY_DRIVER,
    "DRIVER_DISPLAY_LCD_SPI": BY_DRIVER,
    "DRIVER_DISPLAY_LCD_I8080": BY_DRIVER,
    "DRIVER_DISPLAY_LCD_ST7789": BY_DRIVER,
    "DRIVER_DISPLAY_LED": BY_DRIVER,
    "DRIVER_DISPLAY_LED_SERIAL_RGB": BY_DRIVER,
    "DRIVER_STORAGE": BY_DRIVER,
    "DRIVER_POWER": BY_DRIVER,
    "DRIVER_WIRELESS": BY_DRIVER,
    "DRIVER_SYSTEM": BY_DRIVER,
    # --- networking and protocols ---
    "NETWORK": BY_PROTO,
    "PROTO_AT_CLIENT": BY_PROTO,
    "PROTO_AT_SERVER": BY_PROTO,
    "PROTO_CAN": BY_PROTO,
    "PROTO_LTE": BY_PROTO,
    "PROTO_HTTP": BY_PROTO,
    "PROTO_COAP": BY_PROTO,
    # --- filesystem ---
    "FS_ENABLED": (
        "the filesystem backends gate on their own symbols (FS_LITTLEFS, FS_FLASHDB); nothing reads "
        "the umbrella"),
    "FS_FATFS": "declared for a backend the tree does not have; no source mentions it at all",
    # --- fota ---
    "FOTA_MAX_IMAGE_SIZE": (
        "the FOTA component sizes from its own configuration; no source or build file reads this"),
    "NOR_FLASH_XIP": "declared for an XIP mode nothing implements; no source mentions it at all",
    # --- power management ---
    "PM_ENABLED": "the PM component is compiled unconditionally; nothing gates on this",
    "PM_SLEEP_MODES": "no consumer; the sleep modes are a runtime argument, not a build switch",
    "PM_DYNAMIC_VOLTAGE": "no consumer; nothing implements dynamic voltage scaling yet",
    # --- logging and assertions ---
    "LOG_ENABLED": "the logger is compiled unconditionally; nothing gates on this",
    "LOG_COLOR": "no consumer; colour is decided at the call site, not at build time",
    "ASSERT_ENABLED": "no consumer in the build tree; assertions are not compiled out by a switch",
    # --- build tooling ---
    "ENABLE_COVERAGE": "no consumer; coverage is passed as a compiler flag by the CI workflow",
    "ENABLE_SANITIZER": "no consumer; sanitizers are passed as compiler flags, not read from Kconfig",
    # --- sensor/actuator convenience switches ---
    "XY_SENSOR_ENABLE": "no consumer; the canonical sensor owners are built by their board CMakeLists",
    "XY_ACTUATOR_ENABLE": "no consumer; actuators are built by their component CMakeLists",
}


def declared(path: Path = KCONFIG) -> list[str]:
    return DECLARATION.findall(path.read_text(encoding="utf-8"))


def corpus(root: Path = ROOT) -> dict[str, str]:
    """Every file whose text may count as a reference, as relative path -> stripped text.

    The declaration file itself is skipped by exact path, not by a name prefix: `cmake/Kconfig.cmake`
    begins with "Kconfig" and is the CMake integration script, which is where two of the live
    references actually live. Skipping by prefix made a version of this guard report `LOG_LEVEL` and
    `BUILD_SHARED_LIBS` as dead when both are read on every configure.
    """
    found: dict[str, str] = {}
    for directory, subdirectories, filenames in os.walk(root):
        subdirectories[:] = sorted(d for d in subdirectories if d not in PRUNE_DIRS)
        for filename in sorted(filenames):
            path = Path(directory) / filename
            relative = path.relative_to(root).as_posix()
            if relative == "Kconfig":
                continue
            try:
                if path.suffix in C_SUFFIXES:
                    found[relative] = read_code(path)
                elif path.suffix in HASH_SUFFIXES or filename in HASH_NAMES:
                    found[relative] = read_hash_code(path)
            except OSError:
                continue
    return found


def findings(symbols: list[str] | None = None, files: dict[str, str] | None = None,
             pinned: dict[str, str] | None = None) -> list[str]:
    names = declared() if symbols is None else symbols
    texts = corpus() if files is None else files
    table = DECLARED_BUT_UNREAD if pinned is None else pinned

    problems: list[str] = []
    if not names:
        problems.append("no Kconfig symbols were declared - the walk found nothing to judge, and an "
                        "empty walk reports the same result as a clean one")
        return problems
    if not texts:
        problems.append("no files were inspected - the walk is broken")
        return problems

    blob = "\n".join(texts.values())
    read = [name for name in names if consumers_of(name, blob)]

    for name in names:
        if name in read and name in table:
            problems.append(
                f"{name}: pinned as declared-and-unread, but a file now reads CONFIG_{name} - the pin "
                f"is stale, and a stale pin hides the next symbol that goes dead")
        elif name not in read and name not in table:
            problems.append(
                f"{name}: declared in Kconfig and read by nothing - setting it configures nothing. "
                f"Wire it up, delete it, or say in DECLARED_BUT_UNREAD why it is declared anyway")

    for name in sorted(table):
        if name not in names:
            problems.append(
                f"{name}: pinned as declared-and-unread, but Kconfig no longer declares it - the pin "
                f"outlived the symbol")
        elif not table[name].strip():
            problems.append(f"{name}: pinned without a stated reason - a pin nobody can audit is a "
                            f"silent skip")
    return problems


def main() -> int:
    problems = findings()
    if problems:
        print("dead_kconfig_options failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    names = declared()
    read = [name for name in names if consumers_of(name, "\n".join(corpus().values()))]
    print(f"dead_kconfig_options ok ({len(names)} symbols declared, {len(read)} read by the build, "
          f"{len(names) - len(read)} pinned as declared-but-unread with a reason)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
