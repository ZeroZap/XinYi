/**
 * @file xy_hal_i2s.h
 * @brief I2S (Inter-IC Sound) Hardware Abstraction Layer
 * @version 2.0
 * @date 2026-02-27
 */

#ifndef XY_HAL_I2S_H
#define XY_HAL_I2S_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_hal.h"
#include <stdint.h>
#include <stddef.h>

/**
 * @brief I2S 模式
 */
typedef enum {
    XY_HAL_I2S_MODE_SLAVE_TX = 0,   /**< 从机发送 */
    XY_HAL_I2S_MODE_SLAVE_RX,       /**< 从机接收 */
    XY_HAL_I2S_MODE_MASTER_TX,      /**< 主机发送 */
    XY_HAL_I2S_MODE_MASTER_RX,      /**< 主机接收 */
} xy_hal_i2s_mode_t;

/**
 * @brief I2S 标准
 */
typedef enum {
    XY_HAL_I2S_STD_PHILIPS = 0,     /**< Philips 标准 */
    XY_HAL_I2S_STD_MSB,             /**< MSB 先传标准 */
    XY_HAL_I2S_STD_LSB,             /**< LSB 先传标准 */
    XY_HAL_I2S_STD_PCM_SHORT,       /**< PCM 短帧 */
    XY_HAL_I2S_STD_PCM_LONG,        /**< PCM 长帧 */
} xy_hal_i2s_std_t;

/**
 * @brief I2S 数据格式
 */
typedef enum {
    XY_HAL_I2S_DATAFORMAT_16B = 0,      /**< 16 位数据 */
    XY_HAL_I2S_DATAFORMAT_16B_EXTENDED, /**< 16 位扩展 */
    XY_HAL_I2S_DATAFORMAT_24B,          /**< 24 位数据 */
    XY_HAL_I2S_DATAFORMAT_32B,          /**< 32 位数据 */
} xy_hal_i2s_dataformat_t;

/**
 * @brief I2S MCLK 输出
 */
typedef enum {
    XY_HAL_I2S_MCLK_DISABLE = 0,    /**< 禁用 MCLK */
    XY_HAL_I2S_MCLK_ENABLE,         /**< 使能 MCLK */
} xy_hal_i2s_mclk_t;

/**
 * @brief I2S 时钟极性
 */
typedef enum {
    XY_HAL_I2S_CPOL_LOW = 0,    /**< CK 低电平空闲 */
    XY_HAL_I2S_CPOL_HIGH,       /**< CK 高电平空闲 */
} xy_hal_i2s_cpol_t;

/**
 * @brief I2S 音频频率
 */
typedef enum {
    XY_HAL_I2S_AUDIOFREQ_8K = 8000,         /**< 8kHz */
    XY_HAL_I2S_AUDIOFREQ_11K = 11025,       /**< 11.025kHz */
    XY_HAL_I2S_AUDIOFREQ_16K = 16000,       /**< 16kHz */
    XY_HAL_I2S_AUDIOFREQ_22K = 22050,       /**< 22.05kHz */
    XY_HAL_I2S_AUDIOFREQ_32K = 32000,       /**< 32kHz */
    XY_HAL_I2S_AUDIOFREQ_44K = 44100,       /**< 44.1kHz */
    XY_HAL_I2S_AUDIOFREQ_48K = 48000,       /**< 48kHz */
    XY_HAL_I2S_AUDIOFREQ_96K = 96000,       /**< 96kHz */
    XY_HAL_I2S_AUDIOFREQ_192K = 192000,     /**< 192kHz */
} xy_hal_i2s_audiofreq_t;

/**
 * @brief I2S 配置结构
 */
typedef struct {
    xy_hal_i2s_mode_t mode;             /**< I2S 模式 */
    xy_hal_i2s_std_t std;               /**< I2S 标准 */
    xy_hal_i2s_dataformat_t dataformat; /**< 数据格式 */
    xy_hal_i2s_mclk_t mclk_output;      /**< MCLK 输出 */
    xy_hal_i2s_cpol_t cpol;             /**< 时钟极性 */
    uint32_t audio_freq;                /**< 音频频率 */
} xy_hal_i2s_config_t;

/**
 * @brief I2S 事件类型
 */
typedef enum {
    XY_HAL_I2S_EVENT_TX_COMPLETE = 0,   /**< 发送完成 */
    XY_HAL_I2S_EVENT_RX_COMPLETE,       /**< 接收完成 */
    XY_HAL_I2S_EVENT_ERROR,             /**< 错误 */
    XY_HAL_I2S_EVENT_OVR,               /**< 溢出 */
    XY_HAL_I2S_EVENT_UDR,               /**< 欠载 */
} xy_hal_i2s_event_t;

/**
 * @brief I2S 回调类型
 */
typedef void (*xy_hal_i2s_callback_t)(void *i2s, xy_hal_i2s_event_t event, void *arg);

/**
 * @brief 初始化 I2S
 * @param i2s I2S 实例
 * @param config I2S 配置
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_init(void *i2s, const xy_hal_i2s_config_t *config);

/**
 * @brief 反初始化 I2S
 * @param i2s I2S 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_deinit(void *i2s);

/**
 * @brief 发送 I2S 数据 (阻塞)
 * @param i2s I2S 实例
 * @param data 数据缓冲区
 * @param size 数据大小 (16 位单元数)
 * @param timeout 超时时间 (ms)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_transmit(void *i2s, const uint16_t *data,
                                   size_t size, uint32_t timeout);

/**
 * @brief 接收 I2S 数据 (阻塞)
 * @param i2s I2S 实例
 * @param data 数据缓冲区
 * @param size 数据大小 (16 位单元数)
 * @param timeout 超时时间 (ms)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_receive(void *i2s, uint16_t *data,
                                  size_t size, uint32_t timeout);

/**
 * @brief 全双工发送接收 (阻塞)
 * @param i2s I2S 实例
 * @param tx_data 发送数据
 * @param rx_data 接收数据
 * @param size 数据大小
 * @param timeout 超时时间 (ms)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_transmit_receive(void *i2s, const uint16_t *tx_data,
                                           uint16_t *rx_data, size_t size,
                                           uint32_t timeout);

/**
 * @brief 发送 I2S 数据 (DMA)
 * @param i2s I2S 实例
 * @param data 数据缓冲区
 * @param size 数据大小
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_transmit_dma(void *i2s, const uint16_t *data, size_t size);

/**
 * @brief 接收 I2S 数据 (DMA)
 * @param i2s I2S 实例
 * @param data 数据缓冲区
 * @param size 数据大小
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_receive_dma(void *i2s, uint16_t *data, size_t size);

/**
 * @brief 停止 DMA 传输
 * @param i2s I2S 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_stop_dma(void *i2s);

/**
 * @brief 注册 I2S 回调
 * @param i2s I2S 实例
 * @param callback 回调函数
 * @param arg 用户参数
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_register_callback(void *i2s,
                                            xy_hal_i2s_callback_t callback,
                                            void *arg);

/**
 * @brief 使能 I2S
 * @param i2s I2S 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_enable(void *i2s);

/**
 * @brief 禁用 I2S
 * @param i2s I2S 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_disable(void *i2s);

/**
 * @brief 暂停 I2S DMA 传输
 * @param i2s I2S 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_pause_dma(void *i2s);

/**
 * @brief 恢复 I2S DMA 传输
 * @param i2s I2S 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_i2s_resume_dma(void *i2s);

/**
 * @brief 获取 I2S 错误码
 * @param i2s I2S 实例
 * @return 错误码
 */
uint32_t xy_hal_i2s_get_error_code(void *i2s);

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_I2S_H */
