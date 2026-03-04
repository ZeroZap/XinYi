/**
 * @file xy_rgb_noise.h
 * @brief Noise Functions (Simplex/Perlin/FBM) - Reference: FastLED
 * @version 1.0.0
 * @date 2026-03-02
 */

#ifndef XY_RGB_NOISE_H
#define XY_RGB_NOISE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Simplex 噪声 ==================== */

/**
 * @brief 1D Simplex 噪声
 * @param x 输入坐标
 * @return 噪声值 (-1 ~ 1)
 */
float xy_simplex_noise_1d(float x);

/**
 * @brief 2D Simplex 噪声
 * @param x X 坐标
 * @param y Y 坐标
 * @return 噪声值 (-1 ~ 1)
 */
float xy_simplex_noise_2d(float x, float y);

/**
 * @brief 3D Simplex 噪声
 * @param x X 坐标
 * @param y Y 坐标
 * @param z Z 坐标
 * @return 噪声值 (-1 ~ 1)
 */
float xy_simplex_noise_3d(float x, float y, float z);

/* ==================== Perlin 噪声 ==================== */

/**
 * @brief 1D Perlin 噪声
 * @param x 输入坐标
 * @return 噪声值 (-1 ~ 1)
 */
float xy_perlin_noise_1d(float x);

/**
 * @brief 2D Perlin 噪声
 * @param x X 坐标
 * @param y Y 坐标
 * @return 噪声值 (-1 ~ 1)
 */
float xy_perlin_noise_2d(float x, float y);

/**
 * @brief 3D Perlin 噪声
 * @param x X 坐标
 * @param y Y 坐标
 * @param z Z 坐标
 * @return 噪声值 (-1 ~ 1)
 */
float xy_perlin_noise_3d(float x, float y, float z);

/* ==================== 分形布朗运动 ==================== */

/**
 * @brief 1D FBM (分形布朗运动)
 * @param x 输入坐标
 * @param octaves 倍频程数量
 * @param persistence 持久性 (0-1)
 * @param lacunarity  lacunarity
 * @return 噪声值 (0 ~ 1)
 */
float xy_fbm_1d(float x, int octaves, float persistence, float lacunarity);

/**
 * @brief 2D FBM
 * @param x X 坐标
 * @param y Y 坐标
 * @param octaves 倍频程数量
 * @param persistence 持久性 (0-1)
 * @param lacunarity  lacunarity
 * @return 噪声值 (0 ~ 1)
 */
float xy_fbm_2d(float x, float y, int octaves, float persistence, float lacunarity);

/**
 * @brief 3D FBM
 * @param x X 坐标
 * @param y Y 坐标
 * @param z Z 坐标
 * @param octaves 倍频程数量
 * @param persistence 持久性 (0-1)
 * @param lacunarity  lacunarity
 * @return 噪声值 (0 ~ 1)
 */
float xy_fbm_3d(float x, float y, float z, int octaves, float persistence, float lacunarity);

/* ==================== 噪声工具 ==================== */

/**
 * @brief 初始化噪声 (设置种子)
 * @param seed 随机种子
 */
void xy_noise_init(uint32_t seed);

/**
 * @brief 重新映射噪声值
 * @param value 原始值
 * @param in_min 输入最小值
 * @param in_max 输入最大值
 * @param out_min 输出最小值
 * @param out_max 输出最大值
 * @return 重新映射的值
 */
float xy_noise_map(float value, float in_min, float in_max, float out_min, float out_max);

/**
 * @brief 获取噪声字节
 * @param x X 坐标
 * @param y Y 坐标
 * @param time 时间
 * @return 噪声值 (0-255)
 */
uint8_t xy_noise_byte(uint16_t x, uint16_t y, uint16_t time);

#ifdef __cplusplus
}
#endif

#endif /* XY_RGB_NOISE_H */
