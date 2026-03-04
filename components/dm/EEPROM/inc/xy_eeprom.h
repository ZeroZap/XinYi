/**
 * @file xy_eeprom.h
 * @brief Unified EEPROM Interface - Based on FEE
 * @version 1.0.0
 * @date 2026-03-05
 */

#ifndef XY_EEPROM_H
#define XY_EEPROM_H

#include <stdint.h>
#include <stdbool.h>
#include "xy_fee.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 配置 ==================== */

#ifndef XY_EEPROM_MAX_SIZE
#define XY_EEPROM_MAX_SIZE      4096    // 最大 EEPROM 大小
#endif

/* ==================== 数据结构 ==================== */

/**
 * @brief EEPROM 状态码
 */
typedef enum {
    XY_EEPROM_OK = 0,
    XY_EEPROM_ERROR,
    XY_EEPROM_ERROR_PARAM,
    XY_EEPROM_ERROR_NOT_INIT,
    XY_EEPROM_ERROR_FULL,
    XY_EEPROM_ERROR_CRC,
} xy_eeprom_status_t;

/**
 * @brief EEPROM 句柄
 */
typedef struct {
    xy_fee_t *fee;            // 底层 FEE
    uint8_t *cache;           // EEPROM 缓存
    uint16_t size;            // EEPROM 大小
    uint16_t used;            // 已使用大小
    bool initialized;         // 初始化标志
} xy_eeprom_t;

/**
 * @brief EEPROM 配置
 */
typedef struct {
    xy_fee_t *fee;            // FEE 句柄
    uint8_t *cache;           // 缓存缓冲区
    uint16_t size;            // EEPROM 大小
} xy_eeprom_config_t;

/* ==================== API ==================== */

/**
 * @brief 初始化 EEPROM
 * @param eep EEPROM 句柄
 * @param cfg 配置参数
 * @return 状态码
 */
xy_eeprom_status_t xy_eeprom_init(xy_eeprom_t *eep, const xy_eeprom_config_t *cfg);

/**
 * @brief 反初始化 EEPROM
 * @param eep EEPROM 句柄
 * @return 状态码
 */
xy_eeprom_status_t xy_eeprom_deinit(xy_eeprom_t *eep);

/**
 * @brief 读取 EEPROM
 * @param eep EEPROM 句柄
 * @param addr 地址
 * @param data 数据缓冲区
 * @param len 读取长度
 * @return 状态码
 */
xy_eeprom_status_t xy_eeprom_read(xy_eeprom_t *eep, uint16_t addr, 
                                   uint8_t *data, uint16_t len);

/**
 * @brief 写入 EEPROM
 * @param eep EEPROM 句柄
 * @param addr 地址
 * @param data 数据指针
 * @param len 写入长度
 * @return 状态码
 */
xy_eeprom_status_t xy_eeprom_write(xy_eeprom_t *eep, uint16_t addr, 
                                    const uint8_t *data, uint16_t len);

/**
 * @brief 块读取 (优化性能)
 * @param eep EEPROM 句柄
 * @param addr 地址
 * @param data 数据缓冲区
 * @param len 读取长度
 * @return 状态码
 */
xy_eeprom_status_t xy_eeprom_read_block(xy_eeprom_t *eep, uint16_t addr,
                                         uint8_t *data, uint16_t len);

/**
 * @brief 块写入 (优化性能)
 * @param eep EEPROM 句柄
 * @param addr 地址
 * @param data 数据指针
 * @param len 写入长度
 * @return 状态码
 */
xy_eeprom_status_t xy_eeprom_write_block(xy_eeprom_t *eep, uint16_t addr,
                                          const uint8_t *data, uint16_t len);

/**
 * @brief 获取已使用大小
 * @param eep EEPROM 句柄
 * @return 已使用大小
 */
uint16_t xy_eeprom_get_used(xy_eeprom_t *eep);

/**
 * @brief 获取剩余大小
 * @param eep EEPROM 句柄
 * @return 剩余大小
 */
uint16_t xy_eeprom_get_free(xy_eeprom_t *eep);

/**
 * @brief 格式化 EEPROM
 * @param eep EEPROM 句柄
 * @return 状态码
 */
xy_eeprom_status_t xy_eeprom_format(xy_eeprom_t *eep);

/**
 * @brief 获取 EEPROM 信息
 * @param eep EEPROM 句柄
 * @param total 总大小
 * @param used 已使用大小
 * @param free 剩余大小
 */
void xy_eeprom_get_info(xy_eeprom_t *eep, uint16_t *total, 
                        uint16_t *used, uint16_t *free);

/* ==================== 便捷 API ==================== */

/**
 * @brief 读取 8 位数据
 */
static inline xy_eeprom_status_t xy_eeprom_read_u8(xy_eeprom_t *eep, 
                                                    uint16_t addr, uint8_t *val)
{
    return xy_eeprom_read(eep, addr, val, 1);
}

/**
 * @brief 写入 8 位数据
 */
static inline xy_eeprom_status_t xy_eeprom_write_u8(xy_eeprom_t *eep,
                                                     uint16_t addr, uint8_t val)
{
    return xy_eeprom_write(eep, addr, &val, 1);
}

/**
 * @brief 读取 16 位数据
 */
static inline xy_eeprom_status_t xy_eeprom_read_u16(xy_eeprom_t *eep,
                                                     uint16_t addr, uint16_t *val)
{
    return xy_eeprom_read(eep, addr, (uint8_t *)val, 2);
}

/**
 * @brief 写入 16 位数据
 */
static inline xy_eeprom_status_t xy_eeprom_write_u16(xy_eeprom_t *eep,
                                                      uint16_t addr, uint16_t val)
{
    return xy_eeprom_write(eep, addr, (uint8_t *)&val, 2);
}

/**
 * @brief 读取 32 位数据
 */
static inline xy_eeprom_status_t xy_eeprom_read_u32(xy_eeprom_t *eep,
                                                     uint16_t addr, uint32_t *val)
{
    return xy_eeprom_read(eep, addr, (uint8_t *)val, 4);
}

/**
 * @brief 写入 32 位数据
 */
static inline xy_eeprom_status_t xy_eeprom_write_u32(xy_eeprom_t *eep,
                                                      uint16_t addr, uint32_t val)
{
    return xy_eeprom_write(eep, addr, (uint8_t *)&val, 4);
}

#ifdef __cplusplus
}
#endif

#endif /* XY_EEPROM_H */
