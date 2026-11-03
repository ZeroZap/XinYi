#!/usr/bin/env python3
"""One type name, two definitions, and whoever included last decides what it means.

A typedef is a promise about a name. Two headers defining the same name with different bodies do
not merge, and nothing in the build complains until both are visible at once - at which point one is
a hard error and the other is a silent reinterpretation of the same bits.

Both directions are live in this tree, measured rather than supposed:

  * `xy_sensor_type_t` is `TEMP=1` in `components/sensor/inc/xy_sensor_device.h` and `TEMP=0` in
    `components/device/inc/xy_device.h`. A translation unit including both gets
    `redeclaration of enumerator 'XY_SENSOR_TYPE_TEMP'` and
    `conflicting types for 'xy_sensor_type_t'` - and the same TU also loses
    `xy_sensor_value_t`, `struct xy_sensor_device`, `xy_sensor_device_t`, and two incompatible
    prototypes for `xy_sensor_sample_fetch`. A TU including one has temperature at 1 and humidity
    at 2; a TU including the other has them at 0 and 1. Nothing says which subsystem it is in.
  * `xy_fuel_gauge_status_t` is declared in both `components/pm/inc/xy_pm.h` and
    `components/fuel_gauge/inc/xy_fuel_gauge.h` - with different *member names*
    (`XY_FUEL_GAUGE_*` against `XY_FG_*`). The same enum name, two spellings of every value.

The rule judges definition *bodies*, not names: `typedef enum {...} X;` and `typedef struct {...} X;`
with the same name are a collision even if the tags differ. Bodies are compared after comments are
stripped through `read_code()`, so a comment that documents a difference does not create one, and
whitespace is normalised so reformatting does not either.

Two things are deliberately out of scope, each written down rather than silently skipped:

  * Vendored subtrees that ship their own copies of the STM32 standard-peripheral and CMSIS headers.
    They are sample code inside a vendored library, they are not built into this project, and
    reporting their disagreements would bury the real ones under eight STM32F0-versus-STM32F4
    differences that nobody here can act on.
  * Collisions this repository has decided to live with. Each needs the reason, because "we meant to
    fix it" and "nobody looked" are the same absent entry otherwise - and a pinned name whose
    definitions converge, or whose files disappear, has to be reported so the pin cannot rot.
"""

from __future__ import annotations

import os
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from source_scan import read_code  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]

#: Directories whose headers are copied in from somewhere else and never compiled into this project.
#: A vendored sample that ships both an STM32F0 and an STM32F4 peripheral library disagrees with
#: itself about `GPIOSpeed_TypeDef` by construction, and that disagreement is not this repository's.
VENDORED: dict[str, str] = {
    "components/crypto/curve25519-cortexm0-20150813":
        "vendored curve25519 implementation, ships the STM32F0 peripheral headers it was built against",
    "components/net/at/AT-Command-V2":
        "vendored AT command samples, ships both CMSIS and the STM32F4 standard-peripheral driver",
    "components/net/at/ats/ats-simcom":
        "SIMCOM A76XX AT command reference: the vendor's own header defines the command-mode enum, and "
        "its sources are not built (the net CMakeLists lists them commented out) - a vendor header "
        "against ours is a name clash, not two of our own definitions",
}

