#include "xy_fota_boot.h"

#include <stddef.h>
#include <string.h>

#define CRC32_INITIAL 0xFFFFFFFFU
#define CRC32_POLYNOMIAL 0xEDB88320U
#define CRC_CHUNK_SIZE 128U
#define JOURNAL_MAGIC 0x58424A52U
#define JOURNAL_COMMIT UINT64_C(0x434F4D54434F4D54)
#define JOURNAL_INSTALLING 1U
#define JOURNAL_INSTALLED 2U
#define JOURNAL_CONFIRMED 3U
#define JOURNAL_ROLLED_BACK 4U
#define JOURNAL_RESTAGE_AUTHORIZED 5U

typedef struct {
    uint32_t magic;
    uint32_t generation;
    uint32_t state;
    uint32_t image_version;
    uint32_t image_size;
    uint32_t image_crc32;
    uint32_t record_crc32;
    uint32_t boot_attempts;
    uint64_t commit;
} boot_journal_record_t;

static int range_fits(uint32_t start, uint32_t size, uint32_t base, uint32_t limit)
{
    return start >= base && start < limit && size <= limit - start;
}

static int valid_stack_pointer(const xy_fota_boot_candidate_config_t *config, uint32_t value)
{
    if ((value & 7U) != 0U) {
        return 0;
    }
    return value > config->sram_base && value <= config->sram_limit ? 1 :
           value > config->sram2_base && value <= config->sram2_limit;
}

static uint32_t crc32_update(uint32_t crc, const uint8_t *data, uint32_t size)
{
    for (uint32_t index = 0U; index < size; ++index) {
        crc ^= data[index];
        for (uint32_t bit = 0U; bit < 8U; ++bit) {
            crc = (crc >> 1) ^ ((crc & 1U) != 0U ? CRC32_POLYNOMIAL : 0U);
        }
    }
    return crc;
}

static uint32_t journal_crc(const boot_journal_record_t *record)
{
    return ~crc32_update(CRC32_INITIAL, (const uint8_t *)record,
                         offsetof(boot_journal_record_t, record_crc32));
}

static int journal_record_valid(const boot_journal_record_t *record)
{
    return record->magic == JOURNAL_MAGIC && record->commit == JOURNAL_COMMIT &&
           (record->state == JOURNAL_INSTALLING || record->state == JOURNAL_INSTALLED ||
            record->state == JOURNAL_CONFIRMED || record->state == JOURNAL_ROLLED_BACK ||
            record->state == JOURNAL_RESTAGE_AUTHORIZED) &&
           record->record_crc32 == journal_crc(record);
}

static int install_layout_valid(const xy_fota_boot_candidate_config_t *config,
                                const xy_fota_boot_install_ops_t *ops,
                                const xy_fota_boot_candidate_header_t *header)
{
    uint32_t erase_size;

    if (ops == NULL || ops->erase == NULL || ops->write == NULL || ops->read == NULL ||
        ops->program_granule == 0U || ops->program_granule > CRC_CHUNK_SIZE ||
        ops->erase_granule == 0U || header->load_address % ops->program_granule != 0U ||
        header->load_address % ops->erase_granule != 0U ||
        CRC_CHUNK_SIZE % ops->program_granule != 0U ||
        header->image_size > UINT32_MAX - (ops->erase_granule - 1U)) {
        return 0;
    }
    erase_size = ((header->image_size + ops->erase_granule - 1U) / ops->erase_granule) *
                 ops->erase_granule;
    return range_fits(header->load_address, erase_size, config->execution_base,
                      config->execution_limit);
}

static int execution_image_matches(const xy_fota_boot_candidate_config_t *config,
                                   const xy_fota_boot_install_ops_t *ops,
                                   const xy_fota_boot_candidate_header_t *header)
{
    uint8_t source[CRC_CHUNK_SIZE];
    uint8_t installed[CRC_CHUNK_SIZE];
    uint32_t offset = 0U;

    while (offset < header->image_size) {
        uint32_t size = header->image_size - offset;
        int ret;

        if (size > sizeof(source)) {
            size = sizeof(source);
        }
        ret = config->read(config->storage_address + header->image_offset + offset, source, size);
        if (ret != XY_FOTA_OK) {
            return ret;
        }
        ret = ops->read(header->load_address + offset, installed, size);
        if (ret != XY_FOTA_OK) {
            return ret;
        }
        if (memcmp(source, installed, size) != 0) {
            return XY_FOTA_CRC_ERROR;
        }
        offset += size;
    }
    return XY_FOTA_OK;
}

