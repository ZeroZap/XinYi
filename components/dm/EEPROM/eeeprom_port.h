#ifndef _EEEPROM_PORT_H_
#define _EEEPROM_PORT_H_
#define BLOCK_ERASE_ENABLE 0

// Add configuration check
#if (EEE_FLASH_WRITE_SIZE % 4) != 0
#error "Flash write size must be multiple of 4 bytes"
#endif

// Update function prototypes with size parameters
int eee_flash_read(uint32_t addr, uint32_t *data);
int eee_flash_write(uint32_t addr, const uint32_t *data);
int eee_flash_erase(uint32_t addr, uint32_t size);

// Add flash property queries
uint32_t eee_flash_get_write_size(void);
uint32_t eee_flash_get_erase_size(void);

#endif