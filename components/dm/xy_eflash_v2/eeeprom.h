#ifndef _EEEPROM_H_
#define _EEEPROM_H_

typedef enum {
    eee_error_ok = 0,
    eee_error_unwritten,
    eee_error_over_data,
    eee_error_over_page,
    eee_error_index,
    eee_error_block_write,
    eee_error_read_timeout,
    eee_error_write_timeout,
    eee_error_over_cycle,
    eee_error_no_valid_page
} eee_error_t;

typedef struct {
    uint16_t offset;
    uint8_t *data;
} eee_control_t;

typedef struct {
    // device start address, total size, write size, page size, sector size
    int (*read)(uint32_t offset, uint32_t *data, uint16_t length);
    int (*write)(uint32_t offset, uint32_t *data, uint16_t length);
    int (*erase)(uint32_t offset, uint32_t *data, uint16_t length);
    int (*control)(void *data); // sleep, wake up....
}eee_device_t;

typedef struct {

    eee_device_t *device;
}eee_t;

/**
* limitation:
* data length must  be 2*n Bytes
*/
eee_status_t eee_init(eee_t eee, uint8_t *data, uint16_t length);
eee_status_t eee_read_data(eee_t eee, uint16_t offset, uint8_t *data, uint8_t len);
eee_status_t eee_write_data(eee_t eee, uint16_t offset, uint8_t *data, uint8_t len);
eee_status_t eee_reset(eee_t eee, uint8_t *data, uint16_t length);
#endif