# FOTA Component

XinYi FOTA provides a host-testable firmware update state machine for downloading, validating,
installing, and rolling back firmware images. It is intentionally split into a platform-independent
core plus caller-provided Flash operation callbacks so board-specific internal Flash or external NOR
backends can be integrated without changing the component core.

## Current status

- **Build target**: root CMake creates `xy_fota` when `FOTA_ENABLED` is selected.
- **Compatibility alias**: `fota_component` aliases `xy_fota` for existing component references.
- **Unit coverage**: `make test-unit` registers the focused Unity/CTest case `fota_core`.
- **External Flash policy**: `FOTA_EXTERNAL_FLASH` uses the public flash/backup operation hooks; no
  board-specific NOR source is required for the component to compile.
- **Default platform policy**: FOTA is enabled by default for `STM32U5` and remains opt-in on PC unless
  selected through Kconfig overrides or explicit build configuration.

## Public API shape

Include the public header:

```c
#include "xy_fota.h"
```

The main runtime object is `xy_fota_t`, configured with `xy_fota_config_t`:

```c
xy_fota_t fota;
xy_fota_config_t config = {
    .mode = XY_FOTA_MODE_DUAL_BANK,
    .flash_base_addr = 0x08010000u,
    .slot_size = 256u * 1024u,
    .slot_count = 2,
    .enable_rollback = true,
    .min_version = 1,
};

int ret = xy_fota_init(&fota, &config);
```

The component does not own board Flash drivers directly. Register operations before downloading image
data:

```c
static const xy_fota_flash_ops_t flash_ops = {
    .init = board_flash_init,
    .write = board_flash_write,
    .read = board_flash_read,
    .erase = board_flash_erase,
    .deinit = board_flash_deinit,
};

xy_fota_set_flash_ops(&fota, &flash_ops);
```

For single-slot or external-backup flows, register backup operations separately:

```c
xy_fota_set_backup_flash_ops(&fota, &external_nor_ops);
```

## Typical update flow

1. Initialize the FOTA handle with `xy_fota_init()`.
2. Register primary Flash operations with `xy_fota_set_flash_ops()`.
3. Optionally register backup/external Flash operations with `xy_fota_set_backup_flash_ops()`.
4. Start a download with `xy_fota_start_download()`.
5. Feed data chunks through `xy_fota_download_chunk()`.
6. Complete validation/backup processing with `xy_fota_finish_download()`.
7. Begin the install handoff with `xy_fota_start_update()`.
8. Use `xy_fota_needs_rollback()` / `xy_fota_rollback()` for rollback-capable flows.

## Configuration

Relevant root Kconfig symbols:

| Symbol | Meaning |
| --- | --- |
| `FOTA_ENABLED` | Enables the FOTA component target. |
| `FOTA_DUAL_BANK` | Selects dual-bank update support. |
| `FOTA_EXTERNAL_FLASH` | Allows external Flash backup/image storage through registered ops. |
| `FOTA_ROLLBACK` | Enables rollback-related logic. |
| `FOTA_MAX_IMAGE_SIZE` | Sets the maximum image size in bytes. |
| `NOR_FLASH_ENABLED` | Enables the broader external NOR Flash feature; FOTA still relies on ops hooks. |

The legacy component-local `components/fota/Kconfig` contains `XY_FOTA_*` symbols for older component
experiments. The root `Kconfig` / generated `CONFIG_FOTA_*` symbols are the active firmware build
source of truth.

## Verification

Focused FOTA validation:

```bash
cmake -B build/tests/unit -S tests/unit
cmake --build build/tests/unit --target test_fota_core -j$(nproc)
cd build/tests/unit && ctest -R '^fota_core$' --output-on-failure
```

Full active host unit suite:

```bash
make test-unit
```

External Flash compile probe used to guard the callback-based external Flash policy:

```bash
cmake -B build/fota_external_probe -S . -DHAL_PLATFORM=PC -DCMAKE_BUILD_TYPE=Release \
  -DKCONFIG_OVERRIDES='FOTA_ENABLED=ON;FOTA_EXTERNAL_FLASH=ON;NOR_FLASH_ENABLED=ON'
cmake --build build/fota_external_probe --target xy_fota -j$(nproc)
```

## Boundaries and follow-up

- Do not add board-specific NOR or vendor Flash files to `components/fota` unless a real backend source
  exists and is separately verified.
- Keep board pinmux, Flash geometry, bootloader handoff, and hardware logs in board/project validation
  records rather than in this platform-independent core.
- A build-guarded public example is still a useful follow-up slice, but it should remain host-safe and
  use fake Flash callbacks instead of real hardware access.
