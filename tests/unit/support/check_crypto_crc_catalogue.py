#!/usr/bin/env python3
"""A header that declares what the library cannot link is a promise nobody keeps.

`xy_crc.h` published 118 `xy_crcN_xxx()` entry points. The tree contains definitions for 67 of
them. The other 51 were not a documentation gap: `xy_crc_cfg.h` enables a `XY_CRC16_KERMIT_ENABLE`
style switch for each one, the header declares the function, a caller compiles cleanly, and the
link fails with an undefined symbol - the failure arrives at the last possible moment and from a
place that does not mention the header.

The same defect appears in both directions, which is why this checks both:

* `xy_crc8_bluetooth()` was declared and implemented as `xy_crc8_bluetooth_hid()` - the name the
  header published never linked, while the name that worked was invisible.
* `xy_crc16_ccitt()` was declared *and* implemented, but computed CRC-16/KERMIT (0x2189) while its
  name covers four mutually different families, so the one member `xy_crc16_kermit()` that the
  header also offered as a separate function did not exist at all.

And one layer up, `xy_crc32()` / `xy_crc32_update()` were declared by both copies of
`xy_tiny_crypto.h`, documented in the component README and called by
`components/crypto/example.c`, with no definition anywhere.

A `MCU_`/`third_party` symbol is out of scope here; this is a first-party catalogue whose only
definition site is one file. That makes "declared" and "defined" two sets that must agree, with an
explicit table for the entries that do not.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from source_scan import read_code  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
CRYPTO = ROOT / "components" / "crypto"

#: Every header that publishes a CRC entry point. The two `xy_tiny_crypto.h` copies are separate
#: files with separate content that happen to share an include guard, so both are read: whichever
#: one a caller resolves, the promise it sees has to be one the library keeps.
HEADERS = (
    CRYPTO / "xy_crc" / "xy_crc.h",
    CRYPTO / "xy_tiny_crypto.h",
    CRYPTO / "inc" / "xy_tiny_crypto.h",
)

#: The single translation unit that defines them.
DEFINITIONS = CRYPTO / "xy_crc" / "xy_crc.c"

#: Variants are `xy_crc<width>_<standard>`; the tiny-crypto pair has no width suffix. `xy_crc_calc*`
#: are the core API, declared and defined in the same pair, and are not part of the catalogue.
DECLARATION = re.compile(r"\b(xy_crc(?:[0-9]+_)?[a-z0-9_]+)\s*\(")
DEFINITION = re.compile(
    r"\b(static\s+)?uint(?:8|16|32|64)_t\s+(xy_crc(?:[0-9]+_)?[a-z0-9_]+)\s*\([^;{]*\)\s*\{")
CORE = {"xy_crc_calc", "xy_crc_calc_table", "xy_crc_calc_ex", "xy_crc_make_table"}

#: Declared in a header, enabled in `xy_crc_cfg.h`, and never written. Fifty entries is the finding,
#: not an inconvenience: this table is the inventory of what the catalogue promises and the library
#: cannot deliver. An entry may be implemented (and then leaves this table) or deleted from the
#: header - what it may not do is quietly become the fifty-first.
UNWRITTEN = (
    "declared in xy_crc.h and switched on in xy_crc_cfg.h, but no definition exists anywhere in "
    "the tree: a caller compiles and then fails to link")

DECLARED_BUT_UNWRITTEN: dict[str, str] = {
    # --- CRC-5 / CRC-6 / CRC-7: the sub-byte block. Every one of these is a catalogue entry whose
    #     parameters were never transcribed, not a variant that was removed.
    "xy_crc5_bluetooth": UNWRITTEN,
    "xy_crc5_darc": UNWRITTEN,
    "xy_crc5_gsm": UNWRITTEN,
    "xy_crc6_bluetooth": UNWRITTEN,
    "xy_crc6_cdma2000b": UNWRITTEN,
    "xy_crc6_itu": UNWRITTEN,
    "xy_crc6_itu_k": UNWRITTEN,
    "xy_crc6_itu_o": UNWRITTEN,
    "xy_crc6_itu_q": UNWRITTEN,
    "xy_crc6_itu_r": UNWRITTEN,
    "xy_crc6_itu_s": UNWRITTEN,
    "xy_crc7_bluetooth": UNWRITTEN,
    "xy_crc7_gsm_b": UNWRITTEN,
    "xy_crc7_rohc": UNWRITTEN,
    # --- CRC-8 ---
    "xy_crc8_aes": UNWRITTEN,
    "xy_crc8_darc": UNWRITTEN,
    "xy_crc8_gsm_b": UNWRITTEN,
    "xy_crc8_lte": UNWRITTEN,
    "xy_crc8_nrsc_5": UNWRITTEN,
    # --- CRC-16: the largest block, and the one a BLE/LTE stack would reach for first ---
    "xy_crc16_a": UNWRITTEN,
    "xy_crc16_aug_ccitt": UNWRITTEN,
    "xy_crc16_b": UNWRITTEN,
    "xy_crc16_buypass": UNWRITTEN,
    "xy_crc16_cdma2000": UNWRITTEN,
    "xy_crc16_cms": UNWRITTEN,
    "xy_crc16_dds_110": UNWRITTEN,
    "xy_crc16_dect_r": UNWRITTEN,
    "xy_crc16_dect_x": UNWRITTEN,
    "xy_crc16_dectr": UNWRITTEN,
    "xy_crc16_dectx": UNWRITTEN,
    "xy_crc16_en_13757": UNWRITTEN,
    "xy_crc16_epc": UNWRITTEN,
    "xy_crc16_epc_c1g2": UNWRITTEN,
    "xy_crc16_flexray": UNWRITTEN,
    "xy_crc16_genibus": UNWRITTEN,
    "xy_crc16_gsm": UNWRITTEN,
    "xy_crc16_lj1200": UNWRITTEN,
    "xy_crc16_m17": UNWRITTEN,
    "xy_crc16_mcrf4xx": UNWRITTEN,
    "xy_crc16_modbus_arc": UNWRITTEN,
    "xy_crc16_nrsc_5": UNWRITTEN,
    "xy_crc16_opensafety": UNWRITTEN,
    "xy_crc16_opensafety_a": UNWRITTEN,
    "xy_crc16_profibus": UNWRITTEN,
    "xy_crc16_riello": UNWRITTEN,
    "xy_crc16_t10_dif": UNWRITTEN,
    "xy_crc16_tms37157": UNWRITTEN,
    "xy_crc16_umts": UNWRITTEN,
    # --- CRC-32 ---
    "xy_crc32_iscsi": UNWRITTEN,
    "xy_crc32_xfer": UNWRITTEN,
}

#: Defined in the source and invisible to the compiler, or defined without ever being declared.
SOURCE_ONLY_REASON = (
    "has a body in xy_crc.c but no declaration in any published header, so nothing can call it "
    "without inventing a prototype")

SOURCE_ONLY: dict[str, str] = {
    "xy_crc64_redis_cached": (
        "the body sits inside `#if XY_CRC64_REDIS_CACHED_ENABLE`, a macro no configuration file in "
        "the tree defines, so the compiler never sees it - and no header declares it either"),
}


def declarations(paths: tuple[Path, ...] = HEADERS) -> dict[str, list[str]]:
    """Declared variant -> the headers that declare it, relative to the repository root."""
    found: dict[str, list[str]] = {}
    for path in paths:
        text = read_code(path)
        for name in DECLARATION.findall(text):
            if name in CORE:
                continue
            found.setdefault(name, []).append(path.relative_to(ROOT).as_posix())
    return {name: sorted(set(where)) for name, where in found.items()}


def definitions(path: Path = DEFINITIONS) -> set[str]:
    """Externally visible definitions: a `static` helper is not part of any catalogue."""
    found = set()
    for storage, name in DEFINITION.findall(read_code(path)):
        if storage or name in CORE:
            continue
        found.add(name)
    return found


def findings(declared: dict[str, list[str]] | None = None,
             defined: set[str] | None = None,
             unwritten: dict[str, str] | None = None,
             source_only: dict[str, str] | None = None) -> list[str]:
    heads = declarations() if declared is None else declared
    bodies = definitions() if defined is None else defined
    pending = DECLARED_BUT_UNWRITTEN if unwritten is None else unwritten
    orphans = SOURCE_ONLY if source_only is None else source_only

    problems: list[str] = []
    if not heads:
        problems.append("no CRC variant was declared - the header walk found nothing, and an empty "
                        "walk reports the same result as a clean one")
        return problems
    if not bodies:
        problems.append("no CRC variant was defined - the definition scan found nothing")
        return problems

    for name in sorted(heads):
        if name in bodies and name in pending:
            problems.append(f"{name}: pinned as declared-but-unwritten, but xy_crc.c now defines it "
                            f"- the pin is stale, and a stale pin hides the next unwritten entry")
        elif name not in bodies and name not in pending:
            problems.append(f"{name}: declared in {', '.join(heads[name])} and defined nowhere - a "
                            f"caller compiles and then fails to link. Write it, remove the "
                            f"declaration, or say in DECLARED_BUT_UNWRITTEN why it is declared anyway")

    for name in sorted(bodies):
        if name not in heads and name not in orphans:
            problems.append(f"{name}: defined in xy_crc.c but declared in no header - it "
                            f"{SOURCE_ONLY_REASON}. Declare it or list it in SOURCE_ONLY")

    for name in sorted(pending):
        if not pending[name].strip():
            problems.append(f"{name}: pinned in DECLARED_BUT_UNWRITTEN without a stated reason - a "
                            f"pin nobody can audit is a silent skip")
        if name not in heads:
            problems.append(f"{name}: pinned in DECLARED_BUT_UNWRITTEN, but no header declares it any "
                            f"more - the pin outlived the declaration")

    for name in sorted(orphans):
        if not orphans[name].strip():
            problems.append(f"{name}: pinned in SOURCE_ONLY without a stated reason - a pin nobody "
                            f"can audit is a silent skip")
        if name not in bodies:
            problems.append(f"{name}: pinned in SOURCE_ONLY, but xy_crc.c no longer defines it - "
                            f"the pin outlived the definition")
        if name in heads:
            problems.append(f"{name}: pinned in SOURCE_ONLY as undeclared, but a header now declares "
                            f"it - the pin is stale")
    return problems


def main() -> int:
    problems = findings()
    if problems:
        print("crypto_crc_catalogue failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    heads = declarations()
    bodies = definitions()
    print(f"crypto_crc_catalogue ok ({len(heads)} variants declared, {len(bodies)} defined, "
          f"{len(DECLARED_BUT_UNWRITTEN)} pinned as declared-but-unwritten, "
          f"{len(SOURCE_ONLY)} pinned as source-only)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
