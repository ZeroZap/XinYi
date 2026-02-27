/**
 * @file xy_hal_adc.h
 * @brief ADC (Analog-to-Digital Converter) Hardware Abstraction Layer
 * @version 2.0
 * @date 2026-02-27
 */

#ifndef XY_HAL_ADC_H
#define XY_HAL_ADC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_hal.h"
#include <stdint.h>
#include <stddef.h>

/**
 * @brief ADC 分辨率
 */
typedef enum {
    XY_HAL_ADC_RESOLUTION_6B = 0,   /**< 6 位分辨率 */
    XY_HAL_ADC_RESOLUTION_8B,       /**< 8 位分辨率 */
    XY_HAL_ADC_RESOLUTION_10B,      /**< 10 位分辨率 */
    XY_HAL_ADC_RESOLUTION_12B,      /**< 12 位分辨率 */
    XY_HAL_ADC_RESOLUTION_14B,      /**< 14 位分辨率 */
    XY_HAL_ADC_RESOLUTION_16B,      /**< 16 位分辨率 */
} xy_hal_adc_resolution_t;

/**
 * @brief ADC 数据对齐方式
 */
typedef enum {
    XY_HAL_ADC_DATAALIGN_RIGHT = 0, /**< 右对齐 */
    XY_HAL_ADC_DATAALIGN_LEFT,      /**< 左对齐 */
} xy_hal_adc_align_t;

/**
 * @brief ADC 扫描模式
 */
typedef enum {
    XY_HAL_ADC_SCAN_DISABLE = 0,    /**< 禁用扫描模式 */
    XY_HAL_ADC_SCAN_ENABLE,         /**< 启用扫描模式 */
} xy_hal_adc_scan_t;

/**
 * @brief ADC 连续转换模式
 */
typedef enum {
    XY_HAL_ADC_CONTINUOUS_DISABLE = 0,  /**< 单次转换 */
    XY_HAL_ADC_CONTINUOUS_ENABLE,       /**< 连续转换 */
} xy_hal_adc_continuous_t;

/**
 * @brief ADC 触发源
 */
typedef enum {
    XY_HAL_ADC_TRIGGER_SOFTWARE = 0,    /**< 软件触发 */
    XY_HAL_ADC_TRIGGER_EXTI,            /**< 外部中断触发 */
    XY_HAL_ADC_TRIGGER_TIMER,           /**< 定时器触发 */
} xy_hal_adc_trigger_src_t;

/**
 * @brief ADC 配置结构
 */
typedef struct {
    xy_hal_adc_resolution_t resolution;     /**< 分辨率 */
    xy_hal_adc_align_t align;               /**< 数据对齐 */
    xy_hal_adc_scan_t scan_mode;            /**< 扫描模式 */
    xy_hal_adc_continuous_t continuous;     /**< 连续转换模式 */
    xy_hal_adc_trigger_src_t trigger_src;   /**< 触发源 */
    uint32_t clock_div;                     /**< 时钟分频 */
    uint32_t sampling_time;                 /**< 采样时间 */
} xy_hal_adc_config_t;

/**
 * @brief ADC 通道配置
 */
typedef struct {
    uint8_t channel;        /**< 通道号 */
    uint8_t rank;           /**< 转换顺序 */
    uint32_t sampling_time; /**< 采样时间 */
} xy_hal_adc_channel_t;

/**
 * @brief ADC 事件类型
 */
typedef enum {
    XY_HAL_ADC_EVENT_EOC = 0,       /**< 转换完成 */
    XY_HAL_ADC_EVENT_OVR,           /**< 溢出 */
    XY_HAL_ADC_EVENT_AWD,           /**< 模拟看门狗 */
    XY_HAL_ADC_EVENT_JEOC,          /**< 注入转换完成 */
} xy_hal_adc_event_t;

/**
 * @brief ADC 回调类型
 */
typedef void (*xy_hal_adc_callback_t)(void *adc, xy_hal_adc_event_t event, void *arg);

/**
 * @brief 初始化 ADC
 * @param adc ADC 实例
 * @param config ADC 配置
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_init(void *adc, const xy_hal_adc_config_t *config);

/**
 * @brief 反初始化 ADC
 * @param adc ADC 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_deinit(void *adc);

/**
 * @brief 配置 ADC 通道
 * @param adc ADC 实例
 * @param channels 通道配置数组
 * @param count 通道数量
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_config_channels(void *adc,
                                          const xy_hal_adc_channel_t *channels,
                                          size_t count);

/**
 * @brief 启动 ADC 转换
 * @param adc ADC 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_start(void *adc);

/**
 * @brief 停止 ADC 转换
 * @param adc ADC 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_stop(void *adc);

/**
 * @brief 读取 ADC 转换值 (阻塞)
 * @param adc ADC 实例
 * @param channel 通道号
 * @param timeout 超时时间 (ms)
 * @return 转换结果，负值表示错误
 */
int xy_hal_adc_read(void *adc, uint8_t channel, uint32_t timeout);

/**
 * @brief 读取 ADC 转换值 (非阻塞)
 * @param adc ADC 实例
 * @param channel 通道号
 * @return 转换结果，负值表示错误
 */
int xy_hal_adc_read_nb(void *adc, uint8_t channel);

/**
 * @brief 多通道 ADC 读取 (DMA 方式)
 * @param adc ADC 实例
 * @param buffer 数据缓冲区
 * @param count 通道数量
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_read_dma(void *adc, uint32_t *buffer, size_t count);

/**
 * @brief 获取 ADC 分辨率 (位)
 * @param adc ADC 实例
 * @return 分辨率 (位)，负值表示错误
 */
int xy_hal_adc_get_resolution(void *adc);

/**
 * @brief 获取 ADC 最大值
 * @param adc ADC 实例
 * @return 最大转换值 (如 4095 对应 12 位)
 */
uint32_t xy_hal_adc_get_max_value(void *adc);

/**
 * @brief ADC 值转电压
 * @param adc ADC 实例
 * @param value ADC 值
 * @param vref 参考电压 (mV)
 * @return 电压值 (mV)
 */
uint32_t xy_hal_adc_value_to_mv(void *adc, uint32_t value, uint32_t vref);

/**
 * @brief 使能 ADC 中断
 * @param adc ADC 实例
 * @param event 中断事件类型
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_enable_irq(void *adc, xy_hal_adc_event_t event);

/**
 * @brief 禁用 ADC 中断
 * @param adc ADC 实例
 * @param event 中断事件类型
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_disable_irq(void *adc, xy_hal_adc_event_t event);

/**
 * @brief 注册 ADC 回调
 * @param adc ADC 实例
 * @param event 事件类型
 * @param callback 回调函数
 * @param arg 用户参数
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_register_callback(void *adc, xy_hal_adc_event_t event,
                                            xy_hal_adc_callback_t callback, void *arg);

/**
 * @brief 使能温度传感器
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_enable_temp_sensor(void);

/**
 * @brief 禁用温度传感器
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_disable_temp_sensor(void);

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_ADC_H */
