#!/usr/bin/env python3
"""A reference page's prototype is a claim, and the compiler is the only thing that can check it.

`docs/api/API_REFERENCE.md` is the page a reader opens to learn a signature, and until now nothing
had ever compiled it. Measured against the headers it documents, **five of its thirteen prototypes
did not match**:

- `xy_sht30_init()` was written with three parameters - `(dev, i2c_handle, addr)` - and the function
  takes two; the address-taking entry is `xy_sht30_init_addr()`. A reader following this page wrote
  a call that does not exist;
- `xy_hal_gpio_init()` was written with a `uint16_t pin`, where the header takes `uint8_t`;
- `xy_os_thread_create()` and `xy_os_thread_t` named a function and a type no header declares; the
  header's `xy_os_thread_new()` takes an `xy_os_thread_attr_t` and returns an `xy_os_thread_id_t`.

Two more: `xy_os_kernel_init()` / `xy_os_kernel_start()` were documented as returning `int` where
they return `xy_os_status_t` - a warning under `-Wenum-int-mismatch`, an error under `-Werror`.

None of that is visible to a name sweep, which is why this defect survived next door to a guard
that reports `ok` about the same directory: `xy_sht30_init` *is* a declared symbol. Existence and
signature are different claims, and only the second one needs a compiler.

So this compiles the documents. Each fenced C block is appended to a translation unit that first
includes the headers the page is about, with a `#line` directive before every block so a diagnostic
names the page and the page's own line rather than a temporary file. The compiler is asked for
`-fsyntax-only -Wall -Wextra -Werror`: a prototype that disagrees with its header is a
`conflicting types` error, a type the headers do not define is an unknown type name, and a page that
was fine yesterday stays fine without a build.

The component pages are the second body of samples, and they are fifteen separate programs rather
than a list of prototypes: `docs/components/osal/introduction.md` carries two `main()` functions,
`docs/components/ipc/introduction.md` five functions that each open their own session, and a page
whose blocks would have to share one translation unit cannot be judged at all - the second `main()`
is a duplicate definition no matter how right the code is. So a page declared `per_block` compiles
each of its blocks as its own translation unit, and only the merged mode concatenates.

A page also carries blocks that are not samples: a `xy_xxx.h` template with placeholder names, a
`#define` preamble for a configuration header, the body of a function whose locals are declared above
the fence. Those are declared `fragments` by content marker rather than left unselected, because an
unselected block is a block nobody reads - and the quiet version of that defect is a page that grows a
new, never-compiled sample while the guard still reports clean. Every fenced block on a listed page
must therefore be **either compiled or named as a fragment**; one that is neither is reported, and a
fragment marker that stops matching a block is reported too.

Boundaries, stated rather than hidden:

- **Presence is not this guard's job.** A bare `int xy_nothing(void);` declares a function that
  exists nowhere and compiles happily - the symbol sweep is what catches that, and the two rules are
  complementary on purpose rather than one of them being redundant.
- **What a prototype can assert is what is judged**: return type, arity, parameter types. Behaviour,
  ownership, sentinel values and threading rules stay prose; a signature that compiles can still
  mislead about semantics, and this file does not claim otherwise.
- A document with no fenced C block is reported rather than skipped, because an extractor that finds
  nothing reports the same result as a page that compiles cleanly.
- A compiler that cannot be run is reported too. A missing toolchain must not read as a pass.
"""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path
from typing import NamedTuple

sys.path.insert(0, str(Path(__file__).resolve().parent))

from source_scan import fenced_blocks  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
#: Fenced languages that make a compile claim.
CODE_LANGUAGES = ("c", "cpp", "c++")
#: Warnings are errors: `-Wenum-int-mismatch` is what reports a documented `int` against a header's
#: `xy_os_status_t`, and it is only a warning by default. A documentation defect that scrolls past in
#: a warning list is not a gate. The one warning kept out of the gate is an unused local: a snippet
#: that computes a value it does not print is not making a claim about a signature.
COMPILE_ARGS = ("-std=c99", "-Wall", "-Wextra", "-Werror", "-Wno-error=unused-variable",
                "-fsyntax-only")


