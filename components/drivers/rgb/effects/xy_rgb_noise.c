/**
 * @file xy_rgb_noise.c
 * @brief Noise Functions Implementation (Simplex/Perlin/FBM)
 * @version 1.0.0
 * @date 2026-03-02
 * 
 * Reference: FastLED noise functions
 * https://github.com/FastLED/FastLED/blob/master/noise.h
 */

#include "xy_rgb_noise.h"
#include <math.h>
#include <stdlib.h>

/* 置换表 */
static uint8_t g_perm[512];

/**
 * @brief 初始化噪声
 */
void xy_noise_init(uint32_t seed)
{
    srand(seed);
    
    /* 初始化置换表 */
    for (int i = 0; i < 256; i++) {
        g_perm[i] = i;
    }
    
    /* 洗牌 */
    for (int i = 0; i < 256; i++) {
        int j = rand() % 256;
        uint8_t temp = g_perm[i];
        g_perm[i] = g_perm[j];
        g_perm[j] = temp;
    }
    
    /* 复制一份 */
    for (int i = 0; i < 256; i++) {
        g_perm[256 + i] = g_perm[i];
    }
}

/* ==================== 工具函数 ==================== */

static float fade(float t)
{
    return t * t * t * (t * (t * 6 - 15) + 10);
}

static float lerp(float a, float b, float t)
{
    return a + t * (b - a);
}

