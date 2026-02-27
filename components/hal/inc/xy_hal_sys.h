/**
 * @file xy_hal_sys.h
 * @brief System Hardware Abstraction Layer
 * @version 2.0
 * @date 2026-02-27
 */

#ifndef XY_HAL_SYS_H
#define XY_HAL_SYS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief 基本类型定义
 */
typedef uint8_t     xy_u8_t;
typedef int8_t      xy_s8_t;
typedef uint16_t    xy_u16_t;
typedef int16_t     xy_s16_t;
typedef uint32_t    xy_u32_t;
typedef int32_t     xy_s32_t;
typedef uint64_t    xy_u64_t;
typedef int64_t     xy_s64_t;
typedef float       xy_f32_t;
typedef double      xy_f64_t;

/**
 * @brief 系统复位源
 */
typedef enum {
    XY_HAL_RESET_SRC_UNKNOWN = 0,   /**< 未知复位源 */
    XY_HAL_RESET_SRC_POWER_ON,      /**< 上电复位 */
    XY_HAL_RESET_SRC_PIN,           /**< 外部引脚复位 */
    XY_HAL_RESET_SRC_WWDG,          /**< 窗口看门狗复位 */
    XY_HAL_RESET_SRC_IWDG,          /**< 独立看门狗复位 */
    XY_HAL_RESET_SRC_SW,            /**< 软件复位 */
    XY_HAL_RESET_SRC_LPWAKE,        /**< 低功耗唤醒 */
    XY_HAL_RESET_SRC_BOR,           /**< 欠压复位 */
} xy_hal_reset_src_t;

/**
 * @brief 系统时钟配置结构
 */
typedef struct {
    uint32_t sysclk_freq;       /**< 系统时钟频率 (Hz) */
    uint32_t hclk_freq;         /**< AHB 总线时钟频率 (Hz) */
    uint32_t pclk1_freq;        /**< APB1 总线时钟频率 (Hz) */
    uint32_t pclk2_freq;        /**< APB2 总线时钟频率 (Hz) */
    uint32_t systick_freq;      /**< SysTick 时钟频率 (Hz) */
} xy_hal_sys_clock_info_t;

/**
 * @brief 获取系统滴答定时器当前值
 * @return 当前滴答计数值
 */
xy_u32_t xy_hal_sys_tick_get(void);

/**
 * @brief 获取自启动以来的滴答数
 * @return 自启动以来的滴答数
 */
xy_u32_t xy_hal_sys_tick_now(void);

/**
 * @brief 计算经过的滴答数
 * @param tick 起始滴答值
 * @return 经过的滴答数
 */
static inline xy_u32_t xy_hal_sys_tick_since(xy_u32_t tick) {
    return xy_hal_sys_tick_now() - tick;
}

/**
 * @brief 检查是否经过指定滴答数
 * @param tick 起始滴答值
 * @param interval 时间间隔
 * @return 1 已超时，0 未超时
 */
static inline int xy_hal_sys_tick_elapsed(xy_u32_t tick, xy_u32_t interval) {
    return (xy_hal_sys_tick_since(tick) >= interval);
}

/**
 * @brief 微秒级延时
 * @param us 延时微秒数
 */
void xy_hal_sys_delay_us(uint32_t us);

/**
 * @brief 毫秒级延时
 * @param ms 延时毫秒数
 */
void xy_hal_sys_delay_ms(uint32_t ms);

/**
 * @brief 系统复位
 * @param reset_by 复位原因码
 * @return XY_HAL_OK 成功，其他值失败
 */
int xy_hal_sys_reset(int reset_by);

/**
 * @brief 获取复位原因
 * @param data 输出复位原因数据
 * @return XY_HAL_OK 成功，其他值失败
 */
int xy_hal_sys_reset_reason(void *data);

/**
 * @brief 获取芯片 ID
 * @return 芯片 ID，负值表示失败
 */
int xy_hal_sys_get_chip_id(void);

/**
 * @brief 获取芯片版本号
 * @return 版本号，负值表示失败
 */
int xy_hal_sys_get_chip_ver(void);

/**
 * @brief 获取芯片名称
 * @param name 名称输出缓冲区
 * @param len 缓冲区长度
 * @return XY_HAL_OK 成功，其他值失败
 */
int xy_hal_sys_get_chip_name(char *name, int len);

/**
 * @brief 获取芯片 MAC 地址 (二进制)
 * @param mac MAC 地址输出缓冲区 (6 字节)
 * @return XY_HAL_OK 成功，其他值失败
 */
int xy_hal_sys_get_chip_mac(uint8_t *mac);

/**
 * @brief 获取芯片 MAC 地址 (字符串格式)
 * @param mac MAC 地址输出缓冲区
 * @return XY_HAL_OK 成功，其他值失败
 */
int xy_hal_sys_get_chip_mac_str(char *mac);

/**
 * @brief 获取芯片 MAC 地址 (十六进制字符串格式)
 * @param mac MAC 地址输出缓冲区
 * @return XY_HAL_OK 成功，其他值失败
 */
int xy_hal_sys_get_chip_mac_hex_str(char *mac);

/**
 * @brief 获取系统时钟信息
 * @param info 时钟信息输出结构
 * @return XY_HAL_OK 成功，其他值失败
 */
int xy_hal_sys_get_clock_info(xy_hal_sys_clock_info_t *info);

/**
 * @brief 使能外设时钟
 * @param peripheral 外设 ID
 * @return XY_HAL_OK 成功，其他值失败
 */
int xy_hal_sys_enable_periph_clock(uint32_t peripheral);

/**
 * @brief 禁用外设时钟
 * @param peripheral 外设 ID
 * @return XY_HAL_OK 成功，其他值失败
 */
int xy_hal_sys_disable_periph_clock(uint32_t peripheral);

/**
 * @brief 进入睡眠模式
 * @return XY_HAL_OK 成功，其他值失败
 */
int xy_hal_sys_enter_sleep(void);

/**
 * @brief 进入停机模式
 * @return XY_HAL_OK 成功，其他值失败
 */
int xy_hal_sys_enter_stop(void);

/**
 * @brief 进入待机模式
 * @return XY_HAL_OK 成功，其他值失败
 */
int xy_hal_sys_enter_standby(void);

/**
 * @brief 退出低功耗模式
 * @return XY_HAL_OK 成功，其他值失败
 */
int xy_hal_sys_exit_lowpower(void);

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_SYS_H */
