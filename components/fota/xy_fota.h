/**
 * @file xy_fota.h
 * @brief Forwarding header - the FOTA public API lives in `inc/xy_fota.h`
 *
 * This file used to be a second full copy of the FOTA API, under the same `XY_FOTA_H`
 * include guard as `inc/xy_fota.h`. Two headers, one guard: whichever the include path
 * resolved second was silently not read, and the component's own
 * `target_include_directories()` lists this directory first. `inc/` is the copy that also
 * carries `xy_fota_bank_swap()` / `xy_fota_bank_mark_valid()` / `xy_fota_bank_is_valid()`,
 * which `src/xy_fota_bank.c` defines and `src/xy_fota_secure.c` calls - so a consumer of
 * `xy_fota` that resolved this copy could not see three functions the library publishes.
 *
 * Keep `#include "xy_fota.h"` working (the component's sources and `inc/xy_fota_flash.h`
 * all use that spelling) by forwarding to the one canonical header. Do not add
 * declarations here: anything declared in this file is invisible to every consumer that
 * includes `inc/xy_fota.h`. Guarded by `include_guard_collisions` in
 * `tests/unit/support/check_include_guard_collisions.py`.
 */

#ifndef XY_FOTA_ROOT_FORWARD_H
#define XY_FOTA_ROOT_FORWARD_H

#include "inc/xy_fota.h"

#endif /* XY_FOTA_ROOT_FORWARD_H */
