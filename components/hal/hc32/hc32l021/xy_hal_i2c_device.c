/**
 * @file xy_hal_i2c_device.c
 * @brief HC32L021 I2C Device Implementation - Unified HAL API
 * @version 1.0.0
 * @date 2026-03-16
 * 
 * @note 实现统一的 I2C 设备 API，基于 HC32L021 寄存器
 */

#include "../inc/xy_hal_i2c_dev.h"
#include "../inc/xy_hal_i2c_types.h"
#include "../inc/xy_hal_error.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* HC32L021 最小化头文件 */
#include "hc32l021_minimal.h"

/* ==================== Private Definitions ==================== */

#define HC32L021_I2C_COUNT  (2)  /* I2C0/I2C1 */

/* ==================== Private Types ==================== */

/**
 * @brief HC32L021 I2C 设备私有数据
 */
typedef struct {
    uint8_t instance;          /* I2C 实例号 (0/1) */
    bool initialized;          /* 初始化标志 */
    xy_hal_i2c_config_t config; /* 当前配置 */
    bool busy;                 /* 忙标志 */
} hc32_i2c_dev_t;

/* ==================== Private Variables ==================== */

/* I2C 设备实例池 */
static hc32_i2c_dev_t i2c_devices[HC32L021_I2C_COUNT];
static bool i2c_devices_initialized = false;

/* ==================== Private Functions ==================== */

/**
 * @brief 初始化 I2C 设备池
 */
static void i2c_init_devices(void)
{
    if (!i2c_devices_initialized) {
        memset(i2c_devices, 0, sizeof(i2c_devices));
        i2c_devices_initialized = true;
    }
}

/**
 * @brief 查找或分配 I2C 设备
 * @param instance I2C 实例号
 * @return hc32_i2c_dev_t* I2C 设备指针
 */
static hc32_i2c_dev_t *i2c_find_or_alloc(uint8_t instance)
{
    i2c_init_devices();
    
    if (instance >= HC32L021_I2C_COUNT) {
        return NULL;
    }
    
    hc32_i2c_dev_t *dev = &i2c_devices[instance];
    
    if (!dev->initialized) {
        dev->instance = instance;
        dev->initialized = true;
        dev->busy = false;
    }
    
    return dev;
}

/**
 * @brief 解析 I2C 名称
 * @param name I2C 名称 (如 "I2C0", "I2C1")
 * @param instance 输出：实例号
 * @return 0 成功，-1 失败
 */
static int parse_i2c_name(const char *name, uint8_t *instance)
{
    if (!name || !instance) {
        return -1;
    }
    
    if (strncmp(name, "I2C", 3) == 0) {
        if (name[3] >= '0' && name[3] <= '1') {
            *instance = name[3] - '0';
            return 0;
        }
    }
    
    return -1;
}

/* ==================== API Implementation ==================== */

xy_hal_i2c_t xy_hal_i2c_bind(const char *name)
{
    uint8_t instance;
    
    if (parse_i2c_name(name, &instance) != 0) {
        return NULL;
    }
    
    hc32_i2c_dev_t *dev = i2c_find_or_alloc(instance);
    if (!dev) {
        return NULL;
    }
    
    return (xy_hal_i2c_t)dev;
}

xy_hal_error_t xy_hal_i2c_unbind(xy_hal_i2c_t i2c)
{
    if (!i2c) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_i2c_dev_t *dev = (hc32_i2c_dev_t *)i2c;
    dev->initialized = false;
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_configure(xy_hal_i2c_t i2c,
                                    const xy_hal_i2c_config_t *config)
{
    if (!i2c || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_i2c_dev_t *dev = (hc32_i2c_dev_t *)i2c;
    
    /* 保存配置 */
    memcpy(&dev->config, config, sizeof(dev->config));
    
    /* TODO: 配置 HC32L021 I2C 硬件寄存器 */
    /* 1. 配置时钟速度 (100k/400k/1M) */
    /* 2. 配置占空比 */
    /* 3. 使能 I2C */
    
    return XY_HAL_OK;
}

int32_t xy_hal_i2c_master_transmit(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                   const uint8_t *data, size_t length,
                                   uint32_t timeout)
{
    if (!i2c || !data || length == 0) {
        return -1;
    }
    
    hc32_i2c_dev_t *dev = (hc32_i2c_dev_t *)i2c;
    
    if (dev->busy) {
        return -XY_HAL_ERROR_BUSY;
    }
    
    dev->busy = true;
    
    /* TODO: 实现 I2C 主模式发送 */
    /* 1. 发送 START 条件 */
    /* 2. 发送设备地址 (写) */
    /* 3. 发送数据 */
    /* 4. 发送 STOP 条件 */
    
    dev->busy = false;
    
    return (int32_t)length;
}

int32_t xy_hal_i2c_master_receive(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                  uint8_t *data, size_t length,
                                  uint32_t timeout)
{
    if (!i2c || !data || length == 0) {
        return -1;
    }
    
    hc32_i2c_dev_t *dev = (hc32_i2c_dev_t *)i2c;
    
    if (dev->busy) {
        return -XY_HAL_ERROR_BUSY;
    }
    
    dev->busy = true;
    
    /* TODO: 实现 I2C 主模式接收 */
    /* 1. 发送 START 条件 */
    /* 2. 发送设备地址 (读) */
    /* 3. 接收数据 */
    /* 4. 发送 NACK + STOP */
    
    dev->busy = false;
    
    return (int32_t)length;
}

/* ==================== End of File ==================== */
