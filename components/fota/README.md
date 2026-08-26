# FOTA Component

XinYi FOTA provides a host-testable firmware update state machine for downloading, validating,
installing, and rolling back firmware images. It is intentionally split into a platform-independent
core plus caller-provided Flash operation callbacks so board-specific internal Flash or external NOR
backends can be integrated without changing the component core.

## Current status

- **Build target**: root CMake creates `xy_fota` when `FOTA_ENABLED` is selected.
- **Compatibility alias**: `fota_component` aliases `xy_fota` for existing component references.
- **Unit coverage**: `make test-unit` registers the focused Unity/CTest cases `fota_core` and
  `fota_smoke_example`.
- **External Flash policy**: `FOTA_EXTERNAL_FLASH` uses the public flash/backup operation hooks; no
  board-specific NOR source is required for the component to compile.
- **Default platform policy**: FOTA is enabled by default for `STM32U5` and remains opt-in on PC unless
  selected through Kconfig overrides or explicit build configuration.
- **Secure-update policy**: `xy_fota_secure_init()` fails closed unless the caller supplies a signature
  provider. The provider receives an explicit key ID and must perform real message/signature binding;
  the repository's format-only ECDSA placeholder is never used as a fallback.

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

`xy_fota_start_update()` is deliberately fail-closed: the platform must first register a
`xy_fota_boot_handoff_cb` with `xy_fota_set_boot_handoff()`. The callback owns durable boot-slot
selection and must return `XY_FOTA_OK` only after that request is accepted. Without it, the core returns
`XY_FOTA_NOT_SUPPORTED`; callback failure leaves the selected slot uncommitted and moves the handle to
`XY_FOTA_STATE_ERROR`. Host callback acceptance does not prove a bootloader, reset, mark-valid, rollback,
anti-rollback, or power-loss implementation.

Delta mode is also fail-closed. Register a non-NULL `xy_fota_patch_cb` before finishing a delta
download; the core reads the staged patch in bounded chunks and dispatches each chunk to that callback.
Missing callbacks, staged-patch read failures, or callback rejection return `XY_FOTA_DELTA_ERROR` and
move the handle to `XY_FOTA_STATE_ERROR`. The callback owns the actual patch algorithm and durable image
write; Host callback dispatch does not prove a production patch format or power-loss recovery.

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

Build-guarded host-safe public flow smoke:

```bash
cmake -B build/tests/unit -S tests/unit
cmake --build build/tests/unit --target test_fota_smoke_example -j$(nproc)
cd build/tests/unit && ctest -R '^fota_smoke_example$' --output-on-failure
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
- The core now requires an explicit boot-handoff callback instead of simulating a successful slot switch.
  A real board implementation and durable metadata protocol remain pending.
- `fota_smoke_example` is intentionally host-safe: it uses fake Flash callbacks and does not claim
  bootloader, board NOR, or real hardware validation coverage.
- The signature-provider seam and its Host negative tests are only a fail-closed boundary. No provider
  in this repository is security-approved, and Host acceptance/rejection does not constitute a
  cryptographic review, key-provisioning proof, bootloader integration, or hardware evidence.