class Sample(NamedTuple):
    """One document that makes a compile claim, plus what a translation unit needs to judge it."""

    doc: str
    #: Headers the page is *about*. Included before every block so the page's own prototypes are
    #: redeclarations checked against the real ones instead of free-standing declarations.
    includes: tuple[str, ...]
    #: Include roots, relative to the repository root, in the order the page's intended headers need.
    roots: tuple[str, ...]
    #: Content selectors. When non-empty, only the blocks whose text contains one of these markers
    #: are compiled. A design page can carry a module-file preamble and a new-file template beside
    #: its usage sample, and those two are patterns rather than samples - neither compiles at file
    #: scope by construction. Selecting by *content* instead of by line number means an edit above
    #: the sample cannot silently move the selector onto the template, and a selector that stops
    #: matching anything is reported rather than skipped.
    markers: tuple[str, ...] = ()
    #: Compile each selected block as its own translation unit instead of concatenating them. A
    #: component introduction is a set of complete programs - several of them define `main()`, and
    #: two `main()`s in one unit is a duplicate definition that says nothing about the page.
    per_block: bool = False
    #: Content markers for blocks on this page that are deliberately *not* compile claims: a file
    #: template with placeholder names, a configuration-header preamble, a function body whose locals
    #: live above the fence. Naming them is the point - a block that is neither compiled nor declared
    #: here is reported, so a new sample cannot arrive unread.
    fragments: tuple[str, ...] = ()
    #: Wrap each selected block in a function body instead of pasting it at file scope. A guide that
    #: shows one call - `xy_sort_bubble(values, 5);` - is a statement, and a statement at file scope
    #: is a syntax error that says nothing about the call. Its `#include` lines are hoisted above the
    #: wrapper so the block can still name its own header.
    as_body: bool = False
    #: The page teaches an *application*, so its blocks legitimately call functions the reader writes
    #: (`led_toggle()`, `get_sensor_value()`). An implicit declaration is then a warning rather than an
    #: error - except for a name in the project's own namespace: `xy_*` is a call into this tree, and a
    #: header has to declare it. That carve-out is what keeps the tolerance from becoming a licence to
    #: teach a XinYi API that does not exist.
    application: bool = False


#: Include roots every component guide may need: the project's own types and error codes, logging,
#: the kernel's OSAL, and the HAL. A page that carries a complete program still needs the tree to
#: resolve its `#include` lines, and pinning the shared roots here keeps the per-page additions to the
#: directory that owns the header the page is about.
SHARED_ROOTS = (
    "components/clib/xy_clib/inc",
    "components/clib/xy_clib",
    "components/common/inc",
    "components/kernel/misc/inc",
    "components/kernel/osal/inc",
    "components/kernel/osal",
    "components/hal/inc",
    "components/device/inc",
    "components/trace/xy_log/inc",
)


