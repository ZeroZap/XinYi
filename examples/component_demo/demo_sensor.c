/**
 * @file demo_sensor.c
 * @brief Sensor Component Demo
 * @version 1.0.0
 * @date 2026-03-13
 */

#include <stdio.h>
#include "xy_log.h"

#ifdef DEMO_SENSOR

/* 模拟传感器数据 */
typedef struct {
    int temperature;    /* 0.01°C 精度 */
    int humidity;       /* 0.01%RH 精度 */
} sht30_data_t;

typedef struct {
    int accel_x;        /* 0.001g 精度 */
    int accel_y;
    int accel_z;
    int gyro_x;         /* 0.01°/s 精度 */
    int gyro_y;
    int gyro_z;
} mpu6050_data_t;

/**
 * @brief 初始化传感器演示
 */
int demo_sensor_init(void)
{
    xy_log_i("Sensor module initialized\n");
    return 0;
}

/**
 * @brief 运行传感器演示
 */
void demo_sensor_run(void)
{
    /* SHT30 温湿度传感器演示 */
    xy_log_i("SHT30 Temperature/Humidity Sensor:\n");
    
    /* 模拟读取数据 */
    sht30_data_t sht30 = {
        .temperature = 2550,   /* 25.50°C */
        .humidity = 5000       /* 50.00%RH */
    };
    
    xy_log_d("  Temperature: %d.%02d°C\n", 
             sht30.temperature / 100, 
             sht30.temperature % 100);
    xy_log_d("  Humidity: %d.%02d%%RH\n", 
             sht30.humidity / 100, 
             sht30.humidity % 100);
    
    /* MPU6050 加速度计/陀螺仪演示 */
    xy_log_i("MPU6050 6-Axis IMU:\n");
    
    /* 模拟读取数据 */
    mpu6050_data_t mpu = {
        .accel_x = 0,
        .accel_y = 0,
        .accel_z = 1000,      /* 1.00g (重力) */
        .gyro_x = 0,
        .gyro_y = 0,
        .gyro_z = 0
    };
    
    xy_log_d("  Acceleration: (%.2f, %.2f, %.2f)g\n",
             mpu.accel_x / 1000.0,
             mpu.accel_y / 1000.0,
             mpu.accel_z / 1000.0);
    xy_log_d("  Gyroscope: (%.2f, %.2f, %.2f)°/s\n",
             mpu.gyro_x / 100.0,
             mpu.gyro_y / 100.0,
             mpu.gyro_z / 100.0);
    
    xy_log_i("Sensor demo completed\n");
}

#endif /* DEMO_SENSOR */
