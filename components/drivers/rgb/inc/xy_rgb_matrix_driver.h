/**
 * @file xy_rgb_matrix_driver.h
 * @brief LED Matrix Driver - 多路复用 + PWM
 * @version 1.0.0
 * @date 2026-03-02
 * 
 * 特性:
 * - 行/列矩阵扫描
 * - 硬件 PWM 亮度控制
 * - Gamma 校正
 * - 双缓冲防撕裂
 * - 支持多种效果
 */

#ifndef XY_RGB_MATRIX_DRIVER_H
#define XY_RGB_MATRIX_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 配置 ==================== */

/**
 * @brief 矩阵尺寸
 */
#ifndef MATRIX_ROWS
#define MATRIX_ROWS     8       // 8 行
#endif

#ifndef MATRIX_COLS
#define MATRIX_COLS     8       // 8 列
#endif

#define MATRIX_LED_NUM  (MATRIX_ROWS * MATRIX_COLS)

/**
 * @brief PWM 配置
 */
#define MATRIX_PWM_BITS     8   // 8 位 PWM
#define MATRIX_PWM_LEVELS   (1 << MATRIX_PWM_BITS)

/**
 * @brief 扫描频率
 */
#define MATRIX_SCAN_FREQ    200 // 200Hz 行扫描

/**
 * @brief Gamma 校正表大小
 */
#define GAMMA_TABLE_SIZE    256

/* ==================== 数据结构 ==================== */

/**
 * @brief RGB 颜色 (带 PWM)
 */
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} xy_matrix_color_t;

/**
 * @brief 矩阵管理器
 */
typedef struct {
    xy_matrix_color_t buffer[MATRIX_ROWS][MATRIX_COLS]; // 显示缓冲区
    xy_matrix_color_t current_row[MATRIX_COLS];         // 当前行缓冲区
    uint8_t current_row_index;                          // 当前扫描行
    uint8_t brightness;                                 // 全局亮度
    bool enabled;                                       // 使能
    bool dirty;                                         // 需要更新
    
    // 硬件回调
    void (*select_row)(uint8_t row);
    void (*set_col_pwm)(uint8_t col, uint8_t r, uint8_t g, uint8_t b);
    void (*clear_all)(void);
} xy_matrix_driver_t;

/* ==================== API ==================== */

/**
 * @brief 初始化矩阵驱动
 * @param drv 矩阵管理器
 * @param select_row 行选择回调
 * @param set_col_pwm 列 PWM 设置回调
 * @param clear_all 清除所有回调
 * @return 0 成功，其他值失败
 */
int xy_matrix_init(xy_matrix_driver_t *drv,
                   void (*select_row)(uint8_t),
                   void (*set_col_pwm)(uint8_t, uint8_t, uint8_t, uint8_t),
                   void (*clear_all)(void));

/**
 * @brief 设置像素颜色
 * @param drv 矩阵管理器
 * @param x X 坐标
 * @param y Y 坐标
 * @param color 颜色
 */
void xy_matrix_set_pixel(xy_matrix_driver_t *drv,
                         uint8_t x, uint8_t y,
                         xy_matrix_color_t color);

/**
 * @brief 获取像素颜色
 * @param drv 矩阵管理器
 * @param x X 坐标
 * @param y Y 坐标
 * @return 颜色
 */
xy_matrix_color_t xy_matrix_get_pixel(xy_matrix_driver_t *drv,
                                      uint8_t x, uint8_t y);

/**
 * @brief 绘制水平线
 * @param drv 矩阵管理器
 * @param x0 起始 X
 * @param y Y 坐标
 * @param x1 结束 X
 * @param color 颜色
 */
void xy_matrix_draw_hline(xy_matrix_driver_t *drv,
                          uint8_t x0, uint8_t y, uint8_t x1,
                          xy_matrix_color_t color);

/**
 * @brief 绘制垂直线
 * @param drv 矩阵管理器
 * @param x X 坐标
 * @param y0 起始 Y
 * @param y1 结束 Y
 * @param color 颜色
 */
void xy_matrix_draw_vline(xy_matrix_driver_t *drv,
                          uint8_t x, uint8_t y0, uint8_t y1,
                          xy_matrix_color_t color);

/**
 * @brief 绘制直线 (Bresenham 算法)
 * @param drv 矩阵管理器
 * @param x0 起始 X
 * @param y0 起始 Y
 * @param x1 结束 X
 * @param y1 结束 Y
 * @param color 颜色
 */
void xy_matrix_draw_line(xy_matrix_driver_t *drv,
                         uint8_t x0, uint8_t y0,
                         uint8_t x1, uint8_t y1,
                         xy_matrix_color_t color);

/**
 * @brief 绘制矩形
 * @param drv 矩阵管理器
 * @param x X 坐标
 * @param y Y 坐标
 * @param w 宽度
 * @param h 高度
 * @param color 颜色
 * @param filled 是否填充
 */
void xy_matrix_draw_rect(xy_matrix_driver_t *drv,
                         uint8_t x, uint8_t y,
                         uint8_t w, uint8_t h,
                         xy_matrix_color_t color,
                         bool filled);

/**
 * @brief 绘制圆形 (中点算法)
 * @param drv 矩阵管理器
 * @param x0 圆心 X
 * @param y0 圆心 Y
 * @param radius 半径
 * @param color 颜色
 * @param filled 是否填充
 */
void xy_matrix_draw_circle(xy_matrix_driver_t *drv,
                           uint8_t x0, uint8_t y0,
                           uint8_t radius,
                           xy_matrix_color_t color,
                           bool filled);

/**
 * @brief 清除屏幕
 * @param drv 矩阵管理器
 */
void xy_matrix_clear(xy_matrix_driver_t *drv);

/**
 * @brief 填充屏幕
 * @param drv 矩阵管理器
 * @param color 颜色
 */
void xy_matrix_fill(xy_matrix_driver_t *drv, xy_matrix_color_t color);

/**
 * @brief 设置亮度
 * @param drv 矩阵管理器
 * @param brightness 亮度 (0-255)
 */
void xy_matrix_set_brightness(xy_matrix_driver_t *drv, uint8_t brightness);

/**
 * @brief 扫描服务 (在中断或主循环中调用)
 * @param drv 矩阵管理器
 */
void xy_matrix_scan(xy_matrix_driver_t *drv);

/* ==================== 效果 API ==================== */

/**
 * @brief 等离子效果
 * @param drv 矩阵管理器
 * @param speed 速度
 */
void xy_matrix_effect_plasma(xy_matrix_driver_t *drv, uint16_t speed);

/**
 * @brief 生命游戏
 * @param drv 矩阵管理器
 * @param speed 速度
 */
void xy_matrix_effect_game_of_life(xy_matrix_driver_t *drv, uint16_t speed);

/**
 * @brief 矩阵雨效果
 * @param drv 矩阵管理器
 * @param speed 速度
 * @param color 颜色
 */
void xy_matrix_effect_matrix_rain(xy_matrix_driver_t *drv,
                                  uint16_t speed,
                                  xy_matrix_color_t color);

/**
 * @brief 2D 火焰效果
 * @param drv 矩阵管理器
 * @param speed 速度
 */
void xy_matrix_effect_fire_2d(xy_matrix_driver_t *drv, uint16_t speed);

#ifdef __cplusplus
}
#endif

#endif /* XY_RGB_MATRIX_DRIVER_H */
