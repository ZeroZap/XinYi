/**
 * @file pandora_icm20608_owner.h
 * @brief Board-owned canonical ICM20608 owner for the Pandora reference board.
 *
 * `sensor_icm20608_main.c` used to create its device through the legacy
 * `sensor_*` compatibility wrapper in `components/sensor/sensors`
 * (`icm20608_create_accel`) and then reach *back* into
 * `((icm20608_priv_t *)accel->priv_data)->device` to read a sample, so the image
 * depended on the wrapper's private layout.
 *
 * This module makes the canonical Device driver (`xy_icm20608`) the board-owned
 * instance: one owner struct, no legacy framework, no private-layout peeking.
 *
 * Contract notes:
 * - `init` builds a candidate instance and commits it only on success, so a
 *   failed (re)init leaves the caller's owner untouched.
 * - `sample` commits only after the driver returns a complete sample, so a
 *   partial bus fault preserves the caller's previous sample.
 */

#ifndef PANDORA_ICM20608_OWNER_H
#define PANDORA_ICM20608_OWNER_H

#include "xy_icm20608.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* accel(6) + temperature(2) + gyro(6). */
#define PANDORA_ICM20608_RAW_LEN 14U

typedef struct {
    xy_icm20608_t device;
} pandora_icm20608_owner_t;

xy_error_t pandora_icm20608_owner_init(pandora_icm20608_owner_t *owner, void *i2c_handle);
xy_error_t pandora_icm20608_owner_read_reg(pandora_icm20608_owner_t *owner, uint8_t reg,
                                           uint8_t *data, uint16_t len);
xy_error_t pandora_icm20608_owner_read_raw(pandora_icm20608_owner_t *owner,
                                           uint8_t raw[PANDORA_ICM20608_RAW_LEN]);
xy_error_t pandora_icm20608_owner_sample(pandora_icm20608_owner_t *owner,
                                         xy_icm20608_sample_t *sample);

#ifdef __cplusplus
}
#endif

#endif /* PANDORA_ICM20608_OWNER_H */
