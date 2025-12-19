#include <string.h>
#include <stdint.h>
#include "eep_cfg.h"
#include "eep_def.h"
#include "eep_port.h"
#include "eep.h"

#define EEP_MAX_CYCLE    0x00FFFFFF
#define EEP_CYCLE_MASK   0x00FFFFFF
#define EEP_CYCLE_SHIFT  (0)
#define EEP_ADDR_SIZE    (2)
#define EEP_RW_ONE_WORLD (1)

typedef struct {
    uint16_t addr;
    uint16_t value;
} eep_data_t;

typedef union {
    struct {
        uint8_t active_page;
        uint8_t next_page;
    } bits;
    uint8_t data;
} eep_page_t;

typedef struct {
    eep_page_t page;
    uint16_t write_index;
    uint16_t len;
    uint16_t remain_len;
    uint32_t _cycle;
    uint16_t *data;
} eep_cntlr_t;


eep_error_t eep_search_valid_page(void);
eep_error_t eep_search_valid_data(void);
eep_error_t eep_page_shift(void);

eep_cntlr_t _eep_cntlr   = { 0 };
eep_header_t _eep_header = { 0 };

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
 * @param len
 * @return eep_status_t
 */
eep_error_t eep_init(uint16_t *data, uint16_t len)
{
    _eep_cntlr.data = data;
    _eep_cntlr.len  = len;

    // 0. TODO
    eep_flash_init();

    // 1. find valid page
    if (eep_search_valid_page() != eep_error_ok) {
        return eep_error_no_valid_page;
    }
    // 2. get valid data
    if (eep_search_valid_data() != eep_error_ok) {
        return eep_error_no_valid_data;
    }

    // 3.
    return eep_error_ok;
}


eep_error_t eep_search_valid_page(void)
{
    int8_t i;
    int32_t ret;
    uint32_t max_cycle = 0;
    for (i = 0; i < EEP_PAGE_NUM; i++) {
        ret = eep_flash_read_words(EEP_FLASH_BASE + i * EEP_PAGE_SIZE,
                                   &_eep_header.data, EEP_RW_ONE_WORLD);
        if (ret == EEP_RW_ONE_WORLD) {
            // get valid header
            if ((_eep_header.bits.status == eep_page_status_valid)
                && (_eep_header.bits.cycle != EEP_CYCLE_MASK)) {
                if (_eep_header.bits.cycle > max_cycle) {
                    max_cycle                        = _eep_header.bits.cycle;
                    _eep_header.bits.cycle           = max_cycle;
                    _eep_cntlr.page.bits.active_page = i;
                }
            }
        }
    }

    /** there is no valid data */
    if (_eep_header.bits.cycle == 0) {
        return eep_error_no_valid_page;
    } else {
        return eep_error_ok;
    }
}

eep_error_t eep_search_valid_data(void)
{
    eep_data_t data;
    uint16_t i;
    for (i = 1; i < EEP_DATA_ADDR_MAX; i++) {
        if (eep_flash_read_words(
                EEP_FLASH_BASE
                    + _eep_cntlr.page.bits.active_page * EEP_PAGE_SIZE
                    + i * EEP_FLASH_WRITE_SIZE,
                (uint32_t *)&data, EEP_RW_ONE_WORLD)) {
            if (data.addr < _eep_cntlr.len) {
                _eep_cntlr.data[data.addr] = data.value;
            }
        }
    }
    return eep_error_ok;
}


int32_t eep_read_data(uint16_t addr, uint16_t *data, uint16_t len)
{
    int32_t ret = len;
    if (addr >= _eep_cntlr.len) {
        return eep_error_outoff_range;
    }

    memcpy(data, &_eep_cntlr.data[addr], len * sizeof(uint16_t));

    return ret;
}

