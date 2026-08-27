/**
 * @file xy_fota_metadata.h
 * @brief Redundant flash-backed FOTA boot metadata journal
 */

#ifndef XY_FOTA_METADATA_H
#define XY_FOTA_METADATA_H

#include "xy_fota.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XY_FOTA_METADATA_FLAG_PENDING (1U << 0)
#define XY_FOTA_METADATA_NO_SLOT UINT8_MAX

typedef struct {
    uint32_t generation;
    uint32_t active_version;
    uint32_t min_version;
    uint32_t pending_version;
    uint8_t active_slot;
    uint8_t pending_slot;
    uint8_t boot_attempts;
    uint8_t flags;
} xy_fota_metadata_t;

typedef struct {
    const xy_fota_flash_ops_t *ops;
    uint32_t base_addr;
    uint32_t erase_size;
} xy_fota_metadata_flash_t;

/**
 * Load the newest valid copy from the two-slot metadata journal.
 *
 * Returns XY_FOTA_NO_IMAGE when neither copy is valid. Corrupt or partially
 * written copies are ignored rather than promoted to boot state.
 */
int xy_fota_metadata_flash_validate(const xy_fota_metadata_flash_t *backend);

int xy_fota_metadata_flash_load(const xy_fota_metadata_flash_t *backend,
                                xy_fota_metadata_t *metadata);

/**
 * Commit metadata to the inactive journal slot and read it back before success.
 *
 * The generation is assigned by this function. On failure, the previously
 * committed copy remains the newest valid record.
 */
int xy_fota_metadata_flash_commit(const xy_fota_metadata_flash_t *backend,
                                  const xy_fota_metadata_t *metadata,
                                  xy_fota_metadata_t *committed);

int xy_fota_metadata_stage_candidate(xy_fota_metadata_t *metadata, uint8_t slot,
                                     uint32_t version);
int xy_fota_metadata_record_boot_attempt(xy_fota_metadata_t *metadata, uint8_t max_attempts,
                                         bool *rollback_required);
int xy_fota_metadata_confirm_candidate(xy_fota_metadata_t *metadata);

/** Persist a candidate selected by the FOTA core before boot handoff. */
int xy_fota_metadata_boot_handoff(uint8_t slot, uint32_t version, void *user_data);

/**
 * Persist one bootloader attempt before transferring control to a pending image.
 *
 * A successful return means the increment, or the resulting rollback decision,
 * is durable. Callers must not boot the candidate when persistence fails.
 */
int xy_fota_metadata_boot_attempt(uint8_t max_attempts, bool *rollback_required,
                                  void *user_data);

/** Persist confirmation of the running candidate and advance the rollback floor. */
int xy_fota_metadata_boot_confirm(uint8_t slot, uint32_t version, void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* XY_FOTA_METADATA_H */
