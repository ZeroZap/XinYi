/**
 * @file demo_device.c
 * @brief Device Framework Demo (Standalone)
 * @version 1.0.0
 * @date 2026-03-13
 */

#include <stdio.h>
#include <string.h>
#include "osal_pc.h"

#ifdef DEMO_DEVICE

typedef struct {
    const char *name;
    int type;
    int ref_count;
    int power_state;
} device_t;

static device_t g_devices[10];
static int g_device_count = 0;

int demo_device_init(void)
{
    printf("  Device registry initialized (max 10 devices)\n");
    g_device_count = 0;
    return 0;
}

static int device_register(const char *name, int type)
{
    if (g_device_count >= 10) return -1;
    
    g_devices[g_device_count].name = name;
    g_devices[g_device_count].type = type;
    g_devices[g_device_count].ref_count = 0;
    g_devices[g_device_count].power_state = 1; /* ACTIVE */
    g_device_count++;
    return 0;
}

static void device_print_list(void)
{
    printf("  Device list:\n");
    for (int i = 0; i < g_device_count; i++) {
        const char *type_str = (g_devices[i].type == 1) ? "SENSOR" : "OTHER";
        const char *pm_str = (g_devices[i].power_state == 1) ? "ACTIVE" : "SLEEP";
        printf("    %-12s %-8s RefCnt=%d PM=%s\n", 
               g_devices[i].name, type_str, g_devices[i].ref_count, pm_str);
    }
}

void demo_device_run(void)
{
    printf("  Registering devices...\n");
    
    device_register("sht30_1", 1);
    printf("  Device 'sht30_1' registered (SENSOR)\n");
    
    device_register("mpu6050_1", 1);
    printf("  Device 'mpu6050_1' registered (SENSOR)\n");
    
    printf("  Device count: %d\n", g_device_count);
    
    device_print_list();
    
    /* 演示电源管理 */
    printf("  Testing power management...\n");
    g_devices[0].power_state = 0;
    printf("  Device 'sht30_1' entered SLEEP mode\n");
    
    xy_os_delay(50);
    
    g_devices[0].power_state = 1;
    printf("  Device 'sht30_1' woke up\n");
    
    /* 演示引用计数 */
    printf("  Testing reference counting...\n");
    g_devices[0].ref_count++;
    printf("  Device 'sht30_1' acquired (ref_count=%d)\n", g_devices[0].ref_count);
    
    g_devices[0].ref_count--;
    printf("  Device 'sht30_1' released (ref_count=%d)\n", g_devices[0].ref_count);
    
    printf("  Device demo completed\n");
}

#endif /* DEMO_DEVICE */
