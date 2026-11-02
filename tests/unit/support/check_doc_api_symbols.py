#!/usr/bin/env python3
"""A getting-started document that teaches an API the repository does not have.

`docs/getting-started/QUICK_START.md` opens with a "first application" whose whole point is that
a reader can copy it. It used `xy_hal_pin_config_t`, `xy_hal_pin_init()`, `xy_hal_pin_write()`,
`xy_os_thread_new` spelled `xy_os_thread_create()`, and `xy_crypto.h`. None of those names exist:
the declaration is `xy_hal_gpio_config_t` / `xy_hal_gpio_init()` / `xy_hal_gpio_write()`, threads
are created by `xy_os_thread_new()` with an attribute struct, and the crypto umbrella is
`xy_tiny_crypto.h`. Compiled as written, the sample produces twelve errors - and nothing in the
repository noticed, because prose is not built.

So this judges the three things a copied snippet asserts:

  1. an identifier - every `xy_*` token inside a fenced C block must appear in some header;
  2. an include - every `#include "name.h"` must name a header that exists;
  3. a location - every backticked repository path (`examples/...`, `components/...`, `docs/...`)
     must exist.

The first two are checked against headers read through `read_code()`, so a symbol that only appears
in a comment does not count as declared. That direction matters: a name mentioned in a header's
prose is exactly the kind of thing a doc author would copy.

The third rule exists because the first two were not enough: `QUICK_START.md` told a new reader that
the sensor example was `examples/sensor_demo` and that the OSAL guide was
`docs/components/kernel/osal/README.md`. Measured against the tree, **four of the five "reading
order" links and three of the four example rows pointed at paths that do not exist** - the component
docs had been moved one level down (into `components/<x>/`) and nothing read the document as a set of
claims. A path is a claim.

Scope is the documents that instruct: `docs/getting-started/`, the HAL API reference
(`docs/hal/`), the curated API index (`docs/api/`), the HAL component's own usage docs
(`components/hal/`), the current design set (`docs/design/`), the hardware porting and board
guides (`docs/hardware/`), and - since every newcomer lands on
one - every `README.md` under `components/`. That last sweep is README-only on purpose:
`*_PLAN.md`, migration notes and the imported vendor trees are records, and judging them as usage
would punish writing the plan down, which is the mistake this repository has already made once.
`docs/history` records what was true when it was recorded and is not rewritten.

`docs/hardware` was added after its pages were measured: five files, one finding, and that finding
was a **counter-example**. `porting.md` prints `// ❌ 错误 - 不要添加平台后缀` above
`xy_hal_gpio_init_stm32()`, which is the paragraph's entire point - the function must not carry a
platform suffix. Reported as an undeclared symbol it is the guard demanding that a document stop
teaching the one thing it exists to teach. So a negative marker opens a region inside the block and
a positive marker closes it (`counter_example_lines()`): `❌ 错误` ... `✅ 正确` reads as one
counter-example followed by one claim, in the order the reader sees them. Only the marked lines are
excused - the rest of the block, and every other block in the document, is still judged.

The page's real defects were found by the *other* guard. What the reader was told to copy was a
`xy_hal_gpio_init(void *port, uint16_t pin, ...)` that the header declares as
`(xy_hal_gpio_port_t, uint8_t, ...)`, a `xy_hal_pin.c` that no longer exists, an `xy_hal_cfg.h`
that never existed, a `UART1` handle where the HAL takes a `UART_HandleTypeDef *`, and a
`if(PLATFORM STREQUAL "stm32")` dispatch where this tree selects `HAL_PLATFORM`. Names and paths are
this file's half; `check_doc_samples_compile.py` compiles the same page against the real headers.

`docs/reference` was measured and deliberately left out: its two files are TAOCP study notes whose
twenty C blocks are textbook algorithms, not claims about this API. Sweeping them would be a
category error of the same kind as reading a naming pattern as a call.

`docs/design` is the awkward directory and is judged the same way a README sweep is: only the pages
that describe the tree *now*. Thirty-two of its forty-five pages carry a date in the filename
(`xinyi-crypto-source-ownership-map-2026-08-12.md`, `xinyi-gui-font-asset-generation-proposal-2026-08-11.md`),
and a dated filename is the banner the exemption list has to insist on: the page says what was
decided on that day, and rewriting its paths would falsify the record rather than fix a defect. So
`record_document()` skips dated names inside `docs/design` only, and the thirteen undated pages -
`XinYi_Project_Overview.md`, `Code_Style_Design_Guide.md`, `test_layout_analysis.md`,
`xinyi-component-quality-loop.md`, `HAL_Component_Design_Overview.md` and their neighbours - are
judged. Measured on the first run: **fourteen findings across those thirteen pages**, two of them
the defect this whole file exists for.

The findings also taught the rule set three things it had been reading too literally:

- `xy_hal_<peripheral>_<operation>()` in `HAL_Component_Design_Overview.md` is a naming pattern. The
  token regex stopped at the `<` and reported `xy_hal_` as a call, which is the same category error
  the path rule already handled with `PLACEHOLDERS`: a name still being filled in is not a name.
- a module-file template that writes `@file module_name.c` and then `#include "module_name.h"` is
  describing the file a reader will create, not including a header that exists. The block's own
  `@file` names it, so the include is skipped when the two stems agree - and only then.
- `test_layout_analysis.md` records that `components/clib/xy_clib/test_filter.c` **已并入**
  `tests/unit/clib/test_clib_core.c`, and `xinyi-component-quality-loop.md` says the repository
  **没有** `tests/CMakeLists.txt`. Both are sentences about a path, neither is a pointer at one, and
  both were reported. A merge verb now joins the removal verbs, and a line asserting absence joins
  them too - an absent path a reader is *told* is absent is not a broken link.

`docs/api` was added after its page was measured: `API_REFERENCE.md` taught `xy_os_thread_create()`
and `xy_os_thread_t`, neither of which any header declares - the thread entry is `xy_os_thread_new()`
and the handle is `xy_os_thread_id_t`. Names are this guard's half of the claim;
`check_doc_samples_compile.py` compiles the same page's blocks against the real headers, because a
*signature* that disagrees with its header (`uint16_t pin` where the header takes `uint8_t`, an
`xy_sht30_init()` with three parameters where the function has two) is invisible to a symbol index
and obvious to a compiler.

The index page's *tables* were the next half. Each of its sections ends by pointing at
`docs/components/<x>/introduction.md` as the component's own documentation, and those seventeen pages
- the front door for every component - were outside the sweep, so nothing had read them. Measured on
the first run of that root: the OSAL page taught `xy_os_thread_create()`, `xy_os_thread_t`,
`xy_os_sem_create()` / `_post()` / `_wait()` / `xy_os_sem_t`, and `xy_os_timer_sw_create()`; the real
entry points are `xy_os_thread_new()` with an attribute struct, `xy_os_semaphore_new()` /
`xy_os_semaphore_acquire()` / `xy_os_semaphore_release()` over `xy_os_semaphore_id_t`, and
`xy_os_timer_new()`. The sensor page taught `xy_sensor_read()` / `xy_sensor_calibrate()` /
`xy_sensor_set_odr()`; the framework's read path is `sensor_read()` in `sensor_core.h` (or
`xy_sensor_sample_fetch()` in the canonical `xy_sensor.h`). Every one of those was a copy-paste
failure for the reader the page exists for, and each was one link away from the curated index.

Two rules had to exist before those pages could be judged, because the sweep's unit was the fenced
block and their defects are in prose:

- **a backticked `xy_name()` outside a fence is a call.** A table row `| `xy_sensor_read()` | 读取传感器 |`
  asserts a function the same way a snippet calls one, and the reader acts on it the same way - with a
  parameter sketch (`xy_os_delay(ms)`) or without one. Only the call form is judged: a bare backticked
  `xy_trace` or `xy_dm` names a *build target*, which is the thing CMake produces rather than a thing
  anyone calls, and reporting those would demand that the library names stop being written down.
- **a name this repository defines outside a header is still a name.** `xy_log_char()` is the trace
  component's weak output seam: it is defined in `xy_log.c` and declared in no header, on purpose, so
  that a platform backend or a host test can replace it. `xy_add_unit_test()` is a CMake function.
  Judging them as undeclared would delete the documentation of the two seams the tree is built
  around, so `provided_symbols()` reads weak `.c` definitions and CMake `function()`/`macro()` names
  alongside the headers - and only weakly-defined ones, because a strong definition in an unselected
  backend is how a stale API keeps a name (`xy_os_rtx5.c` still implements `xy_os_thread_create()`).

A name being filled in is not a name, here as everywhere else: a driver template writes `xy_xxx_t`
and the device page writes `xy_my_device_t` for the type the reader will write. Those are
`PLACEHOLDER_STEMS`, the prose half of the `PLACEHOLDERS` rule that already keeps `<name>` from being
read as a path.

Measured on the first run of that sweep: **nine component READMEs taught 25 names, headers and
paths that do not exist**. `clib/xy_clib` invented a whole list API (`xy_list_head`,
`xy_list_init_head`, `xy_container_of`, `xy_list.h`) where the real one is a bare
`xy_list_node *head` in `xy_common.h`; `ipc/xy_broker` called `xy_tick_get_ms()` and
`xy_log_info("TAG", ...)`; `kernel` blinked with `xy_gpio_toggle()`; `dm/factory` and
`dm/fee/fee_calculator` wrapped writes in `xy_enter_critical()`; `kernel/osal/baremetal` sent
readers to `components/kernel/xy_tick.h` and `components/osal/baremetal/`. Each one was a
copy-paste failure for the person the page exists for.

The HAL scope was added because that is exactly where the fiction survived: `components/hal/README.md`
opened with `xy_hal_gpio_bind("GPIOC.13")` / `xy_hal_uart_configure()` / an `xy_hal_gpio_t` handle and
"102 unified APIs", and `docs/hal/HAL_UNIFIED_API.md` repeated it with a "✅ 完成" header - while the
real interface takes a *platform handle* and every entry is `xy_hal_<peripheral>_<verb>(handle, ...)`.
A user-facing component README is a usage claim in the same sense a quick start is; a design note is
not, and the seven pre-2.0 HAL notes in that directory are listed in `EXEMPT` with that reason rather
than silently forgiven.

Skipping a document is the one move that can make this guard lie, so the list is judged too
(`exemption_findings()`): an exempt page must open with the deprecation banner, because it is a page
someone still opens and the only page in the sweep still teaching `xy_hal_pin_init()`. Before that
rule, all seven opened with their original claim - "**状态**: ✅ 完成", "## 快速开始", "All XY HAL
functions now return..." - and a reader had no way to tell a live guide from a 2025-10 record. An
entry that names a file which no longer exists is reported for the same reason.

Boundaries, stated rather than hidden: for *symbols*, fenced ```c / ```cpp blocks and backticked
`xy_name()` calls in prose are judged, and nothing else - a bare backticked name is a target or a
module, not a call. The fences are read by the shared `source_scan.fenced_blocks()` so a `~~~` block,
a nested four-backtick block and a block whose terminator went missing are all read the way CommonMark
says rather than by one regex over the whole document. A block may declare its own helper, so a
name the block defines itself is not reported. The path rule reads prose as well, because a table
row names a location as firmly as a sentence does; a line that tells the reader to *create* the path
(`创建` / `新建` / `create`) is an instruction, not a claim that it exists, and a placeholder
(`<name>`, `...`) is not a path yet. A line that records a path was *removed* or *migrated*
(`removed` / `删除` / `移除` / `migrated`) is history rather than a pointer - but the verb is what
matters, not the age: "the legacy `components/charger/` framework" still fails, because a reader can
still try to open it.

Presence is not correctness: this says a symbol exists, not that the sample uses it properly. The
two samples were also compiled - as written, against the real include roots, for both crypto
include orders - because the first draft of this guard would have passed a sample that included
two headers which declared `xy_crc32` with different parameter types. The stale second `xy_crc.h`
that conflict came from is now a compatibility forward to the CRC component's own header; that is
a claim about the build, and `gcc -fsyntax-only` is what proved it.
"""