#: Names with more than one definition that this repository has not resolved. The reason is the point:
#: resolving any of these means deciding which subsystem owns the name, and that is an architecture
#: decision, not a cleanup. Recording them here keeps the set from growing while the decision waits.
#:
#: Two shapes recur, and both are structural rather than accidental:
#:
#:   * one component shipped twice - an `inc/` copy and a root or sub-directory copy (crypto's
#:     `xy_aes.h`/`xy_sha256.h` against `xy_tiny_crypto.h`, gui's `xy_gui.h` against
#:     `inc/xy_gui_widget.h`, pm's `inc/xy_pm.h` against `fuel-gauge/xy_fuel_gauge.h`);
#:   * two parallel frameworks covering one subject (device against sensor, display driver against
#:     the sensor tree's copy of the same driver header).
PINS: dict[str, str] = {
    # -- one component, two copies of the same header tree -------------------------------------------
    "xy_aes_ctx_t":
        "components/crypto/inc/xy_aes.h and components/crypto/inc/xy_tiny_crypto.h - same directory, "
        "different layout (key[32]+round_keys[15][16] against round_keys[60]), so a context allocated "
        "against one and filled by the other overruns; only one of the two can be included in a TU",
    "xy_sha256_ctx_t":
        "components/crypto/inc/xy_sha256.h against components/crypto/inc/xy_tiny_crypto.h - same "
        "directory, so include order cannot disambiguate it",
    "xy_chacha20_ctx_t":
        "components/crypto/inc/xy_chacha20poly1305.h against components/crypto/xy_chacha/"
        "xy_chacha20_poly1305.h - the inc/ header and the implementation directory's own header",
    "xy_poly1305_ctx_t":
        "same pair as xy_chacha20_ctx_t",
    "xy_gui_color_t":
        "components/gui/xy_gui.h against components/gui/inc/xy_gui_widget.h - the widget header and the "
        "top-level header each define the colour type",
    "xy_gui_rect_t":
        "same pair as xy_gui_color_t",
    "xy_rgb_matrix_config_t":
        "the LCD driver tree and the serial-RGB driver tree each own a header called xy_rgb_matrix.h",
    "xy_div_t":
        "components/clib/xy_clib/xy_math.h against components/clib/xy_clib/xy_stdlib.h - identical "
        "today, which is the only thing keeping two owners of one clib type harmless",
    "xy_ldiv_t": "same pair as xy_div_t",
    "xy_lldiv_t": "same pair as xy_div_t",
    # -- two parallel frameworks over one subject ---------------------------------------------------
    # The owner is decided: components/device is canonical, and the components/sensor copies are the
    # part of the old framework that has not been migrated yet. They are still on this list because the
    # migration is not done, not because the answer is open - and the day the last consumer moves, the
    # stale pin above will report itself and this entry can be deleted.
    "xy_sensor_type_t":
        "components/device/inc (TEMP=0, ACCELEROMETER/GYROSCOPE/MAGNETOMETER) is canonical; "
        "components/sensor/inc (TEMP=1, ACCEL/GYRO/MAGN) is the un-migrated old framework - including "
        "both is a hard error and including one leaves every sensor type offset by one",
    "xy_sensor_device_t":
        "same pair as xy_sensor_type_t - canonical is components/device/inc",
    "xy_sensor_value_t":
        "components/device/inc/xy_device.h is canonical; components/sensor/inc/xy_sensor_channel.h is "
        "part of the un-migrated old framework",
    "xy_fuel_gauge_status_t":
        "components/pm/inc spells the members XY_FUEL_GAUGE_*, components/fuel_gauge/inc spells them "
        "XY_FG_* - one enum name, two vocabularies, and which one a consumer sees depends on include "
        "order",
    "xy_fuel_gauge_config_t":
        "components/pm/inc/xy_pm.h against components/pm/fuel-gauge/xy_fuel_gauge.h",
    "xy_battery_state_t":
        "same pair as xy_fuel_gauge_config_t",
    "xy_oled_ssd1306_t":
        "components/drivers/display/oled/ssd1306 and components/sensor/inc both publish a header of "
        "that name - the driver's owner and a copy in the sensor tree",
    "xy_i2c_dev_t":
        "components/drivers/sensor/distance/vl53l1x/xy_vl53l1x.h against the sgp40 header - identical "
        "today; two drivers declaring their own i2c context type under one shared name",
    # -- a project carrying two generations of its own headers --------------------------------------
    "bank_info_t":
        "projects/Bank/bank.h against projects/Bank/bank/inc/bank-def.h - the same project's two "
        "header generations",
    "bank_chrg_state_t":
        "projects/Bank/bank.h declares two members and projects/Bank/bank/inc/bank-def.h declares a "
        "different set starting at bank_chrg_state_reseting",
    "bank_chrg_stop_reason_t":
        "same pair as bank_chrg_state_t",
}

#: Where a type definition can begin. The body is read by brace matching rather than by a pattern,
#: because `typedef struct { int a; } x_t;` puts a semicolon inside the body: a `[^;]*` pattern cannot
#: span it, which silently skipped every struct and left the rule judging enums only.
TYPEDEF_START = re.compile(r"\btypedef\b")

