/**
 * @file xy_hal_gpio_device.c
 * @brief WCH CH32U5 GPIO Device Implementation - Unified HAL API
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 实现统一的 GPIO 设备 API，基于 WCH CH32U5 HAL 库
 * @note 沁恒半导体 CH32U5 系列 (ARM Cortex-M33)
 */

#include "../inc/xy_hal_gpio_dev.h"
#include "../inc/xy_hal_gpio_types.h"
#include <string.h>

/* WCH CH32U5 HAL 头文件 */
#include "ch32u5xx.h"

/* ==================== Private Definitions ==================== */

#define CH32U5_GPIO_PORT_COUNT  (8)  /* GPIOA-GPIOH */

/* GPIO 端口基地址数组 */
static GPIO_TypeDef *const gpio_port_base[CH32U5_GPIO_PORT_COUNT] = {
    GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG, GPIOH
};

/* GPIO 端口时钟使能宏数组 */
static void (*const gpio_clock_enable[CH32U5_GPIO_PORT_COUNT])(void) = {
    RCC_AHB2PeriphClockCmd,  /* GPIOA */
    RCC_AHB2PeriphClockCmd,  /* GPIOB */
    RCC_AHB2PeriphClockCmd,  /* GPIOC */
    RCC_AHB2PeriphClockCmd,  /* GPIOD */
    RCC_AHB2PeriphClockCmd,  /* GPIOE */
    RCC_AHB2PeriphClockCmd,  /* GPIOF */
    RCC_AHB2PeriphClockCmd,  /* GPIOG */
    RCC_AHB2PeriphClockCmd,  /* GPIOH */
};

/* GPIO 端口时钟使能位 */
static uint32_t const gpio_clock_bits[CH32U5_GPIO_PORT_COUNT] = {
    RCC_AHB2Periph_GPIOA,
    RCC_AHB2Periph_GPIOB,
    RCC_AHB2Periph_GPIOC,
    RCC_AHB2Periph_GPIOD,
    RCC_AHB2Periph_GPIOE,
    RCC_AHB2Periph_GPIOF,
    RCC_AHB2Periph_GPIOG,
    RCC_AHB2Periph_GPIOH
};

/* GPIO 端口名称 */
static const char *const gpio_port_names[CH32U5_GPIO_PORT_COUNT] = {
    "GPIOA", "GPIOB", "GPIOC", "GPIOD", "GPIOE", 
    "GPIOF", "GPIOG", "GPIOH"
};

/* ==================== Private Types ==================== */

/**
 * @brief CH32U5 GPIO 设备私有数据
 */
typedef struct {
    GPIO_TypeDef *port;        /* GPIO 端口基地址 */
    uint8_t port_num;          /* 端口号 (0-7) */
    uint32_t pin_mask;         /* 已配置引脚掩码 */
    xy_hal_gpio_config_t configs[16];  /* 每个引脚的配置 */
} ch32u5_gpio_data_t;

/**
 * @brief CH32U5 GPIO 设备实例
 */
static ch32u5_gpio_data_t gpio_devices[CH32U5_GPIO_PORT_COUNT] = {0};

/* ==================== Private Functions ==================== */

/**
 * @brief 将统一模式转换为 WCH HAL 模式
 */
static uint32_t gpio_mode_to_hal(xy_hal_gpio_mode_t mode)
{
    switch (mode) {
        case XY_HAL_GPIO_MODE_INPUT:
            return GPIO_Mode_IN_FLOATING;
        case XY_HAL_GPIO_MODE_OUTPUT:
            return GPIO_Mode_Out_PP;
        case XY_HAL_GPIO_MODE_AF:
            return GPIO_Mode_AF_PP;
        case XY_HAL_GPIO_MODE_ANALOG:
            return GPIO_Mode_AIN;
        case XY_HAL_GPIO_MODE_IT_RISING:
            return GPIO_Mode_IT_Rising;
        case XY_HAL_GPIO_MODE_IT_FALLING:
            return GPIO_Mode_IT_Falling;
        case XY_HAL_GPIO_MODE_IT_BOTH:
            return GPIO_Mode_IT_Rising_Falling;
        default:
            return GPIO_Mode_IN_FLOATING;
    }
}

/**
 * @brief 将统一速度转换为 WCH HAL 速度
 */
static uint32_t gpio_speed_to_hal(xy_hal_gpio_speed_t speed)
{
    switch (speed) {
        case XY_HAL_GPIO_SPEED_LOW:
            return GPIO_Speed_2MHz;
        case XY_HAL_GPIO_SPEED_MEDIUM:
            return GPIO_Speed_10MHz;
        case XY_HAL_GPIO_SPEED_HIGH:
            return GPIO_Speed_25MHz;
        case XY_HAL_GPIO_SPEED_VERY_HIGH:
            return GPIO_Speed_50MHz;
        default:
            return GPIO_Speed_2MHz;
    }
}

