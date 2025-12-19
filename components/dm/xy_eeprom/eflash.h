#ifndef EFLASH_H
#define EFLASH_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file eflash.h
 * @brief Flash interface simulation for EEPROM read/write operations
 * @version 1.0
 * @date 2025-10-22
 */

/* Configuration constants */
#define EFLASH_MAX_PAGES         64   /**< Maximum number of pages */
#define EFLASH_MAX_PAGE_SIZE     4096 /**< Maximum page size in bytes */
#define EFLASH_DEFAULT_PAGE_SIZE 512  /**< Default page size in bytes */

/* Write unit sizes */
typedef enum {
    EFLASH_WRITE_UNIT_32BIT  = 4,  /**< 32-bit write unit (4 bytes) */
    EFLASH_WRITE_UNIT_64BIT  = 8,  /**< 64-bit write unit (8 bytes) */
    EFLASH_WRITE_UNIT_128BIT = 16, /**< 128-bit write unit (16 bytes) */
} eflash_write_unit_t;

/* Error codes */
typedef enum {
    EFLASH_OK = 0,              /**< Operation successful */
    EFLASH_ERROR_INVALID_PARAM, /**< Invalid parameter */
    EFLASH_ERROR_OUT_OF_RANGE,  /**< Address out of range */
    EFLASH_ERROR_ALIGNMENT,     /**< Address/size alignment error */
    EFLASH_ERROR_WRITE_FAIL,    /**< Write operation failed */
    EFLASH_ERROR_ERASE_FAIL,    /**< Erase operation failed */
    EFLASH_ERROR_NOT_INIT,      /**< Device not initialized */
    EFLASH_ERROR_BUSY,          /**< Device is busy */
} eflash_result_t;

/* Flash configuration structure */
typedef struct {
    uint32_t total_size;            /**< Total flash size in bytes */
    uint32_t page_size;             /**< Page size in bytes */
    uint32_t page_count;            /**< Number of pages */
    eflash_write_unit_t write_unit; /**< Minimum write unit size */
    bool auto_erase;                /**< Auto erase before write */
} eflash_config_t;

/* Flash handle structure */
typedef struct {
    eflash_config_t config; /**< Flash configuration */
    uint8_t *memory;        /**< Simulated flash memory */
    bool *page_erased;      /**< Page erase status array */
    bool initialized;       /**< Initialization status */
} eflash_handle_t;

/**
 * @brief Initialize flash device
 * @param handle Pointer to flash handle
 * @param config Pointer to configuration structure
 * @return eflash_result_t Operation result
 */
eflash_result_t eflash_init(eflash_handle_t *handle,
                            const eflash_config_t *config);

/**
 * @brief Deinitialize flash device
 * @param handle Pointer to flash handle
 * @return eflash_result_t Operation result
 */
eflash_result_t eflash_deinit(eflash_handle_t *handle);

/**
 * @brief Read data from flash
 * @param handle Pointer to flash handle
 * @param address Start address to read from
 * @param data Pointer to buffer for read data
 * @param size Number of bytes to read
 * @return eflash_result_t Operation result
 */
eflash_result_t eflash_read(eflash_handle_t *handle, uint32_t address,
                            uint8_t *data, size_t size);

/**
 * @brief Write data to flash
 * @param handle Pointer to flash handle
 * @param address Start address to write to
 * @param data Pointer to data to write
 * @param size Number of bytes to write
 * @return eflash_result_t Operation result
 */
eflash_result_t eflash_write(eflash_handle_t *handle, uint32_t address,
                             const uint8_t *data, size_t size);

/**
 * @brief Erase flash page
 * @param handle Pointer to flash handle
 * @param page_index Page index to erase
 * @return eflash_result_t Operation result
 */
eflash_result_t eflash_erase_page(eflash_handle_t *handle, uint32_t page_index);

/**
 * @brief Erase flash sector (address-based)
 * @param handle Pointer to flash handle
 * @param address Address within the sector to erase
 * @return eflash_result_t Operation result
 */
eflash_result_t eflash_erase_sector(eflash_handle_t *handle, uint32_t address);

/**
 * @brief Erase entire flash
 * @param handle Pointer to flash handle
 * @return eflash_result_t Operation result
 */
eflash_result_t eflash_erase_all(eflash_handle_t *handle);

/**
 * @brief Get flash information
 * @param handle Pointer to flash handle
 * @param config Pointer to buffer for configuration data
 * @return eflash_result_t Operation result
 */
eflash_result_t eflash_get_info(eflash_handle_t *handle,
                                eflash_config_t *config);

/**
 * @brief Check if address range is valid
 * @param handle Pointer to flash handle
 * @param address Start address
 * @param size Size in bytes
 * @return bool true if valid, false otherwise
 */
bool eflash_is_address_valid(eflash_handle_t *handle, uint32_t address,
                             size_t size);

/**
 * @brief Get page index from address
 * @param handle Pointer to flash handle
 * @param address Address
 * @return uint32_t Page index
 */
uint32_t eflash_get_page_index(eflash_handle_t *handle, uint32_t address);

/**
 * @brief Check if page is erased
 * @param handle Pointer to flash handle
 * @param page_index Page index
 * @return bool true if erased, false otherwise
 */
bool eflash_is_page_erased(eflash_handle_t *handle, uint32_t page_index);

/* Helper macros */
#define EFLASH_ALIGN_UP(addr, align)   (((addr) + (align) - 1) & ~((align) - 1))
#define EFLASH_ALIGN_DOWN(addr, align) ((addr) & ~((align) - 1))
#define EFLASH_IS_ALIGNED(addr, align) (((addr) & ((align) - 1)) == 0)

#ifdef __cplusplus
}
#endif

#endif /* EFLASH_H */