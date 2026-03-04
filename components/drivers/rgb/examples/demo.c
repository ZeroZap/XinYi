/**
 * @file demo.c
 * @brief RGB LED Effect Library Demo
 * @version 1.0.0
 * @date 2026-03-02
 */

#include "xy_rgb.h"
#include "xy_rgb_segment.h"
#include "xy_rgb_fx.h"
#include "xy_rgb_drv.h"
#include "xy_log.h"
#include "xy_os.h"

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_INFO

/* 配置 */
#define NUM_LEDS        30      /* LED 数量 */
#define BRIGHTNESS      128     /* 亮度 (0-255) */
#define DATA_PIN        5       /* 数据引脚 */

/* 全局变量 */
static xy_rgb_config_t g_rgb_config;
static xy_rgb_gpio_config_t g_gpio_config;

/**
 * @brief 初始化示例
 */
static void demo_init(void)
{
    /* GPIO 配置 */
    g_gpio_config.data_pin = DATA_PIN;
    g_gpio_config.color_order = XY_RGB_GRB;
    
    /* RGB 配置 */
    g_rgb_config.num_leds = NUM_LEDS;
    g_rgb_config.brightness = BRIGHTNESS;
    g_rgb_config.drv_type = XY_RGB_DRV_GPIO;
    g_rgb_config.drv_handle = &g_gpio_config;
    
    /* 初始化 RGB 库 */
    xy_rgb_init(&g_rgb_config);
    
    xy_log_i("RGB Demo initialized: %d LEDs\n", NUM_LEDS);
}

/**
 * @brief 示例 1: 静态颜色
 */
static void demo_static(void)
{
    xy_log_i("Demo: Static Color\n");
    
    /* 设置所有 LED 为红色 */
    xy_rgb_set_all((rgb_color_t){255, 0, 0});
    xy_rgb_show();
    
    xy_os_delay(1000);
    
    /* 设置所有 LED 为绿色 */
    xy_rgb_set_all((rgb_color_t){0, 255, 0});
    xy_rgb_show();
    
    xy_os_delay(1000);
    
    /* 设置所有 LED 为蓝色 */
    xy_rgb_set_all((rgb_color_t){0, 0, 255});
    xy_rgb_show();
    
    xy_os_delay(1000);
}

/**
 * @brief 示例 2: 彩虹效果
 */
static void demo_rainbow(void)
{
    xy_log_i("Demo: Rainbow\n");
    
    xy_rgb_set_effect(FX_RAINBOW);
    xy_rgb_set_effect_params(128, 128);
    
    for (int i = 0; i < 100; i++) {
        xy_rgb_service();
        xy_rgb_show();
        xy_os_delay(20);
    }
}

/**
 * @brief 示例 3: 分段效果
 */
static void demo_segments(void)
{
    xy_log_i("Demo: Segments\n");
    
    /* 创建 3 个分段 */
    int seg1 = xy_rgb_create_segment(0, 10);      /* 0-9 */
    int seg2 = xy_rgb_create_segment(10, 20);     /* 10-19 */
    int seg3 = xy_rgb_create_segment(20, 30);     /* 20-29 */
    
    /* 设置不同效果 */
    xy_rgb_set_segment_effect(seg1, FX_RAINBOW);
    xy_rgb_set_segment_effect(seg2, FX_CHASE);
    xy_rgb_set_segment_effect(seg3, FX_BREATH);
    
    /* 设置不同颜色 */
    xy_rgb_set_segment_colors(seg1, 
                              (rgb_color_t){255, 0, 0},
                              (rgb_color_t){0, 0, 0},
                              (rgb_color_t){0, 0, 0});
    
    xy_rgb_set_segment_colors(seg2,
                              (rgb_color_t){0, 255, 0},
                              (rgb_color_t){0, 0, 0},
                              (rgb_color_t){0, 0, 0});
    
    xy_rgb_set_segment_colors(seg3,
                              (rgb_color_t){0, 0, 255},
                              (rgb_color_t){0, 0, 0},
                              (rgb_color_t){0, 0, 0});
    
    /* 运行效果 */
    for (int i = 0; i < 200; i++) {
        xy_rgb_service();
        xy_rgb_show();
        xy_os_delay(20);
    }
    
    /* 清除分段 */
    xy_rgb_clear_segments();
}

/**
 * @brief 示例 4: 火焰效果
 */
static void demo_fire(void)
{
    xy_log_i("Demo: Fire\n");
    
    xy_rgb_set_effect(FX_FIRE);
    xy_rgb_set_effect_params(128, 200);
    
    for (int i = 0; i < 200; i++) {
        xy_rgb_service();
        xy_rgb_show();
        xy_os_delay(30);
    }
}

/**
 * @brief 示例 5: 彗星效果
 */
static void demo_comet(void)
{
    xy_log_i("Demo: Comet\n");
    
    xy_rgb_set_effect(FX_COMET);
    xy_rgb_set_effect_params(200, 128);
    
    /* 设置彗星颜色 */
    xy_rgb_segment_t *seg = xy_rgb_get_segment(0);
    if (seg) {
        seg->color1 = (rgb_color_t){255, 255, 255};  /* 白色头部 */
        seg->color2 = (rgb_color_t){0, 0, 128};      /* 蓝色尾部 */
    }
    
    for (int i = 0; i < 150; i++) {
        xy_rgb_service();
        xy_rgb_show();
        xy_os_delay(20);
    }
}

/**
 * @brief 示例 6: 渐变效果
 */
static void demo_gradient(void)
{
    xy_log_i("Demo: Gradient\n");
    
    int seg = xy_rgb_create_segment(0, NUM_LEDS);
    
    xy_rgb_set_segment_effect(seg, FX_GRADIENT);
    xy_rgb_set_segment_colors(seg,
                              (rgb_color_t){255, 0, 0},   /* 红色 */
                              (rgb_color_t){0, 255, 0},   /* 绿色 */
                              (rgb_color_t){0, 0, 255});  /* 蓝色 */
    
    for (int i = 0; i < 100; i++) {
        xy_rgb_service();
        xy_rgb_show();
        xy_os_delay(30);
    }
    
    xy_rgb_delete_segment(seg);
}

/**
 * @brief 主循环
 */
void demo_main(void)
{
    xy_log_i("=== RGB LED Effect Demo ===\n");
    
    demo_init();
    
    while (1) {
        xy_log_i("\n--- Running demos ---\n");
        
        demo_static();
        xy_rgb_clear();
        xy_os_delay(500);
        
        demo_rainbow();
        xy_rgb_clear();
        xy_os_delay(500);
        
        demo_segments();
        xy_rgb_clear();
        xy_os_delay(500);
        
        demo_fire();
        xy_rgb_clear();
        xy_os_delay(500);
        
        demo_comet();
        xy_rgb_clear();
        xy_os_delay(500);
        
        demo_gradient();
        xy_rgb_clear();
        xy_os_delay(500);
        
        xy_log_i("--- Demo complete, restart ---\n");
        xy_os_delay(2000);
    }
}
