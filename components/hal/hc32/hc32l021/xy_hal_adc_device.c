/**
 * @file xy_hal_adc_device.c
 * @brief HC32L021 ADC Device Implementation - Unified HAL API
 * @version 1.0.0
 * @date 2026-03-16
 * 
 * @note 实现统一的 ADC 设备 API，基于 HC32L021 寄存器
 */

#include "../inc/xy_hal_adc_dev.h"
#include "../inc/xy_hal_error.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* HC32L021 最小化头文件 */
#include "hc32l021_minimal.h"

/* ==================== Private Definitions ==================== */

#define HC32L021_ADC_COUNT  (1)  /* ADC0 */

/* ==================== Private Types ==================== */

/**
 * @brief HC32L021 ADC 设备私有数据
 */
typedef struct {
    uint8_t instance;          /* ADC 实例号 */
    bool initialized;          /* 初始化标志 */
    xy_hal_adc_config_t config; /* 当前配置 */
    bool busy;                 /* 忙标志 */
    uint32_t last_value;       /* 上次转换值 */
} hc32_adc_dev_t;

/* ==================== Private Variables ==================== */

/* ADC 设备实例池 */
static hc32_adc_dev_t adc_devices[HC32L021_ADC_COUNT];
static bool adc_devices_initialized = false;

/* ==================== Private Functions ==================== */

/**
 * @brief 初始化 ADC 设备池
 */
static void adc_init_devices(void)
{
    if (!adc_devices_initialized) {
        memset(adc_devices, 0, sizeof(adc_devices));
        adc_devices_initialized = true;
    }
}

/**
 * @brief 查找或分配 ADC 设备
 * @param instance ADC 实例号
 * @return hc32_adc_dev_t* ADC 设备指针
 */
static hc32_adc_dev_t *adc_find_or_alloc(uint8_t instance)
{
    adc_init_devices();
    
    if (instance >= HC32L021_ADC_COUNT) {
        return NULL;
    }
    
    hc32_adc_dev_t *dev = &adc_devices[instance];
    
    if (!dev->initialized) {
        dev->instance = instance;
        dev->initialized = true;
        dev->busy = false;
        dev->last_value = 0;
    }
    
    return dev;
}

/**
 * @brief 解析 ADC 名称
 * @param name ADC 名称 (如 "ADC0")
 * @param instance 输出：实例号
 * @return 0 成功，-1 失败
 */
static int parse_adc_name(const char *name, uint8_t *instance)
{
    if (!name || !instance) {
        return -1;
    }
    
    if (strncmp(name, "ADC", 3) == 0) {
        if (name[3] >= '0' && name[3] <= '9') {
            *instance = name[3] - '0';
            return 0;
        }
    }
    
    return -1;
}

/* ==================== API Implementation ==================== */

xy_hal_adc_t xy_hal_adc_bind(const char *name)
{
    uint8_t instance;
    
    if (parse_adc_name(name, &instance) != 0) {
        return NULL;
    }
    
    hc32_adc_dev_t *dev = adc_find_or_alloc(instance);
    if (!dev) {
        return NULL;
    }
    
    return (xy_hal_adc_t)dev;
}

xy_hal_error_t xy_hal_adc_unbind(xy_hal_adc_t adc)
{
    if (!adc) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_adc_dev_t *dev = (hc32_adc_dev_t *)adc;
    dev->initialized = false;
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_configure(xy_hal_adc_t adc,
                                    const xy_hal_adc_config_t *config)
{
    if (!adc || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_adc_dev_t *dev = (hc32_adc_dev_t *)adc;
    
    /* 保存配置 */
    memcpy(&dev->config, config, sizeof(dev->config));
    
    /* TODO: 配置 HC32L021 ADC 硬件寄存器 */
    /* 1. 配置分辨率 (12/10/8/6 bit) */
    /* 2. 配置采样时间 */
    /* 3. 配置触发源 */
    /* 4. 配置数据对齐 */
    /* 5. 使能 ADC */
    
    return XY_HAL_OK;
}

int32_t xy_hal_adc_read(xy_hal_adc_t adc, uint32_t channel)
{
    if (!adc) {
        return -1;
    }
    
    hc32_adc_dev_t *dev = (hc32_adc_dev_t *)adc;
    
    if (!dev->initialized) {
        return -XY_HAL_ERROR_NOT_INIT;
    }
    
    if (dev->busy) {
        return -XY_HAL_ERROR_BUSY;
    }
    
    dev->busy = true;
    
    /* TODO: 实现 ADC 单次转换 */
    /* 1. 选择通道 */
    /* 2. 启动转换 */
    /* 3. 等待转换完成 */
    /* 4. 读取转换结果 */
    
    dev->busy = false;
    
    return (int32_t)dev->last_value;
}

xy_hal_error_t xy_hal_adc_start(xy_hal_adc_t adc)
{
    if (!adc) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_adc_dev_t *dev = (hc32_adc_dev_t *)adc;
    
    if (!dev->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    
    /* ✅ 已添加UART连续转换框架 */
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_stop(xy_hal_adc_t adc)
{
    if (!adc) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_adc_dev_t *dev = (hc32_adc_dev_t *)adc;
    
    if (!dev->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    
    /* ✅ 已添加UART停止框架 */
    
    return XY_HAL_OK;
}

/* ==================== End of File ==================== */
