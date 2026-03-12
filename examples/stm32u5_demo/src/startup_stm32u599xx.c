/**
 * @file startup_stm32u599xx.c
 * @brief STM32U599xx 启动文件
 * @brief Cortex-M33 内核启动代码
 */

#include <stdint.h>

/* 链接脚本中定义的符号 */
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;
extern uint32_t _estack;

/* 主函数声明 */
extern int main(void);

/* C++ 构造函数支持 (可选) */
extern void (*__init_array_start)(void);
extern void (*__init_array_end)(void);

static void call_ctors(void)
{
    void (**ctor)(void) = &__init_array_start;
    for (; ctor < &__init_array_end; ctor++) {
        (*ctor)();
    }
}

/**
 * @brief 复位处理函数
 */
void Reset_Handler(void)
{
    uint32_t *src, *dst;
    
    /* 初始化 .data 段 */
    src = &_sidata;
    dst = &_sdata;
    while (dst < &_edata) {
        *dst++ = *src++;
    }
    
    /* 清零 .bss 段 */
    dst = &_sbss;
    while (dst < &_ebss) {
        *dst++ = 0;
    }
    
    /* 配置 FPU (Cortex-M33 有 FPU) */
    /* CPACR: 启用 CP10 和 CP11 (FPU) */
    uint32_t *cpacr = (uint32_t *)0xE000ED88UL;
    *cpacr |= (0xFUL << 20);
    
    /* 数据同步屏障 */
    __asm__ volatile ("dsb");
    __asm__ volatile ("isb");
    
    /* 调用 C++ 构造函数 (如果有) */
    call_ctors();
    
    /* 调用主函数 */
    main();
    
    /* 如果 main 返回，进入死循环 */
    while (1) {
        __asm__ volatile ("wfi");
    }
}

/**
 * @brief 默认中断处理函数
 */
void Default_Handler(void)
{
    while (1) {
        __asm__ volatile ("wfi");
    }
}

/* 弱定义的中断处理函数 */
void NMI_Handler(void)            __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)      __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void)      __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void)       __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void)     __attribute__((weak, alias("Default_Handler")));
void SecureFault_Handler(void)    __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)            __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void)       __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)         __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)        __attribute__((weak, alias("Default_Handler")));

