#!/usr/bin/env python3
"""A component that calls the Device bus helpers must be configured with Device.

The canonical bus entry points (`xy_i2c_device_*`, `xy_spi_device_*`) live in
`components/device`. A component that calls them carries a hard, link-time
dependency on that framework, and a static archive makes the violation silent:
the archive is emitted with unresolved references, the build succeeds, and the
break only appears at whatever final link happens first.

That happened twice. `COMPONENT_FUEL_GAUGE` shipped a default-config
`libxy_fuel_gauge.a` with four unresolved `xy_i2c_device_*` references, and
`XY_SENSOR_ENABLE`/`DRIVER_*` shipped `libxy_drivers.a` and
`libxy_sensor_drivers_core.a` with five to seven of them. Neither configuration
had any archive defining those symbols. In both cases the Kconfig relation was
simply missing, and nothing in the build said so.

So the rule is mechanical: every component tree whose sources reference the
Device bus helpers must be reachable only through a root `Kconfig` symbol that
`select COMPONENT_DEVICE`. The mapping below is deliberately explicit rather
than inferred from directory names: a new component that starts calling the bus
helpers must add itself here, and a mapping that no longer has any reference is
reported as stale, so the table cannot rot into a list of things that used to be
true.

Run as `check_device_bus_dependency.py`; exit 0 when every such component
selects the Device framework.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from source_scan import read_code, read_hash_code  # noqa: E402

REPO_ROOT = Path(__file__).resolve().parents[3]

#: Directories that own the helpers rather than consuming them.
OWNER_DIRECTORIES = ("components/device",)

#: Component directory -> the root Kconfig symbols through which it is enabled.
#: Every listed symbol must `select COMPONENT_DEVICE`; every symbol must exist in
#: the root Kconfig (a rename must not leave the rule pointing at nothing).
COMPONENT_SYMBOLS: dict[str, tuple[str, ...]] = {
    "components/drivers": (
        "DRIVER_SENSOR",
        "DRIVER_DISPLAY",
        "DRIVER_STORAGE",
        "DRIVER_POWER",
        "DRIVER_WIRELESS",
        "DRIVER_SYSTEM",
    ),
    "components/fuel_gauge": ("COMPONENT_FUEL_GAUGE",),
    "components/sensor": ("COMPONENT_SENSOR", "XY_SENSOR_ENABLE"),
}

#: The canonical Device bus entry points. Kept narrow on purpose: these are the
#: symbols whose only definition lives in `components/device`, so an unresolved
#: reference is a guaranteed link failure rather than a maybe.
BUS_HELPER = re.compile(r"\bxy_(?:i2c|spi|uart|adc|gpio)_device_\w+")

CONFIG_LINE = re.compile(r"^config\s+(\w+)", re.MULTILINE)
BLOCK_END = re.compile(r"^(?:config|menu|endmenu|if|endif|choice|endchoice)\b", re.MULTILINE)


def parse_selects(text: str) -> dict[str, list[str]]:
    """Return `symbol -> selects` for every `config` block in ``text``."""
    blocks: dict[str, list[str]] = {}
    for match in CONFIG_LINE.finditer(text):
        rest = text[match.end():]
        following = BLOCK_END.search(rest)
        body = rest[: following.start()] if following else rest
        blocks[match.group(1)] = re.findall(r"^\s+select\s+(\w+)", body, re.MULTILINE)
    return blocks


def bus_helper_references(directory: Path) -> list[str]:
    """Files under ``directory`` whose C sources name a Device bus helper."""
    hits: list[str] = []
    for path in sorted(directory.rglob("*.c")):
        if "third_party" in path.parts or "build" in path.parts:
            continue
        code = read_code(path)
        if BUS_HELPER.search(code):
            hits.append(path.as_posix())
    return hits


def component_directories(repo_root: Path) -> list[Path]:
    components = repo_root / "components"
    if not components.is_dir():
        return []
    return [path for path in sorted(components.iterdir())
            if path.is_dir() and path.name != "third_party"]


def check(repo_root: Path,
          mapping: dict[str, tuple[str, ...]] | None = None) -> tuple[list[str], list[str]]:
    """Return ``(failures, notes)`` for the tree under ``repo_root``."""
    if mapping is None:
        mapping = COMPONENT_SYMBOLS
    root_blocks = parse_selects(read_hash_code(repo_root / "Kconfig"))
    failures: list[str] = []
    notes: list[str] = []

    tracked = set()
    for directory in component_directories(repo_root):
        relative = directory.relative_to(repo_root).as_posix()
        if relative in OWNER_DIRECTORIES:
            continue
        references = bus_helper_references(directory)
        symbols = mapping.get(relative)
        if not references:
            if symbols is not None:
                failures.append(
                    f"{relative} is mapped to {', '.join(symbols)} but no source there names a "
                    f"Device bus helper any more - the mapping is stale")
            continue
        tracked.add(relative)
        if symbols is None:
            failures.append(
                f"{relative} calls the Device bus helpers ({len(references)} files, first "
                f"{Path(references[0]).name}) but no root Kconfig symbol is mapped to it; add it "
                f"to COMPONENT_SYMBOLS and make that symbol select COMPONENT_DEVICE")
            continue
        for symbol in symbols:
            if symbol not in root_blocks:
                failures.append(
                    f"{relative} is mapped to {symbol}, which no longer exists in the root Kconfig "
                    f"(renamed or removed) - update the mapping")
                continue
            if "COMPONENT_DEVICE" not in root_blocks[symbol]:
                failures.append(
                    f"{symbol} enables {relative}, whose sources call the Device bus helpers, but "
                    f"it does not select COMPONENT_DEVICE; the archive would ship unresolved "
                    f"xy_*_device_* references")

    for relative in sorted(set(mapping) - tracked):
        if not (repo_root / relative).is_dir():
            failures.append(
                f"mapping names {relative}, which is not a directory in this tree")

    notes.append(f"{len(tracked)} component trees call the Device bus helpers; "
                 f"{sum(len(mapping.get(name, ())) for name in tracked)} enabling symbols checked")
    return failures, notes


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Components calling the Device bus helpers must select COMPONENT_DEVICE")
    parser.add_argument("--repo-root", default=str(REPO_ROOT))
    args = parser.parse_args(argv)

    failures, notes = check(Path(args.repo_root))
    for note in notes:
        print(note)
    for failure in failures:
        print(f"FAIL: {failure}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
