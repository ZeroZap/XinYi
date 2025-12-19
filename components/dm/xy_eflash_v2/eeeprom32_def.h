#ifndef _EEEPROM32_DEF_H_
#define _EEEPROM32_DEF_H_
#include "eeeprom_cfg.h"

// 16Bit 32 Bit 都按照 32 Bit 处理，分 2次 写入 咯
#if (EEE_DATA_WIDTH == EEE_DATA_WIDTH_32BITS)
typedef union {
    struct {
        uint32_t status : 8;
        uint32_t cycle : 24;
    } bits;
    uint32_t data;
} eee_header_t;

#define EEE_MAX_CYCLE   0xFFFFFF
#define EEE_CYCLE_MASK  0xFFFFFF
#define EEE_CYCLE_SHIFT 0
#define EEE_ADDR_SIZE   (2)
#define EEE_ADDR_MAX    (4096)

#define HEADER_SIZE 4

// Add new macro for page status
#define PAGE_STATUS_VALID    0x00
#define PAGE_STATUS_TRANSFER 0x5A
#define PAGE_STATUS_ERASED   0xFF

#endif

#endif