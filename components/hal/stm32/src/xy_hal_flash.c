/**
 * @file xy_hal_flash.c
 * @brief STM32 Flash HAL Implementation
 * @version 1.0.0
 * @date 2026-03-02
 */

#include "xy_hal_flash.h"
#include "xy_log.h"

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

#ifdef MCU_STM32

#include "stm32_hal.h"

/**
 * @brief Flash 解锁
 */
xy_hal_error_t xy_hal_flash_unlock(void)
{
    /* STM32 HAL Flash Unlock */
    HAL_FLASH_Unlock();
    
    xy_log_d("Flash unlocked\n");
    return XY_HAL_OK;
}

/**
 * @brief Flash 锁定
 */
xy_hal_error_t xy_hal_flash_lock(void)
{
    /* STM32 HAL Flash Lock */
    HAL_FLASH_Lock();
    
    xy_log_d("Flash locked\n");
    return XY_HAL_OK;
}

/**
 * @brief Flash 擦除扇区
 */
xy_hal_error_t xy_hal_flash_erase(uint32_t addr, uint32_t size)
{
    FLASH_EraseInitTypeDef erase;
    uint32_t sector_error;
    uint32_t current_addr;
    uint32_t remaining;
    
    if (size == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* 解锁 Flash */
    xy_hal_flash_unlock();
    
    current_addr = addr;
    remaining = size;
    
    /* 配置擦除参数 */
    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;  /* 2.7V-3.6V */
    erase.NbSectors = 1;
    
    while (remaining > 0) {
        /* 计算扇区号 */
        uint32_t sector = FLASH_GetSector(current_addr);
        
        erase.Sector = sector;
        
        /* 执行擦除 */
        if (HAL_FLASHEx_Erase(&erase, &sector_error) != HAL_OK) {
            xy_log_e("Flash erase failed at 0x%08X\n", current_addr);
            xy_hal_flash_lock();
            return XY_HAL_ERROR;
        }
        
        xy_log_d("Flash sector %d erased at 0x%08X\n", sector, current_addr);
        
        current_addr += FLASH_SECTOR_SIZE;
        remaining -= FLASH_SECTOR_SIZE;
    }
    
    /* 锁定 Flash */
    xy_hal_flash_lock();
    
    return XY_HAL_OK;
}

/**
 * @brief Flash 编程 (写入)
 */
xy_hal_error_t xy_hal_flash_program(uint32_t addr, const uint8_t *data, uint32_t size)
{
    uint32_t i;
    uint32_t current_addr;
    
    if (!data || size == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* 检查地址对齐 */
    if (addr & 0x03) {
        xy_log_e("Flash write address not aligned (0x%08X)\n", addr);
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* 解锁 Flash */
    xy_hal_flash_unlock();
    
    current_addr = addr;
    
    /* 按字 (32 位) 写入 */
    for (i = 0; i < size; i += 4) {
        uint32_t word;
        
        /* 组合 32 位数据 (小端) */
        word = ((uint32_t)data[i]) |
               ((uint32_t)data[i + 1] << 8) |
               ((uint32_t)data[i + 2] << 16) |
               ((uint32_t)data[i + 3] << 24);
        
        /* STM32 HAL Flash Program */
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, current_addr, word) != HAL_OK) {
            xy_log_e("Flash program failed at 0x%08X\n", current_addr);
            xy_hal_flash_lock();
            return XY_HAL_ERROR;
        }
        
        /* 验证写入 */
        if (*(volatile uint32_t*)current_addr != word) {
            xy_log_e("Flash verify failed at 0x%08X\n", current_addr);
            xy_hal_flash_lock();
            return XY_HAL_ERROR;
        }
        
        current_addr += 4;
    }
    
    /* 锁定 Flash */
    xy_hal_flash_lock();
    
    xy_log_d("Flash programmed %d bytes at 0x%08X\n", size, addr);
    return XY_HAL_OK;
}

/**
 * @brief Flash 读取
 */
xy_hal_error_t xy_hal_flash_read(uint32_t addr, uint8_t *data, uint32_t size)
{
    if (!data || size == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* 直接内存拷贝 (Flash 映射到内存空间) */
    memcpy(data, (const void *)addr, size);
    
    return XY_HAL_OK;
}

#else

xy_hal_error_t xy_hal_flash_unlock(void)
{
    return XY_HAL_ERROR_NOT_SUPPORT;
}

xy_hal_error_t xy_hal_flash_lock(void)
{
    return XY_HAL_ERROR_NOT_SUPPORT;
}

#endif
