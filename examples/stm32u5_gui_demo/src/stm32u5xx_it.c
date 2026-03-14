/**
 * @file stm32u5xx_it.c
 * @brief STM32U5 Interrupt Service Routines
 * @version 1.0.0
 * @date 2026-03-14
 */

#include <stdint.h>
#include "stm32u5xx.h"

/* External variables */
extern volatile uint32_t g_system_ticks;

/**
 * @brief This function handles Non maskable interrupt.
 */
void NMI_Handler(void)
{
    while (1)
    {
    }
}

/**
 * @brief This function handles Hard fault interrupt.
 */
void HardFault_Handler(void)
{
    while (1)
    {
    }
}

/**
 * @brief This function handles Memory management fault.
 */
void MemManage_Handler(void)
{
    while (1)
    {
    }
}

/**
 * @brief This function handles Pre-fetch fault, memory access fault.
 */
void BusFault_Handler(void)
{
    while (1)
    {
    }
}

/**
 * @brief This function handles Undefined instruction or illegal state.
 */
void UsageFault_Handler(void)
{
    while (1)
    {
    }
}

/**
 * @brief This function handles System service call via SWI instruction.
 */
void SVC_Handler(void)
{
}

/**
 * @brief This function handles Debug monitor.
 */
void DebugMon_Handler(void)
{
}

/**
 * @brief This function handles Pendable request for system service.
 */
void PendSV_Handler(void)
{
}

/**
 * @brief This function handles System tick timer.
 */
void SysTick_Handler(void)
{
    g_system_ticks++;
    
    /* HAL_IncTick(); // Uncomment if using HAL */
}

/**
 * @brief This function handles EXTI line interrupt.
 */
void EXTI0_IRQHandler(void)
{
    /* TODO: Handle EXTI interrupts */
}

/**
 * @brief This function handles EXTI line 1 interrupt.
 */
void EXTI1_IRQHandler(void)
{
    /* TODO: Handle EXTI interrupts */
}

/**
 * @brief This function handles TIM6 global interrupt.
 */
void TIM6_IRQHandler(void)
{
    /* TODO: Handle timer interrupts */
}
