/**
 * @file xy_w25qxx.h
 * @brief W25Qxx SPI Flash Memory Driver
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 支持 W25Q16/W25Q32/W25Q64/W25Q128
 */

#ifndef XY_W25QXX_H
#define XY_W25QXX_H

#include "xy_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== W25Qxx Device Structure ==================== */

/**
 * @brief W25Qxx 设备结构
 */
typedef struct {
    xy_device_t base;              /**< 设备基类 */
    void *spi_handle;              /**< SPI 句柄 */
    void *cs_pin;                  /**< 片选引脚 */
    uint32_t capacity_bytes;       /**< 容量 (字节) */
    uint32_t sector_size;          /**< 扇区大小 (字节) */
    uint32_t block_size;           /**< 块大小 (字节) */
    uint16_t page_size;            /**< 页大小 (字节) */
    uint8_t device_id;             /**< 设备 ID */
    bool initialized;              /**< 是否已初始化 */
} xy_w25qxx_t;

/* ==================== W25Qxx Commands ==================== */

#define W25QXX_CMD_WRITE_ENABLE      0x06
#define W25QXX_CMD_WRITE_DISABLE     0x04
#define W25QXX_CMD_READ_STATUS1      0x05
#define W25QXX_CMD_READ_STATUS2      0x35
#define W25QXX_CMD_WRITE_STATUS      0x01
#define W25QXX_CMD_PAGE_PROGRAM      0x02
#define W25QXX_CMD_SECTOR_ERASE      0x20
#define W25QXX_CMD_BLOCK_ERASE       0xD8
#define W25QXX_CMD_CHIP_ERASE        0xC7
#define W25QXX_CMD_READ_DATA         0x03
#define W25QXX_CMD_FAST_READ         0x0B
#define W25QXX_CMD_FAST_READ_DUAL    0x3B
#define W25QXX_CMD_FAST_READ_QUAD    0x6B
#define W25QXX_CMD_POWER_DOWN        0xB9
#define W25QXX_CMD_RELEASE_POWER_DOWN 0xAB
#define W25QXX_CMD_DEVICE_ID         0x90
#define W25QXX_CMD_JEDEC_ID          0x9F

/* ==================== W25Qxx Capacity ==================== */

typedef enum {
    XY_W25Q16 = 16,                  /**< W25Q16 (2MB) */
    XY_W25Q32 = 32,                  /**< W25Q32 (4MB) */
    XY_W25Q64 = 64,                  /**< W25Q64 (8MB) */
    XY_W25Q128 = 128,                /**< W25Q128 (16MB) */
} xy_w25qxx_capacity_t;

/* ==================== W25Qxx API ==================== */

/**
 * @brief 初始化 W25Qxx
 * @param dev W25Qxx 设备句柄
 * @param spi_handle SPI 句柄
 * @param cs_pin 片选引脚
 * @param capacity 容量
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_w25qxx_init(xy_w25qxx_t *dev, void *spi_handle, 
                   void *cs_pin, xy_w25qxx_capacity_t capacity);

/**
 * @brief 反初始化 W25Qxx
 * @param dev W25Qxx 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_w25qxx_deinit(xy_w25qxx_t *dev);

/**
 * @brief 读取设备 ID
 * @param dev W25Qxx 设备句柄
 * @param manufacturer_id 制造商 ID (输出)
 * @param device_id 设备 ID (输出)
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_w25qxx_read_device_id(xy_w25qxx_t *dev, uint8_t *manufacturer_id,
                             uint8_t *device_id);

/**
 * @brief 读取 JEDEC ID
 * @param dev W25Qxx 设备句柄
 * @param jedec_id JEDEC ID (3 字节)
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_w25qxx_read_jedec_id(xy_w25qxx_t *dev, uint8_t *jedec_id);

/**
 * @brief 读取状态寄存器 1
 * @param dev W25Qxx 设备句柄
 * @param status 状态值 (输出)
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_w25qxx_read_status1(xy_w25qxx_t *dev, uint8_t *status);

/**
 * @brief 等待操作完成
 * @param dev W25Qxx 设备句柄
 * @param timeout_ms 超时时间
 * @return XY_DEVICE_OK 成功，XY_DEVICE_TIMEOUT 超时
 */
int xy_w25qxx_wait_ready(xy_w25qxx_t *dev, uint32_t timeout_ms);

/**
 * @brief 读取数据
 * @param dev W25Qxx 设备句柄
 * @param addr 地址
 * @param buffer 数据缓冲区
 * @param length 数据长度
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_w25qxx_read(xy_w25qxx_t *dev, uint32_t addr, uint8_t *buffer, 
                   size_t length);

/**
 * @brief 写入数据 (页编程)
 * @param dev W25Qxx 设备句柄
 * @param addr 地址
 * @param buffer 数据缓冲区
 * @param length 数据长度
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_w25qxx_write_page(xy_w25qxx_t *dev, uint32_t addr, const uint8_t *buffer,
                         size_t length);

/**
 * @brief 写入数据 (自动页分割)
 * @param dev W25Qxx 设备句柄
 * @param addr 地址
 * @param buffer 数据缓冲区
 * @param length 数据长度
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_w25qxx_write(xy_w25qxx_t *dev, uint32_t addr, const uint8_t *buffer,
                    size_t length);

/**
 * @brief 擦除扇区 (4KB)
 * @param dev W25Qxx 设备句柄
 * @param addr 扇区地址
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_w25qxx_erase_sector(xy_w25qxx_t *dev, uint32_t addr);

/**
 * @brief 擦除块 (64KB)
 * @param dev W25Qxx 设备句柄
 * @param addr 块地址
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_w25qxx_erase_block(xy_w25qxx_t *dev, uint32_t addr);

/**
 * @brief 全片擦除
 * @param dev W25Qxx 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_w25qxx_erase_chip(xy_w25qxx_t *dev);

/**
 * @brief 进入掉电模式
 * @param dev W25Qxx 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_w25qxx_power_down(xy_w25qxx_t *dev);

/**
 * @brief 退出掉电模式
 * @param dev W25Qxx 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_w25qxx_release_power_down(xy_w25qxx_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* XY_W25QXX_H */
