/**
 * @file main.c
 * @brief STM32F405 ADC 测试程序
 * 
 * QEMU olimex-stm32-h405 开发板 ADC 测试
 * - ADC1 单通道转换
 * - PA0 (ADC1_IN0)
 */

#include <stdint.h>

/*============================================================================
 *  半主机调试输出
 *===========================================================================*/

#define SYS_WRITE0  0x04

static inline void semihosting_call(uint32_t op, const void *arg)
{
    __asm__ volatile (
        "mov r0, %0\n"
        "mov r1, %1\n"
        "bkpt 0xAB\n"
        :
        : "r"(op), "r"(arg)
        : "r0", "r1", "memory"
    );
}

static void print_str(const char *str)
{
    semihosting_call(SYS_WRITE0, str);
}

/*============================================================================
 *  STM32F405 寄存器定义
 *===========================================================================*/

/* RCC */
#define RCC_BASE            0x40023800UL
#define RCC_APB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0x44))
#define RCC_APB2ENR_ADC1EN  (1 << 8)   /* ADC1 时钟使能 */
#define RCC_AHB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x30))
#define RCC_AHB1ENR_GPIOAEN (1 << 0)   /* GPIOA 时钟使能 */

/* GPIOA */
#define GPIOA_BASE          0x40020000UL
#define GPIOA_MODER         (*(volatile uint32_t *)(GPIOA_BASE + 0x00))

/* ADC1 */
#define ADC1_BASE           0x40012000UL
#define ADC1_SR             (*(volatile uint32_t *)(ADC1_BASE + 0x00))
#define ADC1_CR1            (*(volatile uint32_t *)(ADC1_BASE + 0x04))
#define ADC1_CR2            (*(volatile uint32_t *)(ADC1_BASE + 0x08))
#define ADC1_SMPR2          (*(volatile uint32_t *)(ADC1_BASE + 0x14))
#define ADC1_SQR1           (*(volatile uint32_t *)(ADC1_BASE + 0x2C))
#define ADC1_SQR3           (*(volatile uint32_t *)(ADC1_BASE + 0x34))
#define ADC1_DR             (*(volatile uint32_t *)(ADC1_BASE + 0x4C))

/* ADC 控制位 */
#define ADC_SR_EOC          (1 << 1)   /* 转换完成 */
#define ADC_SR_AWD          (1 << 0)   /* 模拟看门狗 */

#define ADC_CR1_RES         (0 << 24)  /* 12 位分辨率 */

#define ADC_CR2_ADON        (1 << 0)   /* ADC 使能 */
#define ADC_CR2_CONT        (1 << 1)   /* 连续转换 */
#define ADC_CR2_EOCS        (1 << 10)  /* 每次转换结束 */
#define ADC_CR2_SWSTART     (1 << 30)  /* 软件启动转换 */

/* ADC 采样时间 */
#define ADC_SMPR2_SMP0      (7 << 0)   /* 480 周期 */

/*============================================================================
 *  延时函数
 *===========================================================================*/

static void delay_ms(uint32_t ms)
{
    volatile uint32_t count = ms * 15000;
    while (count--);
}

/*============================================================================
 *  ADC 初始化
 *===========================================================================*/

static void adc_init(void)
{
    /* 1. 使能 GPIOA 和 ADC1 时钟 */
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC_APB2ENR |= RCC_APB2ENR_ADC1EN;
    
    /* 2. 配置 PA0 为模拟模式 */
    GPIOA_MODER |= (0x3 << 0);  /* PA0 模拟模式 */
    
    /* 3. 配置 ADC1 */
    /* 禁用 ADC */
    ADC1_CR2 &= ~ADC_CR2_ADON;
    
    /* 配置：12 位分辨率，连续转换模式 */
    ADC1_CR1 = ADC_CR1_RES;
    ADC1_CR2 = ADC_CR2_CONT | ADC_CR2_EOCS;
    
    /* 配置通道 0 采样时间 (PA0 - ADC1_IN0) */
    ADC1_SMPR2 = ADC_SMPR2_SMP0;
    
    /* 配置规则序列 - 转换通道 0 */
    ADC1_SQR1 = 0;  /* 1 个转换 */
    ADC1_SQR3 = 0;  /* 通道 0 在第一个位置 */
    
    /* 使能 ADC */
    ADC1_CR2 |= ADC_CR2_ADON;
    
    /* 等待 ADC 就绪 */
    delay_ms(1);
}

/*============================================================================
 *  ADC 读取函数
 *===========================================================================*/

static uint16_t adc_read(void)
{
    /* 启动转换 */
    ADC1_CR2 |= ADC_CR2_SWSTART;
    
    /* 等待转换完成 */
    while (!(ADC1_SR & ADC_SR_EOC));
    
    /* 清除 EOC 标志 */
    ADC1_SR &= ~ADC_SR_EOC;
    
    /* 读取结果 */
    return (uint16_t)(ADC1_DR & 0xFFF);
}

/*============================================================================
 *  打印数字
 *===========================================================================*/

static void print_uint(uint16_t value)
{
    char buf[6];
    int i = 0;
    
    if (value == 0) {
        print_str("0");
        return;
    }
    
    while (value > 0 && i < 5) {
        buf[i++] = '0' + (value % 10);
        value /= 10;
    }
    
    while (i > 0) {
        char c[2] = {buf[--i], '\0'};
        print_str(c);
    }
}

/*============================================================================
 *  主函数
 *===========================================================================*/

int main(void)
{
    uint16_t adc_value;
    uint32_t voltage_mv;
    
    print_str("\n");
    print_str("========================================\n");
    print_str("  XinYi STM32F4 ADC Test!\n");
    print_str("  MCU: STM32F405RG (Cortex-M4F)\n");
    print_str("  ADC1: PA0 (ADC1_IN0)\n");
    print_str("  Resolution: 12-bit (0-4095)\n");
    print_str("  Vref: 3.3V\n");
    print_str("========================================\n\n");
    
    /* 初始化 ADC */
    print_str("[INIT] Initializing ADC1...\n");
    adc_init();
    print_str("[INIT] ADC initialized.\n\n");
    
    print_str("[TEST] ADC Sampling (PA0)...\n");
    print_str("[INFO] In QEMU, ADC returns simulated values.\n\n");
    
    /* 连续采样 */
    for (uint8_t i = 0; i < 20; i++) {
        adc_value = adc_read();
        
        /* 计算电压 (mV) - 假设 Vref = 3.3V */
        voltage_mv = (uint32_t)adc_value * 3300 / 4095;
        
        print_str("[ADC] Sample ");
        print_uint(i);
        print_str(": ");
        print_uint(adc_value);
        print_str(" (");
        print_uint((uint16_t)voltage_mv);
        print_str(" mV)\n");
        
        delay_ms(200);
    }
    
    print_str("\n[INFO] ADC continuous monitoring...\n");
    
    /* 持续监控 */
    while (1) {
        adc_value = adc_read();
        voltage_mv = (uint32_t)adc_value * 3300 / 4095;
        
        print_str("[ADC] ");
        print_uint((uint16_t)voltage_mv);
        print_str(" mV\n");
        
        delay_ms(500);
    }
    
    return 0;
}
