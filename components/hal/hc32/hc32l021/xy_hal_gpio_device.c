/**
 * @file xy_hal_gpio_device.c
 * @brief HC32L021 GPIO HAL Implementation
 * @version 1.0.0
 * @date 2026-03-16
 * 
 * @note HC32L021 GPIO 驱动实现 - 基于 xhsc 官方库
 */

#include "xy_hal_gpio.h"
#include "xy_hal_gpio_types.h"
#include "xy_hal_gpio_dev.h"
#include "hc32l021_gpio.h"
#include "hc32l021_sysctrl.h"
#include <string.h>

/* ==================== Private Types ==================== */

/**
 * @brief GPIO 私有数据
 */
typedef struct {
    M0P_GPIO_TypeDef *port;      /*!< GPIO 端口基地址 */
    uint16_t pin;                 /*!< GPIO 引脚 */
    bool initialized;             /*!< 初始化标志 */
} hc32_gpio_data_t;

/* ==================== Private Variables ==================== */

/* GPIO 实例池 (HC32L021: PA0-PA7, PB0-PB7, PC0-PC5, PH0-PH3) */
static hc32_gpio_data_t gpio_instances[32];
static bool gpio_pool_initialized = false;

/* ==================== Private Functions ==================== */

/**
 * @brief 初始化 GPIO 实例池
 */
static void gpio_init_pool(void)
{
    if (!gpio_pool_initialized) {
        memset(gpio_instances, 0, sizeof(gpio_instances));
        gpio_pool_initialized = true;
    }
}

/**
 * @brief 查找或分配 GPIO 实例
 * @param port GPIO 端口
 * @param pin GPIO 引脚
 * @return hc32_gpio_data_t* GPIO 实例指针
 */
static hc32_gpio_data_t *gpio_find_or_alloc(M0P_GPIO_TypeDef *port, uint16_t pin)
{
    gpio_init_pool();
    
    /* 查找已存在的实例 */
    for (int i = 0; i < 32; i++) {
        if (gpio_instances[i].port == port && 
            gpio_instances[i].pin == pin &&
            gpio_instances[i].initialized) {
            return &gpio_instances[i];
        }
    }
    
    /* 分配新实例 */
    for (int i = 0; i < 32; i++) {
        if (!gpio_instances[i].initialized) {
            hc32_gpio_data_t *data = &gpio_instances[i];
            data->port = port;
            data->pin = pin;
            data->initialized = true;
            return data;
        }
    }
    
    return NULL; /* 无可用实例 */
}

/**
 * @brief 使能 GPIO 端口时钟
 * @param port GPIO 端口
 */
static void gpio_enable_clock(M0P_GPIO_TypeDef *port)
{
    /* HC32L021 GPIO 时钟始终使能，无需额外配置 */
    (void)port;
}

/**
 * @brief 转换 GPIO 模式
 * @param mode XinYi GPIO 模式
 * @return HC32 GPIO 模式
 */
static en_gpio_func_t gpio_convert_mode(xy_hal_gpio_mode_t mode)
{
    switch (mode) {
        case XY_HAL_GPIO_MODE_INPUT:
            return GpioFuncPortIn;
        case XY_HAL_GPIO_MODE_OUTPUT:
            return GpioFuncPortOut;
        case XY_HAL_GPIO_MODE_AF:
            return GpioFuncPortOut; /* HC32L021 复用功能配置 */
        case XY_HAL_GPIO_MODE_ANALOG:
            return GpioFuncAnalogIn;
        default:
            return GpioFuncPortIn;
    }
}

/**
 * @brief 转换 GPIO 上下拉
 * @param pull XinYi 上下拉配置
 * @return HC32 上下拉配置
 */
static en_gpio_pu_t gpio_convert_pull(xy_hal_gpio_pull_t pull)
{
    switch (pull) {
        case XY_HAL_GPIO_PULL_UP:
            return GpioPuEnable;
        case XY_HAL_GPIO_PULL_DOWN:
            return GpioPuDisable;
        case XY_HAL_GPIO_PULL_NONE:
        default:
            return GpioPuDisable;
    }
}

/**
 * @brief 转换 GPIO 驱动能力
 * @param speed XinYi 速度配置
 * @return HC32 驱动能力
 */