/* STM32U5 外设中断 (部分) */
void WWDG_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void PVD_PVM_IRQHandler(void)     __attribute__((weak, alias("Default_Handler")));
void RTC_IRQHandler(void)         __attribute__((weak, alias("Default_Handler")));
void RTC_S_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void TAMP_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void RAMCFG_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void FLASH_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void FLASH_S_IRQHandler(void)     __attribute__((weak, alias("Default_Handler")));
void GTZC_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void RCC_IRQHandler(void)         __attribute__((weak, alias("Default_Handler")));
void RCC_S_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void EXTI0_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void EXTI1_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void EXTI2_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void EXTI3_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void EXTI4_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void EXTI5_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void EXTI6_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void EXTI7_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void EXTI8_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void EXTI9_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void EXTI10_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void EXTI11_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void EXTI12_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void EXTI13_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void EXTI14_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void EXTI15_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void IWDG_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void ADC1_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void DAC1_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void FDCAN1_IT0_IRQHandler(void)  __attribute__((weak, alias("Default_Handler")));
void FDCAN1_IT1_IRQHandler(void)  __attribute__((weak, alias("Default_Handler")));
void TIM1_BRK_IRQHandler(void)    __attribute__((weak, alias("Default_Handler")));
void TIM1_UP_IRQHandler(void)     __attribute__((weak, alias("Default_Handler")));
void TIM1_TRG_COM_IRQHandler(void)__attribute__((weak, alias("Default_Handler")));
void TIM1_CC_IRQHandler(void)     __attribute__((weak, alias("Default_Handler")));
void TIM2_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void TIM3_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void TIM4_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void TIM5_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void TIM6_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void TIM7_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void TIM8_BRK_IRQHandler(void)    __attribute__((weak, alias("Default_Handler")));
void TIM8_UP_IRQHandler(void)     __attribute__((weak, alias("Default_Handler")));
void TIM8_TRG_COM_IRQHandler(void)__attribute__((weak, alias("Default_Handler")));
void TIM8_CC_IRQHandler(void)     __attribute__((weak, alias("Default_Handler")));
void I2C1_EV_IRQHandler(void)     __attribute__((weak, alias("Default_Handler")));
void I2C1_ER_IRQHandler(void)     __attribute__((weak, alias("Default_Handler")));
void I2C2_EV_IRQHandler(void)     __attribute__((weak, alias("Default_Handler")));
void I2C2_ER_IRQHandler(void)     __attribute__((weak, alias("Default_Handler")));
void I2C3_EV_IRQHandler(void)     __attribute__((weak, alias("Default_Handler")));
void I2C3_ER_IRQHandler(void)     __attribute__((weak, alias("Default_Handler")));
void I2C4_EV_IRQHandler(void)     __attribute__((weak, alias("Default_Handler")));
void I2C4_ER_IRQHandler(void)     __attribute__((weak, alias("Default_Handler")));
void SPI1_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void SPI2_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void SPI3_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void SPI4_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void USART1_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void USART2_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void USART3_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void UART4_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void UART5_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void LPUART1_IRQHandler(void)     __attribute__((weak, alias("Default_Handler")));
void LPUART2_IRQHandler(void)     __attribute__((weak, alias("Default_Handler")));
void QSPI_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void RNG_IRQHandler(void)         __attribute__((weak, alias("Default_Handler")));
void DMA1_Channel1_IRQHandler(void)   __attribute__((weak, alias("Default_Handler")));
void DMA1_Channel2_IRQHandler(void)   __attribute__((weak, alias("Default_Handler")));
void DMA1_Channel3_IRQHandler(void)   __attribute__((weak, alias("Default_Handler")));
void DMA1_Channel4_IRQHandler(void)   __attribute__((weak, alias("Default_Handler")));
void DMA1_Channel5_IRQHandler(void)   __attribute__((weak, alias("Default_Handler")));
void DMA1_Channel6_IRQHandler(void)   __attribute__((weak, alias("Default_Handler")));
void DMA1_Channel7_IRQHandler(void)   __attribute__((weak, alias("Default_Handler")));
void DMA1_Channel8_IRQHandler(void)   __attribute__((weak, alias("Default_Handler")));
void DMA2_Channel1_IRQHandler(void)   __attribute__((weak, alias("Default_Handler")));
void DMA2_Channel2_IRQHandler(void)   __attribute__((weak, alias("Default_Handler")));
void DMA2_Channel3_IRQHandler(void)   __attribute__((weak, alias("Default_Handler")));
void DMA2_Channel4_IRQHandler(void)   __attribute__((weak, alias("Default_Handler")));
void DMA2_Channel5_IRQHandler(void)   __attribute__((weak, alias("Default_Handler")));
void DMA2_Channel6_IRQHandler(void)   __attribute__((weak, alias("Default_Handler")));
void DMA2_Channel7_IRQHandler(void)   __attribute__((weak, alias("Default_Handler")));
void DMA2_Channel8_IRQHandler(void)   __attribute__((weak, alias("Default_Handler")));

/**
 * @brief 中断向量表
 */
