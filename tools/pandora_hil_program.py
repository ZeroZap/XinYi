#!/usr/bin/env python3
"""Snapshot-gated Pandora programming: no write without a verified, current snapshot.

Why this exists
---------------
`tools/pandora_hil_state.py` made a Pandora flash cycle *reversible*, but it is a tool
an operator has to remember to run. The 2026-10-28 boot-flow run proved that remembering
is the weak link: the bootloader was flashed, its own `install_once()` rewrote
0x08008000, and the previous application image was gone before anyone snapshotted it.

So this wrapper makes the missing step a precondition of the write instead of a habit:

  1. the caller must supply an existing snapshot directory;
  2. that snapshot is re-read and compared byte-for-byte against the live target
     *before* anything is written -- if the board is not the state that was captured,
     the write is refused and nothing is touched;
  3. the image is checked against the region map (a span may not straddle two regions,
     may not land in the RTC block, must fit, must start on a flash page);
  4. after the write the containing region is read back and compared with the image;
  5. bytes that changed outside the image span are classified: flash page-erase tail is
     expected and reported, anything else is a hard failure;
  6. with --reset the regions are sampled again after the boot decision and every region
     that drifted from the snapshot is named.

Fail-closed rules: `program_ok` is printed only when the read-back matches. A refusal
exit code (2) means nothing was written. A failure exit code (1) means the board was
written and the printed restore command should be run.

Usage
-----
  tools/pandora_hil_state.py snapshot --label before-program
  tools/pandora_hil_program.py plan --image build/.../app.bin --address 0x08008000 \\
      --snapshot ~/xinyi-hw-evidence/<stamp>/pandora-state
  tools/pandora_hil_program.py run  --image build/.../app.bin --address 0x08008000 \\
      --snapshot ~/xinyi-hw-evidence/<stamp>/pandora-state --yes [--reset]

`plan` never touches the target. `run` refuses without --yes.
"""

from __future__ import annotations

import argparse
import dataclasses
import datetime
import hashlib
import json
import pathlib
import sys
import tempfile
import time
from typing import Callable, Protocol, Sequence

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

import pandora_hil_state as state  # noqa: E402


class Backend(state.Backend, Protocol):
    """Debugger access that can also reset the target."""

    def reset(self) -> None: ...


# st-info reports pagesize 2048 for the STM32L475VE; a programming operation always
# starts a page, and the erase granularity decides which bytes outside the image are
# allowed to change.
FLASH_PAGE_SIZE = 0x800

PROGRAM_LOG_NAME = "program.json"


class ProgramError(RuntimeError):
    """The image or the target does not satisfy the programming contract."""


class WriteRefused(RuntimeError):
    """A write was requested without explicit confirmation."""


class PrecheckFailed(RuntimeError):
    """The live target is not the state that the snapshot captured."""


@dataclasses.dataclass(frozen=True)
class ImageView:
    path: pathlib.Path
    address: int
    size: int
    sha256: str
    region: state.Region

    @property
    def end(self) -> int:
        return self.address + self.size

    @property
    def page_end(self) -> int:
        """End of the last flash page the image programming operation touches."""

        return _page_round_up(self.end)


def _page_round_up(address: int) -> int:
    return (address + FLASH_PAGE_SIZE - 1) // FLASH_PAGE_SIZE * FLASH_PAGE_SIZE


def _sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _flash_region_for(address: int) -> state.Region:
    for region in state.REGIONS:
        if not region.restorable:
            continue
        if region.address <= address < region.end:
            return region
    raise ProgramError(
        f"{hex(address)} is not inside a restorable flash region "
        f"(RTC backup and unmapped addresses are not programmable)"
    )


def image_view(image_path: pathlib.Path, address: int) -> ImageView:
    """Validate an image against the region map without touching the target."""

    path = pathlib.Path(image_path)
    if not path.is_file():
        raise ProgramError(f"no image at {path}")
    if path.suffix.lower() != ".bin":
        raise ProgramError(
            f"{path.name}: only raw .bin images are programmable; "
            "convert the ELF first so the written span is the bytes you hashed"
        )
    size = path.stat().st_size
    if size == 0:
        raise ProgramError(f"{path.name}: refusing an empty image")
    if address < 0:
        raise ProgramError(f"{hex(address)}: negative address")
    if address % FLASH_PAGE_SIZE:
        raise ProgramError(
            f"{hex(address)}: programming must start on a {FLASH_PAGE_SIZE} B flash page"
        )

    region = _flash_region_for(address)
    if size > region.size:
        raise ProgramError(
            f"{path.name}: {size} B does not fit {region.name} ({region.size} B)"
        )
    if address + size > region.end:
        raise ProgramError(
            f"{path.name}: {hex(address)}+{size} B crosses the end of {region.name} "
            f"({hex(region.end)}); an image may not straddle two regions"
        )

    return ImageView(
        path=path,
        address=address,
        size=size,
        sha256=_sha256(path.read_bytes()),
        region=region,
    )


