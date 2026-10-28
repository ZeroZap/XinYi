#!/usr/bin/env python3
"""Byte-exact Pandora STM32L475 HIL state snapshot / verify / restore.

Why this exists
---------------
The resident FOTA bootloader rewrites the application region at 0x08008000 from the
W25Q128 candidate and rotates the journal at 0x0807E000. One HIL run (2026-10-28
boot-flow capture) therefore replaced the previous application image without a
snapshot, and that image could not be restored byte-for-byte afterwards. Flash plus a
volatile RTC handshake is all the boot decision state there is, so capture it first:
a debugger read of those regions costs seconds and makes the flash cycle reversible.

Regions (kept in sync with the board linker scripts and FOTA owners by tests):

  bootloader      0x08000000  32 KiB  STM32L475VETX_BOOTLOADER.ld FLASH
  application     0x08008000 472 KiB  STM32L475VETX_FOTA_APP.ld FLASH
  fota_metadata   0x0807E000   4 KiB  STM32L475VETX_FLASH.ld FOTA_METADATA
  board_reserved  0x0807F000   4 KiB  STM32L475VETX_FLASH.ld BOARD_RESERVED
  rtc_backup      0x40002850    128 B  RTC->BKP0R..BKP31R (slots 1/2 = FOTA handshake)

The RTC backup block is volatile and is never written back: restoring it needs PWR
write access plus an enabled RTC clock, so the tool captures it (to prove it did not
change across a run) and refuses to restore it.

Usage
-----
  tools/pandora_hil_state.py snapshot --label before-bootloader-run
  tools/pandora_hil_state.py verify  --snapshot ~/xinyi-hw-evidence/<date>/pandora-state
  tools/pandora_hil_state.py plan    --snapshot ~/xinyi-hw-evidence/<date>/pandora-state
  tools/pandora_hil_state.py restore --snapshot <dir> --yes

`verify` and `plan` never write. `restore` refuses without --yes and compares every
region byte-for-byte after writing.
"""

from __future__ import annotations

import argparse
import dataclasses
import datetime
import hashlib
import json
import pathlib
import re
import subprocess
import sys
from typing import Callable, Iterable, Protocol, Sequence

SCHEMA_VERSION = 1
BOARD = "pandora_stm32l475"

FLASH_BASE = 0x08000000
FLASH_SIZE = 0x80000  # STM32L475VE: 512 KiB
RTC_BACKUP_BASE = 0x40002850  # RTC->BKP0R, 32 x 32-bit backup registers
RTC_BACKUP_SIZE = 0x80

MANIFEST_NAME = "manifest.json"
REGION_DIR = "regions"


@dataclasses.dataclass(frozen=True)
class Region:
    """One memory window that has to survive a flash cycle."""

    name: str
    address: int
    size: int
    restorable: bool
    optional: bool
    source: str

    @property
    def end(self) -> int:
        return self.address + self.size

    @property
    def filename(self) -> str:
        return f"{self.name}.bin"


REGIONS: tuple[Region, ...] = (
    Region(
        name="bootloader",
        address=0x08000000,
        size=0x8000,
        restorable=True,
        optional=False,
        source="STM32L475VETX_BOOTLOADER.ld FLASH",
    ),
    Region(
        name="application",
        address=0x08008000,
        size=0x76000,
        restorable=True,
        optional=False,
        source="STM32L475VETX_FOTA_APP.ld FLASH (0x08008000..0x0807E000)",
    ),
    Region(
        name="fota_metadata",
        address=0x0807E000,
        size=0x1000,
        restorable=True,
        optional=False,
        source="STM32L475VETX_FLASH.ld FOTA_METADATA",
    ),
    Region(
        name="board_reserved",
        address=0x0807F000,
        size=0x1000,
        restorable=True,
        optional=False,
        source="STM32L475VETX_FLASH.ld BOARD_RESERVED",
    ),
    Region(
        name="rtc_backup",
        address=RTC_BACKUP_BASE,
        size=RTC_BACKUP_SIZE,
        restorable=False,
        optional=True,
        source="RTC->BKP0R..BKP31R; slots 1/2 hold the FOTA confirm/restage handshake",
    ),
)


