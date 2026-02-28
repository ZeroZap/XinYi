/**
 * @file xy_device.h
 * @brief Device Driver Framework main header
 * @version 1.0.0
 * @date 2026-02-28
 */

#ifndef XY_DEVICE_H
#define XY_DEVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Device 版本宏定义
 */
#define XY_DEVICE_VERSION_MAJOR    1
#define XY_DEVICE_VERSION_MINOR    0
#define XY_DEVICE_VERSION_PATCH    0

/**
 * @brief Device 错误码
 */
typedef enum {
    XY_DEVICE_OK              = 0,
    XY_DEVICE_ERROR           = -1,
    XY_DEVICE_INVALID_PARAM   = -2,
    XY_DEVICE_NOT_SUPPORT     = -3,
    XY_DEVICE_TIMEOUT         = -4,
    XY_DEVICE_BUSY            = -5,
    XY_DEVICE_NO_MEM          = -6,
    XY_DEVICE_IO_ERROR        = -7,
    XY_DEVICE_NOT_INIT        = -8,
    XY_DEVICE_NOT_FOUND       = -9,
} xy_device_error_t;

/**
 * @brief Device 类型枚举
 */
typedef enum {
    XY_DEVICE_TYPE_NONE = 0,
    XY_DEVICE_TYPE_I2C,
    XY_DEVICE_TYPE_SPI,
    XY_DEVICE_TYPE_UART,
    XY_DEVICE_TYPE_GPIO,
    XY_DEVICE_TYPE_ADC,
    XY_DEVICE_TYPE_DAC,
    XY_DEVICE_TYPE_TIMER,
    XY_DEVICE_TYPE_SENSOR,
    XY_DEVICE_TYPE_DISPLAY,
    XY_DEVICE_TYPE_MEMORY,
} xy_device_type_t;

/**
 * @brief Device 基类结构
 */
typedef struct {
    const char *name;
    xy_device_type_t type;
    void *bus;
    uint16_t address;
    bool initialized;
    void *user_data;
} xy_device_t;

/**
 * @brief Device 操作接口
 */
typedef struct {
    int (*init)(xy_device_t *dev);
    int (*deinit)(xy_device_t *dev);
    int (*read)(xy_device_t *dev, uint8_t *data, size_t len);
    int (*write)(xy_device_t *dev, const uint8_t *data, size_t len);
    int (*ioctl)(xy_device_t *dev, int cmd, void *arg);
} xy_device_ops_t;

/* ==================== I2C Device ==================== */

/**
 * @brief I2C 设备结构
 */
typedef struct {
    xy_device_t base;
    void *i2c_handle;
    uint16_t dev_addr;
    uint32_t timeout;
} xy_i2c_device_t;

/**
 * @brief I2C 设备初始化
 */
int xy_i2c_device_init(xy_i2c_device_t *dev, void *i2c_handle, 
                       uint16_t addr, uint32_t timeout);

/**
 * @brief I2C 设备读取寄存器
 */
int xy_i2c_device_read_reg(xy_i2c_device_t *dev, uint8_t reg, 
                           uint8_t *data, size_t len);

/**
 * @brief I2C 设备写入寄存器
 */
int xy_i2c_device_write_reg(xy_i2c_device_t *dev, uint8_t reg, 
                            const uint8_t *data, size_t len);

/**
 * @brief I2C 设备读取数据
 */
int xy_i2c_device_read(xy_i2c_device_t *dev, uint8_t *data, size_t len);

/**
 * @brief I2C 设备写入数据
 */
int xy_i2c_device_write(xy_i2c_device_t *dev, const uint8_t *data, size_t len);

/* ==================== SPI Device ==================== */

/**
 * @brief SPI 设备结构
 */
typedef struct {
    xy_device_t base;
    void *spi_handle;
    void *cs_pin;
    uint32_t speed_hz;
} xy_spi_device_t;

/**
 * @brief SPI 设备初始化
 */
int xy_spi_device_init(xy_spi_device_t *dev, void *spi_handle, 
                       void *cs_pin, uint32_t speed);