SAMPLES: tuple[Sample, ...] = (
    Sample(
        doc="docs/api/API_REFERENCE.md",
        includes=(
            "xy_os.h",
            "xy_hal_gpio.h",
            "xy_sht30.h",
            "xy_mpu6050.h",
            "xy_pid.h",
            "xy_fota.h",
            "xy_oled_ssd1306.h",
            "xy_font.h",
        ),
        roots=(
            "components/kernel/osal",
            "components/kernel/osal/inc",
            "components/kernel/misc/inc",
            "components/hal/inc",
            "components/clib/xy_clib/inc",
            "components/clib/xy_clib",
            "components/common/inc",
            "components/pid/inc",
            "components/fota/inc",
            "components/sensor/inc",
            "components/gui/inc",
            "components/device/inc",
            "components/drivers/sensor/temperature/sht30",
            "components/drivers/sensor/motion/mpu6050",
        ),
    ),
    # The project overview taught `xy_hal_uart_open(port, baudrate)` / `xy_hal_uart_close(handle)`:
    # a handle-allocating API that never existed, next to the real handle-based one. The name sweep
    # catches the two names; this compiles the call, so the arity and the config struct are judged by
    # the header too. Its other two C blocks are a module preamble (`#define`, `#include` and log
    # calls at file scope) and a new-file template, which is what `markers` is for.
    Sample(
        doc="docs/design/XinYi_Project_Overview.md",
        includes=("xy_hal_uart.h",),
        roots=("components/hal/inc",),
        markers=("xy_hal_uart_send(uart",),
        fragments=('xy_log_d("调试消息', "@file module_name.c"),
    ),
    # The porting guide told a reader porting a new platform to write
    # `xy_hal_gpio_init(void *port, uint16_t pin, ...)` where the header declares
    # `(xy_hal_gpio_port_t, uint8_t, ...)`, and to open the board with `UART1` where the HAL takes a
    # `UART_HandleTypeDef *`. Both were copy-paste defects in the one page whose whole job is to be
    # copied. Its third C block is an implementation excerpt that needs the vendor SDK
    # (`stm32f4xx_hal.h`, `GPIO_InitTypeDef`), so the two selectors below name the sample blocks -
    # the naming contract and the functional demo - and a selector that stops matching is reported.
    Sample(
        doc="docs/hardware/porting.md",
        includes=("xy_hal_gpio.h", "xy_hal_uart.h"),
        roots=(
            "components/hal/inc",
            "components/clib/xy_clib/inc",
            "components/clib/xy_clib",
            "components/kernel/misc/inc",
        ),
        markers=("xy_hal_gpio_init_stm32(", "xy_hal_uart_send(uart,"),
        fragments=("stm32f4xx_hal.h",),
    ),
    # -----------------------------------------------------------------------------------------
    # The component guides. Fifteen pages that each carry complete programs - the pages a reader
    # opens when they are about to write the first call for a component - and until now not one of
    # their fifty-odd blocks had ever been compiled. They need `per_block` because a page teaches two
    # or three programs in sequence and each of them defines `main()`.
    # -----------------------------------------------------------------------------------------
    # Two usage fragments rather than programs: a filter is initialised, a buffer is sorted. Wrapped
    # as function bodies, so the calls are checked against their headers instead of being skipped.
    Sample(
        doc="docs/components/clib/introduction.md",
        includes=(),
        roots=SHARED_ROOTS,
        per_block=True,
        as_body=True,
    ),
    # Three programs (AES, MD5, Base64) and four fragment sketches: a configuration preamble, an HMAC
    # call without the buffer setup it needs, a CRC over a buffer the reader has not opened yet, and an
    # incremental hash whose chunks are the reader's. The fragments are declared, not ignored.
    Sample(
        doc="docs/components/crypto/introduction.md",
        includes=(),
        roots=SHARED_ROOTS + ("components/crypto/inc",),
        markers=("int main(void)",),
        fragments=("// xy_crypto_config.h", "xy_hmac_sha256(", "xy_crc32(file_data",
                   "xy_sha256_ctx_t ctx;"),
        per_block=True,
    ),
    # The device page walks from the framework to two real drivers. Its last three blocks are one
    # example in three pieces - a `xy_my_device_t` typedef, then two functions that take it - and they
    # only compile together, which is what a fragment declaration says.
    Sample(
        doc="docs/components/device/introduction.md",
        includes=(),
        roots=SHARED_ROOTS + ("components/drivers/storage/eeprom/24xx",
                              "components/drivers/display/oled/ssd1306"),
        markers=("void i2c_device_example(", "void eeprom_example(", "void oled_example("),
        fragments=("} xy_my_device_t;", "int xy_my_device_init(", "int xy_my_device_read("),
        per_block=True,
    ),
    Sample(
        doc="docs/components/dm/introduction.md",
        includes=(),
        roots=SHARED_ROOTS + ("components/dm", "components/dm/inc", "components/dm/xy_tlv"),
        per_block=True,
    ),
    # The first block is a complete program; the next two are configuration structs, which compile as
    # definitions at file scope with the page's header included; the last is a copy of the public
    # struct, which would collide with the header's own typedef.
    Sample(
        doc="docs/components/fota/introduction.md",
        includes=("xy_fota.h",),
        roots=SHARED_ROOTS + ("components/fota/inc",),
        fragments=("xy_fota_mode_t mode;",),
        per_block=True,
    ),
    Sample(
        doc="docs/components/gui/introduction.md",
        includes=(),
        roots=SHARED_ROOTS + ("components/gui", "components/gui/inc"),
        per_block=True,
        application=True,
    ),
    # The HAL page's first and last blocks are a configuration-header preamble and a two-line
    # placeholder; the three between them are programs that were written against the vendor's instance
    # handles (`GPIOA`, `UART1`, `I2C1`), which is exactly the coupling the HAL exists to remove.
    Sample(
        doc="docs/components/hal/introduction.md",
        includes=(),
        roots=SHARED_ROOTS,
        fragments=("// xy_hal_cfg.h", "XY_HAL_PLATFORM <your_platform>"),
        per_block=True,
    ),
    Sample(
        doc="docs/components/ipc/introduction.md",
        includes=(),
        roots=SHARED_ROOTS + ("components/ipc/inc", "components/ipc/pipe",
                              "components/ipc/xy_broker", "components/ipc/observer"),
        per_block=True,
    ),
    Sample(
        doc="docs/components/net/introduction.md",
        includes=(),
        roots=SHARED_ROOTS + ("components/net/inc", "components/net/xy_iso7816",
                              "components/net/modbus"),
        per_block=True,
    ),
    # Two programs and a configuration preamble. Both programs create a thread, and the first one's
    # `NULL` argument is the page's own claim that `xy_os.h` gives a reader `NULL`.
    Sample(
        doc="docs/components/osal/introduction.md",
        includes=(),
        roots=SHARED_ROOTS,
        fragments=("// xy_os_cfg.h",),
        per_block=True,
        application=True,
    ),
    Sample(
        doc="docs/components/pid/introduction.md",
        includes=(),
        roots=SHARED_ROOTS + ("components/pid/inc",),
        per_block=True,
        application=True,
    ),
    Sample(
        doc="docs/components/sensor/introduction.md",
        includes=(),
        roots=SHARED_ROOTS + ("components/sensor/inc", "components/sensor"),
        per_block=True,
    ),
    Sample(
        doc="docs/components/trace/introduction.md",
        includes=(),
        roots=SHARED_ROOTS,
        per_block=True,
    ),
    # -----------------------------------------------------------------------------------------
    # The two pages that used to be `DEFERRED`, because their samples taught an API this tree
    # never had: `xy_adc_t` / `xy_adc_init()` / a `xy_dac_*` family on one page, and
    # `xy_fuel_gauge_config_t` / `xy_fuel_gauge_update()` on the other. Both were rewrites rather
    # than include lines, and both are now judged.
    #
    # `addc` teaches `components/analog_devices/xy_adc.h` - the ADC/DAC bookkeeping and conversion
    # helper - and the HAL beneath it. The root order is the point, not decoration: `xy_adc.h` exists
    # twice in this tree, and the two copies share the include guard `XY_ADC_H`, so whichever root
    # comes first is the header the page means. The `xy_adc` CMake target puts
    # `components/analog_devices` ahead of `components/device/inc`; this sample has to reproduce that
    # order or it judges the device-side header instead - which is exactly how the page spent a
    # release being called "teaches an API no header declares" while its calls were right.
    Sample(
        doc="docs/components/addc/introduction.md",
        includes=(),
        roots=("components/analog_devices",) + SHARED_ROOTS,
        per_block=True,
    ),
    # `pm` mixes three headers that do not include each other: the charger, the PM core, and the
    # standalone fuel-gauge component. Its fuel-gauge sample is the driver model - register an
    # instance, take it back by name, `init`, `fetch`, then read the cache - which is what the old
    # `xy_fuel_gauge_update(3800, 500, 25)` sample was inventing.
    Sample(
        doc="docs/components/pm/introduction.md",
        includes=(),
        roots=SHARED_ROOTS + ("components/pm/charger", "components/fuel_gauge/inc",
                              "components/fuel_gauge/drivers"),
        per_block=True,
    ),
    # The driver template is entirely pattern: every block is a file the reader is about to create,
    # naming a driver that does not exist yet (`xy_xxx.h`) and Unity's header. Nothing here can
    # compile, and all five blocks say so - which still routes them through `classify()`, so a sixth
    # block added to the page cannot arrive unread.
    Sample(
        doc="docs/components/device/DRIVER_TEMPLATE.md",
        includes=(),
        roots=SHARED_ROOTS,
        fragments=("xy_xxx.h", "xy_xxx.c", "xy_spi_xxx.h", "xy_gpio_xxx.h", "test_xxx.c"),
    ),
)


