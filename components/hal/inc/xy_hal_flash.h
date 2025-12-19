#ifndef _XY_HAL_FLASH_H_
#define _XY_HAL_FLASH_H_

int32_t xy_hal_flash_init(void *flash);
int32_t xy_hal_flash_read(void *flash, uint32_t addr, uint32_t *data,
                          uint32_t size);
int32_t xy_hal_flash_write(void *flash, uint32_t addr, uint32_t *data,
                           uint32_t size);
int32_t xy_hal_flash_erase(void *flash, uint32_t addr, uint32_t size);

int32_t xy_hal_flash_deinit(void *flash);

int32_t xy_hal_flash_lock(void *flash, uint32_t addr, uint32_t size);
int32_t xy_hal_flash_unlock(void *flash, uint32_t addr, uint32_t size);

int32_t xy_hal_flash_set_read_protect(void *flash, uint8_t protect_level);
int32_t xy_hal_flash_get_read_protect(void *flash, uint8_t *protect_level);

#endif
