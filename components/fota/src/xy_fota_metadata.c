#include "xy_fota_metadata.h"

#include <stddef.h>
#include <string.h>

#define XY_FOTA_METADATA_MAGIC 0x58464D44U
#define XY_FOTA_METADATA_FORMAT_VERSION 2U
#define XY_FOTA_METADATA_SLOT_COUNT 2U
#define XY_FOTA_METADATA_COMMITTED 0xA55AC33CU

typedef struct {
    uint32_t magic;
    uint32_t format_version;
    uint32_t generation;
    uint32_t active_version;
    uint32_t min_version;
    uint32_t pending_version;
    uint8_t active_slot;
    uint8_t pending_slot;
    uint8_t boot_attempts;
    uint8_t flags;
    uint32_t crc32;
    uint32_t committed;
} xy_fota_metadata_record_t;

static bool metadata_fields_are_valid(uint32_t active_version, uint32_t min_version,
                                      uint32_t pending_version, uint8_t active_slot,
                                      uint8_t pending_slot, uint8_t boot_attempts, uint8_t flags)
{
    bool pending;

    if (active_slot > 1U || active_version < min_version ||
        (flags & (uint8_t)~XY_FOTA_METADATA_FLAG_PENDING) != 0U) {
        return false;
    }

    pending = (flags & XY_FOTA_METADATA_FLAG_PENDING) != 0U;
    if (pending) {
        return pending_slot <= 1U && pending_slot != active_slot && pending_version != 0U &&
               pending_version >= min_version;
    }

    return pending_slot == XY_FOTA_METADATA_NO_SLOT && pending_version == 0U &&
           boot_attempts == 0U;
}

static bool record_is_valid(const xy_fota_metadata_record_t *record)
{
    uint32_t expected_crc;

    if (record->magic != XY_FOTA_METADATA_MAGIC ||
        record->format_version != XY_FOTA_METADATA_FORMAT_VERSION ||
        record->committed != XY_FOTA_METADATA_COMMITTED) {
        return false;
    }

    expected_crc = xy_fota_calc_crc32((const uint8_t *)record,
                                      (uint32_t)offsetof(xy_fota_metadata_record_t, crc32));
    return record->crc32 == expected_crc &&
           metadata_fields_are_valid(record->active_version, record->min_version,
                                     record->pending_version, record->active_slot,
                                     record->pending_slot, record->boot_attempts, record->flags);
}

static bool generation_order_is_ambiguous(uint32_t first, uint32_t second)
{
    return first != second && first - second == 0x80000000U;
}

static bool generation_is_newer(uint32_t candidate, uint32_t current)
{
    return (int32_t)(candidate - current) > 0;
}

static int validate_backend(const xy_fota_metadata_flash_t *backend)
{
    uint32_t last_slot_offset;

    if (!backend || !backend->ops || !backend->ops->read || !backend->ops->write ||
        !backend->ops->erase || backend->erase_size < sizeof(xy_fota_metadata_record_t)) {
        return XY_FOTA_INVALID_PARAM;
    }

    last_slot_offset = (XY_FOTA_METADATA_SLOT_COUNT - 1U) * backend->erase_size;
    if (backend->base_addr > UINT32_MAX - last_slot_offset ||
        backend->base_addr + last_slot_offset >
            UINT32_MAX - (uint32_t)sizeof(xy_fota_metadata_record_t)) {
        return XY_FOTA_INVALID_PARAM;
    }
    return XY_FOTA_OK;
}

static int validate_metadata(const xy_fota_metadata_t *metadata)
{
    if (!metadata || !metadata_fields_are_valid(metadata->active_version, metadata->min_version,
                                                 metadata->pending_version, metadata->active_slot,
                                                 metadata->pending_slot, metadata->boot_attempts,
                                                 metadata->flags)) {
        return XY_FOTA_INVALID_PARAM;
    }

    return XY_FOTA_OK;
}

static int read_record(const xy_fota_metadata_flash_t *backend, uint32_t slot,
                       xy_fota_metadata_record_t *record)
{
    uint32_t addr = backend->base_addr + slot * backend->erase_size;

    if (backend->ops->read(addr, (uint8_t *)record, sizeof(*record)) != XY_FOTA_OK) {
        return XY_FOTA_FLASH_ERROR;
    }
    return XY_FOTA_OK;
}

static void record_to_metadata(const xy_fota_metadata_record_t *record,
                               xy_fota_metadata_t *metadata)
{
    metadata->generation = record->generation;
    metadata->active_version = record->active_version;
    metadata->min_version = record->min_version;
    metadata->pending_version = record->pending_version;
    metadata->active_slot = record->active_slot;
    metadata->pending_slot = record->pending_slot;
    metadata->boot_attempts = record->boot_attempts;
    metadata->flags = record->flags;
}

