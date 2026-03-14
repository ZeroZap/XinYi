/**
 * @file system_stm32u5xx.c
 * @brief CMSIS System Source File for STM32U5xx
 * @version 1.0.0
 * @date 2026-03-14
 */

#include <stdint.h>
#include "stm32u5xx.h"

/* System Clock Frequency (Core Clock) */
#if defined(STM32U599xx) || defined(STM32U5A9xx)
#define SYSTEM_CLOCK_FREQ 160000000U /* 160 MHz */
#else
#define SYSTEM_CLOCK_FREQ 160000000U /* Default 160 MHz */
#endif

/* System Clock Frequency (Core Clock) Variable */
uint32_t SystemCoreClock = SYSTEM_CLOCK_FREQ;

const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
const uint8_t APBPrescTable[8] = {0, 0, 0, 0, 1, 2, 3, 4};

/**
 * @brief Setup the microcontroller system.
 * @note  This function should be used only after reset.
 */
void SystemInit(void)
{
    /* FPU settings ------------------------------------------------------------*/
    #if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
        SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));  /* set CP10 and CP11 Full Access */
    #endif
    
    /* Reset the RCC clock configuration to the default reset state ------------*/
    /* Set HSION bit */
    RCC->CR |= RCC_CR_HSION;
    
    /* Reset CFGR register */
    RCC->CFGR1 = 0x00000000U;
    RCC->CFGR2 = 0x00000000U;
    
    /* Reset HSEON, CSSON , HSION, and MSION bits */
    RCC->CR &= 0xEAF6FFF7U;
    
    /* Reset PLLCFGR register */
    RCC->PLLCFGR = 0x00000100U;
    
    /* Reset System clock configuration for secondary core (if applicable) */
    #if defined(DUAL_CORE)
        RCC->C2CR &= 0x00000000U;
    #endif
    
    /* Reset HSEBYP bit */
    RCC->CR &= 0xFFFBFFFFU;
    
    /* Disable all interrupts */
    RCC->CIER = 0x00000000U;
    
    /* Configure the Vector Table location -------------------------------------*/
    #ifdef VECT_TAB_SRAM
        SCB->VTOR = SRAM1_BASE; /* Vector Table Relocation in Internal SRAM */
    #else
        SCB->VTOR = FLASH_BASE; /* Vector Table Relocation in Internal FLASH */
    #endif
    
    /* Update SystemCoreClock variable */
    SystemCoreClockUpdate();
}

/**
 * @brief Update SystemCoreClock variable according to Clock Register Values.
 * @note  Whenever the core clock changes, this function must be called
 *        to update SystemCoreClock variable value.
 */
void SystemCoreClockUpdate(void)
{
    uint32_t clksource = 0;
    uint32_t sysclockfreq = 0;
    uint32_t hsefreq = 0;
    uint32_t msifreq = 0;
    uint32_t pllfreq = 0;
    uint32_t pllsource = 0;
    uint32_t plln = 0;
    uint32_t pllm = 0;
    uint32_t pllp = 0;
    uint32_t pllq = 0;
    uint32_t pllr = 0;
    uint32_t hpre = 0;
    
    /* Get SYSCLK source */
    clksource = RCC->CFGR1 & RCC_CFGR1_SWS;
    
    /* Get HSE frequency */
    if ((RCC->CR & RCC_CR_HSEBYP) == RCC_CR_HSEBYP)
    {
        hsefreq = HSE_VALUE;
    }
    else
    {
        hsefreq = HSE_VALUE;
    }
    
    /* Get MSI frequency */
    msifreq = MSI_RANGE;
    
    switch (clksource)
    {
        case RCC_CFGR1_SWS_MSI: /* MSI used as system clock source */
            sysclockfreq = msifreq;
            break;
            
        case RCC_CFGR1_SWS_HSI: /* HSI used as system clock source */
            sysclockfreq = HSI_VALUE;
            break;
            
        case RCC_CFGR1_SWS_HSE: /* HSE used as system clock source */
            sysclockfreq = hsefreq;
            break;
            
        case RCC_CFGR1_SWS_PLL1: /* PLL1 used as system clock source */
            pllsource = (RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC);
            pllm = ((RCC->PLLCFGR & RCC_PLLCFGR_PLLM) >> RCC_PLLCFGR_PLLM_Pos) + 1U;
            plln = ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> RCC_PLLCFGR_PLLN_Pos);
            pllp = (((RCC->PLLCFGR & RCC_PLLCFGR_PLLP) >> RCC_PLLCFGR_PLLP_Pos) + 1U) * 2U;
            pllq = ((RCC->PLLCFGR & RCC_PLLCFGR_PLLQ) >> RCC_PLLCFGR_PLLQ_Pos) + 1U;
            pllr = ((RCC->PLLCFGR & RCC_PLLCFGR_PLLR) >> RCC_PLLCFGR_PLLR_Pos) + 1U;
            
            if (pllsource == RCC_PLLCFGR_PLLSRC_HSI)
            {
                pllfreq = HSI_VALUE;
            }
            else if (pllsource == RCC_PLLCFGR_PLLSRC_HSE)
            {
                pllfreq = hsefreq;
            }
            else
            {
                pllfreq = msifreq;
            }
            
            pllfreq = (pllfreq / pllm) * plln;
            
            /* Check PLL1 output selected */
            if ((RCC->CFGR1 & RCC_CFGR1_SW) == RCC_CFGR1_SW_PLL1CLK)
            {
                sysclockfreq = pllfreq / pllp;
            }
            else if ((RCC->CFGR1 & RCC_CFGR1_SW) == RCC_CFGR1_SW_PLL1QCLK)
            {
                sysclockfreq = pllfreq / pllq;
            }
            else
            {
                sysclockfreq = pllfreq / pllr;
            }
            break;
            
        default:
            sysclockfreq = msifreq;
            break;
    }
    
    /* Get HCLK prescaler */
    hpre = ((RCC->CFGR1 & RCC_CFGR1_HPRE) >> RCC_CFGR1_HPRE_Pos);
    
    if (hpre < 8U)
    {
        SystemCoreClock = sysclockfreq >> AHBPrescTable[hpre];
    }
    else
    {
        SystemCoreClock = sysclockfreq / (1UL << (hpre - 7U));
    }
}