class Backend(Protocol):
    """Debugger access used by snapshot/verify/restore."""

    def read(self, address: int, size: int, destination: pathlib.Path) -> None: ...

    def write(self, source: pathlib.Path, address: int) -> None: ...


class SnapshotError(RuntimeError):
    """A snapshot could not be produced or read back consistently."""


class BackendError(RuntimeError):
    """The debugger access failed."""


class RestoreRefused(RuntimeError):
    """A restore was requested without the explicit confirmation flag."""


def region_by_name(name: str) -> Region:
    for region in REGIONS:
        if region.name == name:
            return region
    raise KeyError(f"unknown region {name!r}")


def _sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _nonzero_bytes(data: bytes) -> int:
    return sum(1 for byte in data if byte)


# --------------------------------------------------------------------------------------
# Backends
# --------------------------------------------------------------------------------------


class StFlashBackend:
    """Debugger access through st-flash (ST-Link)."""

    def __init__(
        self,
        executable: str = "st-flash",
        runner: Callable[..., object] | None = None,
        timeout: int = 300,
    ) -> None:
        self._executable = executable
        self._runner = runner or subprocess.run
        self._timeout = timeout

    def read(self, address: int, size: int, destination: pathlib.Path) -> None:
        destination.parent.mkdir(parents=True, exist_ok=True)
        if destination.exists():
            destination.unlink()
        # st-flash 1.8.0 syntax is `st-flash read [path] [addr] [size]`; the path
        # comes first, unlike `write [path] [addr]`.
        self._run(["read", str(destination), hex(address), hex(size)])
        if not destination.exists():
            raise BackendError(f"st-flash produced no file for {hex(address)}")
        actual = destination.stat().st_size
        if actual != size:
            raise BackendError(
                f"short read at {hex(address)}: expected {size} B, got {actual} B"
            )

    def write(self, source: pathlib.Path, address: int) -> None:
        self._run(["write", str(source), hex(address)])

    def reset(self) -> None:
        self._run(["reset"])

    def _run(self, arguments: Sequence[str]) -> None:
        argv = [self._executable, *arguments]
        completed = self._runner(argv, capture_output=True, text=True, timeout=self._timeout)
        if completed.returncode != 0:
            detail = (completed.stderr or completed.stdout or "").strip()
            raise BackendError(f"{' '.join(argv)} failed rc={completed.returncode}: {detail}")


class MemoryBackend:
    """Deterministic in-memory backend for host tests."""

    def __init__(self, memory: dict[int, bytes] | None = None) -> None:
        self.memory: dict[int, bytes] = dict(memory or {})
        self.writes: list[tuple[int, bytes]] = []
        self.reads: list[tuple[int, int]] = []
        self.fail_reads: set[int] = set()
        self.short_reads: dict[int, int] = {}
        self.corrupt_after_write: set[int] = set()

    def read(self, address: int, size: int, destination: pathlib.Path) -> None:
        if address in self.fail_reads:
            raise BackendError(f"read at {hex(address)} failed")
        self.reads.append((address, size))
        destination.parent.mkdir(parents=True, exist_ok=True)
        payload = self._slice(address, size)
        truncate_to = self.short_reads.get(address)
        destination.write_bytes(payload[:truncate_to] if truncate_to else payload)

    def write(self, source: pathlib.Path, address: int) -> None:
        payload = pathlib.Path(source).read_bytes()
        self.writes.append((address, payload))
        if address in self.corrupt_after_write:
            self.memory[address] = bytes(len(payload))
        else:
            self.memory[address] = payload

    def seed(self, address: int, payload: bytes) -> None:
        self.memory[address] = payload

    def _slice(self, address: int, size: int) -> bytes:
        chunk = self.memory.get(address)
        if chunk is None:
            raise BackendError(f"no memory mapped at {hex(address)}")
        if len(chunk) < size:
            return chunk + bytes(size - len(chunk))
        return chunk[:size]


