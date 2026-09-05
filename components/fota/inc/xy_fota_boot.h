#ifndef XY_FOTA_BOOT_H
#define XY_FOTA_BOOT_H

#include <stdint.h>

#include "xy_fota.h"

#ifdef __cplusplus
extern "C" {
#endif

#define XY_FOTA_BOOT_CANDIDATE_MAGIC 0x58424643U
#define XY_FOTA_BOOT_CANDIDATE_FORMAT_VERSION 1U

typedef struct {
    uint32_t magic;
    uint16_t format_version;
    uint16_t header_size;
    uint32_t image_offset;
    uint32_t image_size;
    uint32_t image_version;
    uint32_t load_address;
    uint32_t image_crc32;
    uint32_t flags;
} xy_fota_boot_candidate_header_t;

typedef int (*xy_fota_boot_candidate_read_cb)(uint32_t address, uint8_t *data, uint32_t size);

typedef struct {
    uint32_t storage_address;
    uint32_t storage_size;
    uint32_t execution_base;
    uint32_t execution_limit;
    uint32_t sram_base;
    uint32_t sram_limit;
    uint32_t sram2_base;
    uint32_t sram2_limit;
    xy_fota_boot_candidate_read_cb read;
} xy_fota_boot_candidate_config_t;

int xy_fota_boot_candidate_validate(const xy_fota_boot_candidate_config_t *config,
                                    xy_fota_boot_candidate_header_t *validated_header);
int xy_fota_boot_candidate_handoff(const xy_fota_boot_candidate_config_t *config, uint8_t slot,
                                   xy_fota_boot_handoff_cb handoff, void *user_data,
                                   xy_fota_boot_candidate_header_t *validated_header);

#ifdef __cplusplus
}
#endif

#endif /* XY_FOTA_BOOT_H */
