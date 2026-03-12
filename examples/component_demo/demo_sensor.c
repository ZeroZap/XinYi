/**
 * @file demo_sensor.c
 * @brief Sensor Component Demo
 * @version 1.0.0
 * @date 2026-03-13
 */

#include <stdio.h>
#include "xy_sht30.h"
#include "xy_mpu6050.h"
#include "xy_bmp280.h"
#include "xy_ads1115.h"

#ifdef DEMO_SENSOR

typedef struct {
    int temperature;
    int humidity;
} sht30_data_t;

typedef struct {
    int accel_x, accel_y, accel_z;
    int gyro_x, gyro_y, gyro_z;
} mpu6050_data_t;

typedef struct {
    int temperature;
    int pressure;
} bmp280_data_t;

typedef struct {
    int adc_value;
    float voltage;
} ads1115_data_t;

int demo_sensor_init(void)
{
    printf("[INFO] Sensor module initialized\n");
    return 0;
}

void demo_sensor_run(void)
{
    printf("--- SHT30 Temperature/Humidity Sensor ---\n");
    sht30_data_t sht30 = { .temperature = 2550, .humidity = 5000 };
    printf("  Temperature: %d.%02d°C\n", sht30.temperature/100, sht30.temperature%100);
    printf("  Humidity: %d.%02d%%RH\n", sht30.humidity/100, sht30.humidity%100);
    
    printf("\n--- MPU6050 6-Axis IMU ---\n");
    mpu6050_data_t mpu = { .accel_x=0, .accel_y=0, .accel_z=1000, .gyro_x=0, .gyro_y=0, .gyro_z=0 };
    printf("  Acceleration: (%.2f, %.2f, %.2f)g\n", mpu.accel_x/1000.0, mpu.accel_y/1000.0, mpu.accel_z/1000.0);
    printf("  Gyroscope: (%.2f, %.2f, %.2f)°/s\n", mpu.gyro_x/100.0, mpu.gyro_y/100.0, mpu.gyro_z/100.0);
    
    printf("\n--- BMP280 Pressure Sensor ---\n");
    bmp280_data_t bmp = { .temperature = 2520, .pressure = 101325 };
    printf("  Temperature: %d.%02d°C\n", bmp.temperature/100, bmp.temperature%100);
    printf("  Pressure: %d Pa (%.2f hPa)\n", bmp.pressure, bmp.pressure/100.0);
    
    printf("\n--- ADS1115 16-bit ADC ---\n");
    ads1115_data_t ads = { .adc_value = 16384, .voltage = 2.048 };
    printf("  ADC Value: %d (16-bit)\n", ads.adc_value);
    printf("  Voltage: %.3f V\n", ads.voltage);
    
    printf("\n[INFO] Sensor demo completed\n");
}

#endif
