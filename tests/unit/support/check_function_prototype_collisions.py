#!/usr/bin/env python3
"""One function name, two incompatible declarations.

C allows a function to be declared more than once, as long as every declaration agrees. It does not
allow them to disagree, and the failure mode when they do is worse than a warning: the compiler keeps
whichever prototype it saw first and converts the call to it, so the wrong one is a silent stack
corruption rather than an error.

Both directions are live in this tree, measured rather than supposed:

  * `xy_base64_decode` is `int (const char *, size_t, uint8_t *, size_t)` in
    `components/crypto/inc/xy_tiny_crypto.h` and
    `uint32_t (const uint8_t *, uint32_t, uint8_t *)` in `components/dm/xy_base64/xy_base64.h`.
    Different arity, different return type, same name.
  * `xy_fuel_gauge_init` is `int (const xy_fuel_gauge_config_t *)` in `components/pm` and
    `xy_fuel_gauge_status_t (xy_fuel_gauge_t *)` in `components/fuel_gauge`.
  * `xy_gui_draw_line` is `int (xy_gui_t *, ...)` in `components/gui/xy_gui.h` and
    `void (...)` with a different parameter list in `components/gui/inc/xy_gui_draw.h`.

Only *incompatible* declarations are reported. Repeating an identical prototype is idiomatic C and
harmless; this rule is about the repeats that are not identical, which is where the defect lives.
That is the difference from `check_type_definition_collisions`, where every repeat is illegal.

The extraction is deliberately shallow - it reads file-scope statements and compares them textually
after normalising whitespace, pointer spelling and `extern "C"` - because the alternative is a C
parser, and a guard that needs a parser to run is a guard that stops running. What it cannot read, it
does not invent: function bodies, type bodies and the preprocessor are removed before anything is
compared, and each removal is the fix for a real false positive found while building this (a file
wrapped in `extern "C"` losing all of its declarations, `static inline` wrappers leaking their bodies
into the next statement, `uint8_t *file` against `uint8_t* file`).
"""

from __future__ import annotations

import os
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from source_scan import read_code  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]

#: Copied-in code that defines its own API under names this project also uses. The FreeRTOS portable
#: layer is Amazon's, the AT samples are RT-Thread's, and their disagreements with this project's
#: headers are expected rather than actionable.
VENDORED: dict[str, str] = {
    "components/crypto/curve25519-cortexm0-20150813":
        "vendored curve25519, ships its own STM32F0 peripheral headers",
    "components/net/at/AT-Command-V2": "vendored AT command samples",
    "components/net/at/rtthread-at": "RT-Thread's AT framework, vendored",
    "components/net/at/FreeRTOS-Cellular-Interface": "AWS cellular interface, vendored",
    "components/kernel/osal/portable":
        "FreeRTOS Kernel V10.4.6 port layer, vendored from Amazon",
}