# --------------------------------------------------------------------------------------
# Layout drift guards
# --------------------------------------------------------------------------------------

_LINKER_REGION = re.compile(
    r"^\s*(?P<name>[A-Z_][A-Z0-9_]*)\s*\(\s*[^)]*\)\s*:\s*"
    r"ORIGIN\s*=\s*(?P<origin>0x[0-9A-Fa-f]+|\d+)\s*,\s*"
    r"LENGTH\s*=\s*(?P<length>\d+)\s*(?P<unit>[KMG]?)\s*$",
    re.MULTILINE,
)

_LENGTH_UNITS = {"": 1, "K": 1024, "M": 1024 * 1024, "G": 1024 * 1024 * 1024}


def parse_linker_regions(text: str) -> dict[str, tuple[int, int]]:
    """Extract ``NAME (...) : ORIGIN = ..., LENGTH = ...`` pairs from a linker script."""

    regions: dict[str, tuple[int, int]] = {}
    for match in _LINKER_REGION.finditer(text):
        origin = int(match.group("origin"), 0)
        size = int(match.group("length")) * _LENGTH_UNITS[match.group("unit")]
        regions[match.group("name")] = (origin, size)
    return regions


def _read_define(text: str, symbol: str) -> int | None:
    match = re.search(rf"#define\s+{symbol}\s+(0x[0-9A-Fa-f]+|\d+)", text)
    if match is None:
        return None
    return int(match.group(1), 0)


def check_region_map(repo_root: pathlib.Path) -> list[str]:
    """Return every mismatch between REGIONS and the board's linker/owner definitions."""

    board = pathlib.Path(repo_root) / "boards" / BOARD
    problems: list[str] = []

    def linker(name: str) -> dict[str, tuple[int, int]]:
        path = board / name
        if not path.exists():
            problems.append(f"missing linker script {path}")
            return {}
        return parse_linker_regions(path.read_text(encoding="utf-8", errors="replace"))

    boot = linker("STM32L475VETX_BOOTLOADER.ld")
    main = linker("STM32L475VETX_FLASH.ld")
    app = linker("STM32L475VETX_FOTA_APP.ld")

    expectations = (
        ("bootloader", boot.get("FLASH"), "STM32L475VETX_BOOTLOADER.ld FLASH"),
        ("application", app.get("FLASH"), "STM32L475VETX_FOTA_APP.ld FLASH"),
        ("fota_metadata", main.get("FOTA_METADATA"), "STM32L475VETX_FLASH.ld FOTA_METADATA"),
        ("board_reserved", main.get("BOARD_RESERVED"), "STM32L475VETX_FLASH.ld BOARD_RESERVED"),
    )
    for name, expected, source in expectations:
        if expected is None:
            problems.append(f"{name}: {source} not found in its linker script")
            continue
        region = region_by_name(name)
        if (region.address, region.size) != expected:
            problems.append(
                f"{name}: tool says {hex(region.address)}+{hex(region.size)} but {source} says "
                f"{hex(expected[0])}+{hex(expected[1])}"
            )

    flash_header = board / "pandora_fota_flash.h"
    if flash_header.exists():
        meta_base = _read_define(
            flash_header.read_text(encoding="utf-8"), "PANDORA_FOTA_METADATA_BASE"
        )
        if meta_base is not None and meta_base != region_by_name("fota_metadata").address:
            problems.append(
                f"fota_metadata base {hex(region_by_name('fota_metadata').address)} != "
                f"PANDORA_FOTA_METADATA_BASE {hex(meta_base)}"
            )
    else:
        problems.append(f"missing {flash_header}")

    install_header = board / "pandora_fota_install_flash.h"
    if install_header.exists():
        text = install_header.read_text(encoding="utf-8")
        app_base = _read_define(text, "PANDORA_FOTA_APP_BASE")
        limit = _read_define(text, "PANDORA_FOTA_EXECUTION_LIMIT")
        region = region_by_name("application")
        if app_base is not None and app_base != region.address:
            problems.append(
                f"application base {hex(region.address)} != PANDORA_FOTA_APP_BASE {hex(app_base)}"
            )
        if limit is not None and limit != region.end:
            problems.append(
                f"application end {hex(region.end)} != PANDORA_FOTA_EXECUTION_LIMIT {hex(limit)}"
            )
    else:
        problems.append(f"missing {install_header}")

    flow_header = board / "pandora_fota_boot_flow.h"
    if flow_header.exists():
        text = flow_header.read_text(encoding="utf-8")
        rtc = region_by_name("rtc_backup")
        for symbol in ("PANDORA_FOTA_RTC_SLOT_CONFIRM", "PANDORA_FOTA_RTC_SLOT_RESTAGE"):
            slot = _read_define(text, symbol)
            if slot is None:
                problems.append(f"{symbol} not found in pandora_fota_boot_flow.h")
                continue
            if slot * 4 + 4 > rtc.size:
                problems.append(
                    f"{symbol}={slot} falls outside the captured RTC backup block "
                    f"({rtc.size} B)"
                )
    else:
        problems.append(f"missing {flow_header}")

    problems.extend(_check_region_geometry())
    return problems