from __future__ import annotations

import difflib
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from source_scan import code_only, fenced_blocks, read_code  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
#: Directories judged as usage. `docs/api` is the curated API index - the page a reader opens to
#: learn a signature, which makes it the most literal example of the thing this guard exists for.
#: `docs/design` is here because it is where the current design lives; its dated pages are records
#: and `record_document()` skips them (see the module docstring).
DOC_ROOTS = ("docs/getting-started", "docs/hal", "docs/api", "docs/design", "docs/hardware",
             "docs/components", "components/hal")
#: A date in the filename is a record's own banner: `xinyi-ipc-event-group-proposal-2026-08-08.md`
#: states what was decided on 2026-08-08. Rewriting its paths would replace the record with a
#: quieter document, and the same reasoning that keeps `docs/history` out of this sweep applies.
DATED_RECORD = re.compile(r"-\d{4}-\d{2}-\d{2}\.md$")
#: Directories where a dated filename means record. Kept narrow on purpose: `docs/design` is the one
#: directory whose undated pages are claims and whose dated pages are decisions, so the rule earns
#: its keep there and nowhere else needs the exception.
RECORD_ROOTS = ("docs/design",)
#: Roots swept for `README.md` only. A component README is its front door - the page a newcomer
#: copies from - so every one of them makes a compile claim. Restricting this sweep to README
#: keeps design records (`*_PLAN.md`, migration notes, vendor articles) out of it; those are
#: history and get their own slices rather than being judged as usage.
README_ROOTS = ("components",)
#: Imported vendor trees keep their own documentation. Their READMEs quote Espressif/AWS APIs by
#: design, so judging them against this project's headers would report correct vendor docs forever.
VENDOR_ROOTS = (
    "components/third_party",
    "components/net/at/AT-Command-V2",
    "components/net/at/FreeRTOS-Cellular-Interface",
)
INDEX_ROOTS = ("components", "boards")
#: Roots that supply *header names* a document may legitimately include. Vendor SDKs live here, so
#: `#include "stm32l4xx_hal.h"` in a porting note is a real header and not a broken claim. Symbols
#: are still judged against INDEX_ROOTS only - a vendor header cannot vouch for an `xy_*` name.
HEADER_ROOTS = ("components", "boards", "MCU", "third_party")
#: Fenced-block languages that make a compile claim. A `bash` block is judged by a different rule
#: (its `make <target>` names a target, not a symbol).
CODE_LANGUAGES = ("c", "cpp", "c++")
#: Documents exempted from the sweep, each with the reason. The seven pre-2.0 HAL notes describe the
#: `*_dev.h` / `xy_hal_pin_*` generation that never landed; they are design and migration records,
#: not usage, and rewriting or retiring them is its own slice rather than something to forgive here.
#:
#: An exemption is not a pardon, so it carries two obligations - see `exemption_findings()`:
#:
#:   1. the document must announce itself. A skipped page is still a page someone opens, and it is
#:      the one page in the sweep that still teaches `xy_hal_pin_init()`. If the file does not open
#:      with the deprecation banner, the exemption is a silent lie: the guard passes, the reader is
#:      misled, and nothing anywhere says so. Measured before this rule existed: all seven opened
#:      with their old claim ("**状态**: ✅ 完成", "## 快速开始") and no warning at all.
#:   2. the entry must name a file that exists. An exemption for a deleted path is dead weight that
#:      hides the fact that the list is no longer describing this tree.
EXEMPT_BANNER = "**已废弃 / Deprecated**"
#: How far down the document the banner may sit. The point is that it is read *before* the content
#: it disclaims - a footnote at the bottom would satisfy a substring search and inform no one.
EXEMPT_BANNER_WINDOW = 15
EXEMPT: dict[str, str] = {
    "components/hal/ERROR_CODES.md":
        "2025-10 error-code reference from the pre-2.0 HAL; the enum itself lives in "
        "xy_hal_error.h and the numbers agree, but the file is a migration record",
    "components/hal/HAL_UNIFIED_API_SPEC.md":
        "the specification for the *_dev.h generation that was never implemented",
    "components/hal/INTEGRATION_GUIDE.md":
        "integration guide for the same never-implemented xy_hal_pin_* generation",
    "components/hal/RESTRUCTURING_NOTES.md":
        "v1.0 -> v2.0 restructuring notes; describes the old layout on purpose",
    "components/hal/RETURN_TYPE_UPDATE.md":
        "2025-10 return-type standardisation record",
    "components/hal/VALIDATION_STRATEGY.md":
        "2026-03 validation plan written against the never-implemented *_dev.h API",
    "components/hal/hal_status.md":
        "placeholder status stub with no example code",
}

