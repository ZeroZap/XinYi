#include "xy_fota_boot.h"

#include <stddef.h>
#include <string.h>

#define CRC32_INITIAL 0xFFFFFFFFU
#define CRC32_POLYNOMIAL 0xEDB88320U
#define CRC_CHUNK_SIZE 128U

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

    if (ops == NULL || ops->erase == NULL || ops->write == NULL || ops->read == NULL ||
        ops->program_granule == 0U || ops->program_granule > sizeof(source) ||
        ops->erase_granule == 0U) {
        return XY_FOTA_INVALID_PARAM;
    }
    ret = xy_fota_boot_candidate_validate(config, &header);
    if (ret != XY_FOTA_OK) {
        return ret;
    }
    if (header.image_size > UINT32_MAX - (ops->erase_granule - 1U)) {
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
