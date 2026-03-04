/**
 * @file xy_eeprom.c
 * @brief Unified EEPROM Interface Implementation
 * @version 1.0.0
 * @date 2026-03-05
 */

#include "xy_eeprom.h"
#include <string.h>

/**
 * @brief 初始化 EEPROM
 */
xy_eeprom_status_t xy_eeprom_init(xy_eeprom_t *eep, const xy_eeprom_config_t *cfg)
{
    if (!eep || !cfg || !cfg->fee || !cfg->cache || cfg->size == 0) {
        return XY_EEPROM_ERROR_PARAM;
    }
    
    memset(eep, 0, sizeof(*eep));
    
    eep->fee = cfg->fee;
    eep->cache = cfg->cache;
    eep->size = cfg->size;
    eep->used = 0;
    eep->initialized = true;
    
    // 从 FEE 恢复数据到缓存
    xy_fee_read(eep->fee, 0, eep->cache, eep->size);
    
    return XY_EEPROM_OK;
}

/**
 * @brief 反初始化 EEPROM
 */
xy_eeprom_status_t xy_eeprom_deinit(xy_eeprom_t *eep)
{
    if (!eep || !eep->initialized) {
        return XY_EEPROM_ERROR_NOT_INIT;
    }
    
    eep->initialized = false;
    return XY_EEPROM_OK;
}

/**
 * @brief 读取 EEPROM
 */
xy_eeprom_status_t xy_eeprom_read(xy_eeprom_t *eep, uint16_t addr, 
                                   uint8_t *data, uint16_t len)
{
    if (!eep || !eep->initialized || !data) {
        return XY_EEPROM_ERROR_NOT_INIT;
    }
    
    if (addr + len > eep->size) {
        return XY_EEPROM_ERROR_PARAM;
    }
    
    // 直接从缓存读取
    memcpy(data, eep->cache + addr, len);
    
    return XY_EEPROM_OK;
}

/**
 * @brief 写入 EEPROM
 */
xy_eeprom_status_t xy_eeprom_write(xy_eeprom_t *eep, uint16_t addr, 
                                    const uint8_t *data, uint16_t len)
{
    xy_eeprom_status_t status;
    
    if (!eep || !eep->initialized || !data) {
        return XY_EEPROM_ERROR_NOT_INIT;
    }
    
    if (addr + len > eep->size) {
        return XY_EEPROM_ERROR_PARAM;
    }
    
    // 更新缓存
    memcpy(eep->cache + addr, data, len);
    
    // 写入 FEE
    status = xy_fee_write(eep->fee, addr, eep->cache + addr, len);
    if (status != FEE_OK) {
        return XY_EEPROM_ERROR;
    }
    
    // 更新使用统计
    if (addr + len > eep->used) {
        eep->used = addr + len;
    }
    
    return XY_EEPROM_OK;
}

/**
 * @brief 块读取 (优化性能)
 */
xy_eeprom_status_t xy_eeprom_read_block(xy_eeprom_t *eep, uint16_t addr,
                                         uint8_t *data, uint16_t len)
{
    // 块读取直接使用缓存，性能最优
    return xy_eeprom_read(eep, addr, data, len);
}

/**
 * @brief 块写入 (优化性能)
 */
xy_eeprom_status_t xy_eeprom_write_block(xy_eeprom_t *eep, uint16_t addr,
                                          const uint8_t *data, uint16_t len)
{
    // 块写入：先更新缓存，再批量写入 FEE
    xy_eeprom_status_t status;
    
    if (!eep || !eep->initialized || !data) {
        return XY_EEPROM_ERROR_NOT_INIT;
    }
    
    if (addr + len > eep->size) {
        return XY_EEPROM_ERROR_PARAM;
    }
    
    // 更新缓存
    memcpy(eep->cache + addr, data, len);
    
    // 批量写入 FEE (优化：减少写入次数)
    status = xy_fee_write(eep->fee, addr, eep->cache + addr, len);
    if (status != FEE_OK) {
        return XY_EEPROM_ERROR;
    }
    
    // 更新使用统计
    if (addr + len > eep->used) {
        eep->used = addr + len;
    }
    
    return XY_EEPROM_OK;
}

/**
 * @brief 获取已使用大小
 */
uint16_t xy_eeprom_get_used(xy_eeprom_t *eep)
{
    if (!eep || !eep->initialized) {
        return 0;
    }
    return eep->used;
}

/**
 * @brief 获取剩余大小
 */
uint16_t xy_eeprom_get_free(xy_eeprom_t *eep)
{
    if (!eep || !eep->initialized) {
        return 0;
    }
    return eep->size - eep->used;
}

/**
 * @brief 格式化 EEPROM
 */
xy_eeprom_status_t xy_eeprom_format(xy_eeprom_t *eep)
{
    if (!eep || !eep->initialized) {
        return XY_EEPROM_ERROR_NOT_INIT;
    }
    
    // 清空缓存
    memset(eep->cache, 0xFF, eep->size);
    
    // 格式化底层 FEE
    xy_fee_status_t status = xy_fee_format(eep->fee);
    if (status != FEE_OK) {
        return XY_EEPROM_ERROR;
    }
    
    eep->used = 0;
    
    return XY_EEPROM_OK;
}

/**
 * @brief 获取 EEPROM 信息
 */
void xy_eeprom_get_info(xy_eeprom_t *eep, uint16_t *total, 
                        uint16_t *used, uint16_t *free)
{
    if (!eep || !eep->initialized) {
        return;
    }
    
    if (total) *total = eep->size;
    if (used) *used = eep->used;
    if (free) *free = eep->size - eep->used;
}