static int journal_load(const xy_fota_boot_journal_config_t *config,
                        boot_journal_record_t *record, uint32_t *slot)
{
    boot_journal_record_t copies[2];
    int valid[2];

    if (config == NULL || record == NULL || slot == NULL || config->read == NULL ||
        config->erase == NULL || config->write == NULL ||
        config->slot_size < sizeof(boot_journal_record_t) ||
        config->address > UINT32_MAX - config->slot_size ||
        config->address + config->slot_size > UINT32_MAX - config->slot_size) {
        return XY_FOTA_INVALID_PARAM;
    }
    for (uint32_t index = 0U; index < 2U; ++index) {
        int ret = config->read(config->address + index * config->slot_size,
                               (uint8_t *)&copies[index], sizeof(copies[index]));
        if (ret != XY_FOTA_OK) {
            return ret;
        }
        valid[index] = journal_record_valid(&copies[index]);
    }
    if (!valid[0] && !valid[1]) {
        memset(record, 0, sizeof(*record));
        *slot = 1U;
        return XY_FOTA_NO_IMAGE;
    }
    if (valid[0] && valid[1]) {
        uint32_t generation_delta = copies[1].generation - copies[0].generation;

        if ((generation_delta == 0U && memcmp(&copies[0], &copies[1], sizeof(copies[0])) != 0) ||
            generation_delta == 0x80000000U) {
            return XY_FOTA_FLASH_ERROR;
        }
        *slot = generation_delta != 0U && generation_delta < 0x80000000U ? 1U : 0U;
    } else {
        *slot = valid[1] ? 1U : 0U;
    }
    *record = copies[*slot];
    return XY_FOTA_OK;
}

static int journal_commit(const xy_fota_boot_journal_config_t *config,
                          const boot_journal_record_t *current, uint32_t current_slot,
                          uint32_t state, uint32_t boot_attempts,
                          const xy_fota_boot_candidate_header_t *header)
{
    boot_journal_record_t record = {
        .magic = JOURNAL_MAGIC,
        .generation = current->generation + 1U,
        .state = state,
        .image_version = header->image_version,
        .image_size = header->image_size,
        .image_crc32 = header->image_crc32,
        .boot_attempts = boot_attempts,
        .commit = UINT64_MAX,
    };
    uint32_t address = config->address + (current_slot ^ 1U) * config->slot_size;
    uint64_t marker = JOURNAL_COMMIT;
    boot_journal_record_t verify;
    int ret;

    record.record_crc32 = journal_crc(&record);
    ret = config->erase(address, config->slot_size);
    if (ret != XY_FOTA_OK) {
        return ret;
    }
    ret = config->write(address, (const uint8_t *)&record,
                        offsetof(boot_journal_record_t, commit));
    if (ret == XY_FOTA_OK) {
        ret = config->write(address + offsetof(boot_journal_record_t, commit),
                            (const uint8_t *)&marker, sizeof(marker));
    }
    if (ret == XY_FOTA_OK) {
        ret = config->read(address, (uint8_t *)&verify, sizeof(verify));
    }
    if (ret != XY_FOTA_OK || !journal_record_valid(&verify) ||
        memcmp(&verify, &record, offsetof(boot_journal_record_t, commit)) != 0) {
        (void)config->erase(address, config->slot_size);
        return ret != XY_FOTA_OK ? ret : XY_FOTA_FLASH_ERROR;
    }
    return XY_FOTA_OK;
}

