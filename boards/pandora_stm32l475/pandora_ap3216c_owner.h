/**
 * @file pandora_ap3216c_owner.h
 * @brief Board-owned canonical AP3216C owner for the Pandora reference board.
 *
 * `sensor_ap3216c_main.c` used to own the onboard light/proximity/IR device
 * through the legacy `sensor_*` compatibility wrapper in
 * `components/sensor/sensors`, which `SENSOR_MALLOC`s one `sensor_device_t` per
 * channel, re-maps every result into `sensor_err_t`, and hides the canonical
 * driver instance behind `priv_data`.
 *
 * This module makes the canonical Device driver (`xy_ap3216c`) the board-owned
 * instance, so the image carries no legacy framework dependency and the board
 * sees `xy_error_t` directly.
 *
 * Contract notes:
 * - `init` builds a candidate instance and commits it only on success, so a
 *   failed (re)init leaves the caller's owner untouched.
 * - `sample` reads every channel into locals and commits only after all of them
 *   succeed, so a partial bus fault preserves the caller's previous sample.
 */

#ifndef PANDORA_AP3216C_OWNER_H
#define PANDORA_AP3216C_OWNER_H

#include "xy_ap3216c.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* IR/ALS/PS raw block: IR_DATA_L(2) + ALS_DATA_L(2) + PS_DATA_L(2). */
#define PANDORA_AP3216C_RAW_LEN 6U

typedef struct {
    xy_ap3216c_t device;
} pandora_ap3216c_owner_t;

typedef struct {
    uint32_t illuminance_millilux;
    uint16_t proximity_raw;
    uint16_t infrared_raw;
} pandora_ap3216c_sample_t;

xy_error_t pandora_ap3216c_owner_init(pandora_ap3216c_owner_t *owner, void *i2c_handle);
xy_error_t pandora_ap3216c_owner_read_reg(pandora_ap3216c_owner_t *owner, uint8_t reg,
                                          uint8_t *data, uint16_t len);
xy_error_t pandora_ap3216c_owner_read_raw(pandora_ap3216c_owner_t *owner,
                                          uint8_t raw[PANDORA_AP3216C_RAW_LEN]);
xy_error_t pandora_ap3216c_owner_sample(pandora_ap3216c_owner_t *owner,
                                        pandora_ap3216c_sample_t *sample);

#ifdef __cplusplus
}
#endif

#endif /* PANDORA_AP3216C_OWNER_H */
