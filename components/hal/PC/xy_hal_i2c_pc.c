/**
 * @file xy_hal_i2c_pc.c
 * @brief PC/Linux simulation layer for I2C HAL
 */

#include "xy_hal_i2c.h"
#include <stdio.h>
#include <string.h>

xy_hal_error_t xy_hal_i2c_master_transmit(void *hi2c, uint16_t dev_addr, 
                                           const uint8_t *data, uint16_t size, 
                                           uint32_t timeout)
{
    (void)hi2c;
    (void)dev_addr;
    (void)data;
    (void)size;
    (void)timeout;
    
    // Simulation: always success
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_master_receive(void *hi2c, uint16_t dev_addr, 
                                          uint8_t *data, uint16_t size, 
                                          uint32_t timeout)
{
    (void)hi2c;
    (void)dev_addr;
    (void)timeout;
    
    // Simulation: zero-fill received data
    if (data && size > 0) {
        memset(data, 0, size);
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_mem_write(void *hi2c, uint16_t dev_addr, 
                                     uint16_t mem_addr, uint16_t mem_add_size,
                                     const uint8_t *data, uint16_t size, 
                                     uint32_t timeout)
{
    (void)hi2c;
    (void)dev_addr;
    (void)mem_addr;
    (void)mem_add_size;
    (void)data;
    (void)size;
    (void)timeout;
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_mem_read(void *hi2c, uint16_t dev_addr, 
                                    uint16_t mem_addr, uint16_t mem_add_size,
                                    uint8_t *data, uint16_t size, 
                                    uint32_t timeout)
{
    (void)hi2c;
    (void)dev_addr;
    (void)mem_addr;
    (void)mem_add_size;
    (void)timeout;
    
    if (data && size > 0) {
        memset(data, 0, size);
    }
    return XY_HAL_OK;
}