#: Pages that carry C blocks, are not judged yet, and say why. This is not an exemption list - it is
#: the measured remainder. Each entry names a page whose examples teach an API this tree does not
#: have, which is a rewrite rather than an include line, and each is printed on every run so the debt
#: stays visible instead of becoming a silence. A page under `docs/components` that is in neither this
#: tuple nor `SAMPLES` is reported by `unlisted_pages()`, so a new guide cannot arrive unwatched.
#:
#: Empty on purpose, and not by deleting the mechanism: `addc` and `pm` were the last two, and both
#: are now in `SAMPLES`. The rules stay - a page that cannot be fixed with an include line has to be
#: deferred *with a reason that prints on every run*, and the self-tests drive those rules against
#: throwaway pages, so an empty tuple is a measured zero rather than a rule that quietly stopped
#: being exercised. Adding a page here is a claim that its samples are wrong and the tree is right.
DEFERRED: tuple[tuple[str, str], ...] = ()


def code_blocks(path: Path) -> list[tuple[int, str]]:
    """(first line number, text) for every fenced C/C++ block in the document."""
    text = path.read_text(encoding="utf-8", errors="replace")
    blocks: list[tuple[int, str]] = []
    for info, start, body in fenced_blocks(text):
        language = info.split()[0].lower() if info.split() else ""
        if language in CODE_LANGUAGES:
            blocks.append((start, body))
    return blocks


