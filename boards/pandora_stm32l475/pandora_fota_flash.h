#ifndef PANDORA_FOTA_FLASH_H
#define PANDORA_FOTA_FLASH_H

#include "xy_fota_boot.h"
#include "xy_fota_metadata.h"

#define PANDORA_FOTA_METADATA_BASE 0x0807E000U
#define PANDORA_FOTA_METADATA_ERASE_SIZE 0x800U

const xy_fota_metadata_flash_t *pandora_fota_metadata_backend(void);
xy_fota_boot_journal_config_t pandora_fota_boot_journal_config(void);

#endif /* PANDORA_FOTA_FLASH_H */