def _check_region_geometry() -> list[str]:
    problems: list[str] = []
    flash_regions = [
        region for region in REGIONS if region.address < RTC_BACKUP_BASE and not region.optional
    ]
    ordered = sorted(flash_regions, key=lambda region: region.address)
    if ordered[0].address != FLASH_BASE:
        problems.append(
            f"first flash region starts at {hex(ordered[0].address)}, not {hex(FLASH_BASE)}"
        )
    previous: Region | None = None
    for region in ordered:
        if previous is not None and previous.end != region.address:
            problems.append(
                f"gap/overlap between {previous.name} (ends {hex(previous.end)}) and "
                f"{region.name} (starts {hex(region.address)})"
            )
        previous = region
    if previous is not None and previous.end != FLASH_BASE + FLASH_SIZE:
        problems.append(
            f"flash regions end at {hex(previous.end)}, not {hex(FLASH_BASE + FLASH_SIZE)}"
        )
    return problems


# --------------------------------------------------------------------------------------
# Snapshot / verify / restore
# --------------------------------------------------------------------------------------


def snapshot_paths(snapshot_dir: pathlib.Path) -> tuple[pathlib.Path, pathlib.Path]:
    return pathlib.Path(snapshot_dir) / MANIFEST_NAME, pathlib.Path(snapshot_dir) / REGION_DIR


