/**
 * @file main.c
 * @brief XinYi Component Demo - Main Entry
 * @version 1.0.0
 * @date 2026-03-13
 */

#include <stdio.h>
#include "xy_os.h"

#ifdef DEMO_OSAL
extern int demo_osal_init(void);
extern void demo_osal_run(void);
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

static void print_header(void)
{
    printf("\n=================================================\n");
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

static void print_footer(void)
{
    printf("\n=================================================\n");
    printf("  Demo completed successfully!\n");
    printf("=================================================\n\n");
}

int main(void)
{
    printf("[INFO] System initialized\n\n");
    
#ifdef DEMO_OSAL
    demo_osal_init();
#endif

#ifdef DEMO_DEVICE
    demo_device_init();
#endif

#ifdef DEMO_SENSOR
    demo_sensor_init();
#endif

#ifdef DEMO_CRYPTO
    demo_crypto_init();
#endif

#ifdef DEMO_OSAL
    printf("--- OSAL Demo ---\n");
    demo_osal_run();
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

    print_footer();
    return 0;
}