static int load_latest_record(const xy_fota_metadata_flash_t *backend,
                              xy_fota_metadata_record_t *latest, uint32_t *latest_slot)
{
    xy_fota_metadata_record_t record;
    bool found = false;
    bool read_failed = false;

    for (uint32_t slot = 0; slot < XY_FOTA_METADATA_SLOT_COUNT; ++slot) {
        if (read_record(backend, slot, &record) != XY_FOTA_OK) {
            read_failed = true;
            continue;
        }
        if (!record_is_valid(&record)) {
            continue;
        }
        if (found && generation_order_is_ambiguous(record.generation, latest->generation)) {
            return XY_FOTA_FLASH_ERROR;
        }
        if (!found || generation_is_newer(record.generation, latest->generation)) {
            *latest = record;
            *latest_slot = slot;
            found = true;
        }
    }

    if (read_failed) {
        return XY_FOTA_FLASH_ERROR;
    }
    return found ? XY_FOTA_OK : XY_FOTA_NO_IMAGE;
}

int xy_fota_metadata_flash_load(const xy_fota_metadata_flash_t *backend,
                                xy_fota_metadata_t *metadata)
{
    xy_fota_metadata_record_t latest;
    uint32_t latest_slot;
    int ret;

    if (!metadata || validate_backend(backend) != XY_FOTA_OK) {
        return XY_FOTA_INVALID_PARAM;
    }

    ret = load_latest_record(backend, &latest, &latest_slot);
    if (ret != XY_FOTA_OK) {
        return ret;
    }

    record_to_metadata(&latest, metadata);
    return XY_FOTA_OK;
}

int xy_fota_metadata_flash_commit(const xy_fota_metadata_flash_t *backend,
                                  const xy_fota_metadata_t *metadata,
                                  xy_fota_metadata_t *committed)
{
    xy_fota_metadata_record_t latest;
    xy_fota_metadata_record_t record;
    xy_fota_metadata_record_t verify;
    uint32_t latest_slot = 0U;
    uint32_t target_slot;
    uint32_t target_addr;
    int ret;

    if (validate_metadata(metadata) != XY_FOTA_OK || validate_backend(backend) != XY_FOTA_OK) {
        return XY_FOTA_INVALID_PARAM;
    }

    ret = load_latest_record(backend, &latest, &latest_slot);
    if (ret != XY_FOTA_OK && ret != XY_FOTA_NO_IMAGE) {
        return ret;
    }

    target_slot = (ret == XY_FOTA_OK) ? (1U - latest_slot) : 0U;
    memset(&record, 0, sizeof(record));
    record.magic = XY_FOTA_METADATA_MAGIC;
    record.format_version = XY_FOTA_METADATA_FORMAT_VERSION;
    record.generation = (ret == XY_FOTA_OK) ? latest.generation + 1U : 1U;
    record.active_version = metadata->active_version;
    record.min_version = metadata->min_version;
    record.pending_version = metadata->pending_version;
    record.active_slot = metadata->active_slot;
    record.pending_slot = metadata->pending_slot;
    record.boot_attempts = metadata->boot_attempts;
    record.flags = metadata->flags;
    record.crc32 = xy_fota_calc_crc32((const uint8_t *)&record,
                                      (uint32_t)offsetof(xy_fota_metadata_record_t, crc32));
    record.committed = XY_FOTA_METADATA_COMMITTED;

    target_addr = backend->base_addr + target_slot * backend->erase_size;
    if (backend->ops->erase(target_addr, backend->erase_size) != XY_FOTA_OK ||
        backend->ops->write(target_addr, (const uint8_t *)&record,
                            (uint32_t)offsetof(xy_fota_metadata_record_t, committed)) != XY_FOTA_OK) {
        return XY_FOTA_FLASH_ERROR;
    }

    if (backend->ops->write(target_addr + (uint32_t)offsetof(xy_fota_metadata_record_t, committed),
                            (const uint8_t *)&record.committed,
                            (uint32_t)sizeof(record.committed)) != XY_FOTA_OK) {
        (void)backend->ops->erase(target_addr, backend->erase_size);
        return XY_FOTA_FLASH_ERROR;
    }

    if (read_record(backend, target_slot, &verify) != XY_FOTA_OK || !record_is_valid(&verify) ||
        memcmp(&record, &verify, sizeof(record)) != 0) {
        (void)backend->ops->erase(target_addr, backend->erase_size);
        return XY_FOTA_FLASH_ERROR;
    }

    if (committed) {
        record_to_metadata(&record, committed);
    }
    return XY_FOTA_OK;
}

int xy_fota_metadata_stage_candidate(xy_fota_metadata_t *metadata, uint8_t slot,
                                     uint32_t version)
{
    if (validate_metadata(metadata) != XY_FOTA_OK || slot > 1U ||
        slot == metadata->active_slot || version == 0U) {
        return XY_FOTA_INVALID_PARAM;
    }
    if ((metadata->flags & XY_FOTA_METADATA_FLAG_PENDING) != 0U) {
        return XY_FOTA_IN_PROGRESS;
    }
    if (version < metadata->min_version) {
        return XY_FOTA_VERSION_ERROR;
    }
    metadata->pending_version = version;
    metadata->pending_slot = slot;
    metadata->boot_attempts = 0U;
    metadata->flags |= XY_FOTA_METADATA_FLAG_PENDING;
    return XY_FOTA_OK;
}

