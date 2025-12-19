#ifndef _EEEPROM32_DEF_H_
#define _EEEPROM32_DEF_H_
#include "eeeprom_cfg.h"

typedef enum {
    eee_page_status_valid    = 0x00,
    eee_page_status_transfer = 0x01,
    eee_page_status_erase    = 0x03,
} eee_page_status_t;

typedef enum {
    eee_error_ok = 0,
    eee_error_index,
    eee_error_write,
    eee_error_read,
    eee_error_timeout,

} eee_error_t;

typedef struct {
    uint32_t flash_base_addr;
    uint16_t sector_size;

} eee_flash_cntlr_t;

#endif