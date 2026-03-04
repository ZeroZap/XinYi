/**
 * @file xy_rgb_3d.h
 * @brief RGB LED 3D Mode (Cube/Voxel Display)
 * @version 1.0.0
 * @date 2026-03-02
 */

#ifndef XY_RGB_3D_H
#define XY_RGB_3D_H

#include "xy_rgb.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 3D 坐标
 */
typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} point3d_t;

/**
 * @brief 3D 配置
 */
typedef struct {
    uint8_t width;            /* 宽度 (X 轴) */
    uint8_t height;           /* 高度 (Y 轴) */
    uint8_t depth;            /* 深度 (Z 轴) */
} xy_rgb_3d_config_t;

/**
 * @brief 初始化 3D 模式
 * @param config 配置
 * @return XY_RGB_OK 成功，其他值失败
 */
int32_t xy_rgb_3d_init(xy_rgb_3d_config_t *config);

/**
 * @brief 设置体素颜色
 * @param x X 坐标
 * @param y Y 坐标
 * @param z Z 坐标
 * @param color 颜色
 */
void xy_rgb_3d_set_voxel(uint8_t x, uint8_t y, uint8_t z, rgb_color_t color);

/**
 * @brief 获取体素颜色
 * @param x X 坐标
 * @param y Y 坐标
 * @param z Z 坐标
 * @return 颜色
 */
rgb_color_t xy_rgb_3d_get_voxel(uint8_t x, uint8_t y, uint8_t z);

/**
 * @brief 绘制 3D 直线
 * @param p0 起始点
 * @param p1 结束点
 * @param color 颜色
 */
void xy_rgb_3d_draw_line(point3d_t p0, point3d_t p1, rgb_color_t color);

/**
 * @brief 绘制平面
 * @param z Z 坐标
 * @param color 颜色
 */
void xy_rgb_3d_draw_plane(uint8_t z, rgb_color_t color);

/**
 * @brief 清除所有体素
 */
void xy_rgb_3d_clear(void);

/**
 * @brief 清除指定层
 * @param z Z 坐标
 */
void xy_rgb_3d_clear_layer(uint8_t z);

/* ==================== 3D 变换 ==================== */

/**
 * @brief 绕 X 轴旋转
 * @param angle 角度 (0-255)
 */
void xy_rgb_3d_rotate_x(uint8_t angle);

/**
 * @brief 绕 Y 轴旋转
 * @param angle 角度 (0-255)
 */
void xy_rgb_3d_rotate_y(uint8_t angle);

/**
 * @brief 绕 Z 轴旋转
 * @param angle 角度 (0-255)
 */
void xy_rgb_3d_rotate_z(uint8_t angle);

/**
 * @brief 投影到 2D
 * @param point 3D 点
 * @return 2D 点
 */
point2d_t xy_rgb_3d_project(point3d_t point);

/* ==================== 3D 效果 ==================== */

/**
 * @brief 3D 雨滴效果
 * @param speed 速度
 * @param color 颜色
 */
void xy_rgb_3d_fx_rain(uint16_t speed, rgb_color_t color);

/**
 * @brief 3D 火焰效果
 * @param speed 速度
 */
void xy_rgb_3d_fx_fire(uint16_t speed);

/**
 * @brief 3D 等离子效果
 * @param speed 速度
 */
void xy_rgb_3d_fx_plasma(uint16_t speed);

/**
 * @brief 旋转立方体
 * @param speed 速度
 * @param color 颜色
 */
void xy_rgb_3d_fx_rotate_cube(uint16_t speed, rgb_color_t color);

/**
 * @brief 3D 迷宫
 * @param speed 速度
 */
void xy_rgb_3d_fx_maze(uint16_t speed);

#ifdef __cplusplus
}
#endif

#endif /* XY_RGB_3D_H */