def translation_unit(doc: str, blocks: list[tuple[int, str]], includes: tuple[str, ...],
                     as_body: bool = False) -> str:
    """One compilation unit: the page's subject headers, then each block at its own line number.

    ``as_body`` wraps a block whose content is statements rather than declarations: the block's own
    ``#include`` lines are hoisted to the top, a function header is emitted, and only then does a
    ``#line`` directive hand the page's line numbers back - so a diagnostic still points at the
    document even though one line of wrapper now sits in front of the block.
    """
    lines = [f'#include "{name}"' for name in includes]
    hoisted: list[str] = []
    for index, (start, body) in enumerate(blocks):
        if as_body:
            kept = []
            for line in body.splitlines():
                if line.lstrip().startswith("#include"):
                    # Hoisted, and blanked in place: deleting the line would shift every diagnostic
                    # below it by one and the report would name a page line that is not the defect.
                    hoisted.append(line)
                    kept.append("")
                else:
                    kept.append(line)
            body = "\n".join(kept)
            lines.append(f"void xy_doc_sample_{index}(void) {{")
        # A diagnostic then reads `docs/api/API_REFERENCE.md:30:16: error: ...` - the page and the
        # page's line, which is the only location a reader can act on.
        lines.append(f'#line {start} "{doc}"')
        lines.append(body.rstrip())
        if as_body:
            lines.append("}")
    return "\n".join(hoisted + lines) + "\n"


def run_compiler(unit: str, roots: tuple[str, ...], compiler: str, root: Path,
                 application: bool = False) -> tuple[int, str]:
    """(exit code, diagnostics) for one unit, read from stdin so no temporary file is written."""
    command = [compiler, *COMPILE_ARGS]
    if application:
        command.append("-Wno-error=implicit-function-declaration")
    command += [f"-I{(root / name).as_posix()}" for name in roots]
    command += ["-x", "c", "-"]
    try:
        finished = subprocess.run(command, input=unit, capture_output=True, text=True, check=False)
    except OSError as error:
        return 1, f"cannot run `{compiler}`: {error}"
    return finished.returncode, (finished.stdout + finished.stderr).strip()


