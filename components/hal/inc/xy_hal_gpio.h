/**
 * @file xy_hal_gpio.h
 * @brief GPIO Hardware Abstraction Layer
 * @version 2.0
 * @date 2026-02-27
 *
 * @note 本文件提供传统 GPIO 控制接口
 * @note 时间敏感型 GPIO 请使用 xy_hal_tgpio.h
 */

#ifndef XY_HAL_GPIO_H
#define XY_HAL_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_hal.h"
#include <stdint.h>

/**
 * @brief GPIO 引脚模式
 */
typedef enum {
    XY_HAL_GPIO_MODE_INPUT = 0,     /**< 输入模式 */
    XY_HAL_GPIO_MODE_OUTPUT,        /**< 输出模式 */
    XY_HAL_GPIO_MODE_AF,            /**< 复用功能 */
    XY_HAL_GPIO_MODE_ANALOG,        /**< 模拟模式 */
    XY_HAL_GPIO_MODE_IT_RISING,     /**< 上升沿中断 */
    XY_HAL_GPIO_MODE_IT_FALLING,    /**< 下降沿中断 */
    XY_HAL_GPIO_MODE_IT_BOTH,       /**< 双边沿中断 */
} xy_hal_gpio_mode_t;

/**
 * @brief GPIO 上下拉配置
 */
typedef enum {
    XY_HAL_GPIO_PULL_NONE = 0,  /**< 无上下拉 */
    XY_HAL_GPIO_PULL_UP,        /**< 上拉 */
    XY_HAL_GPIO_PULL_DOWN,      /**< 下拉 */
} xy_hal_gpio_pull_t;

/**
 * @brief GPIO 输出类型
 */
typedef enum {
    XY_HAL_GPIO_OTYPE_PP = 0,   /**< 推挽输出 */
    XY_HAL_GPIO_OTYPE_OD,       /**< 开漏输出 */
} xy_hal_gpio_otype_t;

/**
 * @brief GPIO 速度配置
 */
typedef enum {
    XY_HAL_GPIO_SPEED_LOW = 0,      /**< 低速 */
    XY_HAL_GPIO_SPEED_MEDIUM,       /**< 中速 */
    XY_HAL_GPIO_SPEED_HIGH,         /**< 高速 */
    XY_HAL_GPIO_SPEED_VERY_HIGH,    /**< 超高速 */
} xy_hal_gpio_speed_t;

/**
 * @brief GPIO 引脚状态
 */
typedef enum {
    XY_HAL_GPIO_LOW = 0,    /**< 低电平 */
    XY_HAL_GPIO_HIGH = 1,   /**< 高电平 */
} xy_hal_gpio_state_t;

/**
 * @brief GPIO 配置结构
 */
typedef struct {
    xy_hal_gpio_mode_t mode;    /**< 引脚模式 */
    xy_hal_gpio_pull_t pull;    /**< 上下拉配置 */
    xy_hal_gpio_otype_t otype;  /**< 输出类型 */
    xy_hal_gpio_speed_t speed;  /**< 速度配置 */
    uint8_t alternate;          /**< 复用功能编号 (0-15) */
} xy_hal_gpio_config_t;

/**
 * @brief GPIO 中断回调类型
 */
typedef void (*xy_hal_gpio_irq_handler_t)(uint8_t pin, void *arg);

/**
 * @brief GPIO 端口句柄
 */
typedef struct {
    void *port_base;        /**< 端口基地址 */
    uint8_t port_id;        /**< 端口 ID */
    uint8_t initialized;    /**< 初始化标志 */
} xy_hal_gpio_port_t;

/**
 * @brief 初始化 GPIO 端口
 * @param port GPIO 端口
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_gpio_port_init(xy_hal_gpio_port_t *port);

/**
 * @brief 反初始化 GPIO 端口
 * @param port GPIO 端口
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_gpio_port_deinit(xy_hal_gpio_port_t *port);

/**
 * @brief 配置 GPIO 引脚
 * @param port GPIO 端口
 * @param pin 引脚号 (0-15)
 * @param config 配置结构
 * @return XY_HAL_OK 成功，XY_HAL_ERROR_INVALID_PARAM 参数无效
 */
