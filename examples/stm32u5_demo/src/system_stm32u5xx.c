/**
 * @file system_stm32u5xx.c
 * @brief STM32U5 系统初始化文件
 * @brief Cortex-M33 系统配置
 */

#include <stdint.h>

/* 系统时钟频率 (默认 MSI 4MHz) */
#define SYSTEM_CLOCK_FREQ     4000000UL

/* 全局系统时钟频率变量 */
uint32_t SystemCoreClock = SYSTEM_CLOCK_FREQ;

/**
 * @brief 更新 SystemCoreClock 变量
 */
void SystemCoreClockUpdate(void)
{
    /* 简化实现：假设使用 MSI 4MHz
     * 实际应用中应根据 RCC 配置计算实际频率
     */
    SystemCoreClock = SYSTEM_CLOCK_FREQ;
}

/**
 * @brief 系统初始化函数
 * @note 在 main() 之前调用
 */
void SystemInit(void)
{
    /* FPU 设置 (Cortex-M33) */
    /* CPACR 寄存器地址 */
    #define CPACR_ADDR    0xE000ED88UL
    
    /* 启用 CP10 和 CP11 (FPU) */
    uint32_t *cpacr = (uint32_t *)CPACR_ADDR;
    *cpacr |= (0xFUL << 20);
    
    /* 数据同步屏障 */
    __asm__ volatile ("dsb");
    __asm__ volatile ("isb");
    
    /* 向量表偏移 (如果需要) */
    /* SCB->VTOR = FLASH_BASE; */
    
    /* 更新系统时钟频率 */
    SystemCoreClockUpdate();
}
