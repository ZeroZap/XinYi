#ifndef PANDORA_FOTA_INSTALL_FLASH_H
#define PANDORA_FOTA_INSTALL_FLASH_H

#include "xy_fota_boot.h"

#define PANDORA_FOTA_APP_BASE 0x08008000U
#define PANDORA_FOTA_EXECUTION_LIMIT 0x0807E000U
#define PANDORA_FOTA_INSTALL_ERASE_SIZE 0x800U
#define PANDORA_FOTA_INSTALL_PROGRAM_SIZE 8U

const xy_fota_boot_install_ops_t *pandora_fota_install_ops(void);
int pandora_fota_application_vectors_valid(void);

#endif
