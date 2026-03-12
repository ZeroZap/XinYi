/**
 * @file device_registry_example.c
 * @brief 设备注册表与电源管理使用示例
 * @version 1.0.0
 * @date 2026-03-12
 * 
 * 演示内容:
 * 1. 设备注册表初始化
 * 2. 设备注册与查找
 * 3. 设备引用计数
 * 4. 设备电源管理
 * 5. 自动休眠功能
 */

#include "xy_device.h"
#include "xy_device_core.h"
#include <stdio.h>
#include <string.h>

/* ==================== 模拟 HAL 实现 ==================== */

/* 模拟系统 tick (实际项目应使用真实定时器) */
static uint32_t g_system_tick = 0;

uint32_t xy_device_get_tick(void)
{
    return g_system_tick;
}

/* 模拟 tick 递增 (应在系统定时器中断中调用) */
void simulate_tick_increment(uint32_t ms)
{
    g_system_tick += ms;
}

/* ==================== 设备实例 ==================== */

/* 定义几个示例设备 */
static xy_i2c_device_t g_sht30_sensor;
static xy_i2c_device_t g_oled_display;
static xy_spi_device_t g_flash_memory;
static xy_gpio_device_t g_led;

/* 模拟 I2C/SPI/GPIO 句柄 */
static void *g_i2c_handle1 = (void *)0x40005000;  /* I2C1 */
static void *g_i2c_handle2 = (void *)0x40005400;  /* I2C2 */
static void *g_spi_handle1 = (void *)0x40003800;  /* SPI1 */
static void *g_gpio_porta = (void *)0x48000000;   /* GPIOA */

/* ==================== 电源管理回调 ==================== */

/**
 * @brief 设备电源管理回调示例
 */
static int device_pm_callback(xy_device_t *dev, 
                              xy_device_pm_event_t event,
                              void *user_data)
{
    const char *dev_name = dev->name ? dev->name : "unknown";
    
    if (event == XY_DEVICE_PM_WAKE) {
        printf("[PM] Device '%s' waking up...\r\n", dev_name);
        /* 这里执行实际的硬件唤醒操作 */
        /* 例如：开启电源、初始化寄存器等 */
    } else if (event == XY_DEVICE_PM_SLEEP) {
        printf("[PM] Device '%s' entering sleep...\r\n", dev_name);
        /* 这里执行实际的硬件休眠操作 */
        /* 例如：关闭电源、保存状态等 */
    }
    
    return XY_DEVICE_OK;
}

/* ==================== 设备操作回调 ==================== */

/**
 * @brief 设备遍历回调示例
 */
static int print_device_info(xy_device_t *dev, void *arg)
{
    (void)arg;
    
    const char *type_str = "UNKNOWN";
    switch (dev->type) {
        case XY_DEVICE_TYPE_I2C:    type_str = "I2C"; break;
        case XY_DEVICE_TYPE_SPI:    type_str = "SPI"; break;
        case XY_DEVICE_TYPE_UART:   type_str = "UART"; break;
        case XY_DEVICE_TYPE_GPIO:   type_str = "GPIO"; break;
        case XY_DEVICE_TYPE_SENSOR: type_str = "SENSOR"; break;
        case XY_DEVICE_TYPE_DISPLAY: type_str = "DISPLAY"; break;
        case XY_DEVICE_TYPE_MEMORY: type_str = "MEMORY"; break;
        default: break;
    }
    
    printf("  - %s (%s)%s\r\n", 
           dev->name ? dev->name : "(null)",
           type_str,
           dev->initialized ? " [INIT]" : "");
    
    return XY_DEVICE_OK;
}

/* ==================== 示例函数 ==================== */

/**
 * @brief 示例 1: 初始化设备注册表
 */
static void example_registry_init(void)
{
    printf("\r\n=== Example 1: Registry Initialization ===\r\n");
    
    int ret = xy_device_registry_init();
    if (ret == XY_DEVICE_OK) {
        printf("Device registry initialized successfully\r\n");
    }
}

/**
 * @brief 示例 2: 注册设备
 */
static void example_register_devices(void)
{
    printf("\r\n=== Example 2: Register Devices ===\r\n");
    
    /* 初始化 SHT30 温湿度传感器 */
    xy_i2c_device_init(&g_sht30_sensor, g_i2c_handle1, 0x44, 1000);
    g_sht30_sensor.base.name = "sht30";
    g_sht30_sensor.base.type = XY_DEVICE_TYPE_SENSOR;
    
    /* 初始化 OLED 显示屏 */
    xy_i2c_device_init(&g_oled_display, g_i2c_handle1, 0x3C, 1000);
    g_oled_display.base.name = "oled";
    g_oled_display.base.type = XY_DEVICE_TYPE_DISPLAY;
    
    /* 初始化 SPI Flash */
    xy_spi_device_init(&g_flash_memory, g_spi_handle1, NULL, 10000000);
    g_flash_memory.base.name = "flash";
    g_flash_memory.base.type = XY_DEVICE_TYPE_MEMORY;
    
    /* 初始化 LED GPIO */
    xy_gpio_device_init(&g_led, g_gpio_porta, 5, XY_GPIO_MODE_OUTPUT, XY_GPIO_PULL_NONE);
    g_led.base.name = "led";
    g_led.base.type = XY_DEVICE_TYPE_GPIO;
    
    /* 注册设备到全局注册表 */
    xy_device_registry_register(&g_sht30_sensor.base);
    xy_device_registry_register(&g_oled_display.base);
    xy_device_registry_register(&g_flash_memory.base);
    xy_device_registry_register(&g_led.base);
    
    printf("Registered 4 devices:\r\n");
    xy_device_foreach(print_device_info, NULL);
}