/**
 * @brief SPI 设备传输
 */
int xy_spi_device_transfer(xy_spi_device_t *dev, const uint8_t *tx, 
                           uint8_t *rx, size_t len);

/**
 * @brief SPI 设备发送
 */
int xy_spi_device_send(xy_spi_device_t *dev, const uint8_t *data, size_t len);

/**
 * @brief SPI 设备接收
 */
int xy_spi_device_recv(xy_spi_device_t *dev, uint8_t *data, size_t len);

/**
 * @brief SPI 片选控制
 */
void xy_spi_device_cs(xy_spi_device_t *dev, bool select);

/* ==================== UART Device ==================== */

/**
 * @brief UART 设备结构
 */
typedef struct {
    xy_device_t base;
    void *uart_handle;
    uint32_t baudrate;
} xy_uart_device_t;

/**
 * @brief UART 设备初始化
 */
int xy_uart_device_init(xy_uart_device_t *dev, void *uart_handle, 
                        uint32_t baudrate);

/**
 * @brief UART 设备发送
 */
int xy_uart_device_send(xy_uart_device_t *dev, const uint8_t *data, size_t len);

/**
 * @brief UART 设备接收
 */
int xy_uart_device_recv(xy_uart_device_t *dev, uint8_t *data, size_t len);

/**
 * @brief UART 设备打印字符串
 */
int xy_uart_device_printf(xy_uart_device_t *dev, const char *fmt, ...);

/* ==================== GPIO Device ==================== */

/**
 * @brief GPIO 模式
 */
typedef enum {
    XY_GPIO_MODE_INPUT = 0,
    XY_GPIO_MODE_OUTPUT,
    XY_GPIO_MODE_AF,
    XY_GPIO_MODE_ANALOG,
} xy_gpio_mode_t;

/**
 * @brief GPIO 上下拉
 */
typedef enum {
    XY_GPIO_PULL_NONE = 0,
    XY_GPIO_PULL_UP,
    XY_GPIO_PULL_DOWN,
} xy_gpio_pull_t;

/**
 * @brief GPIO 设备结构
 */
typedef struct {
    xy_device_t base;
    void *gpio_port;
    uint16_t gpio_pin;
    xy_gpio_mode_t mode;
    xy_gpio_pull_t pull;
} xy_gpio_device_t;

/**
 * @brief GPIO 设备初始化
 */
int xy_gpio_device_init(xy_gpio_device_t *dev, void *port, uint16_t pin,
                        xy_gpio_mode_t mode, xy_gpio_pull_t pull);

/**
 * @brief GPIO 设置
 */
void xy_gpio_device_set(xy_gpio_device_t *dev, bool value);

/**
 * @brief GPIO 读取
 */
bool xy_gpio_device_get(xy_gpio_device_t *dev);

/**
 * @brief GPIO 切换
 */
void xy_gpio_device_toggle(xy_gpio_device_t *dev);

/* ==================== Device Manager ==================== */

/**
 * @brief 设备管理器结构
 */
typedef struct {
    xy_device_t **devices;
    size_t count;
    size_t max_count;
} xy_device_manager_t;

/**
 * @brief 初始化设备管理器
 */
int xy_device_manager_init(xy_device_manager_t *mgr, size_t max_count);

/**
 * @brief 注册设备
 */
int xy_device_manager_register(xy_device_manager_t *mgr, xy_device_t *dev);

/**
 * @brief 注销设备
 */
int xy_device_manager_unregister(xy_device_manager_t *mgr, xy_device_t *dev);

/**
 * @brief 查找设备
 */
xy_device_t *xy_device_manager_find(xy_device_manager_t *mgr, const char *name);

/**
 * @brief 遍历设备
 */
int xy_device_manager_foreach(xy_device_manager_t *mgr, 
                              int (*callback)(xy_device_t *dev, void *arg),
                              void *arg);

#ifdef __cplusplus
}
#endif

#endif /* XY_DEVICE_H */
