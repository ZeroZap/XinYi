#ifndef _EEP_H_
#define _EEP_H_
/**
 * limitation:
 * data len must  be 2*n Bytes
 */
eep_error_t eep_init(uint16_t *data, uint16_t len);
int32_t eep_read_data(uint16_t offset, uint16_t *data, uint16_t len);
int32_t eep_write_data(uint16_t offset, uint16_t *data, uint16_t len);
uint32_t eep_get_cycle(void);
eep_error_t eep_reset(void);
#endif
