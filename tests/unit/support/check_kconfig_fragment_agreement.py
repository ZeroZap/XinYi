#!/usr/bin/env python3
"""A Kconfig fragment the build never reads must not promise more than the root file.

`cmake/kconfig_parser.py` reads exactly one file - the root `Kconfig` - and refuses `source`
outright ("it would silently configure nothing"). The twenty-three `components/**/Kconfig`
fragments are therefore **documentation**: they describe intent, they do not configure anything.

That asymmetry has a sharp failure mode, and it bit during this slice. The fuel-gauge drivers
reach the bus through the canonical Device helpers, so `COMPONENT_FUEL_GAUGE` has to select
`COMPONENT_DEVICE`. Adding that `select` to `components/fuel_gauge/Kconfig` looked exactly like a
fix. It was inert: the regenerated `.config` still said `CONFIG_COMPONENT_DEVICE=n`, and the only
reason the mistake was visible at all is that `components/fuel_gauge/CMakeLists.txt` fails closed
when `xy_device` is missing. A relation that no configuration of the tree is ever asked for is a
worse trap than no relation at all, because the file that claims it is the file people edit.

So the rule is one-directional, from the fragment to the authority:

  * a fragment may repeat a `select` the root already declares, but it may not declare one the
    root does not - that is an imaginary dependency;
  * a fragment may not carry a `depends on` the root does not carry, because the build would
    ignore it while a reader would treat it as real.

Two facts are reported rather than enforced, because they are inventory, not defects: how many
symbols live only in fragments (67 today - declarations the build never sees), and how many
symbols appear in both (14). A divergence can be pinned with a reason when the root is
deliberately stricter, and the pin expires on its own once the two agree again.

Run as `check_kconfig_fragment_agreement.py`; exit 0 when the fragments promise nothing the root
denies.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from source_scan import read_hash_code  # noqa: E402

REPO_ROOT = Path(__file__).resolve().parents[3]

#: Symbols whose fragment `depends on` deliberately differs from the root. Each entry names the
#: fragment, the symbol, and why the root is the stricter of the two - the fragment is the stale
#: side and the root is what the build applies, so the divergence is kept visible instead of being
#: quietly reconciled in whichever file someone happens to open.
PINNED_DEPENDS_DIVERGENCE = {
    ("components/drivers/display/Kconfig", "DRIVER_DISPLAY_SSD1306"):
        "fragment predates the OLED split: root gates SSD1306 on DRIVER_DISPLAY_OLED, "
        "the fragment still says DRIVER_DISPLAY",
    ("components/drivers/display/Kconfig", "DRIVER_DISPLAY_LCD"):
        "root gates the LCD family on DRIVER_DISPLAY_LCD_*; the fragment keeps a blanket "
        "DRIVER_DISPLAY dependency that the build never applies",
    ("components/drivers/display/Kconfig", "DRIVER_DISPLAY_LED"):
        "same blanket DRIVER_DISPLAY dependency as the LCD entry; root is authoritative",
    ("components/kernel/Kconfig", "OSAL_BACKEND_FREERTOS"):
        "root restricts the FreeRTOS backend to the platforms with a matching port; "
        "the fragment would advertise it everywhere",
    ("components/kernel/Kconfig", "OSAL_BACKEND_BAREMETAL"):
        "root also excludes bare-metal when FreeRTOS is selected; the fragment omits that",
}

BLOCK_END = re.compile(r"^(?:config|menu|endmenu|if|endif|choice|endchoice)\b", re.MULTILINE)
CONFIG_LINE = re.compile(r"^config\s+(\w+)", re.MULTILINE)


def _normalize(expr: str) -> str:
    return re.sub(r"\s+", " ", expr).strip()


def parse_blocks(text: str) -> dict[str, dict[str, list[str]]]:
    """Return block-local `select`/`depends on` entries for every `config` in ``text``.

    Block-local means only the directives written inside the `config` block: enclosing `if` blocks
    are not folded in, so root and fragment are read the same way and the comparison stays about
    what each file actually claims.
    """
    blocks: dict[str, dict[str, list[str]]] = {}
    for match in CONFIG_LINE.finditer(text):
        name = match.group(1)
        rest = text[match.end():]
        following = BLOCK_END.search(rest)
        body = rest[: following.start()] if following else rest
        blocks[name] = {
            "selects": re.findall(r"^\s+select\s+(\w+)", body, re.MULTILINE),
            "depends": [_normalize(item) for item in
                        re.findall(r"^\s+depends on\s+(.+)$", body, re.MULTILINE)],
        }
    return blocks


def fragment_paths(repo_root: Path) -> list[Path]:
    components = repo_root / "components"
    return sorted(components.glob("*/Kconfig")) + sorted(components.glob("*/*/Kconfig"))


def check(repo_root: Path,
          pins: dict[tuple[str, str], str] | None = None) -> tuple[list[str], list[str],
                                                                   dict[str, int]]:
    """Return ``(failures, notes, counts)`` for the fragments under ``repo_root``.

    ``pins`` defaults to the module table; fixtures pass their own so the real tree's pins cannot
    decide a synthetic case.
    """
    if pins is None:
        pins = PINNED_DEPENDS_DIVERGENCE
    root_text = read_hash_code(repo_root / "Kconfig")
    root = parse_blocks(root_text)

    failures: list[str] = []
    notes: list[str] = []
    counts = {"fragments": 0, "shared": 0, "fragment_only": 0}
    seen_divergences: set[tuple[str, str]] = set()

    for path in fragment_paths(repo_root):
        relative = path.relative_to(repo_root).as_posix()
        counts["fragments"] += 1
        for symbol, block in parse_blocks(read_hash_code(path)).items():
            if symbol not in root:
                counts["fragment_only"] += 1
                notes.append(f"{relative}: {symbol} is declared only in a fragment "
                             f"(the build never reads it)")
                continue
            counts["shared"] += 1

            imaginary = [item for item in block["selects"] if item not in root[symbol]["selects"]]
            if imaginary:
                failures.append(
                    f"{relative}: {symbol} selects {', '.join(imaginary)} but the root Kconfig "
                    f"does not - the build never applies this select")

            if block["depends"] != root[symbol]["depends"]:
                key = (relative, symbol)
                seen_divergences.add(key)
                message = (f"{relative}: {symbol} depends on {block['depends']} while the root "
                           f"Kconfig says {root[symbol]['depends']}")
                if key in pins:
                    notes.append("pinned: " + message)
                else:
                    failures.append(message + " (add a pin with a reason, or fix the fragment)")

    for key in pins:
        if key not in seen_divergences:
            failures.append(f"pin has expired: {key[0]} {key[1]} no longer diverges from the root "
                            f"Kconfig - remove the pin")

    return failures, notes, counts


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", default=str(REPO_ROOT))
    parser.add_argument("--verbose", action="store_true",
                        help="list every fragment-only symbol and pinned divergence")
    args = parser.parse_args(argv)

    failures, notes, counts = check(Path(args.repo_root))
    if args.verbose:
        for note in notes:
            print("  " + note)

    summary = (f"kconfig_fragment_agreement {'ok' if not failures else 'FAILED'} "
               f"({counts['fragments']} fragments, {counts['shared']} shared symbols, "
               f"{counts['fragment_only']} fragment-only)")
    if failures:
        print(summary)
        for failure in failures:
            print("  " + failure)
        return 1
    print(summary)
    return 0


if __name__ == "__main__":
    sys.exit(main())