int xy_fota_boot_candidate_validate(const xy_fota_boot_candidate_config_t *config,
                                    xy_fota_boot_candidate_header_t *validated_header)
{
    xy_fota_boot_candidate_header_t header;
    uint8_t chunk[CRC_CHUNK_SIZE];
    uint32_t vectors[2];
    uint32_t crc = CRC32_INITIAL;
    uint32_t offset = 0U;
    int ret;

    if (config == NULL || config->read == NULL || config->storage_size < sizeof(header) ||
        config->execution_base >= config->execution_limit ||
        config->sram_base >= config->sram_limit || config->sram2_base >= config->sram2_limit) {
        return XY_FOTA_INVALID_PARAM;
    }
    ret = config->read(config->storage_address, (uint8_t *)&header, sizeof(header));
    if (ret != XY_FOTA_OK) {
        return ret;
    }
    if (header.magic != XY_FOTA_BOOT_CANDIDATE_MAGIC ||
        header.format_version != XY_FOTA_BOOT_CANDIDATE_FORMAT_VERSION ||
        header.header_size != sizeof(header) || header.image_offset < header.header_size ||
        header.image_size < sizeof(vectors) || header.image_version == 0U ||
        header.load_address != config->execution_base || header.image_offset > config->storage_size ||
        header.image_size > config->storage_size - header.image_offset) {
        return XY_FOTA_INVALID_PARAM;
    }
    ret = config->read(config->storage_address + header.image_offset, (uint8_t *)vectors,
                       sizeof(vectors));
    if (ret != XY_FOTA_OK) {
        return ret;
    }
    if (!valid_stack_pointer(config, vectors[0]) || (vectors[1] & 1U) == 0U ||
        !range_fits(vectors[1] & ~1U, 2U, header.load_address,
                    header.load_address + header.image_size) ||
        !range_fits(header.load_address, header.image_size, config->execution_base,
                    config->execution_limit)) {
        return XY_FOTA_INVALID_PARAM;
    }
    while (offset < header.image_size) {
        uint32_t size = header.image_size - offset;
        if (size > sizeof(chunk)) {
            size = sizeof(chunk);
        }
        ret = config->read(config->storage_address + header.image_offset + offset, chunk, size);
        if (ret != XY_FOTA_OK) {
            return ret;
        }
        crc = crc32_update(crc, chunk, size);
        offset += size;
    }
    if (~crc != header.image_crc32) {
        return XY_FOTA_CRC_ERROR;
    }
    if (validated_header != NULL) {
        *validated_header = header;
    }
    return XY_FOTA_OK;
}

int xy_fota_boot_candidate_handoff(const xy_fota_boot_candidate_config_t *config, uint8_t slot,
                                   xy_fota_boot_handoff_cb handoff, void *user_data,
                                   xy_fota_boot_candidate_header_t *validated_header)
{
    xy_fota_boot_candidate_header_t header;
    int ret;

    if (handoff == NULL) {
        return XY_FOTA_INVALID_PARAM;
    }
    ret = xy_fota_boot_candidate_validate(config, &header);
    if (ret != XY_FOTA_OK) {
        return ret;
    }
    ret = handoff(slot, header.image_version, user_data);
    if (ret == XY_FOTA_OK && validated_header != NULL) {
        *validated_header = header;
    }
    return ret;
}

int xy_fota_boot_candidate_install(const xy_fota_boot_candidate_config_t *config,
                                   const xy_fota_boot_install_ops_t *ops,
                                   xy_fota_boot_candidate_header_t *installed_header)
{
    xy_fota_boot_candidate_header_t header;
    uint8_t source[CRC_CHUNK_SIZE];
    uint8_t verify[CRC_CHUNK_SIZE];
    uint32_t erase_size;
    uint32_t offset = 0U;
    int ret;

    ret = xy_fota_boot_candidate_validate(config, &header);
    if (ret != XY_FOTA_OK) {
        return ret;
    }
    if (!install_layout_valid(config, ops, &header)) {
        return XY_FOTA_INVALID_PARAM;
    }
    erase_size = ((header.image_size + ops->erase_granule - 1U) / ops->erase_granule) *
                 ops->erase_granule;
    if (!range_fits(header.load_address, erase_size, config->execution_base,
                    config->execution_limit)) {
        return XY_FOTA_INVALID_PARAM;
    }
    ret = ops->erase(header.load_address, erase_size);
    if (ret != XY_FOTA_OK) {
        return ret;
    }
    while (offset < header.image_size) {
        uint32_t payload_size = header.image_size - offset;
        uint32_t write_size;
        if (payload_size > sizeof(source)) {
            payload_size = sizeof(source);
        }
        write_size = ((payload_size + ops->program_granule - 1U) / ops->program_granule) *
                     ops->program_granule;
        memset(source, 0xFF, write_size);
        ret = config->read(config->storage_address + header.image_offset + offset, source,
                           payload_size);
        if (ret != XY_FOTA_OK) {
            return ret;
        }
        ret = ops->write(header.load_address + offset, source, write_size);
        if (ret != XY_FOTA_OK) {
            return ret;
        }
        ret = ops->read(header.load_address + offset, verify, payload_size);
        if (ret != XY_FOTA_OK) {
            return ret;
        }
        if (memcmp(source, verify, payload_size) != 0) {
            return XY_FOTA_FLASH_ERROR;
        }
        offset += payload_size;
    }
    if (installed_header != NULL) {
        *installed_header = header;
    }
    return XY_FOTA_OK;
}