TOKEN = re.compile(r"\bxy_[A-Za-z0-9_]+\b")
#: A backticked call in prose is the same claim a snippet makes:
#: `| `xy_sensor_read()` | 读取传感器 | `sensor_core.h` |` tells a reader to call a function. Only the
#: `()` form is judged. A bare backticked `xy_trace` / `xy_dm` / `xy_pipe` names the *build target*
#: CMake produces, which is a thing to link against rather than a thing to call, and every component
#: README in this tree has to be able to say which library it builds.
PROSE_CALL = re.compile(r"`(xy_[A-Za-z0-9_]+)\([^)`]*\)`")
#: Roots that hold *implementations*. Only a **weak** definition is counted: it is a seam this tree
#: declares on purpose - `xy_log_char()` is the trace component's weak character sink, headerless so a
#: platform backend or a host test replaces it, and documenting it is the point. A *strong* definition
#: in an unselected backend is the opposite: `components/kernel/osal/backend/rtx5/xy_os_rtx5.c` still
#: implements the pre-2.0 `xy_os_thread_create(name, func, arg, attr)` while every selected backend
#: implements `xy_os_thread_new()`, so reading strong definitions would have let a stale copy vouch for
#: the name the API index taught. A `.c` is not a declaration just because it compiles on its own.
IMPLEMENTED = re.compile(
    r"^(?!static\b)(?!typedef\b)(?:[A-Za-z_][A-Za-z0-9_]*[ \t\*]+)+\**(xy_[A-Za-z0-9_]+)[ \t]*\(")