static float grad(int hash, float x, float y, float z)
{
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

/* ==================== Simplex 噪声 ==================== */

float xy_simplex_noise_1d(float x)
{
    /* 简化实现 */
    return xy_perlin_noise_1d(x);
}

float xy_simplex_noise_2d(float x, float y)
{
    /* 简化实现 */
    return xy_perlin_noise_2d(x, y);
}

float xy_simplex_noise_3d(float x, float y, float z)
{
    /* 简化实现 */
    return xy_perlin_noise_3d(x, y, z);
}

/* ==================== Perlin 噪声 ==================== */

float xy_perlin_noise_1d(float x)
{
    int x0 = (int)floorf(x);
    int x1 = x0 + 1;
    float sx = x - x0;
    
    sx = fade(sx);
    
    int h0 = g_perm[x0 & 255];
    int h1 = g_perm[x1 & 255];
    
    float n0 = grad(h0, x - x0, 0, 0);
    float n1 = grad(h1, x - x1, 0, 0);
    
    return lerp(n0, n1, sx);
}

float xy_perlin_noise_2d(float x, float y)
{
    int x0 = (int)floorf(x);
    int x1 = x0 + 1;
    int y0 = (int)floorf(y);
    int y1 = y0 + 1;
    
    float sx = x - x0;
    float sy = y - y0;
    
    sx = fade(sx);
    sy = fade(sy);
    
    int h00 = g_perm[g_perm[x0 & 255] + (y0 & 255)];
    int h10 = g_perm[g_perm[x1 & 255] + (y0 & 255)];
    int h01 = g_perm[g_perm[x0 & 255] + (y1 & 255)];
    int h11 = g_perm[g_perm[x1 & 255] + (y1 & 255)];
    
    float n00 = grad(h00, x - x0, y - y0, 0);
    float n10 = grad(h10, x - x1, y - y0, 0);
    float n01 = grad(h01, x - x0, y - y1, 0);
    float n11 = grad(h11, x - x1, y - y1, 0);
    
    float nx0 = lerp(n00, n10, sx);
    float nx1 = lerp(n01, n11, sx);
    
    return lerp(nx0, nx1, sy);
}

float xy_perlin_noise_3d(float x, float y, float z)
{
    int x0 = (int)floorf(x);
    int x1 = x0 + 1;
    int y0 = (int)floorf(y);
    int y1 = y0 + 1;
    int z0 = (int)floorf(z);
    int z1 = z0 + 1;
    
    float sx = x - x0;
    float sy = y - y0;
    float sz = z - z0;
    
    sx = fade(sx);
    sy = fade(sy);
    sz = fade(sz);
    
    int h000 = g_perm[g_perm[g_perm[x0 & 255] + (y0 & 255)] + (z0 & 255)];
    int h100 = g_perm[g_perm[g_perm[x1 & 255] + (y0 & 255)] + (z0 & 255)];
    int h010 = g_perm[g_perm[g_perm[x0 & 255] + (y1 & 255)] + (z0 & 255)];
    int h110 = g_perm[g_perm[g_perm[x1 & 255] + (y1 & 255)] + (z0 & 255)];
    int h001 = g_perm[g_perm[g_perm[x0 & 255] + (y0 & 255)] + (z1 & 255)];
    int h101 = g_perm[g_perm[g_perm[x1 & 255] + (y0 & 255)] + (z1 & 255)];
    int h011 = g_perm[g_perm[g_perm[x0 & 255] + (y1 & 255)] + (z1 & 255)];
    int h111 = g_perm[g_perm[g_perm[x1 & 255] + (y1 & 255)] + (z1 & 255)];
    
    float n000 = grad(h000, x - x0, y - y0, z - z0);
    float n100 = grad(h100, x - x1, y - y0, z - z0);
    float n010 = grad(h010, x - x0, y - y1, z - z0);
    float n110 = grad(h110, x - x1, y - y1, z - z0);
    float n001 = grad(h001, x - x0, y - y0, z - z1);
    float n101 = grad(h101, x - x1, y - y0, z - z1);
    float n011 = grad(h011, x - x0, y - y1, z - z1);
    float n111 = grad(h111, x - x1, y - y1, z - z1);
    
    float nx00 = lerp(n000, n100, sx);
    float nx10 = lerp(n010, n110, sx);
    float nx01 = lerp(n001, n101, sx);
    float nx11 = lerp(n011, n111, sx);
    
    float nxy0 = lerp(nx00, nx10, sy);
    float nxy1 = lerp(nx01, nx11, sy);
    
    return lerp(nxy0, nxy1, sz);
}

/* ==================== 分形布朗运动 ==================== */

float xy_fbm_1d(float x, int octaves, float persistence, float lacunarity)
{
    float total = 0;
    float frequency = 1;
    float amplitude = 1;
    float max_value = 0;
    
    for (int i = 0; i < octaves; i++) {
        total += xy_perlin_noise_1d(x * frequency) * amplitude;
        max_value += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }
    
    return total / max_value;
}

float xy_fbm_2d(float x, float y, int octaves, float persistence, float lacunarity)
{
    float total = 0;
    float frequency = 1;
    float amplitude = 1;
    float max_value = 0;
    
    for (int i = 0; i < octaves; i++) {
        total += xy_perlin_noise_2d(x * frequency, y * frequency) * amplitude;
        max_value += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }
    
    return total / max_value;
}

float xy_fbm_3d(float x, float y, float z, int octaves, float persistence, float lacunarity)
{
    float total = 0;
    float frequency = 1;
    float amplitude = 1;
    float max_value = 0;
    
    for (int i = 0; i < octaves; i++) {
        total += xy_perlin_noise_3d(x * frequency, y * frequency, z * frequency) * amplitude;
        max_value += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }
    
    return total / max_value;
}

/* ==================== 噪声工具 ==================== */

float xy_noise_map(float value, float in_min, float in_max, float out_min, float out_max)
{
    return (value - in_min) * (out_max - out_min) / (out_max - in_min) + out_min;
}

uint8_t xy_noise_byte(uint16_t x, uint16_t y, uint16_t time)
{
    float nx = x / 256.0f;
    float ny = y / 256.0f;
    float nt = time / 256.0f;
    
    float value = xy_perlin_noise_2d(nx + nt, ny);
    value = xy_noise_map(value, -1, 1, 0, 255);
    
    return (uint8_t)value;
}
