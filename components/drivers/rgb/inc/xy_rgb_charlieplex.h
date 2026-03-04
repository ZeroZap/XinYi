/**
 * @file xy_rgb_charlieplex.h
 * @brief Charlieplexing LED Matrix Driver - 高效复用技术
 * @version 1.0.0
 * @date 2026-03-02
 * 
 * 特性:
 * - N 个 IO 控制 N*(N-1) 个 LED
 * - 支持 PWM 亮度控制
 * - BAM (Bit Angle Modulation) 软件调光
 * - 支持 Charlieplexing 和矩阵扫描
 */

#ifndef XY_RGB_CHARLIEPLEX_H
#define XY_RGB_CHARLIEPLEX_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 配置 ==================== */

/**
 * @brief IO 数量 (决定可控制 LED 数量)
 * 
 * IO 数    LED 数
 * 2        2
 * 3        6
 * 4        12
 * 5        20
 * 6        30
 * 8        56
 */
#ifndef CHARLIE_IO_NUM
#define CHARLIE_IO_NUM      4       // 4 IO = 12 LED
#endif

#define CHARLIE_LED_NUM     (CHARLIE_IO_NUM * (CHARLIE_IO_NUM - 1))

/**
 * @brief PWM 配置
 */
#define CHARLIE_PWM_BITS    8       // 8 位 PWM = 256 级亮度
#define CHARLIE_PWM_LEVELS  (1 << CHARLIE_PWM_BITS)

/**
 * @brief 扫描频率 (Hz)
 * 需要 > 100Hz 以避免闪烁
 */
#define CHARLIE_SCAN_FREQ   200

/* ==================== 数据结构 ==================== */

/**
 * @brief IO 状态
 */
typedef enum {
    CHARLIE_IO_HIZ = 0,     // 高阻态 (输入)
    CHARLIE_IO_LOW,         // 低电平
    CHARLIE_IO_HIGH,        // 高电平
    CHARLIE_IO_PWM,         // PWM 输出
} xy_charlie_io_state_t;

/**
 * @brief LED 连接定义
 */
typedef struct {
    uint8_t anode_pin;      // 阳极 IO 索引
    uint8_t cathode_pin;    // 阴极 IO 索引
} xy_charlie_led_t;

/**
 * @brief Charlieplex 管理器
 */
typedef struct {
    xy_charlie_led_t leds[CHARLIE_LED_NUM]; // LED 映射
    uint8_t brightness[CHARLIE_LED_NUM];    // 亮度缓冲区
    uint8_t current_led;                    // 当前扫描 LED
    bool enabled;                           // 使能
    uint8_t io_pins[CHARLIE_IO_NUM];        // IO 引脚映射
    void (*set_io_state)(uint8_t pin, xy_charlie_io_state_t state);
    void (*set_pwm_duty)(uint8_t pin, uint8_t duty);
} xy_charlieplex_t;

/* ==================== API ==================== */

/**
 * @brief 初始化 Charlieplex
 * @param mgr Charlieplex 管理器
 * @param io_pins IO 引脚数组
 * @param set_io_state IO 状态设置回调
 * @param set_pwm_duty PWM 占空比设置回调
 * @return 0 成功，其他值失败
 */
int xy_charlieplex_init(xy_charlieplex_t *mgr, 
                        uint8_t *io_pins,
                        void (*set_io_state)(uint8_t, xy_charlie_io_state_t),
                        void (*set_pwm_duty)(uint8_t, uint8_t));

/**
 * @brief 设置 LED 亮度
 * @param mgr Charlieplex 管理器
 * @param led_index LED 索引
 * @param brightness 亮度 (0-255)
 */
void xy_charlieplex_set_brightness(xy_charlieplex_t *mgr,
                                   uint8_t led_index,
                                   uint8_t brightness);

/**
 * @brief 设置所有 LED 亮度
 * @param mgr Charlieplex 管理器
 * @param brightness 亮度 (0-255)
 */
void xy_charlieplex_set_all(xy_charlieplex_t *mgr, uint8_t brightness);

/**
 * @brief 清除所有 LED
 * @param mgr Charlieplex 管理器
 */
void xy_charlieplex_clear(xy_charlieplex_t *mgr);

/**
 * @brief 扫描服务 (在中断或主循环中调用)
 * @param mgr Charlieplex 管理器
 */
void xy_charlieplex_scan(xy_charlieplex_t *mgr);

/**
 * @brief 使能/禁用显示
 * @param mgr Charlieplex 管理器
 * @param enable true=使能，false=禁用
 */
void xy_charlieplex_enable(xy_charlieplex_t *mgr, bool enable);

/**
 * @brief 获取 LED 数量
 * @param mgr Charlieplex 管理器
 * @return LED 数量
 */
uint16_t xy_charlieplex_get_led_count(xy_charlieplex_t *mgr);

/* ==================== 效果 API ==================== */

/**
 * @brief 呼吸灯效果
 * @param mgr Charlieplex 管理器
 * @param led_index LED 索引
 * @param period_ms 周期 (ms)
 */
void xy_charlieplex_effect_breath(xy_charlieplex_t *mgr,
                                  uint8_t led_index,
                                  uint16_t period_ms);

/**
 * @brief 流水灯效果
 * @param mgr Charlieplex 管理器
 * @param speed 速度
 * @param direction 方向 (0=正向，1=反向)
 */
void xy_charlieplex_effect_marquee(xy_charlieplex_t *mgr,
                                   uint8_t speed,
                                   uint8_t direction);

/**
 * @brief 随机闪烁效果
 * @param mgr Charlieplex 管理器
 * @param density 密度 (0-255)
 */
void xy_charlieplex_effect_twinkle(xy_charlieplex_t *mgr, uint8_t density);

#ifdef __cplusplus
}
#endif

#endif /* XY_RGB_CHARLIEPLEX_H */