def take_snapshot(
    backend: Backend,
    snapshot_dir: pathlib.Path,
    *,
    label: str = "",
    git_head: str = "",
    utc: datetime.datetime | None = None,
    regions: Iterable[Region] = REGIONS,
) -> dict:
    """Read every region and write the manifest. Fails closed: no manifest on failure."""

    snapshot_dir = pathlib.Path(snapshot_dir)
    manifest_path, region_dir = snapshot_paths(snapshot_dir)
    region_dir.mkdir(parents=True, exist_ok=True)

    entries: list[dict] = []
    for region in regions:
        destination = region_dir / region.filename
        try:
            backend.read(region.address, region.size, destination)
        except BackendError as error:
            destination.unlink(missing_ok=True)
            if not region.optional:
                raise SnapshotError(f"{region.name}: {error}") from error
            entries.append(
                {
                    "name": region.name,
                    "address": hex(region.address),
                    "size": region.size,
                    "file": None,
                    "sha256": None,
                    "nonzero_bytes": None,
                    "captured": False,
                    "error": str(error),
                    "restorable": region.restorable,
                    "source": region.source,
                }
            )
            continue
        payload = destination.read_bytes()
        if len(payload) != region.size:
            destination.unlink(missing_ok=True)
            raise SnapshotError(
                f"{region.name}: captured {len(payload)} B, expected {region.size} B"
            )
        entries.append(
            {
                "name": region.name,
                "address": hex(region.address),
                "size": region.size,
                "file": f"{REGION_DIR}/{region.filename}",
                "sha256": _sha256(payload),
                "nonzero_bytes": _nonzero_bytes(payload),
                "captured": True,
                "restorable": region.restorable,
                "source": region.source,
            }
        )

    manifest = {
        "schema": SCHEMA_VERSION,
        "tool": "tools/pandora_hil_state.py",
        "board": BOARD,
        "label": label,
        "git_head": git_head,
        "created_utc": (utc or datetime.datetime.now(datetime.timezone.utc))
        .replace(microsecond=0)
        .isoformat(),
        "regions": entries,
    }
    temporary = manifest_path.with_suffix(".json.tmp")
    temporary.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    temporary.replace(manifest_path)
    return manifest


def load_manifest(snapshot_dir: pathlib.Path) -> dict:
    manifest_path, _ = snapshot_paths(snapshot_dir)
    if not manifest_path.exists():
        raise SnapshotError(f"no manifest at {manifest_path}")
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if manifest.get("schema") != SCHEMA_VERSION:
        raise SnapshotError(
            f"manifest schema {manifest.get('schema')!r} is not {SCHEMA_VERSION}"
        )
    if manifest.get("board") != BOARD:
        raise SnapshotError(f"manifest board {manifest.get('board')!r} is not {BOARD!r}")
    return manifest


@dataclasses.dataclass(frozen=True)
class Comparison:
    name: str
    captured: bool
    identical: bool
    expected_sha256: str | None
    observed_sha256: str | None
    first_diff: int | None = None
    detail: str = ""

    def render(self) -> str:
        if not self.captured:
            return f"{self.name}: not captured ({self.detail or 'optional region unreadable'})"
        if self.identical:
            return f"{self.name}: identical sha256={(self.expected_sha256 or '')[:16]}"
        if self.first_diff is not None:
            return (
                f"{self.name}: DIFFERS first_diff=+{self.first_diff} "
                f"expected={(self.expected_sha256 or '')[:16]} "
                f"observed={(self.observed_sha256 or '')[:16]}"
            )
        return f"{self.name}: DIFFERS {self.detail}"


def _read_bytes(backend: Backend, region: Region, scratch: pathlib.Path) -> bytes:
    backend.read(region.address, region.size, scratch)
    payload = pathlib.Path(scratch).read_bytes()
    pathlib.Path(scratch).unlink(missing_ok=True)
    return payload


def compare_region(expected: bytes, observed: bytes) -> tuple[bool, int | None]:
    if len(expected) == len(observed):
        for offset in range(len(expected)):
            if expected[offset] != observed[offset]:
                return False, offset
        return True, None
    limit = min(len(expected), len(observed))
    for offset in range(limit):
        if expected[offset] != observed[offset]:
            return False, offset
    return False, limit