WEAK_MARKER = re.compile(r"weak", re.IGNORECASE)
IMPLEMENTATION_ROOTS = ("components", "boards")
#: CMake `function()` / `macro()` names are documented API too - `xy_add_unit_test()` is the helper
#: every new unit target is written with, and it lives in `tests/unit/CMakeLists.txt`.
CMAKE_FUNCTION = re.compile(r"^[ \t]*(?:function|macro)[ \t]*\([ \t]*(xy_[A-Za-z0-9_]+)", re.MULTILINE)
CMAKE_ROOTS = ("cmake", "components", "boards", "tests")
#: A name still being filled in is not a name: a driver template writes `xy_xxx_t`, the device page
#: writes `xy_my_device_t` for the reader's own type. Prose half of `PLACEHOLDERS`.
PLACEHOLDER_STEMS = ("xxx",)
PLACEHOLDER_PREFIXES = ("xy_my_",)
INCLUDE = re.compile(r'^\s*#\s*include\s+"([^"]+)"')
DEFINES_ITSELF = re.compile(r"^\s*#\s*define\s+(xy_[A-Za-z0-9_]+)")
#: The `@file` line of a Doxygen header block: `@file module_name.c` names the file the block is
#: about, which is how a template block declares that the header it includes is one a reader will
#: create. Only a stem match makes an include a placeholder - everything else is still judged.
OWN_FILE = re.compile(r"[@\\]file\s+([A-Za-z0-9_]+)\.[ch]\b")
#: A token that runs into `<` is a naming pattern, exactly as `PLACEHOLDERS` treats a path:
#: `xy_hal_<peripheral>_<operation>()` says how names are shaped, not that `xy_hal_` is a function.
PATTERN_MARKERS = ("<", ">")
#: A block may show what *not* to write. `docs/hardware/porting.md` prints
#: `// ❌ 错误 - 不要添加平台后缀` above `xy_hal_gpio_init_stm32()`, and that is the whole point of
#: the paragraph: the name must not carry a platform suffix. Reported as an undeclared symbol it is
#: the guard telling a document to stop teaching the one thing it exists to teach - the same
#: category error as reading `<name>` as a path. A negative marker opens a region inside the block
#: and a positive marker closes it, so `❌ 错误` ... `✅ 正确` reads as one counter-example followed
#: by one claim, in the order the reader meets them.
NEGATIVE_MARKERS = ("❌", "错误", "不要", "wrong", "Wrong", "incorrect", "Incorrect")
POSITIVE_MARKERS = ("✅", "正确", "right", "Right", "correct", "Correct")

