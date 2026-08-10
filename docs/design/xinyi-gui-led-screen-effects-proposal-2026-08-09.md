# XinYi GUI LED-screen / RGB extended effects proposal

**Status**: host-contract partial / implementation source pending
**Date**: 2026-08-09  
**Scope**: `components/gui/effects/xy_led_screen*.{h,c}` and RGB extended effect helpers only; no HAL/vendor/display-driver hardware binding in this slice.

## Why this proposal exists

The GUI component now has host CTest coverage for:

- `gui_core`
- `gui_display_backend`
- `gui_widget_theme`
- `gui_widgets`
- `gui_effects_headers`
- `gui_effects`
- `gui_fonts`

That coverage deliberately stops at the display-independent GUI core, basic effects algorithms, and host fake display/LED adapter bridge. The remaining effect-related files under `components/gui/effects/` include LED-screen and RGB/3D/music/matrix helpers whose product boundary is less clear: some look like pure framebuffer/effect math, while others imply LED-strip or music-input integration. They should not be marked complete just because the basic effects host target passes.

## Current boundary decision

Keep three layers separate:

1. **Pure GUI/basic effects**
   - Existing target: `gui_effects`
   - Already covers fade/blink/breath/slide/rotate object lifecycle and boundary contracts.

2. **LED-screen framebuffer/effect math**
   - Current target: `gui_led_screen_effects`
   - Current coverage is intentionally limited to public-header self-containment, public types, enums, and function signatures because the repository currently has `xy_led_screen.h` / `xy_gui_screen_fx.h` but no matching `.c` implementation files.
   - When implementation sources are added, extend the same target with a host-only fake LED-screen/framebuffer fixture.
   - Future implementation coverage may cover drawing primitives, clipping, swap/update behavior, and deterministic effect-step math.
   - Must not call real HAL GPIO/SPI/I2C/UART or real display drivers.

3. **RGB/extended/hardware-adjacent effects**
   - Current target: `gui_rgb_extended_effects_compile`
   - Current coverage links `xy_rgb_fx_extended.c`, `xy_rgb_fx_music.c`, `xy_rgb_fx_matrix.c`, and `xy_rgb_fx_3d.c` through a fake serial-RGB seam to guard compile boundaries and deterministic low-risk smoke paths.
   - Hardware timing, audio/music input quality, LED-strip electrical behavior, and real screen output remain validation-record territory, not host fake proof.

## Completed host-contract slice

The first safe code/test slice has already landed:

1. `tests/unit/gui/test_gui_led_screen_effects.c` now verifies `xy_led_screen.h` and `xy_gui_screen_fx.h` public types, enums, and function-signature contracts without linking hardware or nonexistent implementation sources.
2. `tests/unit/gui/test_gui_rgb_extended_effects_compile.c` now verifies the RGB extended implementation files through fake `xy_rgb_*` strip callbacks, test-owned `g_frame_count`, and local delay/color helper seams.
3. `components/gui/README.md` and `docs/design/unit-test-inventory.md` already describe these targets as host-only contract guards, not hardware or visual-quality validation.

## Remaining implementation slice

Only after real `xy_led_screen*.c` / `xy_gui_screen_fx*.c` implementation files exist, extend `test_gui_led_screen_effects` to link those sources and use a fake in-memory framebuffer. Cover existing public contracts only:

- init rejects NULL/zero geometry and accepts a small fixed buffer;
- set/get pixel clips out-of-range coordinates without side effects;
- clear/fill/swap/update mutate only the expected buffer state;
- line/rectangle primitives clip to the framebuffer;
- one deterministic simple effect step, if the implementation exposes stable state.

## Explicit non-goals

- Do not enable or rewrite real LED matrix/OLED/LCD hardware paths.
- Do not edit `MCU/` or `third_party/`.
- Do not claim real screen, LED-strip timing, animation quality, audio-reactive behavior, or product UX validation from host fake tests.
- Do not merge this with GUI font-rendering or display-driver hardware validation work.
- Do not change the default component enablement policy unless a separate Kconfig/CMake proposal proves the boundary.

## Verification plan

```bash
cmake -B build/tests/unit -S tests/unit
cmake --build build/tests/unit --target test_gui_led_screen_effects -j$(nproc)
cmake --build build/tests/unit --target test_gui_rgb_extended_effects_compile -j$(nproc)
cd build/tests/unit && ctest --output-on-failure -R '^gui_led_screen_effects$'
cd build/tests/unit && ctest --output-on-failure -R '^gui_rgb_extended_effects_compile$'
make test-unit
git diff --check
```

If a future focused build exposes stale API drift in newly added `xy_led_screen*.c`, fix only the smallest contract required by the focused host test and keep the implementation hardware-free.

## Current conclusion

This proposal now records the closed first step: LED-screen headers and RGB extended effects have host-only contract guards, but no result here represents visual algorithm quality, LED timing, or real screen hardware validation. The remaining LED-screen work is implementation-source driven: do not add fake implementation tests until matching production `.c` files exist, and do not mark hardware validation complete without board logs.