def verify_snapshot(
    backend: Backend,
    snapshot_dir: pathlib.Path,
    scratch_dir: pathlib.Path | None = None,
) -> list[Comparison]:
    """Re-read every captured region and compare it byte-for-byte with the snapshot."""

    snapshot_dir = pathlib.Path(snapshot_dir)
    manifest = load_manifest(snapshot_dir)
    scratch_root = pathlib.Path(scratch_dir or (snapshot_dir.parent / ".verify-scratch"))
    scratch_root.mkdir(parents=True, exist_ok=True)

    comparisons: list[Comparison] = []
    for entry in manifest["regions"]:
        name = entry["name"]
        if not entry.get("captured"):
            comparisons.append(
                Comparison(
                    name=name,
                    captured=False,
                    identical=False,
                    expected_sha256=None,
                    observed_sha256=None,
                    detail=entry.get("error", ""),
                )
            )
            continue
        region = region_by_name(name)
        stored = (snapshot_dir / entry["file"]).read_bytes()
        scratch = scratch_root / f"{name}.bin"
        try:
            observed = _read_bytes(backend, region, scratch)
        except BackendError as error:
            comparisons.append(
                Comparison(
                    name=name,
                    captured=True,
                    identical=False,
                    expected_sha256=entry["sha256"],
                    observed_sha256=None,
                    detail=f"re-read failed: {error}",
                )
            )
            continue
        identical, first_diff = compare_region(stored, observed)
        comparisons.append(
            Comparison(
                name=name,
                captured=True,
                identical=identical,
                expected_sha256=entry["sha256"],
                observed_sha256=_sha256(observed),
                first_diff=first_diff,
            )
        )
    return [comparison for comparison in comparisons]


@dataclasses.dataclass(frozen=True)
class RestoreStep:
    name: str
    address: int
    size: int
    source: pathlib.Path
    sha256: str

    def render(self) -> str:
        return f"{self.name}: write {self.size} B to {hex(self.address)} from {self.source.name}"


def restore_plan(snapshot_dir: pathlib.Path) -> list[RestoreStep]:
    """Return the ordered writes a restore would perform; never touches the target."""

    snapshot_dir = pathlib.Path(snapshot_dir)
    manifest = load_manifest(snapshot_dir)
    steps: list[RestoreStep] = []
    for entry in manifest["regions"]:
        if not entry.get("captured") or not entry.get("restorable"):
            continue
        region = region_by_name(entry["name"])
        relative = entry.get("file")
        sha256 = entry.get("sha256")
        if not relative or not sha256:
            raise SnapshotError(f"{region.name}: captured entry has no file/hash")
        steps.append(
            RestoreStep(
                name=region.name,
                address=region.address,
                size=region.size,
                source=snapshot_dir / relative,
                sha256=sha256,
            )
        )
    return sorted(steps, key=lambda step: step.address)


def restore_snapshot(
    backend: Backend,
    snapshot_dir: pathlib.Path,
    *,
    confirm: bool,
    scratch_dir: pathlib.Path | None = None,
) -> list[Comparison]:
    """Write every restorable region back, then read it back and compare."""

    if not confirm:
        raise RestoreRefused("restore requires explicit confirmation (--yes)")
    snapshot_dir = pathlib.Path(snapshot_dir)
    steps = restore_plan(snapshot_dir)
    if not steps:
        raise SnapshotError("snapshot has no restorable captured region")
    scratch_root = pathlib.Path(scratch_dir or (snapshot_dir.parent / ".restore-scratch"))
    scratch_root.mkdir(parents=True, exist_ok=True)

    results: list[Comparison] = []
    for step in steps:
        stored = step.source.read_bytes()
        if _sha256(stored) != step.sha256:
            raise SnapshotError(f"{step.name}: snapshot file no longer matches its manifest hash")
        backend.write(step.source, step.address)
        observed = _read_bytes(
            backend, region_by_name(step.name), scratch_root / f"{step.name}.bin"
        )
        identical, first_diff = compare_region(stored, observed)
        results.append(
            Comparison(
                name=step.name,
                captured=True,
                identical=identical,
                expected_sha256=step.sha256,
                observed_sha256=_sha256(observed),
                first_diff=first_diff,
            )
        )
        if not identical:
            break
    return results


# --------------------------------------------------------------------------------------
# CLI
# --------------------------------------------------------------------------------------


def _git_head(repo_root: pathlib.Path) -> str:
    try:
        completed = subprocess.run(
            ["git", "rev-parse", "HEAD"],
            cwd=str(repo_root),
            capture_output=True,
            text=True,
            timeout=30,
        )
    except OSError:
        return ""
    return completed.stdout.strip() if completed.returncode == 0 else ""


