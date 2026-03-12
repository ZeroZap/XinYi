/**
 * @file demo_device.c
 * @brief Device Framework Demo
 * @version 1.0.0
 * @date 2026-03-13
 */

#include <stdio.h>
#include "xy_device.h"
#include "xy_device_core.h"

#ifdef DEMO_DEVICE

static xy_i2c_device_t g_sht30;
static xy_i2c_device_t g_mpu6050;

int demo_device_init(void)
{
    int ret;
    
    ret = xy_device_registry_init();
    if (ret != 0) {
        printf("[ERROR] Device registry init failed: %d\n", ret);
        return -1;
    }
    
    printf("[INFO] Device registry initialized (max %d devices)\n", 
           XY_DEVICE_REGISTRY_MAX);
    
    ret = xy_i2c_device_init(&g_sht30, NULL, 0x44, 1000);
    if (ret != XY_DEVICE_OK) return -1;
    g_sht30.base.name = "sht30_1";
    g_sht30.base.type = XY_DEVICE_TYPE_SENSOR;
    
    ret = xy_i2c_device_init(&g_mpu6050, NULL, 0x68, 1000);
    if (ret != XY_DEVICE_OK) return -1;
    g_mpu6050.base.name = "mpu6050_1";
    g_mpu6050.base.type = XY_DEVICE_TYPE_SENSOR;
    
    return 0;
}

void demo_device_run(void)
{
    xy_device_t *dev;
    xy_device_stats_t stats;
    
    printf("[INFO] Registering devices...\n");
    xy_device_registry_register(&g_sht30.base);
    printf("[DEBUG] Device 'sht30_1' registered\n");
    
    xy_device_registry_register(&g_mpu6050.base);
    printf("[DEBUG] Device 'mpu6050_1' registered\n");
    
    printf("[INFO] Device count: %zu\n", xy_device_get_count());
    
    printf("[INFO] Finding devices...\n");
    dev = xy_device_find_by_name("sht30_1");
    if (dev) {
        printf("[DEBUG] Found: %s (type=%d)\n", dev->name, dev->type);
    }
    
    dev = xy_device_find_by_type(XY_DEVICE_TYPE_SENSOR, 0);
    if (dev) {
        printf("[DEBUG] Found sensor: %s\n", dev->name);
    }
    
    xy_device_get_stats(&stats);
    printf("[INFO] Statistics: Total=%zu, I2C=%zu, Sensor=%zu\n",
           stats.total_devices, stats.i2c_count, stats.sensor_count);
    
    printf("[INFO] Device list:\n");
    xy_device_print_list();
    
    printf("[INFO] Testing power management...\n");
    xy_device_sleep(&g_sht30.base);
    printf("[DEBUG] sht30_1 entered SLEEP\n");
    
    xy_os_delay(100);
    
    xy_device_wake(&g_sht30.base);
    printf("[DEBUG] sht30_1 woke up\n");
    
    printf("[INFO] Device demo completed\n");
}

#endif
