#ifndef PANDORA_FOTA_INSTALL_FLASH_H
#define PANDORA_FOTA_INSTALL_FLASH_H

#include "xy_fota_boot.h"

#define PANDORA_FOTA_APP_BASE 0x08008000U
#define PANDORA_FOTA_EXECUTION_LIMIT 0x0807E000U
#define PANDORA_FOTA_INSTALL_ERASE_SIZE 0x800U
#define PANDORA_FOTA_INSTALL_PROGRAM_SIZE 8U

const xy_fota_boot_install_ops_t *pandora_fota_install_ops(void);

/**
 * Read the application vector table at PANDORA_FOTA_APP_BASE and validate it.
 *
 * The address-mapped fetch is target-only; the decision rules live in
 * pandora_fota_application_vectors_check() so they are host-testable.
 */
int pandora_fota_application_vectors_valid(void);

/**
 * Validate the two initial vector words of an application image.
 *
 * @param initial_sp     Initial stack pointer (vector[0]).
 * @param reset_handler  Reset handler address (vector[1]).
 * @return 1 when the image is a plausible Pandora application image, else 0.
 */
int pandora_fota_application_vectors_check(uint32_t initial_sp, uint32_t reset_handler);

#endif