def _default_snapshot_dir() -> pathlib.Path:
    stamp = datetime.datetime.now().strftime("%Y-%m-%dT%H%M%S")
    return pathlib.Path.home() / "xinyi-hw-evidence" / stamp / "pandora-state"


def _print_manifest(manifest: dict) -> None:
    print(f"snapshot label={manifest['label'] or '-'} created={manifest['created_utc']}")
    for entry in manifest["regions"]:
        if entry.get("captured"):
            print(
                f"  {entry['name']:<14} {entry['address']} {entry['size']:>7} B "
                f"nonzero={entry['nonzero_bytes']:>7} sha256={entry['sha256']}"
            )
        else:
            print(f"  {entry['name']:<14} not captured: {entry.get('error', '')}")


def main(argv: Sequence[str] | None = None, repo_root: pathlib.Path | None = None) -> int:
    parser = argparse.ArgumentParser(description=(__doc__ or "").splitlines()[0])
    subparsers = parser.add_subparsers(dest="command", required=True)

    snapshot_parser = subparsers.add_parser("snapshot", help="capture every region plus a manifest")
    snapshot_parser.add_argument("--out", type=pathlib.Path, default=None)
    snapshot_parser.add_argument("--label", default="")

    for name, help_text in (
        ("verify", "re-read the target and compare byte-for-byte (no writes)"),
        ("plan", "print the writes a restore would perform (no writes)"),
        ("restore", "write the snapshot back and verify the read-back"),
    ):
        sub = subparsers.add_parser(name, help=help_text)
        sub.add_argument("--snapshot", type=pathlib.Path, required=True)
        if name == "restore":
            sub.add_argument("--yes", action="store_true", help="confirm the write")

    check_parser = subparsers.add_parser(
        "check-layout", help="compare REGIONS with the board sources"
    )
    check_parser.add_argument("--repo-root", type=pathlib.Path, default=None)

    arguments = parser.parse_args(argv)
    root = pathlib.Path(repo_root or pathlib.Path(__file__).resolve().parent.parent)

    try:
        if arguments.command == "check-layout":
            problems = check_region_map(pathlib.Path(arguments.repo_root or root))
            for problem in problems:
                print(f"LAYOUT_MISMATCH {problem}")
            print("layout_ok" if not problems else f"layout_errors={len(problems)}")
            return 0 if not problems else 1

        if arguments.command == "snapshot":
            backend = StFlashBackend()
            out = pathlib.Path(arguments.out) if arguments.out else _default_snapshot_dir()
            manifest = take_snapshot(
                backend,
                out,
                label=arguments.label,
                git_head=_git_head(root),
            )
            _print_manifest(manifest)
            print(f"snapshot_ok dir={out}")
            return 0

        snapshot_dir = pathlib.Path(arguments.snapshot)
        if arguments.command == "verify":
            comparisons = verify_snapshot(StFlashBackend(), snapshot_dir)
            for comparison in comparisons:
                print(comparison.render())
            captured = [comparison for comparison in comparisons if comparison.captured]
            mismatched = [comparison for comparison in captured if not comparison.identical]
            if mismatched:
                print(f"verify_failed mismatched={len(mismatched)}")
                return 1
            print(f"verify_ok regions={len(captured)}")
            return 0

        if arguments.command == "plan":
            steps = restore_plan(snapshot_dir)
            for step in steps:
                print(step.render())
            print(f"plan_ok steps={len(steps)}")
            return 0

        results = restore_snapshot(StFlashBackend(), snapshot_dir, confirm=arguments.yes)
        for comparison in results:
            print(comparison.render())
        if all(result.identical for result in results):
            print(f"restore_ok regions={len(results)}")
            return 0
        print("restore_failed")
        return 1
    except (SnapshotError, BackendError, RestoreRefused, KeyError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