#: Top-level directories a document may point a reader at. Restricting the prefix to these keeps
#: the rule off prose that merely contains a slash.
PATH_ROOTS = ("examples", "components", "boards", "tests", "projects", "tools", "docs")
PATH_REFERENCE = re.compile(
    r"`((?:" + "|".join(PATH_ROOTS) + r")/[A-Za-z0-9_./-]+)`")
#: A line that tells the reader to create the path is not a claim that it exists today.
CREATION_WORDS = ("创建", "新建", "create", "Create", "your")
#: A line that records where something *used to be* is history, not a pointer. Only removal and
#: migration verbs count: a bare mention of an old directory ("the legacy `components/charger/`
#: framework") still fails, because a reader can still try to open it. Merge and converge verbs are
#: in the same family - `test_filter.c` was not deleted, it was folded into `test_clib_core.c`.
HISTORY_WORDS = ("removed", "Removed", "no longer", "deleted", "删除", "移除", "已迁移", "migrated",
                 "renamed", "并入", "合并", "收敛", "merged", "converged")
#: A line that asserts the path is *absent* is not a pointer at it - the reader has been told.
#: Unlike a bare old-path mention, there is no way to follow this sentence and be surprised.
#: `不再存在` is the same statement in the tense the HAL's own README needed it in ("`xy_hal_pin_init()` /
#: `xy_hal_pin_config_t` **不再存在**"), and it is not reachable from `不存在` by substring.
ABSENCE_WORDS = ("不存在", "不再存在", "没有", "does not exist", "not present", "nothing at")
#: ... and a placeholder is not a path yet.
PLACEHOLDERS = ("...", "<", ">", "*")


def record_document(relative: str) -> bool:
    """True when a swept path is a dated record rather than a page describing the tree today.

    Only inside `RECORD_ROOTS`. The date must sit in the filename, because that is what a reader
    sees in a directory listing and what the author wrote to announce "this is what we decided
    then" - the same announcement the `EXEMPT` list demands of a banner, in the only place a page
    can make it without editing its body.
    """
    if not any(relative == root or relative.startswith(root + "/") for root in RECORD_ROOTS):
        return False
    return bool(DATED_RECORD.search(relative.rsplit("/", 1)[-1]))


def documents() -> list[Path]:
    found: list[Path] = []
    for root in DOC_ROOTS:
        directory = ROOT / root
        if not directory.is_dir():
            continue
        for path in sorted(directory.rglob("*.md")):
            relative = path.relative_to(ROOT).as_posix()
            if relative in EXEMPT or record_document(relative):
                continue
            found.append(path)
    for root in README_ROOTS:
        directory = ROOT / root
        if not directory.is_dir():
            continue
        for path in sorted(directory.rglob("README.md")):
            relative = path.relative_to(ROOT).as_posix()
            if relative in EXEMPT:
                continue
            if any(relative == vendor or relative.startswith(vendor + "/")
                   for vendor in VENDOR_ROOTS):
                continue
            found.append(path)
    return sorted(set(found))