int xy_fota_metadata_record_boot_attempt(xy_fota_metadata_t *metadata, uint8_t max_attempts,
                                         bool *rollback_required)
{
    if (!rollback_required || max_attempts == 0U || validate_metadata(metadata) != XY_FOTA_OK) {
        return XY_FOTA_INVALID_PARAM;
    }
    if ((metadata->flags & XY_FOTA_METADATA_FLAG_PENDING) == 0U ||
        metadata->pending_slot == XY_FOTA_METADATA_NO_SLOT) {
        return XY_FOTA_NO_IMAGE;
    }
    if (metadata->boot_attempts >= max_attempts) {
        *rollback_required = true;
    } else {
        metadata->boot_attempts++;
        *rollback_required = metadata->boot_attempts >= max_attempts;
    }
    if (*rollback_required) {
        metadata->pending_version = 0U;
        metadata->pending_slot = XY_FOTA_METADATA_NO_SLOT;
        metadata->boot_attempts = 0U;
        metadata->flags &= (uint8_t)~XY_FOTA_METADATA_FLAG_PENDING;
    }
    return XY_FOTA_OK;
}

int xy_fota_metadata_confirm_candidate(xy_fota_metadata_t *metadata)
{
    if (validate_metadata(metadata) != XY_FOTA_OK) {
        return XY_FOTA_INVALID_PARAM;
    }
    if ((metadata->flags & XY_FOTA_METADATA_FLAG_PENDING) == 0U ||
        metadata->pending_slot == XY_FOTA_METADATA_NO_SLOT || metadata->pending_version == 0U) {
        return XY_FOTA_NO_IMAGE;
    }
    metadata->active_slot = metadata->pending_slot;
    metadata->active_version = metadata->pending_version;
    if (metadata->min_version < metadata->active_version) {
        metadata->min_version = metadata->active_version;
    }
    metadata->pending_version = 0U;
    metadata->pending_slot = XY_FOTA_METADATA_NO_SLOT;
    metadata->boot_attempts = 0U;
    metadata->flags &= (uint8_t)~XY_FOTA_METADATA_FLAG_PENDING;
    return XY_FOTA_OK;
}

int xy_fota_metadata_boot_handoff(uint8_t slot, uint32_t version, void *user_data)
{
    xy_fota_metadata_flash_t *backend = (xy_fota_metadata_flash_t *)user_data;
    xy_fota_metadata_t metadata;
    int ret;

    if (validate_backend(backend) != XY_FOTA_OK) {
        return XY_FOTA_INVALID_PARAM;
    }

    ret = xy_fota_metadata_flash_load(backend, &metadata);
    if (ret != XY_FOTA_OK) {
        return ret;
    }
    ret = xy_fota_metadata_stage_candidate(&metadata, slot, version);
    if (ret != XY_FOTA_OK) {
        return ret;
    }
    return xy_fota_metadata_flash_commit(backend, &metadata, NULL);
}

int xy_fota_metadata_boot_attempt(uint8_t max_attempts, bool *rollback_required,
                                  void *user_data)
{
    xy_fota_metadata_flash_t *backend = (xy_fota_metadata_flash_t *)user_data;
    xy_fota_metadata_t metadata;
    bool rollback;
    int ret;

    if (!rollback_required || validate_backend(backend) != XY_FOTA_OK) {
        return XY_FOTA_INVALID_PARAM;
    }

    ret = xy_fota_metadata_flash_load(backend, &metadata);
    if (ret != XY_FOTA_OK) {
        return ret;
    }
    ret = xy_fota_metadata_record_boot_attempt(&metadata, max_attempts, &rollback);
    if (ret != XY_FOTA_OK) {
        return ret;
    }
    ret = xy_fota_metadata_flash_commit(backend, &metadata, NULL);
    if (ret != XY_FOTA_OK) {
        return ret;
    }
    *rollback_required = rollback;
    return XY_FOTA_OK;
}

int xy_fota_metadata_boot_confirm(uint8_t slot, uint32_t version, void *user_data)
{
    xy_fota_metadata_flash_t *backend = (xy_fota_metadata_flash_t *)user_data;
    xy_fota_metadata_t metadata;
    int ret;

    if (validate_backend(backend) != XY_FOTA_OK) {
        return XY_FOTA_INVALID_PARAM;
    }

    ret = xy_fota_metadata_flash_load(backend, &metadata);
    if (ret != XY_FOTA_OK) {
        return ret;
    }
    if (slot != metadata.pending_slot) {
        return XY_FOTA_INVALID_PARAM;
    }
    if (version != metadata.pending_version) {
        return XY_FOTA_VERSION_ERROR;
    }
    ret = xy_fota_metadata_confirm_candidate(&metadata);
    if (ret != XY_FOTA_OK) {
        return ret;
    }
    return xy_fota_metadata_flash_commit(backend, &metadata, NULL);
}
