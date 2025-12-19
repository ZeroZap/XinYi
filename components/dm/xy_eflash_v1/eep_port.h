#ifndef _EEP_PORT_H_
#define _EEP_PORT_H_
#include <stdint.h>

// Update function prototypes with size parameters
int32_t eep_flash_read_words(uint32_t addr, uint32_t *data, uint16_t len);
int32_t eep_flash_write_words(uint32_t addr, const uint32_t *data,
                              uint16_t len);
int32_t eep_flash_erase(uint32_t addr, uint32_t len);

int32_t eep_flash_init(void);

void eep_log(const char *fmt, ...);
#endif
