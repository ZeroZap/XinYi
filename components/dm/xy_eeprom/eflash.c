#include "eflash.h"
#include <string.h>
#include <stdlib.h>

/**
 * @file eflash.c
 * @brief Flash interface simulation for EEPROM read/write operations
 * @version 1.0
 * @date 2025-10-22
 */

#define EFLASH_ERASED_VALUE 0xFF /**< Erased flash value */

/**
 * @brief Initialize flash device
 */
eflash_result_t eflash_init(eflash_handle_t *handle,
                            const eflash_config_t *config)
{
    if (handle == NULL || config == NULL) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    /* Validate configuration */
    if (config->page_size == 0 || config->page_size > EFLASH_MAX_PAGE_SIZE) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    if (config->page_count == 0 || config->page_count > EFLASH_MAX_PAGES) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    /* Calculate total size */
    uint32_t total_size = config->page_size * config->page_count;
    if (config->total_size > 0 && config->total_size != total_size) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    /* Validate write unit */
    if (config->write_unit != EFLASH_WRITE_UNIT_32BIT
        && config->write_unit != EFLASH_WRITE_UNIT_64BIT
        && config->write_unit != EFLASH_WRITE_UNIT_128BIT) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    /* Clean up existing memory if already initialized */
    if (handle->initialized && handle->memory != NULL) {
        free(handle->memory);
        handle->memory = NULL;
    }
    if (handle->initialized && handle->page_erased != NULL) {
        free(handle->page_erased);
        handle->page_erased = NULL;
    }

    /* Allocate memory for simulated flash */
    handle->memory = (uint8_t *)malloc(total_size);
    if (handle->memory == NULL) {
        return EFLASH_ERROR_WRITE_FAIL;
    }

    /* Allocate page erase status array */
    handle->page_erased = (bool *)malloc(config->page_count * sizeof(bool));
    if (handle->page_erased == NULL) {
        free(handle->memory);
        handle->memory = NULL;
        return EFLASH_ERROR_WRITE_FAIL;
    }

    /* Initialize configuration */
    handle->config            = *config;
    handle->config.total_size = total_size;

    /* Initialize flash memory to erased state */
    memset(handle->memory, EFLASH_ERASED_VALUE, total_size);

    /* Mark all pages as erased */
    for (uint32_t i = 0; i < config->page_count; i++) {
        handle->page_erased[i] = true;
    }

    handle->initialized = true;

    return EFLASH_OK;
}

/**
 * @brief Deinitialize flash device
 */
eflash_result_t eflash_deinit(eflash_handle_t *handle)
{
    if (handle == NULL) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    if (!handle->initialized) {
        return EFLASH_ERROR_NOT_INIT;
    }

    /* Free allocated memory */
    if (handle->memory != NULL) {
        free(handle->memory);
        handle->memory = NULL;
    }

    if (handle->page_erased != NULL) {
        free(handle->page_erased);
        handle->page_erased = NULL;
    }

    handle->initialized = false;

    return EFLASH_OK;
}

/**
 * @brief Read data from flash
 */
eflash_result_t eflash_read(eflash_handle_t *handle, uint32_t address,
                            uint8_t *data, size_t size)
{
    if (handle == NULL || data == NULL) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    if (!handle->initialized) {
        return EFLASH_ERROR_NOT_INIT;
    }

    if (size == 0) {
        return EFLASH_OK;
    }

    /* Check address range */
    if (!eflash_is_address_valid(handle, address, size)) {
        return EFLASH_ERROR_OUT_OF_RANGE;
    }

    /* Read data from simulated flash */
    memcpy(data, &handle->memory[address], size);

    return EFLASH_OK;
}

/**
 * @brief Write data to flash
 */