#: Names that carry two incompatible declarations and have not been resolved. Resolving one means
#: deciding which side is the real API and moving the callers, not renaming a function.
#:
#: Eight families, and none of them is a typo - each is one component shipped twice, or two subsystems
#: that both claimed a name:
#:
#:   * crypto: `inc/` against the implementation directory, and `inc/` against `dm/` (8);
#:   * sensor leftovers: the un-migrated framework against its canonical owners (9);
#:   * rgb_matrix: the LCD driver tree against the serial-RGB driver tree (7);
#:   * osal: `xy_os.h` against `inc/` against an example's own `osal_pc.h` (5);
#:   * gui, hal rng, clib, fuel_gauge: `inc/`-style duplicates (9).
PINS: dict[str, str] = {
    # -- crypto: inc/ against the implementation directory and against dm/ ------------------------
    "xy_sha256_init":
        "components/crypto/inc/xy_sha256.h against components/crypto/inc/xy_tiny_crypto.h - same "
        "directory, one component's two headers",
    "xy_sha256_update":
        "same pair as xy_sha256_init",
    "xy_base64_decode":
        "components/crypto/inc/xy_tiny_crypto.h returns int and takes (const char *, size_t, uint8_t "
        "*, size_t); components/dm/xy_base64/xy_base64.h returns uint32_t and takes (const uint8_t *, "
        "uint32_t, uint8_t *) - different arity and different return type under one name",
    "xy_base64_encode":
        "same pair as xy_base64_decode",
    "xy_chacha20_init":
        "components/crypto/inc/xy_chacha20poly1305.h returns void, components/crypto/xy_chacha/"
        "xy_chacha20_poly1305.h returns int - a caller that checks the init failure compiles against "
        "one of them and silently gets garbage from the other",
    "xy_poly1305_init": "components/crypto/inc/xy_chacha20poly1305.h against the xy_chacha directory's "
                        "own header",
    "xy_poly1305_update": "same pair as xy_poly1305_init",
    "xy_poly1305_finish": "same pair as xy_poly1305_init",
    # -- sensor leftovers: the un-migrated framework against canonical owners --------------------
    "xy_oled_ssd1306_init":
        "components/sensor/inc/xy_oled_ssd1306.h takes (dev, i2c_handle) where the canonical "
        "components/drivers/display/oled/ssd1306 header takes three parameters",
    "xy_oled_ssd1306_clear":
        "components/sensor/inc returns int where the canonical driver returns void",
    "xy_oled_ssd1306_draw_pixel": "components/sensor/inc against the canonical ssd1306 driver header",
    "xy_oled_ssd1306_draw_char": "components/sensor/inc against the canonical ssd1306 driver header",
    "xy_oled_ssd1306_draw_string": "components/sensor/inc against the canonical ssd1306 driver header",
    "xy_mlx90614_get_emissivity":
        "components/sensor/inc/xy_mlxeeprom.h takes a bare void *i2c_handle where the canonical "
        "drivers/sensor/temperature/mlx90614 header takes xy_mlx90614_t *dev",
    "xy_mlx90614_set_emissivity": "same pair as xy_mlx90614_get_emissivity",
    "xy_sensor_sample_fetch":
        "components/device/inc is canonical; components/sensor/inc/xy_sensor.h is the un-migrated old "
        "framework and declares an incompatible signature",
    "xy_sensor_channel_get": "same pair as xy_sensor_sample_fetch",
    # -- rgb_matrix: the LCD driver tree against the serial-RGB driver tree ----------------------
    "xy_rgb_matrix_init":
        "components/drivers/display/lcd_drivers/xy_rgb_matrix.h against led_drivers/serial_rgb/"
        "xy_rgb_matrix.h - two driver trees publishing one header name",
    "xy_rgb_matrix_clear": "same pair as xy_rgb_matrix_init",
    "xy_rgb_matrix_set_pixel": "same pair as xy_rgb_matrix_init",
    "xy_rgb_matrix_get_pixel": "same pair as xy_rgb_matrix_init",
    "xy_rgb_matrix_draw_rect": "same pair as xy_rgb_matrix_init",
    "xy_rgb_matrix_draw_hline": "same pair as xy_rgb_matrix_init",
    "xy_rgb_matrix_draw_vline": "same pair as xy_rgb_matrix_init",
    # -- osal: xy_os.h against inc/ and against an example's own PC stub -------------------------
    "xy_os_delay":
        "components/kernel/osal/xy_os.h, components/kernel/osal/inc/xy_os_delay.h and "
        "examples/component_demo/osal_pc.h each declare it; the example's stub is meant to stand in "
        "for the real one and disagrees with it",
    "xy_os_kernel_init": "components/kernel/osal/xy_os.h against examples/component_demo/osal_pc.h",
    "xy_os_kernel_start": "components/kernel/osal/xy_os.h against examples/component_demo/osal_pc.h",
    "xy_os_msgqueue_get": "components/kernel/osal/xy_os.h against examples/component_demo/osal_pc.h",
    "xy_os_msgqueue_put": "components/kernel/osal/xy_os.h against examples/component_demo/osal_pc.h",
    # -- gui: the top-level header against inc/xy_gui_draw.h ------------------------------------
    "xy_gui_draw_line":
        "components/gui/xy_gui.h returns int and takes a xy_gui_t *; inc/xy_gui_draw.h returns void "
        "and takes a bare framebuffer - different arity under one name",
    "xy_gui_draw_rect": "same pair as xy_gui_draw_line",
    "xy_gui_draw_string": "same pair as xy_gui_draw_line",
    # -- hal rng: the crypto component's copy against the hal component's -----------------------
    "xy_hal_rng_init":
        "components/crypto/inc/xy_hal_crypto.h against components/hal/inc/xy_hal_rng.h - the crypto "
        "component republishes a HAL entry point with its own signature",
    "xy_hal_rng_deinit": "same pair as xy_hal_rng_init",
    # -- clib: one allocator declared in three places -------------------------------------------
    "xy_malloc":
        "components/clib/xy_clib/xy_stdio.h, xy_clib/xy_stdlib.h and components/dm/xy_mem/xy_mem.h "
        "each declare the allocator; an allocator with two prototypes is the worst possible one to "
        "get wrong",
    "xy_realloc": "same triple as xy_malloc",
    # -- fuel_gauge: pm against fuel_gauge, three ways -------------------------------------------
    "xy_fuel_gauge_init":
        "components/pm (both inc/xy_pm.h and fuel-gauge/xy_fuel_gauge.h) returns int and takes a "
        "config; components/fuel_gauge/inc returns xy_fuel_gauge_status_t and takes a "
        "xy_fuel_gauge_t * - the two fuel-gauge stacks use one name for different functions",
    "xy_fuel_gauge_deinit": "same triple as xy_fuel_gauge_init",
}

