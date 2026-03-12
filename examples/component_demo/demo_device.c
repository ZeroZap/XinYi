/**
 * @file demo_device.c
 * @brief Device Framework Demo
 * @version 1.0.0
 * @date 2026-03-13
 */

#include <stdio.h>
#include "xy_device.h"
#include "xy_device_core.h"
#include "xy_log.h"

#ifdef DEMO_DEVICE

/* 模拟设备实例 */
static xy_i2c_device_t g_sht30;
static xy_i2c_device_t g_mpu6050;

/**
 * @brief 初始化设备演示
 */
int demo_device_init(void)
{
    int ret;
    
    /* 初始化设备注册表 */
    ret = xy_device_registry_init();
    if (ret != 0) {
        xy_log_e("Device registry init failed: %d\n", ret);
        return -1;
    }
    
    xy_log_i("Device registry initialized (max %d devices)\n", 
             XY_DEVICE_REGISTRY_MAX);
    
    /* 初始化模拟 I2C 设备 */
    ret = xy_i2c_device_init(&g_sht30, NULL, 0x44, 1000);
    if (ret != XY_DEVICE_OK) {
        xy_log_e("SHT30 init failed: %d\n", ret);
        return -1;
    }
    g_sht30.base.name = "sht30_1";
    g_sht30.base.type = XY_DEVICE_TYPE_SENSOR;
    
    ret = xy_i2c_device_init(&g_mpu6050, NULL, 0x68, 1000);
    if (ret != XY_DEVICE_OK) {
        xy_log_e("MPU6050 init failed: %d\n", ret);
        return -1;
    }
    g_mpu6050.base.name = "mpu6050_1";
    g_mpu6050.base.type = XY_DEVICE_TYPE_SENSOR;
    
    return 0;
}

/**
 * @brief 运行设备演示
 */
void demo_device_run(void)
{
    int ret;
    xy_device_t *dev;
    xy_device_stats_t stats;
    
    /* 注册设备 */
    xy_log_i("Registering devices...\n");
    
    ret = xy_device_registry_register(&g_sht30.base);
    if (ret == XY_DEVICE_OK) {
        xy_log_d("Device 'sht30_1' registered (I2C)\n");
    }
    
    ret = xy_device_registry_register(&g_mpu6050.base);
    if (ret == XY_DEVICE_OK) {
        xy_log_d("Device 'mpu6050_1' registered (I2C)\n");
    }
    
    /* 获取设备数量 */
    size_t count = xy_device_get_count();
    xy_log_i("Device count: %zu\n", count);
    
    /* 查找设备 */
    xy_log_i("Finding devices...\n");
    
    dev = xy_device_find_by_name("sht30_1");
    if (dev) {
        xy_log_d("Found device: %s (type=%d)\n", dev->name, dev->type);
    }
    
    dev = xy_device_find_by_type(XY_DEVICE_TYPE_SENSOR, 0);
    if (dev) {
        xy_log_d("Found sensor device: %s\n", dev->name);
    }
    
    /* 获取统计信息 */
    ret = xy_device_get_stats(&stats);
    if (ret == XY_DEVICE_OK) {
        xy_log_i("Device statistics:\n");
        xy_log_i("  Total: %zu\n", stats.total_devices);
        xy_log_i("  I2C: %zu\n", stats.i2c_count);
        xy_log_i("  SPI: %zu\n", stats.spi_count);
        xy_log_i("  Sensor: %zu\n", stats.sensor_count);
    }
    
    /* 打印设备列表 */
    xy_log_i("Device list:\n");
    xy_device_print_list();
    
    /* 演示电源管理 */
    xy_log_i("Testing power management...\n");
    
    ret = xy_device_sleep(&g_sht30.base);
    if (ret == XY_DEVICE_OK) {
        xy_log_d("Device 'sht30_1' entered SLEEP mode\n");
    }
    
    xy_os_delay(100);
    
    ret = xy_device_wake(&g_sht30.base);
    if (ret == XY_DEVICE_OK) {
        xy_log_d("Device 'sht30_1' woke up\n");
    }
    
    /* 演示引用计数 */
    xy_log_i("Testing reference counting...\n");
    
    xy_device_acquire(&g_sht30.base);
    xy_log_d("Device 'sht30_1' acquired (ref_count=%d)\n", 
             xy_device_get_ref_count(&g_sht30.base));
    
    xy_device_release(&g_sht30.base);
    xy_log_d("Device 'sht30_1' released (ref_count=%d)\n", 
             xy_device_get_ref_count(&g_sht30.base));
    
    xy_log_i("Device demo completed\n");
}

#endif /* DEMO_DEVICE */
