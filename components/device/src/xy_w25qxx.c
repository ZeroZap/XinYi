/**
 * @file xy_w25qxx.c
 * @brief W25Qxx SPI Flash Memory Driver Implementation
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 支持 W25Q16/W25Q32/W25Q64/W25Q128
 */

#include "xy_w25qxx.h"
#include <string.h>

/* ==================== Private Functions ==================== */

/**
 * @brief 片选使能
 */
static void cs_enable(xy_w25qxx_t *dev)
{
    /* TODO: 拉低片选引脚 */
    (void)dev;
}

/**
 * @brief 片选禁用
 */
static void cs_disable(xy_w25qxx_t *dev)
{
    /* TODO: 拉高片选引脚 */
    (void)dev;
}

/**
 * @brief SPI 发送/接收
 */
static int spi_transfer(xy_w25qxx_t *dev, const uint8_t *tx, uint8_t *rx, 
                        size_t length)
{
    /* TODO: 实现 SPI 传输 */
    (void)dev;
    (void)tx;
    (void)rx;
    (void)length;
    return 0;
}

/**
 * @brief 发送命令
 */
static int send_command(xy_w25qxx_t *dev, uint8_t cmd, const uint8_t *data, 
                        size_t data_len)
{
    cs_enable(dev);
    
    /* 发送命令 */
    spi_transfer(dev, &cmd, NULL, 1);
    
    /* 发送数据 */
    if (data && data_len > 0) {
        spi_transfer(dev, data, NULL, data_len);
    }
    
    cs_disable(dev);
    
    return XY_DEVICE_OK;
}

/**
 * @brief 发送命令并读取数据
 */
static int send_command_read(xy_w25qxx_t *dev, uint8_t cmd, uint8_t *rx, 
                             size_t rx_len)
{
    cs_enable(dev);
    
    /* 发送命令 */
    spi_transfer(dev, &cmd, NULL, 1);
    
    /* 读取数据 */
    if (rx && rx_len > 0) {
        uint8_t dummy = 0xFF;
        spi_transfer(dev, &dummy, rx, rx_len);
    }
    
    cs_disable(dev);
    
    return XY_DEVICE_OK;
}

/* ==================== Public Implementation ==================== */

int xy_w25qxx_init(xy_w25qxx_t *dev, void *spi_handle, 
                   void *cs_pin, xy_w25qxx_capacity_t capacity)
{
    if (!dev || !spi_handle || !cs_pin) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    memset(dev, 0, sizeof(*dev));
    
    dev->spi_handle = spi_handle;
    dev->cs_pin = cs_pin;
    
    /* 根据容量设置参数 */
    switch (capacity) {
        case XY_W25Q16:
            dev->capacity_bytes = 2 * 1024 * 1024; /* 2MB */
            dev->sector_size = 4096; /* 4KB */
            dev->block_size = 64 * 1024; /* 64KB */
            break;
        case XY_W25Q32:
            dev->capacity_bytes = 4 * 1024 * 1024; /* 4MB */
            dev->sector_size = 4096;
            dev->block_size = 64 * 1024;
            break;
        case XY_W25Q64:
            dev->capacity_bytes = 8 * 1024 * 1024; /* 8MB */
            dev->sector_size = 4096;
            dev->block_size = 64 * 1024;
            break;
        case XY_W25Q128:
            dev->capacity_bytes = 16 * 1024 * 1024; /* 16MB */
            dev->sector_size = 4096;
            dev->block_size = 64 * 1024;
            break;
        default:
            return XY_DEVICE_INVALID_PARAM;
    }
    
    dev->page_size = 256; /* 所有型号页大小都是 256 字节 */
    
    /* 读取设备 ID 验证 */
    uint8_t manufacturer_id, device_id;
    int ret = xy_w25qxx_read_device_id(dev, &manufacturer_id, &device_id);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    dev->device_id = device_id;
    dev->initialized = true;
    
    return XY_DEVICE_OK;
}

int xy_w25qxx_deinit(xy_w25qxx_t *dev)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    dev->initialized = false;
    
    return XY_DEVICE_OK;
}

int xy_w25qxx_read_device_id(xy_w25qxx_t *dev, uint8_t *manufacturer_id,
                             uint8_t *device_id)
{
    if (!dev || !manufacturer_id || !device_id) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    uint8_t cmd[4] = {W25QXX_CMD_DEVICE_ID, 0, 0, 0};
    
    cs_enable(dev);
    spi_transfer(dev, cmd, NULL, 4);
    
    uint8_t rx[2];
    spi_transfer(dev, NULL, rx, 2);
    cs_disable(dev);
    
    *manufacturer_id = rx[0];
    *device_id = rx[1];
    
    return XY_DEVICE_OK;
}

int xy_w25qxx_read_jedec_id(xy_w25qxx_t *dev, uint8_t *jedec_id)
{
    if (!dev || !jedec_id) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    return send_command_read(dev, W25QXX_CMD_JEDEC_ID, jedec_id, 3);
}

int xy_w25qxx_read_status1(xy_w25qxx_t *dev, uint8_t *status)
{
    if (!dev || !status) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    return send_command_read(dev, W25QXX_CMD_READ_STATUS1, status, 1);
}

int xy_w25qxx_wait_ready(xy_w25qxx_t *dev, uint32_t timeout_ms)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    uint32_t start = 0; /* TODO: 获取开始时间 */
    
    while (1) {
        uint8_t status;
        int ret = xy_w25qxx_read_status1(dev, &status);
        if (ret != XY_DEVICE_OK) {
            return ret;
        }
        
        /* 检查 BUSY 位 (bit 0) */
        if ((status & 0x01) == 0) {
            return XY_DEVICE_OK; /* 就绪 */
        }
        
        /* 检查超时 */
        /* TODO: 实现超时检查 */
        if (timeout_ms > 0) {
            /* 简单延迟 */
            for (volatile int i = 0; i < 1000; i++);
        }
    }
}

