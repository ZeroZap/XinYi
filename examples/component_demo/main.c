/**
 * @file main.c
 * @brief XinYi Component Demo - Main Entry (Standalone PC Version)
 * @version 1.0.0
 * @date 2026-03-13
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Demo function declarations */
#ifdef DEMO_OSAL
extern int demo_osal_init(void);
extern void demo_osal_run(void);
#endif

#ifdef DEMO_DEVICE
extern int demo_device_init(void);
extern void demo_device_run(void);
#endif

#ifdef DEMO_CRYPTO
extern int demo_crypto_init(void);
extern void demo_crypto_run(void);
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
    printf("  Platform: PC Simulator\n");
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
    
    printf("[INFO] System initialized\n\n");
    
    /* 运行 OSAL 演示 */
    #ifdef DEMO_OSAL
    printf("--- OSAL Demo ---\n");
    ret = demo_osal_init();
    if (ret == 0) {
        demo_osal_run();
    }
    printf("\n");
    #endif
    
    /* 运行设备演示 */
    #ifdef DEMO_DEVICE
    printf("--- Device Demo ---\n");
    ret = demo_device_init();
    if (ret == 0) {
        demo_device_run();
    }
    printf("\n");
    #endif
    
    /* 运行加密演示 */
    #ifdef DEMO_CRYPTO
    printf("--- Crypto Demo ---\n");
    ret = demo_crypto_init();
    if (ret == 0) {
        demo_crypto_run();
    }
    printf("\n");
    #endif
    
    print_footer();
    
    return 0;
}
