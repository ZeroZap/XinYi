#ifndef XY_FOTA_BOOT_H
#define XY_FOTA_BOOT_H

#include <stdint.h>

#include "xy_fota.h"

#ifdef __cplusplus
extern "C" {
#endif

#define XY_FOTA_BOOT_CANDIDATE_MAGIC 0x58424643U
#define XY_FOTA_BOOT_CANDIDATE_FORMAT_VERSION 1U
#define XY_FOTA_BOOT_RESTAGE_AUTHORIZATION_MAGIC 0x58524155U
#define XY_FOTA_BOOT_RESTAGE_AUTHORIZATION_VERSION 1U

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

typedef int (*xy_fota_boot_erase_cb)(uint32_t address, uint32_t size);
typedef int (*xy_fota_boot_write_cb)(uint32_t address, const uint8_t *data, uint32_t size);

typedef struct {
    xy_fota_boot_erase_cb erase;
    xy_fota_boot_write_cb write;
    xy_fota_boot_candidate_read_cb read;
    uint32_t program_granule;
    uint32_t erase_granule;
} xy_fota_boot_install_ops_t;

typedef struct {
    uint32_t address;
    uint32_t slot_size;
    xy_fota_boot_candidate_read_cb read;
    xy_fota_boot_erase_cb erase;
    xy_fota_boot_write_cb write;
} xy_fota_boot_journal_config_t;

typedef struct {
    uint32_t magic;
    uint16_t format_version;
    uint16_t size;
    uint32_t image_version;
    uint32_t image_size;
    uint32_t image_crc32;
} xy_fota_boot_restage_authorization_t;

int xy_fota_boot_candidate_validate(const xy_fota_boot_candidate_config_t *config,
                                    xy_fota_boot_candidate_header_t *validated_header);
int xy_fota_boot_candidate_handoff(const xy_fota_boot_candidate_config_t *config, uint8_t slot,
                                   xy_fota_boot_handoff_cb handoff, void *user_data,
                                   xy_fota_boot_candidate_header_t *validated_header);
int xy_fota_boot_candidate_install(const xy_fota_boot_candidate_config_t *config,
                                   const xy_fota_boot_install_ops_t *ops,
                                   xy_fota_boot_candidate_header_t *installed_header);
int xy_fota_boot_candidate_install_once(const xy_fota_boot_candidate_config_t *config,
                                        const xy_fota_boot_install_ops_t *ops,
                                        const xy_fota_boot_journal_config_t *journal,
                                        int *installed);
int xy_fota_boot_candidate_record_attempt(const xy_fota_boot_candidate_config_t *config,
                                          const xy_fota_boot_install_ops_t *ops,
                                          const xy_fota_boot_journal_config_t *journal,
                                          uint32_t max_attempts, int *rollback_required);
int xy_fota_boot_candidate_confirm(const xy_fota_boot_candidate_config_t *config,
                                   const xy_fota_boot_install_ops_t *ops,
                                   const xy_fota_boot_journal_config_t *journal);
int xy_fota_boot_candidate_authorize_restage(
    const xy_fota_boot_candidate_config_t *config, const xy_fota_boot_install_ops_t *ops,
    const xy_fota_boot_journal_config_t *journal,
    const xy_fota_boot_restage_authorization_t *authorization);

#ifdef __cplusplus
}
#endif

#endif /* XY_FOTA_BOOT_H */