eflash_result_t eflash_write(eflash_handle_t *handle, uint32_t address,
                             const uint8_t *data, size_t size)
{
    if (handle == NULL || data == NULL) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    if (!handle->initialized) {
        return EFLASH_ERROR_NOT_INIT;
    }

    if (size == 0) {
        return EFLASH_OK;
    }

    /* Check address range */
    if (!eflash_is_address_valid(handle, address, size)) {
        return EFLASH_ERROR_OUT_OF_RANGE;
    }

    /* Check alignment */
    uint32_t write_unit = (uint32_t)handle->config.write_unit;
    if (!EFLASH_IS_ALIGNED(address, write_unit)
        || !EFLASH_IS_ALIGNED(size, write_unit)) {
        return EFLASH_ERROR_ALIGNMENT;
    }

    /* Auto erase if enabled */
    if (handle->config.auto_erase) {
        uint32_t start_page = eflash_get_page_index(handle, address);
        uint32_t end_page   = eflash_get_page_index(handle, address + size - 1);

        for (uint32_t page = start_page; page <= end_page; page++) {
            if (!handle->page_erased[page]) {
                eflash_result_t result = eflash_erase_page(handle, page);
                if (result != EFLASH_OK) {
                    return result;
                }
            }
        }
    }

    /* Simulate flash write operation (can only change 1 to 0) */
    for (size_t i = 0; i < size; i++) {
        uint32_t addr   = address + i;
        uint8_t current = handle->memory[addr];
        uint8_t new_val = data[i];

        /* Flash write can only clear bits (1 -> 0), not set them (0 -> 1) */
        if ((current & new_val) != new_val) {
            /* Trying to write 1 where there is 0 - need to erase first */
            if (!handle->config.auto_erase) {
                return EFLASH_ERROR_WRITE_FAIL;
            }
        }

        handle->memory[addr] = current & new_val;
    }

    /* Mark affected pages as not erased */
    uint32_t start_page = eflash_get_page_index(handle, address);
    uint32_t end_page   = eflash_get_page_index(handle, address + size - 1);
    for (uint32_t page = start_page; page <= end_page; page++) {
        handle->page_erased[page] = false;
    }

    return EFLASH_OK;
}

/**
 * @brief Erase flash page
 */
eflash_result_t eflash_erase_page(eflash_handle_t *handle, uint32_t page_index)
{
    if (handle == NULL) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    if (!handle->initialized) {
        return EFLASH_ERROR_NOT_INIT;
    }

    if (page_index >= handle->config.page_count) {
        return EFLASH_ERROR_OUT_OF_RANGE;
    }

    /* Erase page (set all bytes to 0xFF) */
    uint32_t page_offset = page_index * handle->config.page_size;
    memset(&handle->memory[page_offset], EFLASH_ERASED_VALUE,
           handle->config.page_size);

    /* Mark page as erased */
    handle->page_erased[page_index] = true;

    return EFLASH_OK;
}

/**
 * @brief Erase flash sector (address-based)
 */
eflash_result_t eflash_erase_sector(eflash_handle_t *handle, uint32_t address)
{
    if (handle == NULL) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    if (!handle->initialized) {
        return EFLASH_ERROR_NOT_INIT;
    }

    /* Get page index from address */
    uint32_t page_index = eflash_get_page_index(handle, address);

    return eflash_erase_page(handle, page_index);
}

/**
 * @brief Erase entire flash
 */
eflash_result_t eflash_erase_all(eflash_handle_t *handle)
{
    if (handle == NULL) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    if (!handle->initialized) {
        return EFLASH_ERROR_NOT_INIT;
    }

    /* Erase all flash memory */
    memset(handle->memory, EFLASH_ERASED_VALUE, handle->config.total_size);

    /* Mark all pages as erased */
    for (uint32_t i = 0; i < handle->config.page_count; i++) {
        handle->page_erased[i] = true;
    }

    return EFLASH_OK;
}

/**
 * @brief Get flash information
 */
eflash_result_t eflash_get_info(eflash_handle_t *handle,
                                eflash_config_t *config)
{
    if (handle == NULL || config == NULL) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    if (!handle->initialized) {
        return EFLASH_ERROR_NOT_INIT;
    }

    *config = handle->config;

    return EFLASH_OK;
}

/**
 * @brief Check if address range is valid
 */
bool eflash_is_address_valid(eflash_handle_t *handle, uint32_t address,
                             size_t size)
{
    if (handle == NULL || !handle->initialized) {
        return false;
    }

    if (address >= handle->config.total_size) {
        return false;
    }

    if (address + size > handle->config.total_size) {
        return false;
    }

    return true;
}

/**
 * @brief Get page index from address
 */
uint32_t eflash_get_page_index(eflash_handle_t *handle, uint32_t address)
{
    if (handle == NULL || !handle->initialized) {
        return 0;
    }

    return address / handle->config.page_size;
}

/**
 * @brief Check if page is erased
 */
bool eflash_is_page_erased(eflash_handle_t *handle, uint32_t page_index)
{
    if (handle == NULL || !handle->initialized) {
        return false;
    }

    if (page_index >= handle->config.page_count) {
        return false;
    }

    return handle->page_erased[page_index];
}
