# XinYi FOTA External Flash Build Closure (2026-08-08)

## Slice

Close the FOTA external-flash compile gap exposed by the design-stage component loop.

## Finding

`components/fota/CMakeLists.txt` attempted to append `components/fota/src/xy_fota_nor.c` whenever both
`FOTA_EXTERNAL_FLASH` and `NOR_FLASH_ENABLED` were enabled. That source file is not present in the
tracked tree, so this valid Kconfig combination failed during CMake generation before the `xy_fota`
target could be built.

The existing FOTA implementation already exposes generic flash-operation hooks and backup flash hooks
through `xy_fota_set_flash_ops()` / `xy_fota_set_backup_flash_ops()`. The external flash path therefore
must remain buildable without a board-specific NOR backend source.

## Change

- Keep `xy_fota` source collection limited to tracked `*.c` and `src/*.c` files.
- When `CONFIG_FOTA_EXTERNAL_FLASH && CONFIG_NOR_FLASH_ENABLED` is selected, report that external flash
  hooks are enabled instead of appending a non-existent `xy_fota_nor.c`.
- Do not change FOTA runtime behavior or default Kconfig policy.

## Verification

Focused commands used for this slice:

```bash
cmake -B build/tests/unit -S tests/unit >/dev/null \
  && cmake --build build/tests/unit --target test_fota_core -j$(nproc) \
  && cd build/tests/unit && ctest -R '^fota_core$' --output-on-failure

cmake -B build/fota_external_probe -S . -DHAL_PLATFORM=PC -DCMAKE_BUILD_TYPE=Release \
  -DKCONFIG_OVERRIDES='FOTA_ENABLED=ON;FOTA_EXTERNAL_FLASH=ON;NOR_FLASH_ENABLED=ON' \
  && cmake --build build/fota_external_probe --target xy_fota -j$(nproc)
```

Broader gate for the committed slice:

```bash
make test-unit
git diff --check
```

## Follow-up

FOTA still lacks a root `components/fota/README.md` and a build-guarded public example under the component
itself. Those are documentation/example slices, not blockers for the external-flash build configuration.