static en_gpio_drv_t gpio_convert_speed(xy_hal_gpio_speed_t speed)
{
    switch (speed) {
        case XY_HAL_GPIO_SPEED_LOW:
            return GpioDrvL;
        case XY_HAL_GPIO_SPEED_HIGH:
        case XY_HAL_GPIO_SPEED_VERY_HIGH:
        default:
            return GpioDrvH;
    }
}

/* ==================== Public Implementation ==================== */

xy_hal_gpio_t xy_hal_gpio_bind(const char *name)
{
    if (!name) {
        return NULL;
    }
    
    /* 解析引脚名称 (如 "GPIOA.0", "PA0") */
    M0P_GPIO_TypeDef *port = NULL;
    uint16_t pin = 0;
    
    if (strncmp(name, "PA", 2) == 0) {
        port = M0P_GPIO_PA;
        pin = (1 << (name[2] - '0'));
    } else if (strncmp(name, "PB", 2) == 0) {
        port = M0P_GPIO_PB;
        pin = (1 << (name[2] - '0'));
    } else if (strncmp(name, "PC", 2) == 0) {
        port = M0P_GPIO_PC;
        pin = (1 << (name[2] - '0'));
    } else if (strncmp(name, "PH", 2) == 0) {
        port = M0P_GPIO_PH;
        pin = (1 << (name[2] - '0'));
    }
    
    if (!port) {
        return NULL;
    }
    
    /* 查找或分配 GPIO 实例 */
    hc32_gpio_data_t *data = gpio_find_or_alloc(port, pin);
    if (!data) {
        return NULL;
    }
    
    return (xy_hal_gpio_t)data;
}

xy_hal_error_t xy_hal_gpio_configure(xy_hal_gpio_t gpio, 
                                     xy_hal_gpio_pin_t pin,
                                     const xy_hal_gpio_config_t *config)
{
    if (!gpio || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_gpio_data_t *data = (hc32_gpio_data_t *)gpio;
    
    /* 使能时钟 */
    gpio_enable_clock(data->port);
    
    /* 配置 GPIO */
    stc_gpio_cfg_t gpio_cfg;
    DDL_ZERO_STRUCT(gpio_cfg);
    
    /* 设置功能 */
    gpio_cfg.enFunc = gpio_convert_mode(config->mode);
    
    /* 设置上下拉 */
    if (config->pull == XY_HAL_GPIO_PULL_UP) {
        gpio_cfg.enPu = GpioPuEnable;
    } else if (config->pull == XY_HAL_GPIO_PULL_DOWN) {
        gpio_cfg.enPd = GpioPdEnable;
    } else {
        gpio_cfg.enPu = GpioPuDisable;
        gpio_cfg.enPd = GpioPdDisable;
    }
    
    /* 设置驱动能力 */
    gpio_cfg.enDrv = gpio_convert_speed(config->speed);
    
    /* 设置开漏/推挽 */
    if (config->open_drain) {
        /* HC32L021 通过上拉电阻配置实现开漏效果 */
        gpio_cfg.enPu = GpioPuDisable;
    }
    
    /* 应用配置 */
    GPIO_Init(data->port, data->pin, &gpio_cfg);
    
    return XY_HAL_ERROR_OK;
}

xy_hal_error_t xy_hal_gpio_write(xy_hal_gpio_t gpio, 
                                 xy_hal_gpio_pin_t pin, 
                                 uint8_t value)
{
    if (!gpio) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_gpio_data_t *data = (hc32_gpio_data_t *)gpio;
    
    if (value) {
        GPIO_WriteBit(data->port, data->pin, TRUE);
    } else {
        GPIO_WriteBit(data->port, data->pin, FALSE);
    }
    
    return XY_HAL_ERROR_OK;
}

int32_t xy_hal_gpio_read(xy_hal_gpio_t gpio, xy_hal_gpio_pin_t pin)
{
    if (!gpio) {
        return -1;
    }
    
    hc32_gpio_data_t *data = (hc32_gpio_data_t *)gpio;
    
    en_flag_t level = GPIO_ReadInputDataBit(data->port, data->pin);
    return (level == TRUE) ? 1 : 0;
}

xy_hal_error_t xy_hal_gpio_toggle(xy_hal_gpio_t gpio, xy_hal_gpio_pin_t pin)
{
    if (!gpio) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_gpio_data_t *data = (hc32_gpio_data_t *)gpio;
    
    GPIO_ToggleBits(data->port, data->pin);
    
    return XY_HAL_ERROR_OK;
}