def exemption_findings(root: Path | None = None,
                       exempt: dict[str, str] | None = None) -> list[str]:
    """Problems with the exemption list itself: each entry must exist and must announce itself.

    Called from `main()` rather than `findings()` on purpose. `findings()` judges *documents*; this
    judges the *list of documents not judged*, which is the one place the sweep can go quietly
    wrong. Injected `root`/`exempt` so the self-test can drive both reds without touching the tree.
    """
    tree = ROOT if root is None else root
    entries = EXEMPT if exempt is None else exempt
    problems: list[str] = []
    for relative in sorted(entries):
        path = tree / relative
        if not path.is_file():
            problems.append(
                f"EXEMPT names `{relative}`, which is not a file in this tree - an exemption for a "
                f"path nobody can open describes a list that has drifted from the repository")
            continue
        head = path.read_text(encoding="utf-8", errors="replace").splitlines()
        window = "\n".join(head[:EXEMPT_BANNER_WINDOW])
        if EXEMPT_BANNER not in window:
            problems.append(
                f"`{relative}` is exempt from this sweep but does not open with "
                f"`{EXEMPT_BANNER}` - it is skipped here and still teaches its old API to whoever "
                f"opens it, so the exemption informs no one. Add the banner or remove the entry")
    return problems


def header_index(roots: tuple[str, ...] = INDEX_ROOTS,
                 header_roots: tuple[str, ...] = HEADER_ROOTS) -> tuple[set[str], set[str]]:
    """(every identifier a header under ``roots`` declares or uses, every header basename).

    The two sets deliberately have different reach: a symbol must be declared by project code, while
    a *header name* the document includes may live in a vendor SDK under ``MCU/`` or ``third_party/``.
    Reading vendor headers for symbols would let an unrelated SDK name vouch for an ``xy_*`` token.
    """
    symbols: set[str] = set()
    names: set[str] = set()
    for root in roots:
        directory = ROOT / root
        if not directory.is_dir():
            continue
        for path in directory.rglob("*.h"):
            symbols.update(TOKEN.findall(read_code(path)))
    for root in header_roots:
        directory = ROOT / root
        if not directory.is_dir():
            continue
        for path in directory.rglob("*.h"):
            names.add(path.name)
    return symbols, names


def provided_symbols(root: Path | None = None,
                     implementation_roots: tuple[str, ...] = IMPLEMENTATION_ROOTS,
                     cmake_roots: tuple[str, ...] = CMAKE_ROOTS) -> set[str]:
    """Names this repository provides without a header: weak `.c` seams and CMake functions.

    `xy_log_char()` is the trace component's weak character seam, declared in no header on purpose
    because the point is that a platform backend or a host test replaces it; `xy_add_unit_test()` is
    the CMake helper every new unit target is written with.

    The weak requirement is what keeps this index honest. A strong `.c` definition can be a leftover:
    the rtx5 backend still implements the pre-2.0 `xy_os_thread_create(name, func, argument, attr)`
    signature while every selected backend implements `xy_os_thread_new(func, argument, attr)`, so
    reading strong definitions would let the stale copy vouch for the name the API index taught. A
    weak definition is the opposite kind of statement - the tree declaring "this symbol exists and is
    meant to be replaced", which is exactly the case that must stop being reported.

    Only column 0 counts (an indented call site is not a provision), and a marker on the same line
    (`XY_LOG_WEAK void xy_log_char(...)`) or the line above (`__attribute__((weak))` then the
    signature) makes a definition weak.
    """
    tree = ROOT if root is None else root
    provided: set[str] = set()
    for top in implementation_roots:
        directory = tree / top
        if not directory.is_dir():
            continue
        for path in directory.rglob("*.c"):
            previous = ""
            for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
                match = IMPLEMENTED.match(line)
                if match and (WEAK_MARKER.search(line) or WEAK_MARKER.search(previous)):
                    provided.add(match.group(1))
                if line.strip():
                    previous = line
    for top in cmake_roots:
        directory = tree / top
        if not directory.is_dir():
            continue
        for pattern in ("CMakeLists.txt", "*.cmake"):
            for path in directory.rglob(pattern):
                provided.update(m.group(1) for m in CMAKE_FUNCTION.finditer(
                    path.read_text(encoding="utf-8", errors="replace")))
    return provided


