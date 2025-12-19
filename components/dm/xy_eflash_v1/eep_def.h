#ifndef _EEP_DEF_H_
#define _EEP_DEF_H_

typedef enum {
    eep_page_status_valid    = 0x0A,
    eep_page_status_transfer = 0x5A,
    eep_page_status_erase    = 0xFF,
} eep_page_status_t;

typedef enum {
    eep_error_unkkonw = -1000,
    eep_error_unwritten,
    eep_error_over_data,
    eep_error_over_page,
    eep_error_addr,
    eep_error_block_write,
    eep_error_read_timeout,
    eep_error_outoff_range,
    eep_error_write_timeout,
    eep_error_page_no_data,
    eep_error_over_cycle,
    eep_error_index, // Add error code for index
    eep_error_no_valid_data,
    eep_error_no_valid_page = -1,
    eep_error_ok            = 0,
} eep_error_t;


typedef union {
    struct {
        uint32_t status : 8;
        uint32_t cycle : 24;
    } bits;
    uint32_t data;
} eep_header_t;

#define EEP_WRITE_RETRY_CNT 5

#endif