/**
 * @brief 示例 3: 查找设备
 */
static void example_find_devices(void)
{
    printf("\r\n=== Example 3: Find Devices ===\r\n");
    
    /* 按名称查找 */
    xy_device_t *dev = xy_device_find_by_name("sht30");
    if (dev) {
        printf("Found device by name 'sht30': type=%d, initialized=%d\r\n",
               dev->type, dev->initialized);
    }
    
    /* 按类型查找 */
    dev = xy_device_find_by_type(XY_DEVICE_TYPE_I2C, 0);
    if (dev) {
        printf("Found first I2C device: '%s'\r\n", dev->name ? dev->name : "(null)");
    }
    
    dev = xy_device_find_by_type(XY_DEVICE_TYPE_I2C, 1);
    if (dev) {
        printf("Found second I2C device: '%s'\r\n", dev->name ? dev->name : "(null)");
    }
}

/**
 * @brief 示例 4: 设备引用计数
 */
static void example_ref_count(void)
{
    printf("\r\n=== Example 4: Reference Counting ===\r\n");
    
    xy_device_t *dev = xy_device_find_by_name("sht30");
    if (!dev) return;
    
    printf("Initial ref count: %d\r\n", xy_device_get_ref_count(dev));
    
    /* 使用设备前增加引用 */
    xy_device_acquire(dev);
    printf("After acquire: %d\r\n", xy_device_get_ref_count(dev));
    
    xy_device_acquire(dev);
    printf("After second acquire: %d\r\n", xy_device_get_ref_count(dev));
    
    /* 使用设备后减少引用 */
    xy_device_release(dev);
    printf("After release: %d\r\n", xy_device_get_ref_count(dev));
    
    xy_device_release(dev);
    printf("After second release: %d\r\n", xy_device_get_ref_count(dev));
}

/**
 * @brief 示例 5: 电源管理
 */
static void example_power_management(void)
{
    printf("\r\n=== Example 5: Power Management ===\r\n");
    
    xy_device_t *dev = xy_device_find_by_name("oled");
    if (!dev) return;
    
    /* 设置电源管理回调 */
    xy_device_set_pm_callback(dev, device_pm_callback, NULL);
    
    /* 设置休眠超时为 5 秒 */
    xy_device_set_sleep_timeout(dev, 5000);
    
    printf("PM state: %d (0=UNKNOWN, 1=ACTIVE, 2=SLEEP)\r\n", 
           xy_device_get_pm_state(dev));
    
    /* 手动进入休眠 */
    printf("Putting device to sleep...\r\n");
    xy_device_sleep(dev);
    printf("PM state after sleep: %d\r\n", xy_device_get_pm_state(dev));
    
    /* 唤醒设备 (acquire 会自动唤醒) */
    printf("Waking up device...\r\n");
    xy_device_acquire(dev);
    printf("PM state after wake: %d\r\n", xy_device_get_pm_state(dev));
    
    xy_device_release(dev);
}

/**
 * @brief 示例 6: 自动电源管理检查
 */
static void example_auto_pm(void)
{
    printf("\r\n=== Example 6: Auto Power Management ===\r\n");
    
    xy_device_t *dev = xy_device_find_by_name("flash");
    if (!dev) return;
    
    /* 设置电源管理回调和 3 秒超时 */
    xy_device_set_pm_callback(dev, device_pm_callback, NULL);
    xy_device_set_sleep_timeout(dev, 3000);
    
    printf("Simulating time passage...\r\n");
    
    /* 模拟时间流逝 */
    for (int i = 0; i < 5; i++) {
        simulate_tick_increment(1000); /* 增加 1 秒 */
        printf("Tick: %d, PM state: %d\r\n", 
               g_system_tick, xy_device_get_pm_state(dev));
        
        /* 定期检查电源管理 */
        xy_device_pm_check();
    }
    
    printf("Device should be in sleep now\r\n");
}

/**
 * @brief 示例 7: 设备统计信息
 */
static void example_statistics(void)
{
    printf("\r\n=== Example 7: Device Statistics ===\r\n");
    
    xy_device_stats_t stats;
    xy_device_get_stats(&stats);
    
    printf("Total devices: %zu\r\n", stats.total_devices);
    printf("  I2C:     %zu\r\n", stats.i2c_count);
    printf("  SPI:     %zu\r\n", stats.spi_count);
    printf("  UART:    %zu\r\n", stats.uart_count);
    printf("  GPIO:    %zu\r\n", stats.gpio_count);
    printf("  Sensor:  %zu\r\n", stats.sensor_count);
    printf("  Display: %zu\r\n", stats.display_count);
    printf("  Memory:  %zu\r\n", stats.memory_count);
    printf("  Other:   %zu\r\n", stats.other_count);
    printf("  Sleeping: %zu\r\n", stats.sleep_count);
}

/**
 * @brief 示例 8: 打印设备列表
 */
static void example_print_list(void)
{
    printf("\r\n=== Example 8: Print Device List ===\r\n");
    xy_device_print_list();
}

/* ==================== 主函数 ==================== */

/**
 * @brief 运行所有示例
 */
int main(void)
{
    printf("\r\n");
    printf("╔════════════════════════════════════════════════╗\r\n");
    printf("║  XinYi Device Framework Examples               ║\r\n");
    printf("║  Device Registry & Power Management            ║\r\n");
    printf("╚════════════════════════════════════════════════╝\r\n");
    
    /* 运行示例 */
    example_registry_init();
    example_register_devices();
    example_find_devices();
    example_ref_count();
    example_power_management();
    example_auto_pm();
    example_statistics();
    example_print_list();
    
    printf("\r\n=== All Examples Completed ===\r\n\r\n");
    
    return 0;
}