def code_blocks(text: str) -> list[tuple[int, list[str]]]:
    """(first line number, lines) for every fenced C/C++ block, read by the shared reader."""
    blocks: list[tuple[int, list[str]]] = []
    for info, start, body in fenced_blocks(text):
        language = info.split()[0].lower() if info.split() else ""
        if language in CODE_LANGUAGES:
            blocks.append((start, body.splitlines()))
    return blocks


def prose_lines(text: str) -> list[tuple[int, str]]:
    """(line number, line) for every line that is not inside any fence.

    Prose is where the API index's tables live, so it has to be read - but a block's own text is
    already judged by the symbol rule, and reading it twice would report one defect twice. The mask
    is line-based on purpose: every fence the shared reader sees, whatever language, because a `c`
    block and a ```` ```text ```` block are equally not prose.
    """
    lines = text.splitlines()
    masked = list(lines)
    for _info, start, body in fenced_blocks(text):
        count = body.count("\n") + (0 if body.endswith("\n") else 1)
        for index in range(start, min(start + count, len(masked))):
            masked[index] = ""
    return [(number, line) for number, line in enumerate(masked, start=1) if line]


def prose_call_claims(text: str) -> list[tuple[int, str]]:
    """(line number, symbol) for every backticked `xy_name()` call this document's prose makes."""
    claims: list[tuple[int, str]] = []
    for number, line in prose_lines(text):
        for match in PROSE_CALL.finditer(line):
            claims.append((number, match.group(1)))
    return claims


def is_placeholder(token: str) -> bool:
    """True for a name the reader is meant to fill in (`xy_xxx_t`, `xy_my_device_t`)."""
    return (any(stem in token for stem in PLACEHOLDER_STEMS)
            or any(token.startswith(prefix) for prefix in PLACEHOLDER_PREFIXES))


def counter_example_lines(lines: list[str]) -> set[int]:
    """Indices of the block lines a negative marker has put out of scope.

    Line-scoped on purpose. A block that opens with `// ❌ 错误` and never closes the region marks
    everything after it; one that closes with `// ✅ 正确` goes back to being a claim. Excusing the
    whole block instead would let a document hide a genuine defect above the counter-example, and
    excusing the whole document would let it hide one in the next block.
    """
    wrong = False
    marked: set[int] = set()
    for index, line in enumerate(lines):
        if any(marker in line for marker in POSITIVE_MARKERS):
            wrong = False
        elif any(marker in line for marker in NEGATIVE_MARKERS):
            wrong = True
        if wrong:
            marked.add(index)
    return marked


def path_references(text: str) -> list[tuple[int, str]]:
    """(line number, path) for every repository path this document points a reader at.

    Placeholders and lines that ask the reader to create something are skipped: neither asserts
    that the path is there today, and reporting them would punish a document for explaining how to
    start a new one.
    """
    found: list[tuple[int, str]] = []
    for number, line in enumerate(text.splitlines(), start=1):
        if any(word in line for word in CREATION_WORDS):
            continue
        if any(word in line for word in HISTORY_WORDS):
            continue
        if any(word in line for word in ABSENCE_WORDS):
            continue
        for match in PATH_REFERENCE.finditer(line):
            token = match.group(1).rstrip("/")
            if any(mark in token for mark in PLACEHOLDERS):
                continue
            found.append((number, token))
    return found