def restore_command(snapshot_dir: pathlib.Path) -> str:
    return (
        f"tools/pandora_hil_state.py restore --snapshot {pathlib.Path(snapshot_dir)} --yes"
    )


@dataclasses.dataclass(frozen=True)
class SpanTruth:
    """What the read-back says about the span and about everything around it."""

    span_identical: bool
    first_diff: int | None
    observed_sha256: str
    erased_tail: int
    unexpected_changes: int
    unexpected_first: int | None

    def render(self) -> str:
        detail = f"span_identical={str(self.span_identical).lower()}"
        if not self.span_identical and self.first_diff is not None:
            detail += f" first_diff=+{self.first_diff}"
        detail += f" observed_sha256={self.observed_sha256[:16]}"
        detail += f" erased_tail={self.erased_tail}"
        detail += f" unexpected_changes={self.unexpected_changes}"
        return detail


def classify_readback(
    image: ImageView, observed: bytes, before: bytes | None
) -> SpanTruth:
    """Compare a region read-back with the image and with the pre-write snapshot."""

    if len(observed) < image.size:
        raise ProgramError(
            f"read-back short: {len(observed)} B < image {image.size} B"
        )
    span = observed[: image.size]
    identical, first_diff = state.compare_region(image.path.read_bytes(), span)

    region_offset = image.address - image.region.address
    tail_end = min(image.page_end - image.region.address, len(observed))
    tail = observed[image.size : max(image.size, tail_end)]
    erased_tail = sum(1 for byte in tail if byte == 0xFF)

    unexpected = 0
    unexpected_first: int | None = None
    if before is not None and len(before) == len(observed):
        window_end = max(tail_end, image.size)
        for offset in range(len(observed)):
            relative = offset - region_offset
            if 0 <= relative < window_end:
                continue  # inside the image or its erase page: reported separately
            if observed[offset] != before[offset]:
                unexpected += 1
                if unexpected_first is None:
                    unexpected_first = offset - region_offset

    return SpanTruth(
        span_identical=identical,
        first_diff=first_diff,
        observed_sha256=_sha256(span),
        erased_tail=erased_tail,
        unexpected_changes=unexpected,
        unexpected_first=unexpected_first,
    )


@dataclasses.dataclass(frozen=True)
class Drift:
    name: str
    identical: bool
    first_diff: int | None

    def render(self) -> str:
        if self.identical:
            return f"{self.name}: unchanged"
        return f"{self.name}: CHANGED first_diff=+{self.first_diff}"


@dataclasses.dataclass
class Outcome:
    image: ImageView
    snapshot_dir: pathlib.Path
    truth: SpanTruth
    drift: list[Drift]
    reset: bool
    log_path: pathlib.Path

    @property
    def ok(self) -> bool:
        return (
            self.truth.span_identical
            and self.truth.unexpected_changes == 0
        )


def _scan_drift(
    backend: Backend, snapshot_dir: pathlib.Path, scratch: pathlib.Path
) -> list[Drift]:
    manifest = state.load_manifest(snapshot_dir)
    drift: list[Drift] = []
    for entry in manifest["regions"]:
        if not entry.get("captured"):
            continue
        region = state.region_by_name(entry["name"])
        stored = (pathlib.Path(snapshot_dir) / entry["file"]).read_bytes()
        observed = state._read_bytes(backend, region, scratch / f"{region.name}.bin")
        identical, first_diff = state.compare_region(stored, observed)
        drift.append(Drift(name=region.name, identical=identical, first_diff=first_diff))
    return drift


