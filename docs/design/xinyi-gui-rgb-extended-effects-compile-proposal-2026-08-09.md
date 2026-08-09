# XinYi GUI RGB extended effects compile-boundary proposal

**Status**: proposal / no runtime implementation change  
**Date**: 2026-08-09  
**Scope**: `components/gui/effects/xy_rgb_fx_{extended,matrix,3d,music}.c`, serial-RGB public headers, and future host compile tests only.

## Why this proposal exists

The GUI LED-screen header target is now host-guarded by `gui_led_screen_effects`, but the adjacent RGB extended effect implementation files are still a different risk class:

- they live under `components/gui/effects/`;
- they include `xy_rgb_fx.h` / `xy_rgb_color.h`, whose current public headers are owned by `components/drivers/display/led_drivers/serial_rgb/`;
- they reference global RGB-strip helpers such as `xy_rgb_set_pixel()`, `xy_rgb_get_pixel()`, `xy_rgb_clear()`, `xy_rgb_show()`, and `g_frame_count`;
- they are not currently part of the `xy_gui_effects` CMake source list or a focused CTest;
- several effects depend on randomness, static state, music/spectrum inputs, or matrix/cube dimensions, so a broad algorithm test would be too large for a design-stage slice.

That means these files should not be pulled into the main GUI effect target just to increase coverage count. First we need to fix the compile/ownership boundary and add a narrow host-safe guard.

## Boundary decision

Keep the ownership split explicit:

| Area | Owner | Allowed next proof |
| --- | --- | --- |
| Basic GUI effects (`fade/blink/breath/slide/rotate`) | `components/gui/effects` | Existing `gui_effects` algorithm CTest |
| LED-screen public types/engine headers | `components/gui/effects` | Existing `gui_led_screen_effects` header CTest |
| RGB strip primitives and public effect API | `components/drivers/display/led_drivers/serial_rgb` | Serial-RGB header/source compile or display-driver CTest |
| GUI RGB extended effect implementation files | design-stage compatibility layer | Future host compile CTest with fake RGB-strip backend only |

The RGB extended files must not directly claim GUI component completion until they compile against a documented public seam and are covered by a focused host target.

## Proposed future implementation slice

A safe next code/test slice is:

1. Add `test_gui_rgb_extended_effects_compile` / CTest `gui_rgb_extended_effects_compile`.
2. Link exactly one low-risk implementation file first, preferably `xy_rgb_fx_music.c` or `xy_rgb_fx_matrix.c`, only after direct compile identifies the missing seam.
3. Provide test-local fake RGB-strip functions and a test-local `g_frame_count` if the public serial-RGB headers intentionally expose that contract.
4. If the public header does not expose the required globals/functions, stop and write/patch a serial-RGB API proposal instead of adding private externs to production code.
5. Cover only compile/self-containment and one deterministic setter/helper path in the first code slice; leave visual quality, hardware timing, music input quality, and random animations to later validation records.
6. Keep `XY_NET`/Display/GUI default enablement unchanged; do not edit `MCU/` or `third_party/`.

## Non-goals

- No runtime behavior changes in this proposal slice.
- No HAL GPIO/SPI/I2C/UART binding.
- No hardware validation claims.
- No broad rewrite of RGB effect algorithms or directory migration.
- No claim that RGB extended effects are complete because the basic `gui_effects` target passes.

## Verification for this proposal slice

This is a docs-only boundary sync. Expected verification:

```bash
make test-unit
git diff --check
```

The full unit gate is intentional: it proves this status sync did not disturb the already-registered GUI/Display tests before the next implementation slice is selected.

## Current conclusion

The RGB extended effect compile-boundary CTest now covers `xy_rgb_fx_music.c` and `xy_rgb_fx_matrix.c` through a fake serial-RGB seam. Remaining work should continue file-by-file for `xy_rgb_fx_extended.c` / `xy_rgb_fx_3d.c`, not as a broad algorithm or hardware slice; `components/gui/README.md` should continue describing RGB extended effects as host compile-boundary guarded rather than visually or hardware complete.
