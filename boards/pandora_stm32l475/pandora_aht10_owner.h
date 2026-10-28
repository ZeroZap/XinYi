/**
 * @file pandora_aht10_owner.h
 * @brief Board-owned canonical AHT10 owner for the Pandora reference board.
 *
 * `main.c` (the board smoke image) still drove U7 AHT10 by hand: it built an
 * `xy_i2c_device_t` inline, sent the `0xE1 0x08 0x00` calibration command, sent
 * `0xAC 0x33 0x00` per measurement and re-implemented the 20-bit humidity and
 * temperature decode. That is a second copy of the driver protocol living in
 * board code, outside every host test and outside the canonical Device driver.
 *
 * This module makes `xy_aht10` the board-owned instance, mirroring
 * `pandora_ap3216c_owner` / `pandora_icm20608_owner`, so the three onboard
 * sensor ICs are owned the same way.
 *
 * Contract notes:
 * - `init` builds a candidate instance and commits it only on success, so a
 *   failed (re)init leaves the caller's owner untouched.
 * - `sample` commits only after the driver returns a complete converted frame,
 *   so a busy status or a partial bus fault preserves the caller's previous
 *   sample.
 */

#ifndef PANDORA_AHT10_OWNER_H
#define PANDORA_AHT10_OWNER_H

#include "xy_aht10.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    xy_aht10_t device;
} pandora_aht10_owner_t;

xy_error_t pandora_aht10_owner_init(pandora_aht10_owner_t* owner, void* i2c_handle);
xy_error_t pandora_aht10_owner_deinit(pandora_aht10_owner_t* owner);
xy_error_t pandora_aht10_owner_sample(pandora_aht10_owner_t* owner, xy_aht10_data_t* sample);

#ifdef __cplusplus
}
#endif

#endif /* PANDORA_AHT10_OWNER_H */