int xy_fota_boot_candidate_install_once(const xy_fota_boot_candidate_config_t *config,
                                        const xy_fota_boot_install_ops_t *ops,
                                        const xy_fota_boot_journal_config_t *journal,
                                        int *installed)
{
    xy_fota_boot_candidate_header_t header;
    boot_journal_record_t current;
    uint32_t current_slot;
    int ret;

    if (installed == NULL) {
        return XY_FOTA_INVALID_PARAM;
    }
    *installed = 0;
    ret = xy_fota_boot_candidate_validate(config, &header);
    if (ret != XY_FOTA_OK) {
        return ret;
    }
    if (!install_layout_valid(config, ops, &header)) {
        return XY_FOTA_INVALID_PARAM;
    }
    ret = journal_load(journal, &current, &current_slot);
    if (ret != XY_FOTA_OK && ret != XY_FOTA_NO_IMAGE) {
        return ret;
    }
    if (ret == XY_FOTA_OK &&
        (current.state == JOURNAL_INSTALLED || current.state == JOURNAL_CONFIRMED) &&
        current.image_version == header.image_version && current.image_size == header.image_size &&
        current.image_crc32 == header.image_crc32) {
        ret = execution_image_matches(config, ops, &header);
        if (ret == XY_FOTA_OK) {
            return XY_FOTA_OK;
        }
        if (ret != XY_FOTA_CRC_ERROR) {
            return ret;
        }
    }
    if (ret == XY_FOTA_NO_IMAGE) {
        memset(&current, 0, sizeof(current));
    }
    if (ret == XY_FOTA_OK && current.state == JOURNAL_ROLLED_BACK &&
        current.image_version == header.image_version && current.image_size == header.image_size &&
        current.image_crc32 == header.image_crc32) {
        return XY_FOTA_VERSION_ERROR;
    }
    if (ret == XY_FOTA_OK && current.state == JOURNAL_RESTAGE_AUTHORIZED &&
        (current.image_version != header.image_version || current.image_size != header.image_size ||
         current.image_crc32 != header.image_crc32)) {
        return XY_FOTA_VERSION_ERROR;
    }
    ret = journal_commit(journal, &current, current_slot, JOURNAL_INSTALLING, 0U, &header);
    if (ret != XY_FOTA_OK) {
        return ret;
    }
    current.generation++;
    current_slot ^= 1U;
    ret = xy_fota_boot_candidate_install(config, ops, NULL);
    if (ret != XY_FOTA_OK) {
        return ret;
    }
    ret = journal_commit(journal, &current, current_slot, JOURNAL_INSTALLED, 0U, &header);
    if (ret != XY_FOTA_OK) {
        return ret;
    }
    *installed = 1;
    return XY_FOTA_OK;
}

static int load_matching_installed_candidate(const xy_fota_boot_candidate_config_t *config,
                                             const xy_fota_boot_install_ops_t *ops,
                                             const xy_fota_boot_journal_config_t *journal,
                                             xy_fota_boot_candidate_header_t *header,
                                             boot_journal_record_t *current,
                                             uint32_t *current_slot)
{
    int ret = xy_fota_boot_candidate_validate(config, header);

    if (ret != XY_FOTA_OK) {
        return ret;
    }
    if (!install_layout_valid(config, ops, header)) {
        return XY_FOTA_INVALID_PARAM;
    }
    ret = journal_load(journal, current, current_slot);
    if (ret != XY_FOTA_OK) {
        return ret;
    }
    if (current->image_version != header->image_version ||
        current->image_size != header->image_size || current->image_crc32 != header->image_crc32 ||
        (current->state != JOURNAL_INSTALLED && current->state != JOURNAL_CONFIRMED)) {
        return XY_FOTA_NO_IMAGE;
    }
    return execution_image_matches(config, ops, header);
}