/**
 * @brief 获取 GPIO 设备实例
 */
static ch32u5_gpio_data_t *gpio_get_device(uint8_t port_num)
{
    if (port_num >= CH32U5_GPIO_PORT_COUNT) {
        return NULL;
    }
    return &gpio_devices[port_num];
}

/* ==================== Device Model API Implementation ==================== */

xy_hal_gpio_t xy_hal_gpio_bind(const char *name)
{
    /* 查找匹配的端口名称 */
    for (int i = 0; i < CH32U5_GPIO_PORT_COUNT; i++) {
        if (strcmp(name, gpio_port_names[i]) == 0) {
            ch32u5_gpio_data_t *dev = &gpio_devices[i];
            
            /* 使能时钟 */
            RCC_AHB2PeriphClockCmd(gpio_clock_bits[i], ENABLE);
            
            /* 初始化设备数据 */
            dev->port = gpio_port_base[i];
            dev->port_num = (uint8_t)i;
            dev->pin_mask = 0;
            memset(dev->configs, 0, sizeof(dev->configs));
            
            return (xy_hal_gpio_t)dev;
        }
    }
    
    return NULL;
}

xy_hal_error_t xy_hal_gpio_unbind(xy_hal_gpio_t gpio)
{
    if (!gpio) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    ch32u5_gpio_data_t *dev = (ch32u5_gpio_data_t *)gpio;
    
    /* 反配置所有引脚 */
    for (int pin = 0; pin < 16; pin++) {
        if (dev->pin_mask & (1 << pin)) {
            /* WCH HAL 没有标准的 DeInit 函数，手动重置 */
            dev->port->CFGLR &= ~(0xF << (pin * 4));
            dev->port->CFGHR &= ~(0xF << (pin * 4));
        }
    }
    
    dev->pin_mask = 0;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_configure(xy_hal_gpio_t gpio, 
                                     xy_hal_gpio_pin_t pin,
                                     const xy_hal_gpio_config_t *config)
{
    if (!gpio || !config || pin >= 16) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    ch32u5_gpio_data_t *dev = (ch32u5_gpio_data_t *)gpio;
    
    /* 配置 GPIO */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    GPIO_InitStruct.GPIO_Pin = (1U << pin);
    GPIO_InitStruct.GPIO_Mode = gpio_mode_to_hal(config->mode);
    GPIO_InitStruct.GPIO_Speed = gpio_speed_to_hal(config->speed);
    
    /* WCH CH32 没有上下拉配置，忽略 */
    (void)config->pull;
    (void)config->otype;
    (void)config->alternate;
    
    GPIO_Init(dev->port, &GPIO_InitStruct);
    
    /* 保存配置 */
    dev->pin_mask |= (1U << pin);
    dev->configs[pin] = *config;
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_write(xy_hal_gpio_t gpio, 
                                 xy_hal_gpio_pin_t pin, 
                                 uint8_t value)
{
    if (!gpio || pin >= 16) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    ch32u5_gpio_data_t *dev = (ch32u5_gpio_data_t *)gpio;
    
    if (value) {
        dev->port->BSHR = (1U << pin);
    } else {
        dev->port->BCR = (1U << pin);
    }
    
    return XY_HAL_OK;
}

int32_t xy_hal_gpio_read(xy_hal_gpio_t gpio, xy_hal_gpio_pin_t pin)
{
    if (!gpio || pin >= 16) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    ch32u5_gpio_data_t *dev = (ch32u5_gpio_data_t *)gpio;
    
    return (dev->port->INDR & (1U << pin)) ? 1 : 0;
}

xy_hal_error_t xy_hal_gpio_toggle(xy_hal_gpio_t gpio, xy_hal_gpio_pin_t pin)
{
    if (!gpio || pin >= 16) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    ch32u5_gpio_data_t *dev = (ch32u5_gpio_data_t *)gpio;
    
    dev->port->OUTDR ^= (1U << pin);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_port_write(xy_hal_gpio_t gpio,
                                      xy_hal_gpio_mask_t mask,
                                      xy_hal_gpio_value_t value)
{
    if (!gpio) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    ch32u5_gpio_data_t *dev = (ch32u5_gpio_data_t *)gpio;
    
    /* 清除指定的引脚 */
    dev->port->BCR = (~value & mask);
    /* 设置指定的引脚 */
    dev->port->BSHR = (value & mask);
    
    return XY_HAL_OK;
}

xy_hal_gpio_value_t xy_hal_gpio_port_read(xy_hal_gpio_t gpio,
                                          xy_hal_gpio_mask_t mask)
{
    if (!gpio) {
        return 0;
    }
    
    ch32u5_gpio_data_t *dev = (ch32u5_gpio_data_t *)gpio;
    
    return (xy_hal_gpio_value_t)(dev->port->INDR & mask);
}

xy_hal_error_t xy_hal_gpio_set_interrupt(xy_hal_gpio_t gpio,
                                         xy_hal_gpio_pin_t pin,
                                         xy_hal_gpio_irq_mode_t mode,
                                         xy_hal_gpio_irq_handler_t handler,
                                         void *arg)
{
    if (!gpio || pin >= 16 || !handler) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* 
     * WCH CH32U5 中断配置需要:
     * 1. 配置 GPIO 为 AFIO 模式 (GPIO_Mode_AF_PP)
     * 2. 调用 GPIO_AFIOExtiLineConfig() 选择引脚源
     * 3. 配置 EXTI_Init() 设置触发模式和中断/事件模式
     * 4. 实现对应的中断服务函数 (EXTI0-15_IRQHandler)
     * 
     * 注意: 需要集成 CH32U5 SDK (ch32u5xx.h) 才能实现
     * 当前仓库中缺少 CH32U5 系列 MCU 头文件
     */
    (void)gpio;
    (void)pin;
    (void)mode;
    (void)handler;
    (void)arg;
    
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_gpio_enable_interrupt(xy_hal_gpio_t gpio,
                                            xy_hal_gpio_pin_t pin)
{
    if (!gpio || pin >= 16) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* 
     * 需要集成 CH32U5 SDK 后调用:
     * EXTI->IMR |= (1 << pin);  // 启用中断掩码
     */
    (void)gpio;
    (void)pin;
    
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_gpio_disable_interrupt(xy_hal_gpio_t gpio,
                                             xy_hal_gpio_pin_t pin)
{
    if (!gpio || pin >= 16) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* 
     * 需要集成 CH32U5 SDK 后调用:
     * EXTI->IMR &= ~(1 << pin);  // 禁用中断掩码
     */
    (void)gpio;
    (void)pin;
    
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_gpio_get_config(xy_hal_gpio_t gpio,
                                      xy_hal_gpio_pin_t pin,
                                      xy_hal_gpio_config_t *config)
{
    if (!gpio || !config || pin >= 16) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    ch32u5_gpio_data_t *dev = (ch32u5_gpio_data_t *)gpio;
    
    if (!(dev->pin_mask & (1U << pin))) {
        return XY_HAL_ERROR_NOT_CONFIGURED;
    }
    
    *config = dev->configs[pin];
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_lock(xy_hal_gpio_t gpio, xy_hal_gpio_pin_t pin)
{
    if (!gpio || pin >= 16) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* WCH CH32 没有 GPIO 锁定机制 */
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

/* ==================== Legacy API Compatibility ==================== */

/**
 * @brief 传统 API 适配层 - 保持向后兼容
 */

xy_hal_error_t xy_hal_gpio_init(GPIO_TypeDef *port, uint8_t pin,
                                const xy_hal_gpio_config_t *config)
{
    /* 查找对应的设备实例 */
    for (int i = 0; i < CH32U5_GPIO_PORT_COUNT; i++) {
        if (gpio_port_base[i] == port) {
            xy_hal_gpio_t gpio = (xy_hal_gpio_t)&gpio_devices[i];
            return xy_hal_gpio_configure(gpio, pin, config);
        }
    }
    
    return XY_HAL_ERROR_INVALID_PARAM;
}

xy_hal_error_t xy_hal_gpio_deinit(GPIO_TypeDef *port, uint8_t pin)
{
    for (int i = 0; i < CH32U5_GPIO_PORT_COUNT; i++) {
        if (gpio_port_base[i] == port) {
            /* 手动重置引脚配置 */
            port->CFGLR &= ~(0xF << (pin * 4));
            port->CFGHR &= ~(0xF << (pin * 4));
            gpio_devices[i].pin_mask &= ~(1U << pin);
            return XY_HAL_OK;
        }
    }
    
    return XY_HAL_ERROR_INVALID_PARAM;
}

xy_hal_error_t xy_hal_gpio_write(GPIO_TypeDef *port, uint8_t pin, uint8_t value)
{
    if (value) {
        port->BSHR = (1U << pin);
    } else {
        port->BCR = (1U << pin);
    }
    return XY_HAL_OK;
}

int32_t xy_hal_gpio_read(GPIO_TypeDef *port, uint8_t pin)
{
    return (port->INDR & (1U << pin)) ? 1 : 0;
}

xy_hal_error_t xy_hal_gpio_toggle(GPIO_TypeDef *port, uint8_t pin)
{
    port->OUTDR ^= (1U << pin);
    return XY_HAL_OK;
}

/* ==================== End of File ==================== */