int32_t eep_verify_write(uint32_t addr, const uint32_t *data, uint16_t len)
{
    int32_t ret   = len;
    uint8_t retry = 0;
    uint32_t verify_buf[EEP_FLASH_WORD_NUM];

    while (retry < EEP_WRITE_RETRIES) {
        if (eep_flash_write_words(addr, data, len) != ret) {
            retry++;
            continue;
        }

        if (eep_flash_read_words(addr, verify_buf, len) != ret) {
            retry++;
            continue;
        }

        if (memcmp(data, verify_buf, len * sizeof(uint32_t)) == 0) {
            return ret;
        }

        retry++;
    }

    return eep_error_write_timeout;
}

int32_t eep_write_data(uint16_t addr, uint16_t *data, uint16_t len)
{
    uint32_t write_addr;
    uint16_t i;
    int32_t ret;
    if (addr >= _eep_cntlr.len) {
        return eep_error_outoff_range;
    }

    // get current write index;
    write_addr = EEP_FLASH_BASE
                 + (_eep_cntlr.page.bits.active_page * EEP_PAGE_SIZE)
                 + _eep_cntlr.write_index * EEP_FLASH_WRITE_SIZE;

    for (i = 0; i < len; i++) {
        if (_eep_cntlr.write_index == EEP_DATA_ADDR_MAX) {
            eep_page_shift();
            write_addr = EEP_FLASH_BASE
                         + (_eep_cntlr.page.bits.active_page * EEP_PAGE_SIZE)
                         + _eep_cntlr.write_index * EEP_FLASH_WRITE_SIZE;
        }
        eep_verify_write(write_addr, &_eep_header.data, EEP_RW_ONE_WORLD);
        write_addr += 4;
        _eep_cntlr.write_index += 1;
    }


    return eep_error_ok;
}

eep_error_t eep_page_shift(void)
{
    uint32_t write_addr;
    uint32_t flash_data;
    uint16_t i;

    // get next page header data
    write_addr =
        EEP_FLASH_BASE + (_eep_cntlr.page.bits.next_page * EEP_PAGE_SIZE);

    eep_flash_read_words(write_addr, &_eep_header.data, EEP_RW_ONE_WORLD);

    // get header data check if need to erase
    if (_eep_header.bits.status != eep_page_status_erase
        || _eep_header.bits.cycle != EEP_CYCLE_MASK) {
        // erase page
        eep_flash_erase(write_addr, EEP_PAGE_SIZE);
    }

    _eep_header.bits.status = eep_page_status_transfer;

    eep_flash_write_words(write_addr, &_eep_header.data, EEP_RW_ONE_WORLD);


    write_addr += EEP_FLASH_WRITE_SIZE;

    // move data to new page
    for (i = 0; i < EEP_DATA_NUM; i++) {
        flash_data = (i << 16) | _eep_cntlr.data[i];
        eep_flash_write_words(
            write_addr + i * 4, &flash_data, EEP_RW_ONE_WORLD);
    }

    // set header.status = valid
    _eep_header.bits.status = eep_page_status_valid;
    _eep_cntlr._cycle += 1;
    _eep_header.bits.cycle = _eep_cntlr._cycle;
    eep_flash_write_words(
        write_addr, (uint32_t *)&_eep_header.data, EEP_RW_ONE_WORLD);

    // erase current page
    write_addr =
        EEP_FLASH_BASE + (_eep_cntlr.page.bits.active_page * EEP_PAGE_SIZE);
    eep_flash_erase(write_addr, EEP_PAGE_SIZE);

    // set header.status = erase
    _eep_cntlr.page.bits.active_page = _eep_cntlr.page.bits.next_page;
    _eep_cntlr.page.bits.next_page   = _eep_cntlr.page.bits.active_page + 1;
    if (_eep_cntlr.page.bits.next_page >= EEP_PAGE_NUM) {
        _eep_cntlr.page.bits.next_page = 0;
    }

    _eep_cntlr.write_index = 1;

    return eep_error_ok;
}

uint32_t eep_get_cycle(void)
{
    return _eep_cntlr._cycle;
}

eep_error_t eep_reset(void)
{
    _eep_cntlr._cycle = 0;
    return eep_error_ok;
}
