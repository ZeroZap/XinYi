/**
 * @file xy_rgb_gpio.c
 * @brief RGB LED GPIO Driver (Bit-bang)
 * @version 1.0.0
 * @date 2026-03-02
 */

#include "xy_rgb_drv.h"
#include "xy_hal_gpio.h"
#include "xy_log.h"

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* WS2812 时序 (单位：微秒) */
#define WS2812_T0H    0.4   /* 0 码高电平时间 */
#define WS2812_T0L    0.85  /* 0 码低电平时间 */
#define WS2812_T1H    0.8   /* 1 码高电平时间 */
#define WS2812_T1L    0.45  /* 1 码低电平时间 */
#define WS2812_RES    50    /* 复位时间 */

static uint8_t g_data_pin = 0xFF;
static uint8_t g_color_order = XY_RGB_GRB;

/**
 * @brief 微秒延时
 */
static void delay_us(uint32_t us)
{
    /* TODO: 实现微秒延时 */
    /* 根据 MCU 主频调整 */
    volatile uint32_t count = us * 10;
    while (count--);
}

/**
 * @brief 发送一个 bit
 */
static void send_bit(bool bit)
{
    if (bit) {
        /* 发送 1 */
        xy_hal_gpio_write(g_data_pin, 1);
        delay_us((uint32_t)(WS2812_T1H * 10));
        xy_hal_gpio_write(g_data_pin, 0);
        delay_us((uint32_t)(WS2812_T1L * 10));
    } else {
        /* 发送 0 */
        xy_hal_gpio_write(g_data_pin, 1);
        delay_us((uint32_t)(WS2812_T0H * 10));
        xy_hal_gpio_write(g_data_pin, 0);
        delay_us((uint32_t)(WS2812_T0L * 10));
    }
}

/**
 * @brief 发送一个字节
 */
static void send_byte(uint8_t data)
{
    for (int i = 7; i >= 0; i--) {
        send_bit((data >> i) & 1);
    }
}

/**
 * @brief GPIO 初始化
 */
static int32_t xy_rgb_gpio_init(void *handle)
{
    xy_rgb_gpio_config_t *config = (xy_rgb_gpio_config_t*)handle;
    
    if (!config) {
        return XY_RGB_ERROR_INVALID_PARAM;
    }
    
    g_data_pin = config->data_pin;
    g_color_order = config->color_order;
    
    /* 配置 GPIO 为输出 */
    xy_hal_gpio_config_t gpio_cfg = {
        .mode = XY_HAL_GPIO_MODE_OUTPUT,
        .pull = XY_HAL_GPIO_PULL_NONE,
        .speed = XY_HAL_GPIO_SPEED_HIGH,
    };
    xy_hal_gpio_init(g_data_pin, &gpio_cfg);
    
    /* 初始低电平 */
    xy_hal_gpio_write(g_data_pin, 0);
    
    xy_log_d("RGB GPIO init: pin=%d, order=%d\n", g_data_pin, g_color_order);
    
    return XY_RGB_OK;
}

/**
 * @brief GPIO 反初始化
 */
static int32_t xy_rgb_gpio_deinit(void *handle)
{
    (void)handle;
    g_data_pin = 0xFF;
    return XY_RGB_OK;
}

/**
 * @brief GPIO 发送数据
 */
static int32_t xy_rgb_gpio_send(void *handle, rgb_color_t *leds, uint16_t num_leds)
{
    (void)handle;
    
    if (!leds || num_leds == 0 || g_data_pin == 0xFF) {
        return XY_RGB_ERROR_INVALID_PARAM;
    }
    
    /* 关闭中断 (确保时序准确) */
    /* __disable_irq(); */
    
    for (uint16_t i = 0; i < num_leds; i++) {
        /* 按颜色顺序发送 */
        uint8_t colors[3];
        
        switch (g_color_order) {
            case XY_RGB_RGB:
                colors[0] = leds[i].r;
                colors[1] = leds[i].g;
                colors[2] = leds[i].b;
                break;
            case XY_RGB_GRB:
            default:
                colors[0] = leds[i].g;
                colors[1] = leds[i].r;
                colors[2] = leds[i].b;
                break;
        }
        
        /* 发送 GRB/RGB */
        send_byte(colors[0]);
        send_byte(colors[1]);
        send_byte(colors[2]);
    }
    
    /* 开启中断 */
    /* __enable_irq(); */
    
    /* 复位时间 */
    delay_us(WS2812_RES * 10);
    
    return XY_RGB_OK;
}

/**
 * @brief GPIO 驱动接口
 */
static const xy_rgb_drv_if_t g_gpio_drv = {
    .init = xy_rgb_gpio_init,
    .deinit = xy_rgb_gpio_deinit,
    .send = xy_rgb_gpio_send,
};

const xy_rgb_drv_if_t* xy_rgb_drv_gpio(void)
{
    return &g_gpio_drv;
}