#: Subtrees holding build output, vendored code or host tests. `tests/` is skipped because a fake
#: vendor header is supposed to differ from the real one and the two never meet.
SKIPPED = frozenset({"build", "third_party", ".git", "MCU", ".venv", "__pycache__", "node_modules",
                     "tests"})

TYPE_KEYWORDS = ("struct", "union", "enum", "typedef")
NOT_A_NAME = frozenset({"if", "while", "for", "switch", "return", "sizeof", "defined", "do", "else"})
TRAILING_NAME = re.compile(r"([A-Za-z_]\w*)\s*$")
ATTRIBUTE_TAIL = re.compile(r"\s*__attribute__\s*\(\(.*?\)\)\s*$", re.S)
POINTER = re.compile(r"\s*\*\s*")
EXTERN_C = re.compile(r'^(?:extern\s+|\s|"C"\s*)+')
DECLARATION_KEYWORD = re.compile(r"(?:^|\s)extern(?:\s|$)")


def without_preprocessor(text: str) -> str:
    """Drop `#` lines and their backslash continuations: a macro body is not a declaration."""
    kept = []
    continuing = False
    for line in text.splitlines():
        joined = continuing
        continuing = line.rstrip().endswith("\\")
        if joined or line.lstrip().startswith(("#", "\\")):
            kept.append("")
        else:
            kept.append(line)
    return "\n".join(kept)


def file_scope(text: str) -> str:
    """The text at file scope: type bodies and function bodies dropped, `extern "C"` unwrapped.

    A transparent block is walked again rather than copied, because the `static inline` helpers inside
    it carry their own bodies: copying the block wholesale leaked those bodies into the following
    statement, where their call arguments then read as a declaration of the function they call.
    """
    kept = []
    index = 0
    while index < len(text):
        if text[index] != "{":
            kept.append(text[index])
            index += 1
            continue
        before = "".join(kept)
        statement_start = max(before.rfind(";"), before.rfind("}"), 0)
        prefix = before[statement_start:]
        is_function_body = before.rstrip().endswith(")")
        is_type_body = any(keyword in prefix.split()[-6:] for keyword in TYPE_KEYWORDS)
        depth, cursor = 0, index
        while cursor < len(text):
            if text[cursor] == "{":
                depth += 1
            elif text[cursor] == "}":
                depth -= 1
                if depth == 0:
                    break
            cursor += 1
        inner = text[index + 1:cursor]
        kept.append(" " if (is_function_body or is_type_body) else file_scope(inner))
        index = cursor + 1
    return "".join(kept)


def opening_paren(statement: str) -> int:
    """The `(` matching the statement's final `)`, found by scanning backwards.

    Scanning forwards finds the *first* group, which for `int f(void (*cb)(int))` is the callback's
    parameter list - and then the callback's parameter name is read as the function being declared.
    """
    if not statement.endswith(")"):
        return -1
    depth = 0
    for index in range(len(statement) - 1, -1, -1):
        if statement[index] == ")":
            depth += 1
        elif statement[index] == "(":
            depth -= 1
            if depth == 0:
                return index
    return -1


