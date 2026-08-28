/**
 * @file examples/weather_station/main.c
 * @brief 智能气象站综合示例
 * @version 1.0.0
 * @date 2026-03-01 YOLO 通宵
 */

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_INFO
#include "xy_log.h"
#include "xy_os.h"
#include "xy_sht30.h"
#include "xy_mpu6050.h"
#include "xy_oled_ssd1306.h"
#include "xy_pid.h"
#include "xy_json.h"
#include "xy_mq.h"
#include "xy_fota.h"

/* ==================== 系统配置 ==================== */

#define TEMP_TARGET       25.0f       /* 目标温度 */
#define SAMPLE_INTERVAL   1000        /* 采样间隔 1 秒 */
#define DISPLAY_INTERVAL  5000        /* 显示间隔 5 秒 */

/* ==================== 全局对象 ==================== */

static xy_sht30_t g_sht30;
static xy_mpu6050_t g_mpu;
static xy_oled_ssd1306_t g_oled;
static xy_pid_t g_temp_pid;
static xy_mq_t g_data_mq;

typedef struct {
    float temperature;
    float humidity;
    float pressure;
    uint32_t timestamp;
} sensor_data_t;

/* ==================== 任务 ==================== */

/**
 * @brief 传感器数据采集任务
 */
static void sensor_task(void *arg)
{
    (void)arg;
    sensor_data_t data;
    
    xy_log_i("Sensor Task Started\n");
    
    while (1) {
        /* 读取温湿度 */
        if (xy_sht30_read(&g_sht30) == XY_DEVICE_OK) {
            data.temperature = (float)g_sht30.temperature / 100.0f;
            data.humidity = (float)g_sht30.humidity / 100.0f;
        }
        
        /* 读取姿态 */
        if (xy_mpu6050_read_accel(&g_mpu, NULL, NULL, NULL) == XY_MPU6050_OK) {
            /* 有加速度数据 */
        }
        
        data.timestamp = xy_os_tick_get();
        
        /* 发送到消息队列 */
        xy_mq_send(&g_data_mq, &data, 0);
        
        xy_log_d("Sensor: T=%.2f, H=%.2f\n", data.temperature, data.humidity);
        xy_os_delay(SAMPLE_INTERVAL);
    }
}

/**
 * @brief PID 温度控制任务
 */
static void pid_task(void *arg)
{
    (void)arg;
    sensor_data_t data;
    float output;
    
    xy_log_i("PID Task Started\n");
    
    /* 配置 PID */
    xy_pid_config_t cfg = {
        .kp = 2.0f,
        .ki = 0.5f,
        .kd = 1.0f,
        .output_min = 0,
        .output_max = 100,
    };
    xy_pid_init(&g_temp_pid, &cfg);
    xy_pid_set_setpoint(&g_temp_pid, TEMP_TARGET);
    
    while (1) {
        /* 从队列获取数据 */
        if (xy_mq_recv(&g_data_mq, &data, 100) == XY_MQ_OK) {
            /* 计算 PID 输出 */
            output = xy_pid_compute(&g_temp_pid, data.temperature);
            
            xy_log_d("PID: SP=%.2f, PV=%.2f, OUT=%.2f\n",
                     TEMP_TARGET, data.temperature, output);
        }
        
        xy_os_delay(100);
    }
}

/**
 * @brief OLED 显示任务
 */
static void display_task(void *arg)
{
    (void)arg;
    char buf[32];
    sensor_data_t data;
    
    xy_log_i("Display Task Started\n");
    
    /* 初始化 OLED */
    // xy_oled_ssd1306_init(&g_oled, NULL);
    
    while (1) {
        /* 获取最新数据 */
        if (xy_mq_try_recv(&g_data_mq, &data) == XY_MQ_OK) {
            /* 清屏 */
            // xy_oled_ssd1306_clear(&g_oled);
            
            /* 显示温度 */
            snprintf(buf, sizeof(buf), "T: %.1f C", data.temperature);
            // xy_oled_ssd1306_draw_string(&g_oled, 0, 0, buf, 1);
            
            /* 显示湿度 */
            snprintf(buf, sizeof(buf), "H: %.1f %%", data.humidity);
            // xy_oled_ssd1306_draw_string(&g_oled, 0, 16, buf, 1);
            
            /* 刷新 */
            // xy_oled_ssd1306_refresh(&g_oled);
        }
        
        xy_os_delay(DISPLAY_INTERVAL);
    }
}

/**
 * @brief FOTA 升级任务
 */
static void fota_task(void *arg)
{
    (void)arg;
    xy_fota_t fota;
    
    xy_log_i("FOTA Task Started\n");
    
    /* 配置 FOTA */
    xy_fota_config_t cfg = {
        .flash_base_addr = 0x08060000,
        .slot_size = 0x20000,
        .slot_count = 2,
    };
    xy_fota_init(&fota, &cfg);
    
    /* 等待升级命令 */
    while (1) {
        /* TODO: 检查升级标志 */
        xy_os_delay(1000);
    }
}

/**
 * @brief 系统初始化
 */
static int system_init(void)
{
    xy_log_i("=== Weather Station Demo ===\n");
    
    /* 初始化 OS */
    xy_os_kernel_init();
    
    /* 初始化传感器 */
    // xy_sht30_init(&g_sht30, NULL);
    // xy_mpu6050_init(&g_mpu, NULL, 0x68);
    
    /* 初始化消息队列 */
    xy_mq_config_t mq_cfg = {
        .msg_size = sizeof(sensor_data_t),
        .max_msgs = 10,
        .overwrite_old = true,
    };
    xy_mq_init(&g_data_mq, &mq_cfg);
    
    xy_log_i("System Initialized\n");
    return 0;
}

/**
 * @brief 主函数
 */
int main(void)
{
    xy_os_thread_t threads[4];
    static uint8_t stacks[4][512];
    
    system_init();
    
    /* 创建任务 */
    xy_os_thread_create(&threads[0], "Sensor", sensor_task, NULL, 5, stacks[0], 512);
    xy_os_thread_create(&threads[1], "PID", pid_task, NULL, 6, stacks[1], 512);
    xy_os_thread_create(&threads[2], "Display", display_task, NULL, 4, stacks[2], 512);
    xy_os_thread_create(&threads[3], "FOTA", fota_task, NULL, 3, stacks[3], 512);
    
    /* 启动内核 */
    xy_os_kernel_start();
    
    return 0;
}