#: An implicitly declared function in the project's own namespace. `application` lets a page call the
#: reader's helpers, and this is the line that keeps the tolerance honest: `xy_*` is a call into this
#: tree, so a header has to declare it or the reader is being taught an API that does not exist.
#: Both quote styles are accepted - gcc quotes identifiers with U+2018/U+2019 under a UTF-8 locale.
IMPLICIT_PROJECT_CALL = re.compile(r"implicit declaration of function [‘'](xy_[A-Za-z0-9_]+)[’']")


def classify(sample: Sample, blocks: list[tuple[int, str]]) -> tuple[
        list[tuple[int, str]], list[str]]:
    """(blocks to compile, problems) for one page's fenced blocks.

    Every block is placed in exactly one class, which is what makes the page's coverage countable: a
    block is compiled, or it is named by a `fragments` marker as something other than a compile
    claim, or it is reported. The middle class exists because the alternative - quietly ignoring the
    blocks a selector does not match - is how a page keeps reporting clean while its newest sample is
    read by nobody.
    """
    problems: list[str] = []
    selected: list[tuple[int, str]] = []
    for start, body in blocks:
        # A `markers` entry is a claim that this block *is* a sample. Its presence makes the
        # ambiguity below reportable; without markers the page is judged whole, and a fragment
        # declaration is the only thing that can remove a block from that set.
        claimed = bool(sample.markers) and any(mark in body for mark in sample.markers)
        chosen = claimed or not sample.markers
        fragment = any(mark in body for mark in sample.fragments)
        if claimed and fragment:
            problems.append(
                f"{sample.doc}:{start}: this block is both compiled and declared a fragment "
                f"({', '.join(repr(m) for m in sample.fragments if m in body)}) - the page is "
                f"telling the guard two things about one block")
            continue
        if fragment:
            continue
        if not chosen:
            problems.append(
                f"{sample.doc}:{start}: this C block is neither compiled nor declared a fragment - "
                f"no selector matches it, so nothing reads it; widen the selectors or name its "
                f"placeholder in `fragments`")
            continue
        selected.append((start, body))
    stale = [mark for mark in sample.fragments if not any(mark in body for _, body in blocks)]
    if stale:
        problems.append(
            f"{sample.doc}: {', '.join(repr(mark) for mark in stale)} names no block on the page - "
            f"the declared fragment is gone, so the classification has drifted from the document")
    return selected, problems


def unlisted_pages(root: Path | None = None) -> list[str]:
    """Component guides that carry C blocks and are in neither `SAMPLES` nor `DEFERRED`.

    The coverage of this guard is a list, and a list's failure mode is silence: a new page appears,
    nobody adds it, and every run still says ok. Measured the day the component guides were listed,
    sixteen pages carried fifty-three C blocks. This is what makes that count checkable instead of
    remembered.
    """
    tree = ROOT if root is None else root
    judged = {sample.doc for sample in SAMPLES} | {doc for doc, _ in DEFERRED}
    problems: list[str] = []
    for page in sorted((tree / "docs" / "components").rglob("*.md")):
        relative = page.relative_to(tree).as_posix()
        if relative in judged:
            continue
        blocks = code_blocks(page)
        if blocks:
            problems.append(
                f"{relative}: carries {len(blocks)} fenced C blocks and is in neither `SAMPLES` nor "
                f"`DEFERRED` - a page nobody listed is a page nobody compiles")
    for doc, reason in DEFERRED:
        if not (tree / doc).is_file():
            problems.append(f"{doc}: deferred here, but the file is gone - the list has drifted")
        if not reason.strip():
            problems.append(f"{doc}: deferred without a reason - an unexplained omission is a silence")
    return problems


