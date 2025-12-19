#include "eeeprom_cfg.h"
#include "eeeprom_port.h"
#include "eeeprom.h"
#include "eeeprom_helper.h"


#define PAGE_ACTIVE
#define PAGE_UNUSED   0xff
#define PAGE_TRANSFER 0x5a
#define PAGE_ACTIVE   0x00


// 当写入的和读取的不一致时，切换到下一个地址写
#define EEE_WRITE_RETRY_CNT 5


#define EEE_ADDR_MASK
#define EEE_ADDR_SHIFT
#define EEE_DATA_MASK
#define EEE_DATA_SHIFT

uint16_t eee_get_cycle(void);
// Add function prototypes
eee_error_t eee_search_valid_page(uint8_t *valid_page);
eee_error_t eee_page_shift(void);
// t同步写入，当写入数据过大时
eee_error_t eee_sync(ui)


uint8_t _active_page    = 0;
uint16_t _active_cursor = 0;
uint16_t *eee_data      = 0;
uint16_t _cycle         = 0;
uint16_t _len_max       = 0;
uint8_t eee_init_flag   = 0;

/**
 * @brief
 * -----------------------------------------------------------------------------
 *                \   Page1 valid    |    Page 1 transfer   |   Page 1 Erased  |
 * ----------------------------------------------------------------------------|
 *                |   Erase Page0
 * Page0 valid    |   Page0 valid
 *                |   Erase Page1
 *  ----------------------------------------------------------------------------
 *                |
 * Page0 transfer |
 *                |
 * -----------------------------------------------------------------------------
 *                |
 * Page0  Erased  |
 *                |
 * -----------------------------------------------------------------------------
 *
 * @param data
 * @param length
 * @return eee_status_t
 */
eee_error_t eee_init(uint8_t *data, uint16_t length)
{
    eee_error_t ret = eee_error_ok;

    // eee_search_valid_page()
    // eee_search_valid_data()
    // eee_search_valid_cursor()
    // if (invalid_page) {eee_reset()}

    return ret;
}

eee_error_t eee_read_data(uint16_t index, uint8_t *data, uint8_t len)
{
    if (index >= _len_max) {
        return eee_error_index;
    }

    return eee_error_ok;
}

static eee_error_t eee_verify_write(uint32_t addr, const uint8_t *data,
                                    uint16_t len)
{
    uint8_t retry = 0;
    uint8_t verify_buf[EEE_FLASH_WRITE_SIZE];

    while (retry < EEE_WRITE_RETRIES) {
        if (eee_flash_write(addr, (uint32_t *)data) != 0) {
            retry++;
            continue;
        }

        if (eee_flash_read(addr, (uint32_t *)verify_buf) != 0) {
            retry++;
            continue;
        }

        if (memcmp(data, verify_buf, len) == 0) {
            return eee_error_ok;
        }

        retry++;
    }

    return eee_error_write_timeout;
}

eee_error_t eee_write_data(uint16_t index, uint8_t *data, uint8_t len)
{
    if (index >= _len_max) {
        return eee_error_index;
    }

    uint32_t write_addr = EEE_FLASH_BASE + (_active_page * EEE_PAGE_SIZE)
                          + HEADER_SIZE + (index * EEE_DATA_ALIGNMENT);

    // Align data to flash write size
    uint8_t aligned_buf[EEE_FLASH_WRITE_SIZE];
    uint16_t remaining = len;
    uint16_t offset    = 0;

    while (remaining > 0) {
        uint16_t write_size = MIN(remaining, EEE_FLASH_WRITE_SIZE);
        memset(aligned_buf, 0xFF, EEE_FLASH_WRITE_SIZE);
        memcpy(aligned_buf, data + offset, write_size);

        eee_error_t ret = eee_verify_write(
            write_addr + offset, aligned_buf, EEE_FLASH_WRITE_SIZE);
        if (ret != eee_error_ok) {
            return ret;
        }

        remaining -= write_size;
        offset += write_size;
    }

    return eee_error_ok;
}

eee_error_t eee_search_valid_page(uint8_t *valid_page)
{
    int8_t i;
    eee_error_t ret;
    uint32_t header;
    for (i = 0; i < EEE_PAGE_NUM; i++) {
        ret = eee_flash_read_word(EEE_FLASH_BASE + i * EEE_PAGE_SIZE, &header);
        if (ret == eee_error_ok) {
            // get valid header
            if (header) {
                *valid_page = i;
            }
        }
    }

    /** there is no valid data */
    if (i == EEE_PAGE_NUM) {
        return eee_error_no_valid_page;
    } else {
        return eee_error_ok;
    }
}

eee_error_t eee_page_shift(void)
{
    uint8_t i, new_page;
    uint8_t page_status = PAGE_TRANSFER;
    uint8_t check_sum   = 28;

    if (_active_page + 1 == EEE_PAGE_NUM) {
        new_page = 0;
    } else {
        new_page = _active_page + 1;
    }

    /** check next page header is unused.. or erase status */

    /** Page header update */
    eee_flash_write_word(EEE_FLASH_BASE + new_page * EEE_PAGE_SIZE,
                         (page_status << 24) | _cycle);

    /** Copy the rest of data. */
    for (i = HEADER_SIZE; i < _len_max; i += 2) {
        eee_flash_write_word(
            EEE_FLASH_BASE + new_page * EEE_PAGE_SIZE + i, data[i - 2]);
    }

    /** Set cursor to new page */

    /** If there is one valid data left, write to flash. */

    /** Erase the old page. */

    /** Point to the new valid page. */

    return eee_error_ok;
}

uint16_t eee_get_cycle(void)
{
    return _cycle;
}

eee_error_t eee_reset(void)
{
    _cycle         = 0;
    _active_cursor = 0;
    _active_page   = 0;
    return eee_error_ok;
}