int xy_fota_boot_candidate_record_attempt(const xy_fota_boot_candidate_config_t *config,
                                          const xy_fota_boot_install_ops_t *ops,
                                          const xy_fota_boot_journal_config_t *journal,
                                          uint32_t max_attempts, int *rollback_required)
{
    xy_fota_boot_candidate_header_t header;
    boot_journal_record_t current;
    uint32_t current_slot;
    uint32_t attempts;
    uint32_t state;
    int ret;

    int local_rollback_required = 0;

    if (rollback_required == NULL || max_attempts == 0U) {
        return XY_FOTA_INVALID_PARAM;
    }
    ret = load_matching_installed_candidate(config, ops, journal, &header, &current,
                                            &current_slot);
    if (ret != XY_FOTA_OK) {
        return ret;
    }
    if (current.state == JOURNAL_CONFIRMED) {
        *rollback_required = local_rollback_required;
        return XY_FOTA_OK;
    }
    attempts = current.boot_attempts == UINT32_MAX ? 1U : current.boot_attempts + 1U;
    state = attempts >= max_attempts ? JOURNAL_ROLLED_BACK : JOURNAL_INSTALLED;
    local_rollback_required = state == JOURNAL_ROLLED_BACK;
    ret = journal_commit(journal, &current, current_slot, state, attempts, &header);
    if (ret == XY_FOTA_OK) {
        *rollback_required = local_rollback_required;
    }
    return ret;
}

int xy_fota_boot_candidate_confirm(const xy_fota_boot_candidate_config_t *config,
                                   const xy_fota_boot_install_ops_t *ops,
                                   const xy_fota_boot_journal_config_t *journal)
{
    xy_fota_boot_candidate_header_t header;
    boot_journal_record_t current;
    uint32_t current_slot;
    int ret = load_matching_installed_candidate(config, ops, journal, &header, &current,
                                                &current_slot);

    if (ret != XY_FOTA_OK) {
        return ret;
    }
    if (current.state == JOURNAL_CONFIRMED) {
        return XY_FOTA_OK;
    }
    return journal_commit(journal, &current, current_slot, JOURNAL_CONFIRMED, 0U, &header);
}

int xy_fota_boot_candidate_authorize_restage(
    const xy_fota_boot_candidate_config_t *config, const xy_fota_boot_install_ops_t *ops,
    const xy_fota_boot_journal_config_t *journal,
    const xy_fota_boot_restage_authorization_t *authorization)
{
    xy_fota_boot_candidate_header_t header;
    boot_journal_record_t current;
    uint32_t current_slot;
    int ret;

    if (authorization == NULL ||
        authorization->magic != XY_FOTA_BOOT_RESTAGE_AUTHORIZATION_MAGIC ||
        authorization->format_version != XY_FOTA_BOOT_RESTAGE_AUTHORIZATION_VERSION ||
        authorization->size != sizeof(*authorization)) {
        return XY_FOTA_INVALID_PARAM;
    }
    ret = xy_fota_boot_candidate_validate(config, &header);
    if (ret != XY_FOTA_OK) {
        return ret;
    }
    if (!install_layout_valid(config, ops, &header) ||
        authorization->image_version != header.image_version ||
        authorization->image_size != header.image_size ||
        authorization->image_crc32 != header.image_crc32) {
        return XY_FOTA_INVALID_PARAM;
    }
    ret = journal_load(journal, &current, &current_slot);
    if (ret != XY_FOTA_OK) {
        return ret;
    }
    if (current.state != JOURNAL_ROLLED_BACK || current.image_version != header.image_version ||
        current.image_size != header.image_size || current.image_crc32 != header.image_crc32) {
        return XY_FOTA_VERSION_ERROR;
    }
    return journal_commit(journal, &current, current_slot, JOURNAL_RESTAGE_AUTHORIZED, 0U, &header);
}

int xy_fota_boot_candidate_authorize_reviewed_restage(
    const xy_fota_boot_candidate_config_t *config, const xy_fota_boot_install_ops_t *ops,
    const xy_fota_boot_journal_config_t *journal,
    const xy_fota_boot_reviewed_restage_authorization_t *authorization,
    const uint32_t expected_source_commit[5])
{
    if (authorization == NULL || expected_source_commit == NULL ||
        memcmp(authorization->source_commit, expected_source_commit,
               sizeof(authorization->source_commit)) != 0) {
        return XY_FOTA_INVALID_PARAM;
    }
    return xy_fota_boot_candidate_authorize_restage(config, ops, journal,
                                                     &authorization->candidate);
}
