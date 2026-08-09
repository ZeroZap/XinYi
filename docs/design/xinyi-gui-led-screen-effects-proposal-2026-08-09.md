# XinYi GUI LED-screen / RGB extended effects proposal

**Status**: proposal / host-contract first  
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
   - Candidate future target: `gui_led_screen_effects`
   - Should use a host-only fake LED-screen/framebuffer fixture.
   - May cover drawing primitives, clipping, swap/update behavior, and deterministic effect-step math.
   - Must not call real HAL GPIO/SPI/I2C/UART or real display drivers.

3. **RGB/extended/hardware-adjacent effects**
   - Candidate future target(s): `gui_rgb_effects_headers` first, then narrower implementation tests only where the public contract is deterministic.
   - Hardware timing, audio/music input quality, LED-strip electrical behavior, and real screen output remain validation-record territory, not host fake proof.

## Proposed first implementation slice

A safe next code/test slice is:

1. Add `tests/unit/gui/test_gui_led_screen_effects.c`.
2. Link only the minimal LED-screen/effect sources required by the test.
3. Use a fake in-memory framebuffer; no Display driver, HAL, or vendor tree.
4. Cover existing public contracts only:
   - init rejects NULL/zero geometry and accepts a small fixed buffer;
   - set/get pixel clips out-of-range coordinates without side effects;
   - clear/fill/swap/update mutate only the expected buffer state;
   - line/rectangle primitives clip to the framebuffer;
   - one deterministic simple effect step, if current implementation already exposes stable state.
5. Wire as CTest name `gui_led_screen_effects` and build target `test_gui_led_screen_effects`.
6. Update `components/gui/README.md` and `docs/design/unit-test-inventory.md` only after the focused target passes.

## Explicit non-goals

- Do not enable or rewrite real LED matrix/OLED/LCD hardware paths.
- Do not edit `MCU/` or `third_party/`.
- Do not claim real screen, LED-strip timing, animation quality, audio-reactive behavior, or product UX validation from host fake tests.
- Do not merge this with GUI font-rendering or display-driver hardware validation work.
- Do not change the default component enablement policy unless a separate Kconfig/CMake proposal proves the boundary.

## Verification plan for the future code slice

```bash
cmake -B build/tests/unit -S tests/unit
cmake --build build/tests/unit --target test_gui_led_screen_effects -j$(nproc)
cd build/tests/unit && ctest --output-on-failure -R '^gui_led_screen_effects$'
make test-unit
git diff --check
```

If the first focused build exposes stale API drift in `xy_led_screen*.c`, fix only the smallest contract required by the focused host test and keep the implementation hardware-free.

## Current conclusion

This proposal closes the design ambiguity for the next GUI effects step: LED-screen/RGB extended effects should proceed through a separate host-contract target before any hardware or UX claims. Until that target exists and passes, `components/gui/README.md` should continue describing extended effects as pending rather than complete.
