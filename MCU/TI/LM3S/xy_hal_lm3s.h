/**
 * @file xy_hal_lm3s.h
 * @brief XinYi HAL - TI LM3S6965 (QEMU) 接口定义
 */

#ifndef __XY_HAL_LM3S_H__
#define __XY_HAL_LM3S_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 *  类型定义
 *===========================================================================*/

/** HAL 状态码 */
typedef enum {
    XY_HAL_STATUS_OK = 0,
    XY_HAL_STATUS_ERROR,
    XY_HAL_STATUS_TIMEOUT,
    XY_HAL_STATUS_BUSY
} xy_hal_status_t;

/** GPIO 引脚定义 */
typedef enum {
    XY_HAL_GPIO_PIN_0  = 0x01,
    XY_HAL_GPIO_PIN_1  = 0x02,
    XY_HAL_GPIO_PIN_2  = 0x04,
    XY_HAL_GPIO_PIN_3  = 0x08,
    XY_HAL_GPIO_PIN_4  = 0x10,
    XY_HAL_GPIO_PIN_5  = 0x20,
    XY_HAL_GPIO_PIN_6  = 0x40,
    XY_HAL_GPIO_PIN_7  = 0x80,
    XY_HAL_GPIO_PIN_ALL = 0xFF
} xy_hal_gpio_pin_t;

/** GPIO 模式 */
typedef enum {
    XY_HAL_GPIO_MODE_INPUT,
    XY_HAL_GPIO_MODE_OUTPUT,
    XY_HAL_GPIO_MODE_AF,
    XY_HAL_GPIO_MODE_ANALOG
} xy_hal_gpio_mode_t;

/** GPIO 上下拉 */
typedef enum {
    XY_HAL_GPIO_PULL_NONE,
    XY_HAL_GPIO_PULL_UP,
    XY_HAL_GPIO_PULL_DOWN
} xy_hal_gpio_pull_t;

/** GPIO 速度 */
typedef enum {
    XY_HAL_GPIO_SPEED_LOW,
    XY_HAL_GPIO_SPEED_MEDIUM,
    XY_HAL_GPIO_SPEED_HIGH
} xy_hal_gpio_speed_t;

/** GPIO 配置结构 */
typedef struct {
    xy_hal_gpio_mode_t mode;
    xy_hal_gpio_pull_t pull;
    xy_hal_gpio_speed_t speed;
} xy_hal_gpio_config_t;

/** GPIO 句柄 */
typedef uint16_t xy_hal_gpio_t;
#define XY_HAL_GPIO_INVALID  0xFFFF

/** UART 定义 */
typedef enum {
    XY_HAL_UART_0 = 0,
    XY_HAL_UART_1,
    XY_HAL_UART_2,
    XY_HAL_UART_INVALID
} xy_hal_uart_t;

/** UART 校验位 */
typedef enum {
    XY_HAL_UART_PARITY_NONE,
    XY_HAL_UART_PARITY_EVEN,
    XY_HAL_UART_PARITY_ODD
} xy_hal_uart_parity_t;

/** UART 配置结构 */
typedef struct {
    uint32_t baudrate;
    uint8_t data_bits;
    uint8_t stop_bits;
    xy_hal_uart_parity_t parity;
} xy_hal_uart_config_t;

/*============================================================================
 *  系统函数
 *===========================================================================*/

/**
 * @brief 系统初始化
 */
void xy_hal_system_init(void);

/*============================================================================
 *  GPIO 函数
 *===========================================================================*/

/**
 * @brief 绑定 GPIO 引脚
 * @param pin_name 引脚名称，如 "GPIOA.5" 或 "PA5"
 * @return GPIO 句柄
 */
xy_hal_gpio_t xy_hal_gpio_bind(const char *pin_name);

/**
 * @brief 配置 GPIO
 * @param gpio GPIO 句柄
 * @param pins 引脚掩码
 * @param config 配置参数
 * @return 状态码
 */
xy_hal_status_t xy_hal_gpio_configure(xy_hal_gpio_t gpio, 
                                       xy_hal_gpio_pin_t pins,
                                       const xy_hal_gpio_config_t *config);

/**
 * @brief 写入 GPIO
 * @param gpio GPIO 句柄
 * @param pins 引脚掩码
 * @param value 值 (0 或 1)
 * @return 状态码
 */
xy_hal_status_t xy_hal_gpio_write(xy_hal_gpio_t gpio, 
                                   xy_hal_gpio_pin_t pins,
                                   uint8_t value);

/**
 * @brief 读取 GPIO
 * @param gpio GPIO 句柄
 * @param pins 引脚掩码
 * @param value 输出值
 * @return 状态码
 */
xy_hal_status_t xy_hal_gpio_read(xy_hal_gpio_t gpio, 
                                  xy_hal_gpio_pin_t pins,
                                  uint8_t *value);

/**
 * @brief 翻转 GPIO
 * @param gpio GPIO 句柄
 * @param pins 引脚掩码
 * @return 状态码
 */
xy_hal_status_t xy_hal_gpio_toggle(xy_hal_gpio_t gpio, 
                                    xy_hal_gpio_pin_t pins);

/*============================================================================
 *  UART 函数
 *===========================================================================*/

/**
 * @brief 绑定 UART 端口
 * @param uart_name UART 名称，如 "UART0"
 * @return UART 句柄
 */
xy_hal_uart_t xy_hal_uart_bind(const char *uart_name);

/**
 * @brief 配置 UART
 * @param uart UART 句柄
 * @param config 配置参数
 * @return 状态码
 */
xy_hal_status_t xy_hal_uart_configure(xy_hal_uart_t uart, 
                                       const xy_hal_uart_config_t *config);

/**
 * @brief UART 发送
 * @param uart UART 句柄
 * @param data 数据缓冲区
 * @param length 数据长度
 * @return 状态码
 */
xy_hal_status_t xy_hal_uart_write(xy_hal_uart_t uart, 
                                   const uint8_t *data, 
                                   uint16_t length);

/**
 * @brief UART 接收
 * @param uart UART 句柄
 * @param data 数据缓冲区
 * @param length 数据长度
 * @return 状态码
 */
xy_hal_status_t xy_hal_uart_read(xy_hal_uart_t uart, 
                                  uint8_t *data, 
                                  uint16_t length);

/*============================================================================
 *  延时函数
 *===========================================================================*/

/**
 * @brief 毫秒延时
 * @param ms 毫秒数
 */
void xy_hal_delay_ms(uint32_t ms);

/**
 * @brief 微秒延时
 * @param us 微秒数
 */
void xy_hal_delay_us(uint32_t us);

#ifdef __cplusplus
}
#endif

#endif /* __XY_HAL_LM3S_H__ */