def without_parameter_names(parameters: str) -> str:
    """Drop parameter names: `int f(int count)` and `int f(int flags)` declare the same function.

    Without this the rule reports a rename as an incompatibility, which is the same defect it exists
    to find - a claim that does not match reality - only pointing the other way.

    The `*` is kept when the name is dropped from a pointer parameter (`const char *format` becomes
    `const char *`), and an array parameter keeps its brackets (`uint8_t sig[N]` becomes
    `uint8_t []`), because both are part of the type.
    """
    pieces = []
    depth = 0
    current = ""
    for character in parameters:
        if character == "(":
            depth += 1
        elif character == ")":
            depth -= 1
        if character == "," and depth == 0:
            pieces.append(current)
            current = ""
        else:
            current += character
    pieces.append(current)

    cleaned = []
    for piece in pieces:
        text = " ".join(piece.split())
        if not text or text == "..." or text == "void":
            cleaned.append(text)
            continue
        # `void (*on_event)(int code)` - name the pointer, and clean the callback's own parameters.
        pointer = re.match(r"^(.*\(\s*\*)\s*([A-Za-z_]\w*)\s*(\)\s*\()(.*)(\)\s*)$", text)
        if pointer is not None:
            cleaned.append(pointer.group(1) + pointer.group(3)
                           + without_parameter_names(pointer.group(4)) + pointer.group(5))
            continue
        tokens = text.split(" ")
        if len(tokens) < 2:
            cleaned.append(text)
            continue
        last = tokens[-1]
        if re.fullmatch(r"\*+[A-Za-z_]\w*", last):
            tokens[-1] = "*" * (len(last) - len(last.lstrip("*")))
        elif re.fullmatch(r"[A-Za-z_]\w*", last):
            tokens.pop()
        elif re.fullmatch(r"[A-Za-z_]\w*\[.*\]", last):
            tokens[-1] = re.sub(r"^[A-Za-z_]\w*", "", last)
        cleaned.append(" ".join(tokens))
    return ",".join(cleaned)


def normalise(text: str) -> str:
    """One spelling per meaning: spacing around `*`, `,`, `(` and `)` is not a difference."""
    collapsed = POINTER.sub("*", " ".join(text.split()))
    collapsed = collapsed.replace(",", ", ").replace("( ", "(").replace(" )", ")")
    return collapsed


def declarations(text: str):
    """File-scope function declarations as (name, normalised prototype)."""
    for statement in text.split(";"):
        stripped = " ".join(statement.split())
        stripped = ATTRIBUTE_TAIL.sub("", stripped).strip()
        if not stripped or "(" not in stripped or ")" not in stripped:
            continue
        if stripped.startswith(("typedef", "static")):
            continue
        if "=" in stripped or "__asm" in stripped:
            continue
        opening = opening_paren(stripped)
        if opening == -1:
            continue
        head = stripped[:opening].rstrip()
        name_match = TRAILING_NAME.search(head)
        if name_match is None:
            continue
        name = name_match.group(1)
        if name in NOT_A_NAME:
            continue
        return_type = EXTERN_C.sub("", head[:name_match.start()].strip()).strip().strip('"').strip()
        if not return_type or return_type == "C":
            continue
        parameters = without_parameter_names(stripped[opening + 1:-1])
        yield name, normalise(f"{return_type} {name}({parameters})")


def corpus(root: Path = ROOT) -> dict[str, dict[str, str]]:
    """Every function name declared in the project's headers, mapped to files and prototypes."""
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
            text = file_scope(without_preprocessor(read_code(path)))
            for name, prototype in declarations(text):
                found.setdefault(name, {}).setdefault(path.relative_to(root).as_posix(), prototype)
    return found


def findings(root: Path = ROOT, pins: dict[str, str] | None = None) -> list[str]:
    pins = PINS if pins is None else pins
    declared = corpus(root)
    problems: list[str] = []

    if not declared:
        return ["walk is broken: no declaration was found anywhere, so nothing was compared"]

    conflicts = {
        name: files for name, files in declared.items()
        if len(files) > 1 and len(set(files.values())) > 1
    }

    for name in sorted(conflicts):
        if name in pins:
            continue
        files = sorted(conflicts[name])
        problems.append(
            f"{name}: {len(files)} incompatible declarations ({', '.join(files)}) with no pin - a "
            f"caller gets whichever prototype was included first, and the other one silently "
            f"converts")

    for name, reason in sorted(pins.items()):
        if not reason.strip():
            problems.append(f"{name}: pinned without stating why the incompatibility is tolerable")
        elif name not in conflicts:
            problems.append(
                f"{name}: pinned as incompatible, but it is now declared consistently - the pin has "
                f"expired")
        elif any(not (root / path).is_file() for path in conflicts[name]):
            problems.append(f"{name}: pinned against a file that no longer exists")

    return problems


def main() -> int:
    problems = findings()
    if problems:
        print("function_prototype_collisions failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    declared = corpus()
    conflicts = sum(1 for files in declared.values()
                    if len(files) > 1 and len(set(files.values())) > 1)
    print(f"function_prototype_collisions ok ({len(declared)} function names, {conflicts} declared "
          f"incompatibly, each with a reason; {len(VENDORED)} vendored subtrees excluded)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