def findings(paths: list[Path] | None = None,
             symbols: set[str] | None = None,
             names: set[str] | None = None,
             provided: set[str] | None = None,
             root: Path | None = None) -> list[str]:
    walked_symbols: set[str] = set()
    walked_names: set[str] = set()
    if symbols is None or names is None:
        walked_symbols, walked_names = header_index()
    index_symbols = symbols if symbols is not None else walked_symbols
    index_names = names if names is not None else walked_names
    index_provided = provided_symbols() if provided is None else provided
    tree = ROOT if root is None else root
    sweep = paths is None
    candidates = documents() if paths is None else paths
    problems: list[str] = []
    inspected_blocks = 0
    inspected_paths = 0
    inspected_claims = 0
    for path in candidates:
        text = path.read_text(encoding="utf-8", errors="replace")
        relative = path.relative_to(ROOT).as_posix() if path.is_relative_to(ROOT) else path.name
        for number, token in path_references(text):
            inspected_paths += 1
            if not (tree / token).exists():
                problems.append(
                    f"{relative}:{number}: points readers at `{token}`, which does not exist in "
                    f"this tree - a reader who follows it finds nothing")
        # A table row is a call claim: `| `xy_sensor_read()` | 读取传感器 |` tells the reader which
        # function to write, and `xy_os_delay(ms)` sketches the same claim with a typed placeholder.
        # The only exemption is an absence statement ("没有 `xy_os_thread_create()`") plus
        # `is_placeholder()` - a name in a call is never a path to be created, and the path rule's
        # removal verbs (`创建任务`, `删除任务`) are what a table's 说明 column *does*. Reusing them
        # here excused exactly the three rows that were wrong: `xy_os_thread_create()` (创建任务),
        # `xy_os_thread_delete()` (删除任务) and `xy_os_timer_sw_create()` (创建定时器).
        for number, token in prose_call_claims(text):
            inspected_claims += 1
            line = text.splitlines()[number - 1]
            if any(word in line for word in ABSENCE_WORDS):
                continue
            if is_placeholder(token):
                continue
            if token in index_symbols or token in index_provided:
                continue
            near = difflib.get_close_matches(token, index_symbols, n=1)
            hint = f" (did you mean {near[0]}?)" if near else ""
            problems.append(
                f"{relative}:{number}: tells readers to call {token}(){hint}, which no header "
                f"declares and no weak definition provides - the reader who writes it cannot link")
        for start, lines in code_blocks(text):
            inspected_blocks += 1
            local = {m.group(1) for line in lines for m in [DEFINES_ITSELF.match(line)] if m}
            # A template block names the file it is about (`@file module_name.c`) and then includes
            # the header that file will have. Matching stems make the include a placeholder for the
            # reader's own module; any other missing header is still a broken claim.
            own = {m.group(1) for line in lines for m in [OWN_FILE.search(line)] if m}
            # A sample's own comments are prose: a block that writes down "the old spelling was
            # xy_hal_pin_write()" is explaining, not calling. Same rule as read_code() for headers.
            code = code_only("\n".join(lines)).splitlines()
            wrong = counter_example_lines(lines)
            for offset, line in enumerate(code):
                where = f"{relative}:{start + offset}"
                if offset in wrong:
                    # A counter-example is the document saying this name must *not* be written.
                    continue
                include = INCLUDE.match(line)
                if include:
                    header = Path(include.group(1)).name
                    if Path(header).stem in own:
                        continue
                    # `#include "xy_xxx.h"` is the header the reader's own driver will have, the same
                    # placeholder the template's `@file module_name.c` line declares.
                    if is_placeholder(Path(header).stem):
                        continue
                    if header not in index_names:
                        problems.append(
                            f"{where}: includes \"{header}\", which is not a header in this repository - "
                            f"the reader cannot compile it")
                    continue
                for match in TOKEN.finditer(line):
                    token = match.group()
                    # `xy_hal_<peripheral>_<operation>()` is a naming pattern: the `<` says the name
                    # is not finished. Reporting `xy_hal_` as a call is the same mistake as reading
                    # a `<name>` path as a location.
                    if line[match.end():match.end() + 1] in PATTERN_MARKERS:
                        continue
                    if is_placeholder(token):
                        continue
                    if token in index_symbols or token in local or token in index_provided:
                        continue
                    near = difflib.get_close_matches(token, index_symbols, n=1)
                    hint = f" (did you mean {near[0]}?)" if near else ""
                    problems.append(
                        f"{where}: calls {token}{hint}, which no header declares and no weak "
                        f"definition provides - a copied sample that does not compile is not an "
                        f"example")
    if inspected_blocks == 0:
        problems.append(
            f"no fenced C block was found under {', '.join(DOC_ROOTS)} - the walk is broken, and a "
            f"walk that inspects nothing reports the same result as a clean one")
    if sweep and inspected_paths == 0:
        problems.append(
            f"no repository path reference was found under {', '.join(DOC_ROOTS)} - the path walk "
            f"is broken, and a walk that inspects nothing reports the same result as a clean one")
    return problems


def judge(symbols: set[str] | None = None,
          names: set[str] | None = None,
          provided: set[str] | None = None) -> list[str]:
    """Every rule this guard enforces, over the real tree.

    One entry point so the self-test can drive the whole judgement (including the exemption list)
    with `ROOT`/`DOC_ROOTS`/`EXEMPT` substituted, instead of proving each rule separately and hoping
    `main()` happens to call them all. `main()` hands in the indexes it built once, so the success
    line can report what was measured without walking the headers twice.
    """
    if symbols is None or names is None:
        walked_symbols, walked_names = header_index()
        symbols = walked_symbols if symbols is None else symbols
        names = walked_names if names is None else names
    return (findings(symbols=symbols, names=names, provided=provided)
            + exemption_findings())


def main() -> int:
    symbols, names = header_index()
    provided = provided_symbols()
    problems = judge(symbols, names, provided)
    if problems:
        for problem in problems:
            print(problem)
        return 1
    claims = sum(len(prose_call_claims(path.read_text(encoding="utf-8", errors="replace")))
                 for path in documents())
    print(f"doc_api_symbols ok ({len(documents())} documents, {claims} prose call claims, "
          f"{len(EXEMPT)} exempt and announced, {len(symbols)} declared identifiers, "
          f"{len(provided)} implemented outside headers, {len(names)} headers)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
