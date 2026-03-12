/**
 * @file demo_pm.c
 * @brief Power Management Demo
 * @version 1.0.0
 * @date 2026-03-13
 */

#include <stdio.h>

/* Simplified log macros for demo */
#define xy_log_i(fmt, ...) printf(fmt, ##__VA_ARGS__)
#define xy_log_d(fmt, ...) printf(fmt, ##__VA_ARGS__)

#ifdef DEMO_PM

/**
 * @brief 初始化电源管理演示
 */
int demo_pm_init(void)
{
    xy_log_i("Power Management module initialized\n");
    return 0;
}

/**
 * @brief 运行电源管理演示
 */
void demo_pm_run(void)
{
    /* 电池电压演示 */
    xy_log_i("Battery Monitoring:\n");
    
    /* 模拟电池电压 (mV) */
    unsigned int voltage_mV = 3700;  /* 3.7V */
    xy_log_d("  Battery Voltage: %d.%02dV\n", 
             voltage_mV / 1000, 
             (voltage_mV % 1000) / 10);
    
    /* 估算电量百分比 */
    unsigned int percent;
    if (voltage_mV >= 4200) percent = 100;
    else if (voltage_mV >= 4000) percent = 80;
    else if (voltage_mV >= 3800) percent = 60;
    else if (voltage_mV >= 3700) percent = 50;
    else if (voltage_mV >= 3600) percent = 40;
    else if (voltage_mV >= 3500) percent = 30;
    else if (voltage_mV >= 3400) percent = 20;
    else if (voltage_mV >= 3300) percent = 10;
    else percent = 0;
    
    xy_log_d("  Battery Level: %d%%\n", percent);
    
    /* 充电状态 */
    int is_charging = 0;  /* 模拟未充电 */
    xy_log_d("  Charging: %s\n", is_charging ? "YES" : "NO");
    
    /* 低功耗模式演示 */
    xy_log_i("Low Power Mode Demo:\n");
    
    xy_log_d("  Entering LOW_POWER mode...\n");
    xy_os_delay(100);
    xy_log_d("  System in low power state\n");
    
    xy_os_delay(100);
    xy_log_d("  Waking up from low power mode\n");
    
    /* 关机演示 */
    xy_log_i("Shutdown Demo:\n");
    xy_log_d("  Preparing for shutdown...\n");
    xy_log_d("  Saving system state...\n");
    xy_log_d("  Closing peripherals...\n");
    xy_log_d("  System ready for shutdown\n");
    
    xy_log_i("Power Management demo completed\n");
}

#endif /* DEMO_PM */
