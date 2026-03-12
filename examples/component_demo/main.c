/**
 * @file main.c
 * @brief XinYi Component Demo - Main Entry
 * @version 1.0.0
 * @date 2026-03-13
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xy_os.h"
#include "xy_log.h"
#include "xy_device.h"
#include "xy_device_core.h"

/* Demo function declarations */
#ifdef DEMO_OSAL
extern int demo_osal_init(void);
extern void demo_osal_run(void);
#endif

#ifdef DEMO_HAL
extern int demo_hal_init(void);
extern void demo_hal_run(void);
#endif

#ifdef DEMO_DEVICE
extern int demo_device_init(void);
extern void demo_device_run(void);
#endif

#ifdef DEMO_SENSOR
extern int demo_sensor_init(void);
extern void demo_sensor_run(void);
#endif

#ifdef DEMO_CRYPTO
extern int demo_crypto_init(void);
extern void demo_crypto_run(void);
#endif

#ifdef DEMO_PM
extern int demo_pm_init(void);
extern void demo_pm_run(void);
#endif

/**
 * @brief 打印演示标题
 */
static void print_header(void)
{
    printf("\n");
    printf("=================================================\n");
    printf("  XinYi Component Demo\n");
    printf("  Version: 1.0.0\n");
    printf("  Platform: %s\n", 
    #ifdef HAL_PLATFORM_PC
        "PC Simulator"
    #else
        "Embedded"
    #endif
    );
    printf("=================================================\n\n");
}

/**
 * @brief 打印演示结束
 */
static void print_footer(void)
{
    printf("\n");
    printf("=================================================\n");
    printf("  Demo completed successfully!\n");
    printf("=================================================\n\n");
}

/**
 * @brief 主函数
 */
int main(void)
{
    int ret;
    
    print_header();
    
    /* 初始化日志系统 */
    xy_log_init();
    xy_log_i("System initialized\n");
    
    /* 初始化 OSAL */
    #ifdef DEMO_OSAL
    xy_log_i("Initializing OSAL...\n");
    ret = demo_osal_init();
    if (ret != 0) {
        xy_log_e("OSAL init failed: %d\n", ret);
        return -1;
    }
    #endif
    
    /* 初始化 HAL */
    #ifdef DEMO_HAL
    xy_log_i("Initializing HAL...\n");
    ret = demo_hal_init();
    if (ret != 0) {
        xy_log_e("HAL init failed: %d\n", ret);
        return -1;
    }
    #endif
    
    /* 初始化设备框架 */
    #ifdef DEMO_DEVICE
    xy_log_i("Initializing Device framework...\n");
    ret = demo_device_init();
    if (ret != 0) {
        xy_log_e("Device init failed: %d\n", ret);
        return -1;
    }
    #endif
    
    /* 初始化传感器 */
    #ifdef DEMO_SENSOR
    xy_log_i("Initializing Sensors...\n");
    ret = demo_sensor_init();
    if (ret != 0) {
        xy_log_e("Sensor init failed: %d\n", ret);
        return -1;
    }
    #endif
    
    /* 初始化加密模块 */
    #ifdef DEMO_CRYPTO
    xy_log_i("Initializing Crypto...\n");
    ret = demo_crypto_init();
    if (ret != 0) {
        xy_log_e("Crypto init failed: %d\n", ret);
        return -1;
    }
    #endif
    
    /* 初始化电源管理 */
    #ifdef DEMO_PM
    xy_log_i("Initializing Power Management...\n");
    ret = demo_pm_init();
    if (ret != 0) {
        xy_log_e("PM init failed: %d\n", ret);
        return -1;
    }
    #endif
    
    xy_log_i("All components initialized\n\n");
    
    /* 运行演示 */
    #ifdef DEMO_OSAL
    printf("--- OSAL Demo ---\n");
    demo_osal_run();
    printf("\n");
    #endif
    
    #ifdef DEMO_HAL
    printf("--- HAL Demo ---\n");
    demo_hal_run();
    printf("\n");
    #endif
    
    #ifdef DEMO_DEVICE
    printf("--- Device Demo ---\n");
    demo_device_run();
    printf("\n");
    #endif
    
    #ifdef DEMO_SENSOR
    printf("--- Sensor Demo ---\n");
    demo_sensor_run();
    printf("\n");
    #endif
    
    #ifdef DEMO_CRYPTO
    printf("--- Crypto Demo ---\n");
    demo_crypto_run();
    printf("\n");
    #endif
    
    #ifdef DEMO_PM
    printf("--- Power Management Demo ---\n");
    demo_pm_run();
    printf("\n");
    #endif
    
    print_footer();
    
    return 0;
}