int xy_w25qxx_read(xy_w25qxx_t *dev, uint32_t addr, uint8_t *buffer, 
                   size_t length)
{
    if (!dev || !buffer || length == 0) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    if (addr + length > dev->capacity_bytes) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    /* 发送读命令 + 3 字节地址 */
    uint8_t cmd[4] = {
        W25QXX_CMD_FAST_READ,
        (uint8_t)(addr >> 16),
        (uint8_t)(addr >> 8),
        (uint8_t)(addr)
    };
    
    cs_enable(dev);
    spi_transfer(dev, cmd, NULL, 4);
    
    /* 读取数据 */
    uint8_t dummy = 0xFF; /* 快速读需要一个 dummy 字节 */
    spi_transfer(dev, &dummy, NULL, 1);
    spi_transfer(dev, NULL, buffer, length);
    
    cs_disable(dev);
    
    return XY_DEVICE_OK;
}

int xy_w25qxx_write_page(xy_w25qxx_t *dev, uint32_t addr, const uint8_t *buffer,
                         size_t length)
{
    if (!dev || !buffer || length == 0) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    if (length > dev->page_size) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    /* 检查地址是否跨页 */
    uint32_t page_offset = addr % dev->page_size;
    if (page_offset + length > dev->page_size) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    /* 等待就绪 */
    int ret = xy_w25qxx_wait_ready(dev, 1000);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    /* 发送写使能 */
    send_command(dev, W25QXX_CMD_WRITE_ENABLE, NULL, 0);
    
    /* 发送页编程命令 + 地址 + 数据 */
    uint8_t cmd[4] = {
        W25QXX_CMD_PAGE_PROGRAM,
        (uint8_t)(addr >> 16),
        (uint8_t)(addr >> 8),
        (uint8_t)(addr)
    };
    
    cs_enable(dev);
    spi_transfer(dev, cmd, NULL, 4);
    spi_transfer(dev, buffer, NULL, length);
    cs_disable(dev);
    
    /* 等待编程完成 */
    return xy_w25qxx_wait_ready(dev, 1000);
}

int xy_w25qxx_write(xy_w25qxx_t *dev, uint32_t addr, const uint8_t *buffer,
                    size_t length)
{
    if (!dev || !buffer || length == 0) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    size_t remaining = length;
    const uint8_t *src = buffer;
    uint32_t dst = addr;
    
    while (remaining > 0) {
        /* 计算当前页可写入的字节数 */
        uint32_t page_offset = dst % dev->page_size;
        size_t to_write = dev->page_size - page_offset;
        if (to_write > remaining) {
            to_write = remaining;
        }
        
        /* 写入一页 */
        int ret = xy_w25qxx_write_page(dev, dst, src, to_write);
        if (ret != XY_DEVICE_OK) {
            return ret;
        }
        
        src += to_write;
        dst += to_write;
        remaining -= to_write;
    }
    
    return XY_DEVICE_OK;
}

int xy_w25qxx_erase_sector(xy_w25qxx_t *dev, uint32_t addr)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    if (addr >= dev->capacity_bytes) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    /* 等待就绪 */
    int ret = xy_w25qxx_wait_ready(dev, 1000);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    /* 发送写使能 */
    send_command(dev, W25QXX_CMD_WRITE_ENABLE, NULL, 0);
    
    /* 发送扇区擦除命令 + 地址 */
    uint8_t cmd[4] = {
        W25QXX_CMD_SECTOR_ERASE,
        (uint8_t)(addr >> 16),
        (uint8_t)(addr >> 8),
        (uint8_t)(addr)
    };
    
    send_command(dev, cmd, NULL, 0);
    
    /* 等待擦除完成 */
    return xy_w25qxx_wait_ready(dev, 3000); /* 扇区擦除通常需要 300ms */
}

int xy_w25qxx_erase_block(xy_w25qxx_t *dev, uint32_t addr)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    if (addr >= dev->capacity_bytes) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    /* 等待就绪 */
    int ret = xy_w25qxx_wait_ready(dev, 1000);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    /* 发送写使能 */
    send_command(dev, W25QXX_CMD_WRITE_ENABLE, NULL, 0);
    
    /* 发送块擦除命令 + 地址 */
    uint8_t cmd[4] = {
        W25QXX_CMD_BLOCK_ERASE,
        (uint8_t)(addr >> 16),
        (uint8_t)(addr >> 8),
        (uint8_t)(addr)
    };
    
    send_command(dev, cmd, NULL, 0);
    
    /* 等待擦除完成 */
    return xy_w25qxx_wait_ready(dev, 5000); /* 块擦除通常需要 500ms */
}

int xy_w25qxx_erase_chip(xy_w25qxx_t *dev)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    /* 等待就绪 */
    int ret = xy_w25qxx_wait_ready(dev, 1000);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    /* 发送写使能 */
    send_command(dev, W25QXX_CMD_WRITE_ENABLE, NULL, 0);
    
    /* 发送全片擦除命令 */
    send_command(dev, W25QXX_CMD_CHIP_ERASE, NULL, 0);
    
    /* 等待擦除完成 */
    return xy_w25qxx_wait_ready(dev, 10000); /* 全片擦除可能需要数秒 */
}

int xy_w25qxx_power_down(xy_w25qxx_t *dev)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    return send_command(dev, W25QXX_CMD_POWER_DOWN, NULL, 0);
}

int xy_w25qxx_release_power_down(xy_w25qxx_t *dev)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    return send_command(dev, W25QXX_CMD_RELEASE_POWER_DOWN, NULL, 0);
}

/* ==================== End of File ==================== */
