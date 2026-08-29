/**
 * @file main.c
 * @brief XinYi 综合示例项目 - 智能温湿度计
 * @version 1.0.0
 * @date 2026-03-01
 */

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_INFO
#include "xy_log.h"
#include "xy_os.h"
#include "xy_hal_gpio.h"
#include "xy_hal_i2c.h"
#include "xy_sht30.h"
#include "xy_oled.h"
#include "xy_bmp280.h"

/* ==================== 配置 ==================== */

#define SAMPLE_INTERVAL_MS  1000    /* 采样间隔 1 秒 */
#define DISPLAY_INTERVAL_MS 5000    /* 显示间隔 5 秒 */

/* ==================== 全局变量 ==================== */

static xy_sht30_t g_sht30;
static xy_bmp280_t g_bmp280;
static xy_oled_t g_oled;

static int16_t g_temperature = 0;
static uint16_t g_humidity = 0;
static uint32_t g_pressure = 0;

/* ==================== 任务 ==================== */

/**
 * @brief 传感器数据采集任务
 */
static void sensor_task(void *arg)
{
    (void)arg;
    int ret;
    
    xy_log_i("Sensor task started\n");
    
    while (1) {
        /* 读取 SHT30 温湿度 */
        ret = xy_sht30_read(&g_sht30);
        if (ret == XY_DEVICE_OK) {
            g_temperature = g_sht30.temperature;
            g_humidity = g_sht30.humidity;
            xy_log_d("SHT30: T=%d.%02d°C, H=%d.%02d%%RH\n", 
                     g_temperature / 100, g_temperature % 100,
                     g_humidity / 100, g_humidity % 100);
        }
        
        /* 读取 BMP280 气压 */
        ret = xy_bmp280_read(&g_bmp280);
        if (ret == XY_DEVICE_OK) {
            g_pressure = g_bmp280.pressure;
            xy_log_d("BMP280: P=%dPa\n", g_pressure);
        }
        
        xy_os_delay(SAMPLE_INTERVAL_MS);
    }
}

/**
 * @brief 显示刷新任务
 */
static void display_task(void *arg)
{
    (void)arg;
    char buffer[32];
    
    xy_log_i("Display task started\n");
    
    /* 初始化 OLED */
    xy_oled_init(&g_oled, NULL);  /* I2C 句柄需要根据实际平台提供 */
    xy_oled_clear(&g_oled);
    
    while (1) {
        /* 清屏 */
        xy_oled_clear(&g_oled);
        
        /* 显示温度 */
        snprintf(buffer, sizeof(buffer), "T: %d.%02d C", 
                 g_temperature / 100, g_temperature % 100);
        xy_oled_draw_string(&g_oled, 0, 0, buffer, 1);
        
        /* 显示湿度 */
        snprintf(buffer, sizeof(buffer), "H: %d.%02d %%", 
                 g_humidity / 100, g_humidity % 100);
        xy_oled_draw_string(&g_oled, 0, 16, buffer, 1);
        
        /* 显示气压 */
        snprintf(buffer, sizeof(buffer), "P: %d Pa", g_pressure);
        xy_oled_draw_string(&g_oled, 0, 32, buffer, 1);
        
        /* 刷新显示 */
        xy_oled_refresh(&g_oled);
        
        xy_os_delay(DISPLAY_INTERVAL_MS);
    }
}

/**
 * @brief 按键检测任务
 */
static void button_task(void *arg)
{
    (void)arg;
    
    xy_log_i("Button task started\n");
    
    while (1) {
        /* 这里添加按键检测逻辑 */
        /* 检测到按键后切换显示模式 */
        
        xy_os_delay(100);
    }
}

/**
 * @brief 系统初始化
 */
static int system_init(void)
{
    int ret;
    
    xy_log_i("=== XinYi Smart Hygrometer ===\n");
    xy_log_i("System initializing...\n");
    
    /* 初始化 OS */
    xy_os_kernel_init();
    
    /* 初始化 I2C */
    /* xy_hal_i2c_init(&hi2c1, &i2c_config); */
    
    /* 初始化 SHT30 */
    ret = xy_sht30_init(&g_sht30, NULL);  /* I2C 句柄需要根据实际平台提供 */
    if (ret != XY_DEVICE_OK) {
        xy_log_e("Failed to initialize SHT30\n");
        return ret;
    }
    xy_log_i("SHT30 initialized\n");
    
    /* 初始化 BMP280 */
    ret = xy_bmp280_init_addr(&g_bmp280, NULL, BMP280_ADDR_DEFAULT);
    if (ret != XY_DEVICE_OK) {
        xy_log_e("Failed to initialize BMP280\n");
        return ret;
    }
    xy_log_i("BMP280 initialized\n");
    
    xy_log_i("System initialized successfully\n");
    return XY_OK;
}

/**
 * @brief 主函数
 */
int main(void)
{
    int ret;
    xy_os_thread_t sensor_thread;
    xy_os_thread_t display_thread;
    xy_os_thread_t button_thread;
    static uint8_t sensor_stack[512];
    static uint8_t display_stack[512];
    static uint8_t button_stack[256];
    
    /* 系统初始化 */
    ret = system_init();
    if (ret != XY_OK) {
        xy_log_e("System initialization failed\n");
        return ret;
    }
    
    /* 创建传感器任务 */
    ret = xy_os_thread_create(
        &sensor_thread,
        "Sensor",
        sensor_task,
        NULL,
        5,
        sensor_stack,
        sizeof(sensor_stack)
    );
    if (ret != XY_OS_OK) {
        xy_log_e("Failed to create sensor task\n");
        return ret;
    }
    
    /* 创建显示任务 */
    ret = xy_os_thread_create(
        &display_thread,
        "Display",
        display_task,
        NULL,
        4,
        display_stack,
        sizeof(display_stack)
    );
    if (ret != XY_OS_OK) {
        xy_log_e("Failed to create display task\n");
        return ret;
    }
    
    /* 创建按键任务 */
    ret = xy_os_thread_create(
        &button_thread,
        "Button",
        button_task,
        NULL,
        6,
        button_stack,
        sizeof(button_stack)
    );
    if (ret != XY_OS_OK) {
        xy_log_e("Failed to create button task\n");
        return ret;
    }
    
    /* 启动内核 */
    xy_os_kernel_start();
    
    /* 不应该到达这里 */
    return 0;
}