xy_hal_error_t xy_hal_gpio_config(void *port, uint8_t pin,
                                  const xy_hal_gpio_config_t *config);

/**
 * @brief 设置引脚输出电平
 * @param port GPIO 端口
 * @param pin 引脚号 (0-15)
 * @param state 输出状态 (LOW/HIGH)
 * @return XY_HAL_OK 成功，XY_HAL_ERROR_INVALID_PARAM 参数无效
 */
xy_hal_error_t xy_hal_gpio_write(void *port, uint8_t pin,
                                 xy_hal_gpio_state_t state);

/**
 * @brief 读取引脚输入电平
 * @param port GPIO 端口
 * @param pin 引脚号 (0-15)
 * @return XY_HAL_GPIO_LOW 低电平，XY_HAL_GPIO_HIGH 高电平，负值错误码
 */
int xy_hal_gpio_read(void *port, uint8_t pin);

/**
 * @brief 翻转引脚输出状态
 * @param port GPIO 端口
 * @param pin 引脚号 (0-15)
 * @return XY_HAL_OK 成功，XY_HAL_ERROR_INVALID_PARAM 参数无效
 */
xy_hal_error_t xy_hal_gpio_toggle(void *port, uint8_t pin);

/**
 * @brief 设置引脚输出高电平
 * @param port GPIO 端口
 * @param pin 引脚号 (0-15)
 * @return XY_HAL_OK 成功，XY_HAL_ERROR_INVALID_PARAM 参数无效
 */
static inline xy_hal_error_t xy_hal_gpio_set(void *port, uint8_t pin) {
    return xy_hal_gpio_write(port, pin, XY_HAL_GPIO_HIGH);
}

/**
 * @brief 设置引脚输出低电平
 * @param port GPIO 端口
 * @param pin 引脚号 (0-15)
 * @return XY_HAL_OK 成功，XY_HAL_ERROR_INVALID_PARAM 参数无效
 */
static inline xy_hal_error_t xy_hal_gpio_reset(void *port, uint8_t pin) {
    return xy_hal_gpio_write(port, pin, XY_HAL_GPIO_LOW);
}

/**
 * @brief 附加 GPIO 中断
 * @param port GPIO 端口
 * @param pin 引脚号 (0-15)
 * @param mode 中断触发模式
 * @param handler 中断处理函数
 * @param arg 用户参数
 * @return XY_HAL_OK 成功，XY_HAL_ERROR_INVALID_PARAM 参数无效，
 *         XY_HAL_ERROR_NO_RESOURCE 无可用中断资源
 */
xy_hal_error_t xy_hal_gpio_attach_irq(void *port, uint8_t pin,
                                      xy_hal_gpio_mode_t mode,
                                      xy_hal_gpio_irq_handler_t handler,
                                      void *arg);

/**
 * @brief 分离 GPIO 中断
 * @param port GPIO 端口
 * @param pin 引脚号 (0-15)
 * @return XY_HAL_OK 成功，XY_HAL_ERROR_INVALID_PARAM 参数无效
 */
xy_hal_error_t xy_hal_gpio_detach_irq(void *port, uint8_t pin);

/**
 * @brief 使能 GPIO 中断
 * @param port GPIO 端口
 * @param pin 引脚号 (0-15)
 * @return XY_HAL_OK 成功，XY_HAL_ERROR_INVALID_PARAM 参数无效
 */
xy_hal_error_t xy_hal_gpio_irq_enable(void *port, uint8_t pin);

/**
 * @brief 禁用 GPIO 中断
 * @param port GPIO 端口
 * @param pin 引脚号 (0-15)
 * @return XY_HAL_OK 成功，XY_HAL_ERROR_INVALID_PARAM 参数无效
 */
xy_hal_error_t xy_hal_gpio_irq_disable(void *port, uint8_t pin);

/**
 * @brief 清除 GPIO 中断标志
 * @param port GPIO 端口
 * @param pin 引脚号 (0-15)
 * @return XY_HAL_OK 成功，XY_HAL_ERROR_INVALID_PARAM 参数无效
 */
xy_hal_error_t xy_hal_gpio_clear_irq_flag(void *port, uint8_t pin);

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_GPIO_H */