def findings(samples: tuple[Sample, ...] | None = None, root: Path | None = None,
             compiler: str = "cc") -> list[str]:
    """Every document's blocks must compile against the headers that document is about."""
    tree = ROOT if root is None else root
    judged = SAMPLES if samples is None else samples
    problems: list[str] = [] if samples is not None else unlisted_pages(tree)
    inspected = 0
    for sample in judged:
        path = tree / sample.doc
        if not path.is_file():
            problems.append(
                f"{sample.doc}: listed here as a document that makes a compile claim, but the file "
                f"is gone - the list has drifted from the tree")
            continue
        blocks = code_blocks(path)
        if not blocks:
            problems.append(
                f"{sample.doc}: no fenced C block was found - the extractor is broken, and a page "
                f"with no blocks compiles exactly as cleanly as a correct one")
            continue
        selected, class_problems = classify(sample, blocks)
        problems.extend(class_problems)
        if not selected and not sample.fragments:
            problems.append(
                f"{sample.doc}: none of its {len(blocks)} C blocks contains "
                f"{', '.join(repr(mark) for mark in sample.markers)} and none is declared a "
                f"fragment - the selector no longer points at the sample, and compiling nothing "
                f"reads the same as passing")
            continue
        if not selected:
            # A page that is entirely pattern - the driver template, whose every block names a file
            # the reader is about to create - still passes through `classify()`, so a new block on it
            # is reported as unclassified rather than silently unread.
            continue
        # A page with more than one sample needs one marker per sample, and with an OR selector a
        # single stale marker silently shrinks the compiled set instead of failing. Measured on
        # `porting.md`: two markers select the naming contract and the functional demo, and losing
        # either one leaves the other compiling - so each marker is required to still select a block.
        stale = [mark for mark in sample.markers
                 if not any(mark in body for _, body in blocks)]
        if stale:
            problems.append(
                f"{sample.doc}: {', '.join(repr(mark) for mark in stale)} appears in none of its "
                f"{len(blocks)} C blocks - that sample is no longer being compiled, and a silently "
                f"smaller set reports the same result as a clean one")
        inspected += len(selected)
        for unit_blocks in ([[one] for one in selected] if sample.per_block else [selected]):
            where = (f"its block at line {unit_blocks[0][0]}" if sample.per_block
                     else f"its {len(unit_blocks)} C blocks")
            status, output = run_compiler(translation_unit(sample.doc, unit_blocks,
                                                           sample.includes, sample.as_body),
                                          sample.roots, compiler, tree, sample.application)
            if status != 0:
                problems.append(
                    f"{sample.doc}: {where} do not compile against the headers it documents - a "
                    f"signature the header contradicts is a call the reader cannot write:\n{output}")
                continue
            # A green exit code is not yet a claim about the project's own API: `application` lets a
            # missing declaration slide by as a warning, so project-namespace calls are read back out
            # of the warnings.
            undeclared = sorted(set(IMPLICIT_PROJECT_CALL.findall(output)))
            if undeclared:
                problems.append(
                    f"{sample.doc}: {where} calls {', '.join(undeclared)} without a declaration - no "
                    f"header in this tree provides the name, so the page teaches an API that does "
                    f"not exist")
    if inspected == 0:
        problems.append(
            "no fenced C block was compiled - the walk is broken, and a walk that inspects nothing "
            "reports the same result as a clean one")
    return problems


def main(argv: list[str] | None = None) -> int:
    args = sys.argv[1:] if argv is None else argv
    compiler = "cc"
    for index, argument in enumerate(args):
        if argument == "--cc" and index + 1 < len(args):
            compiler = args[index + 1]
        elif argument.startswith("--cc="):
            compiler = argument.split("=", 1)[1]
    problems = findings(compiler=compiler)
    if problems:
        for problem in problems:
            print(problem)
        return 1
    blocks = 0
    fragments = 0
    for sample in SAMPLES:
        page = ROOT / sample.doc
        if not page.is_file():
            continue
        selected, _ = classify(sample, code_blocks(page))
        blocks += len(selected)
        fragments += len(sample.fragments)
    print(f"doc_samples_compile ok ({len(SAMPLES)} documents, {blocks} C blocks compiled with "
          f"`{compiler}` against {sum(len(s.roots) for s in SAMPLES)} include roots; "
          f"{fragments} blocks declared as patterns rather than samples; "
          f"{len(DEFERRED)} component pages deferred, each with its reason)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