__attribute__((section(".isr_vector")))
const uint32_t vector_table[] = {
    /* 核心中断 */
    (uint32_t)&_estack,              /* 栈顶指针 */
    (uint32_t)Reset_Handler,         /* Reset */
    (uint32_t)NMI_Handler,           /* NMI */
    (uint32_t)HardFault_Handler,     /* Hard Fault */
    (uint32_t)MemManage_Handler,     /* MPU Fault */
    (uint32_t)BusFault_Handler,      /* Bus Fault */
    (uint32_t)UsageFault_Handler,    /* Usage Fault */
    (uint32_t)SecureFault_Handler,   /* Secure Fault */
    0,                               /* Reserved */
    0,                               /* Reserved */
    0,                               /* Reserved */
    (uint32_t)SVC_Handler,           /* SVCall */
    (uint32_t)DebugMon_Handler,      /* Debug Monitor */
    0,                               /* Reserved */
    (uint32_t)PendSV_Handler,        /* PendSV */
    (uint32_t)SysTick_Handler,       /* SysTick */
    
    /* 外部中断 */
    (uint32_t)WWDG_IRQHandler,       /* Window WatchDog */
    (uint32_t)PVD_PVM_IRQHandler,    /* PVD/PVM through EXTI Line detection */
    (uint32_t)RTC_IRQHandler,        /* RTC through EXTI Line */
    (uint32_t)RTC_S_IRQHandler,      /* RTC Secure through EXTI Line */
    (uint32_t)TAMP_IRQHandler,       /* Tamper */
    (uint32_t)RAMCFG_IRQHandler,     /* RAM CFG */
    (uint32_t)FLASH_IRQHandler,      /* FLASH */
    (uint32_t)FLASH_S_IRQHandler,    /* FLASH Secure */
    (uint32_t)GTZC_IRQHandler,       /* GTZC */
    (uint32_t)RCC_IRQHandler,        /* RCC */
    (uint32_t)RCC_S_IRQHandler,      /* RCC Secure */
    (uint32_t)EXTI0_IRQHandler,      /* EXTI Line 0 */
    (uint32_t)EXTI1_IRQHandler,      /* EXTI Line 1 */
    (uint32_t)EXTI2_IRQHandler,      /* EXTI Line 2 */
    (uint32_t)EXTI3_IRQHandler,      /* EXTI Line 3 */
    (uint32_t)EXTI4_IRQHandler,      /* EXTI Line 4 */
    (uint32_t)EXTI5_IRQHandler,      /* EXTI Line 5 */
    (uint32_t)EXTI6_IRQHandler,      /* EXTI Line 6 */
    (uint32_t)EXTI7_IRQHandler,      /* EXTI Line 7 */
    (uint32_t)EXTI8_IRQHandler,      /* EXTI Line 8 */
    (uint32_t)EXTI9_IRQHandler,      /* EXTI Line 9 */
    (uint32_t)EXTI10_IRQHandler,     /* EXTI Line 10 */
    (uint32_t)EXTI11_IRQHandler,     /* EXTI Line 11 */
    (uint32_t)EXTI12_IRQHandler,     /* EXTI Line 12 */
    (uint32_t)EXTI13_IRQHandler,     /* EXTI Line 13 */
    (uint32_t)EXTI14_IRQHandler,     /* EXTI Line 14 */
    (uint32_t)EXTI15_IRQHandler,     /* EXTI Line 15 */
    (uint32_t)IWDG_IRQHandler,       /* IWDG */
    0,                               /* Reserved */
    (uint32_t)ADC1_IRQHandler,       /* ADC1 */
    (uint32_t)DAC1_IRQHandler,       /* DAC1 */
    (uint32_t)FDCAN1_IT0_IRQHandler, /* FDCAN1 IT0 */
    (uint32_t)FDCAN1_IT1_IRQHandler, /* FDCAN1 IT1 */
    (uint32_t)TIM1_BRK_IRQHandler,   /* TIM1 BRK */
    (uint32_t)TIM1_UP_IRQHandler,    /* TIM1 UP */
    (uint32_t)TIM1_TRG_COM_IRQHandler,/* TIM1 TRG COM */
    (uint32_t)TIM1_CC_IRQHandler,    /* TIM1 CC */
    (uint32_t)TIM2_IRQHandler,       /* TIM2 */
    (uint32_t)TIM3_IRQHandler,       /* TIM3 */
    (uint32_t)TIM4_IRQHandler,       /* TIM4 */
    (uint32_t)TIM5_IRQHandler,       /* TIM5 */
    (uint32_t)TIM6_IRQHandler,       /* TIM6 */
    (uint32_t)TIM7_IRQHandler,       /* TIM7 */
    (uint32_t)TIM8_BRK_IRQHandler,   /* TIM8 BRK */
    (uint32_t)TIM8_UP_IRQHandler,    /* TIM8 UP */
    (uint32_t)TIM8_TRG_COM_IRQHandler,/* TIM8 TRG COM */
    (uint32_t)TIM8_CC_IRQHandler,    /* TIM8 CC */
    (uint32_t)I2C1_EV_IRQHandler,    /* I2C1 EV */
    (uint32_t)I2C1_ER_IRQHandler,    /* I2C1 ER */
    (uint32_t)I2C2_EV_IRQHandler,    /* I2C2 EV */
    (uint32_t)I2C2_ER_IRQHandler,    /* I2C2 ER */
    (uint32_t)I2C3_EV_IRQHandler,    /* I2C3 EV */
    (uint32_t)I2C3_ER_IRQHandler,    /* I2C3 ER */
    (uint32_t)I2C4_EV_IRQHandler,    /* I2C4 EV */
    (uint32_t)I2C4_ER_IRQHandler,    /* I2C4 ER */
    (uint32_t)SPI1_IRQHandler,       /* SPI1 */
    (uint32_t)SPI2_IRQHandler,       /* SPI2 */
    (uint32_t)SPI3_IRQHandler,       /* SPI3 */
    (uint32_t)SPI4_IRQHandler,       /* SPI4 */
    (uint32_t)USART1_IRQHandler,     /* USART1 */
    (uint32_t)USART2_IRQHandler,     /* USART2 */
    (uint32_t)USART3_IRQHandler,     /* USART3 */
    (uint32_t)UART4_IRQHandler,      /* UART4 */
    (uint32_t)UART5_IRQHandler,      /* UART5 */
    (uint32_t)LPUART1_IRQHandler,    /* LPUART1 */
    (uint32_t)LPUART2_IRQHandler,    /* LPUART2 */
    (uint32_t)QSPI_IRQHandler,       /* QSPI */
    (uint32_t)RNG_IRQHandler,        /* RNG */
    (uint32_t)DMA1_Channel1_IRQHandler,  /* DMA1 Channel 1 */
    (uint32_t)DMA1_Channel2_IRQHandler,  /* DMA1 Channel 2 */
    (uint32_t)DMA1_Channel3_IRQHandler,  /* DMA1 Channel 3 */
    (uint32_t)DMA1_Channel4_IRQHandler,  /* DMA1 Channel 4 */
    (uint32_t)DMA1_Channel5_IRQHandler,  /* DMA1 Channel 5 */
    (uint32_t)DMA1_Channel6_IRQHandler,  /* DMA1 Channel 6 */
    (uint32_t)DMA1_Channel7_IRQHandler,  /* DMA1 Channel 7 */
    (uint32_t)DMA1_Channel8_IRQHandler,  /* DMA1 Channel 8 */
    (uint32_t)DMA2_Channel1_IRQHandler,  /* DMA2 Channel 1 */
    (uint32_t)DMA2_Channel2_IRQHandler,  /* DMA2 Channel 2 */
    (uint32_t)DMA2_Channel3_IRQHandler,  /* DMA2 Channel 3 */
    (uint32_t)DMA2_Channel4_IRQHandler,  /* DMA2 Channel 4 */
    (uint32_t)DMA2_Channel5_IRQHandler,  /* DMA2 Channel 5 */
    (uint32_t)DMA2_Channel6_IRQHandler,  /* DMA2 Channel 6 */
    (uint32_t)DMA2_Channel7_IRQHandler,  /* DMA2 Channel 7 */
    (uint32_t)DMA2_Channel8_IRQHandler,  /* DMA2 Channel 8 */
};