#: The name a definition introduces, immediately after its closing brace.
DEFINITION_TAIL = re.compile(r"\}\s*([A-Za-z_]\w*)\s*;")


def typedefs(text: str):
    """Every `typedef ... } NAME;` in `text`, as (name, normalised body).

    Nested braces are followed rather than guessed at, so a struct holding an anonymous union is one
    definition and not two half ones. A typedef with no braced body (`typedef int x_t;`) is not a
    candidate: there is no tag body to disagree with, and two scalar aliases of the same name are the
    same type by construction.
    """
    for start in TYPEDEF_START.finditer(text):
        brace = text.find("{", start.end())
        if brace == -1:
            continue
        semicolon = text.find(";", start.end())
        if semicolon != -1 and semicolon < brace:
            continue
        depth = 0
        position = brace
        while position < len(text):
            character = text[position]
            if character == "{":
                depth += 1
            elif character == "}":
                depth -= 1
                if depth == 0:
                    break
            position += 1
        if depth != 0:
            continue
        tail = DEFINITION_TAIL.match(text, position)
        if tail is None:
            continue
        body = re.sub(r"\s+", " ", text[start.start():tail.end()]).strip()
        yield tail.group(1), body

#: Subtrees that hold build output, vendored code or the compiler's own: nothing there defines a name
#: this project owns.
SKIPPED = frozenset({"build", "third_party", ".git", "MCU", ".venv", "__pycache__", "node_modules",
                     "tests"})


def corpus(root: Path = ROOT) -> dict[str, dict[str, str]]:
    """Every typedef name in the project's headers, mapped to the files defining it and their bodies.

    Bodies are normalised (comments stripped by `read_code()`, whitespace collapsed) because the
    question is whether two definitions *mean* the same thing, not whether they were formatted alike.

    `tests/` is skipped on purpose: a host test's fake vendor header is supposed to differ from the
    real one, and the two never meet. The one case where a test fake collides with a product header is
    pinned above rather than screened out here, because it is real and worth being able to see.
    """
    found: dict[str, dict[str, str]] = {}
    for directory, subdirectories, filenames in os.walk(root):
        subdirectories[:] = [name for name in subdirectories if name not in SKIPPED]
        relative = os.path.relpath(directory, root).replace(os.sep, "/")
        if any(relative == vendored or relative.startswith(vendored + "/") for vendored in VENDORED):
            continue
        for filename in filenames:
            if not filename.endswith(".h"):
                continue
            path = Path(directory) / filename
            for name, body in typedefs(read_code(path)):
                found.setdefault(name, {}).setdefault(path.relative_to(root).as_posix(), body)
    return found


def findings(root: Path = ROOT, pins: dict[str, str] | None = None) -> list[str]:
    pins = PINS if pins is None else pins
    types = corpus(root)
    problems: list[str] = []

    # A walk that read nothing is broken, not clean.
    if not types:
        return ["walk is broken: no typedef was found anywhere, so nothing was compared"]

    collisions = {name: files for name, files in types.items() if len(files) > 1}

    for name in sorted(collisions):
        if name in pins:
            continue
        files = sorted(collisions[name])
        problems.append(
            f"{name}: defined in {len(files)} headers ({', '.join(files)}) with no pin - either one "
            f"of them is the owner and the other is a copy, or the name means two different things")

    for name, reason in sorted(pins.items()):
        if not reason.strip():
            problems.append(f"{name}: pinned without stating why the collision is tolerable")
            continue
        if name not in collisions:
            if name in types:
                problems.append(
                    f"{name}: pinned as a collision, but it is now defined once - the pin has expired")
            else:
                problems.append(f"{name}: pinned as a collision, but no header defines it any more")
            continue
        files = sorted(collisions[name])
        if any(not (root / path).is_file() for path in files):
            problems.append(f"{name}: pinned against {files}, but one of those files is gone")

    return problems


def main() -> int:
    problems = findings()
    if problems:
        print("type_definition_collisions failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    types = corpus()
    collisions = sum(1 for files in types.values() if len(files) > 1)
    print(f"type_definition_collisions ok ({len(types)} typedef names, {collisions} defined more "
          f"than once, each with a reason; {len(VENDORED)} vendored subtrees excluded)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