def program(
    backend: Backend,
    image: ImageView,
    snapshot_dir: pathlib.Path,
    *,
    confirm: bool,
    reset: bool = False,
    settle: float = 3.0,
    sleep: Callable[[float], None] = time.sleep,
    scratch_dir: pathlib.Path | None = None,
    utc: datetime.datetime | None = None,
) -> Outcome:
    """Run the gated sequence. Raises before writing if any precondition fails."""

    if not confirm:
        raise WriteRefused("programming requires explicit confirmation (--yes)")
    snapshot_dir = pathlib.Path(snapshot_dir)

    workspace = pathlib.Path(scratch_dir or tempfile.mkdtemp(prefix="pandora-program-"))
    workspace.mkdir(parents=True, exist_ok=True)

    # 1. The board must still be the state that was captured, or the write would destroy
    #    changes nobody recorded.
    pre = state.verify_snapshot(backend, snapshot_dir, workspace / "pre")
    drifted = [comparison for comparison in pre if comparison.captured and not comparison.identical]
    # The volatile RTC block is captured but never restored, so an unreadable capture
    # there must not veto a write; an unreadable *restorable* region must.
    unreadable = [
        comparison
        for comparison in pre
        if not comparison.captured and state.region_by_name(comparison.name).restorable
    ]
    if drifted or unreadable:
        detail = ", ".join(
            [comparison.render() for comparison in drifted + unreadable]
        )
        raise PrecheckFailed(
            "live target does not match the snapshot, so a write would destroy "
            f"unrecorded state ({detail}). Re-run "
            f"`tools/pandora_hil_state.py snapshot --label <why>` first."
        )

    # 2. Re-hash the image immediately before the write: a file that changed after the
    #    plan was reviewed must not be programmed under the old hash.
    if _sha256(image.path.read_bytes()) != image.sha256:
        raise ProgramError(f"{image.path.name}: image changed after it was hashed")

    before = (snapshot_dir / state.REGION_DIR / image.region.filename).read_bytes()

    # 3. Write, then read the containing region back.
    backend.write(image.path, image.address)
    observed = state._read_bytes(
        backend, image.region, workspace / f"readback-{image.region.name}.bin"
    )
    truth = classify_readback(image, observed, before)

    # 4. Optionally let the board execute one boot decision, then look for runtime
    #    rewrites (the bootloader installing a candidate is exactly this case).
    drift: list[Drift] = []
    if reset:
        backend.reset()
        sleep(settle)
        drift = _scan_drift(backend, snapshot_dir, workspace / "post")

    log_path = snapshot_dir / PROGRAM_LOG_NAME
    log = {
        "schema": 1,
        "tool": "tools/pandora_hil_program.py",
        "board": state.BOARD,
        "created_utc": (utc or datetime.datetime.now(datetime.timezone.utc))
        .replace(microsecond=0)
        .isoformat(),
        "image": {
            "path": str(image.path),
            "size": image.size,
            "sha256": image.sha256,
            "address": hex(image.address),
            "region": image.region.name,
        },
        "readback": dataclasses.asdict(truth),
        "reset": reset,
        "drift": [dataclasses.asdict(item) for item in drift],
        "restore_command": restore_command(snapshot_dir),
    }
    temporary = log_path.with_suffix(".json.tmp")
    temporary.write_text(json.dumps(log, indent=2) + "\n", encoding="utf-8")
    temporary.replace(log_path)

    return Outcome(
        image=image,
        snapshot_dir=snapshot_dir,
        truth=truth,
        drift=drift,
        reset=reset,
        log_path=log_path,
    )


def _print_outcome(outcome: Outcome) -> None:
    image = outcome.image
    print(
        f"image {image.path.name} size={image.size} sha256={image.sha256[:16]} "
        f"target={image.region.name}@{hex(image.address)}"
    )
    print(f"readback {outcome.truth.render()}")
    for item in outcome.drift:
        print(f"drift {item.render()}")
    print(f"restore_command={restore_command(outcome.snapshot_dir)}")
    print(f"log={outcome.log_path}")


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=(__doc__ or "").splitlines()[0])
    subparsers = parser.add_subparsers(dest="command", required=True)

    for name, help_text in (
        ("plan", "validate the image against the region map (no target access)"),
        ("run", "snapshot-gate the write, program, and verify the read-back"),
    ):
        sub = subparsers.add_parser(name, help=help_text)
        sub.add_argument("--image", type=pathlib.Path, required=True)
        sub.add_argument("--address", required=True, help="target address, e.g. 0x08008000")
        sub.add_argument("--snapshot", type=pathlib.Path, required=True)
        if name == "run":
            sub.add_argument("--yes", action="store_true", help="confirm the write")
            sub.add_argument("--reset", action="store_true", help="reset after writing")
            sub.add_argument("--settle", type=float, default=3.0)

    arguments = parser.parse_args(argv)

    try:
        address = int(arguments.address, 0)
        image = image_view(arguments.image, address)
        # Both subcommands require a usable snapshot: programming without one is the
        # defect this tool exists to prevent.
        state.load_manifest(arguments.snapshot)

        if arguments.command == "plan":
            print(
                f"would write {image.size} B ({image.sha256[:16]}) to "
                f"{image.region.name}@{hex(image.address)}..{hex(image.end)}"
            )
            print(
                f"erase_pages={image.address:#x}..{image.page_end:#x} "
                f"({(image.page_end - image.address) // FLASH_PAGE_SIZE} page(s))"
            )
            print(f"snapshot={arguments.snapshot}")
            print("plan_ok writes=0")
            return 0

        outcome = program(
            state.StFlashBackend(),
            image,
            arguments.snapshot,
            confirm=arguments.yes,
            reset=arguments.reset,
            settle=arguments.settle,
        )
        _print_outcome(outcome)
        if outcome.ok:
            print("program_ok")
            return 0
        print("program_failed")
        return 1
    except WriteRefused as error:
        print(f"refused: {error}", file=sys.stderr)
        return 2
    except PrecheckFailed as error:
        print(f"precheck_failed: {error}", file=sys.stderr)
        return 2
    except (ProgramError, state.SnapshotError, state.BackendError, KeyError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
