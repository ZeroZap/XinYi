/**
 * @file xy_hal_sys.c
 * @brief WCH CH32V30x bounded System HAL implementation
 */

#include "xy_hal_sys.h"

#ifdef MCU_CH32

#include "ch32v30x.h"
#include "ch32v30x_dbgmcu.h"
#include <stdio.h>
#include <string.h>

static uint8_t s_initialized;

xy_hal_error_t xy_hal_sys_init(void) {
    SystemCoreClockUpdate();
    s_initialized = 1U;
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_sys_deinit(void) {
    s_initialized = 0U;
    return XY_HAL_OK;
}
uint32_t xy_hal_sys_get_tick_count(void) {
    return (uint32_t)(SysTick->CNT / (SystemCoreClock / 8U / 1000U));
}
uint32_t xy_hal_sys_get_tick_freq(void) {
    return XY_HAL_SYS_TICK_FREQ;
}
void xy_hal_sys_tick_irq_handler(void) {}
xy_hal_error_t xy_hal_sys_get_clock_info(xy_hal_sys_clock_info_t* info) {
    RCC_ClocksTypeDef clocks;
    if (info == NULL)
        return XY_HAL_ERROR_INVALID_PARAM;
    RCC_GetClocksFreq(&clocks);
    info->sysclk = clocks.SYSCLK_Frequency;
    info->hclk = clocks.HCLK_Frequency;
    info->pclk1 = clocks.PCLK1_Frequency;
    info->pclk2 = clocks.PCLK2_Frequency;
    info->pclk3 = 0U;
    info->hsi_ready = RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == SET;
    info->hse_ready = RCC_GetFlagStatus(RCC_FLAG_HSERDY) == SET;
    info->pll_ready = RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == SET;
    return XY_HAL_OK;
}
void xy_hal_sys_reset(void) {
    NVIC_SystemReset();
    for (;;) {
    }
}
xy_hal_error_t xy_hal_sys_software_reset(void) {
    NVIC_SystemReset();
    return XY_HAL_ERROR_FAIL;
}
uint32_t xy_hal_sys_get_reset_reason(void) {
    uint32_t reason = XY_HAL_SYS_RESET_REASON_NONE;
    if (RCC_GetFlagStatus(RCC_FLAG_PINRST) == SET)
        reason |= XY_HAL_SYS_RESET_REASON_EXTERNAL_PIN;
    if (RCC_GetFlagStatus(RCC_FLAG_SFTRST) == SET)
        reason |= XY_HAL_SYS_RESET_REASON_SOFTWARE;
    if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) == SET || RCC_GetFlagStatus(RCC_FLAG_WWDGRST) == SET)
        reason |= XY_HAL_SYS_RESET_REASON_WATCHDOG;
    if (RCC_GetFlagStatus(RCC_FLAG_LPWRRST) == SET)
        reason |= XY_HAL_SYS_RESET_REASON_LOW_POWER;
    return reason;
}
xy_hal_error_t xy_hal_sys_clear_reset_reason(void) {
    RCC_ClearFlag();
    return XY_HAL_OK;
}
int32_t xy_hal_sys_get_chip_id(void) {
    return (int32_t)DBGMCU_GetCHIPID();
}
xy_hal_error_t xy_hal_sys_get_unique_id(uint32_t id[3]) {
    if (id == NULL)
        return XY_HAL_ERROR_INVALID_PARAM;
    id[0] = *(const volatile uint32_t*)0x1FFFF7E8U;
    id[1] = *(const volatile uint32_t*)0x1FFFF7ECU;
    id[2] = *(const volatile uint32_t*)0x1FFFF7F0U;
    return XY_HAL_OK;
}
int32_t xy_hal_sys_get_chip_version(void) {
    return (int32_t)DBGMCU_GetREVID();
}
xy_hal_error_t xy_hal_sys_get_chip_name(char* name, size_t size) {
    static const char chip[] = "CH32V307VCT6";
    if (name == NULL || size < sizeof(chip))
        return XY_HAL_ERROR_INVALID_PARAM;
    memcpy(name, chip, sizeof(chip));
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_sys_get_chip_serial(char* serial, size_t size) {
    uint32_t id[3];
    int written;
    if (serial == NULL || size == 0U || xy_hal_sys_get_unique_id(id) != XY_HAL_OK)
        return XY_HAL_ERROR_INVALID_PARAM;
    written = snprintf(serial, size, "%08lX%08lX%08lX", (unsigned long)id[0], (unsigned long)id[1],
                       (unsigned long)id[2]);
    return written > 0 && (size_t)written < size ? XY_HAL_OK : XY_HAL_ERROR_INVALID_PARAM;
}
uint64_t xy_hal_sys_get_uptime_ms(void) {
    return xy_hal_sys_get_tick_count();
}
uint32_t xy_hal_sys_get_uptime_sec(void) {
    return xy_hal_sys_get_tick_count() / 1000U;
}
xy_hal_error_t xy_hal_sys_enter_pwr_mode(xy_hal_sys_pwr_mode_t mode) {
    if (mode != XY_HAL_SYS_PWR_SLEEP)
        return XY_HAL_ERROR_NOT_SUPPORTED;
    __WFI();
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_sys_exit_pwr_mode(void) {
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_sys_set_pwr_domain_config(xy_hal_sys_pwr_domain_t domain,
                                                const xy_hal_sys_pwr_config_t* config) {
    (void)domain;
    (void)config;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
xy_hal_error_t xy_hal_sys_get_pwr_domain_config(xy_hal_sys_pwr_domain_t domain,
                                                xy_hal_sys_pwr_config_t* config) {
    (void)domain;
    (void)config;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
xy_hal_error_t xy_hal_sys_enable_pwr_domain(xy_hal_sys_pwr_domain_t domain, uint8_t enable) {
    (void)domain;
    (void)enable;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
int32_t xy_hal_sys_get_pwr_domain_state(xy_hal_sys_pwr_domain_t domain) {
    (void)domain;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
int32_t xy_hal_sys_get_clk_freq(xy_hal_sys_clkdomain_t domain) {
    RCC_ClocksTypeDef clocks;
    RCC_GetClocksFreq(&clocks);
    switch (domain) {
        case XY_HAL_SYS_CLKDOMAIN_SYS:
            return (int32_t)clocks.SYSCLK_Frequency;
        case XY_HAL_SYS_CLKDOMAIN_AHB:
            return (int32_t)clocks.HCLK_Frequency;
        case XY_HAL_SYS_CLKDOMAIN_APB1:
            return (int32_t)clocks.PCLK1_Frequency;
        case XY_HAL_SYS_CLKDOMAIN_APB2:
            return (int32_t)clocks.PCLK2_Frequency;
        case XY_HAL_SYS_CLKDOMAIN_ADC:
            return (int32_t)clocks.ADCCLK_Frequency;
        default:
            return XY_HAL_ERROR_NOT_SUPPORTED;
    }
}
xy_hal_error_t xy_hal_sys_set_clk_freq(xy_hal_sys_clkdomain_t domain, uint32_t freq) {
    (void)domain;
    (void)freq;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
xy_hal_error_t xy_hal_sys_set_clk_source(xy_hal_sys_clkdomain_t domain,
                                         xy_hal_sys_clksrc_t source) {
    (void)domain;
    (void)source;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
int32_t xy_hal_sys_get_clk_source(xy_hal_sys_clkdomain_t domain) {
    return domain == XY_HAL_SYS_CLKDOMAIN_SYS ? XY_HAL_SYS_CLKSRC_HSI : XY_HAL_ERROR_NOT_SUPPORTED;
}
void xy_hal_sys_enable_irq(void) {
    __enable_irq();
}
void xy_hal_sys_disable_irq(void) {
    __disable_irq();
}
uint32_t xy_hal_sys_enter_critical(void) {
    uint32_t state = __get_MSTATUS();
    __disable_irq();
    return state;
}
void xy_hal_sys_exit_critical(uint32_t state) {
    __set_MSTATUS(state);
}
uint32_t xy_hal_sys_save_irq_state(void) {
    return xy_hal_sys_enter_critical();
}
void xy_hal_sys_restore_irq_state(uint32_t state) {
    xy_hal_sys_exit_critical(state);
}
uint32_t xy_hal_sys_get_irq_nest_level(void) {
    return 0U;
}
int32_t xy_hal_sys_get_current_irq(void) {
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
static int valid_irq(uint32_t irq) {
    return irq < 256U;
}
xy_hal_error_t xy_hal_sys_set_irq_priority(uint32_t irq, uint32_t priority) {
    if (!valid_irq(irq) || priority > 255U)
        return XY_HAL_ERROR_INVALID_PARAM;
    NVIC_SetPriority((IRQn_Type)irq, (uint8_t)priority);
    return XY_HAL_OK;
}
int32_t xy_hal_sys_get_irq_priority(uint32_t irq) {
    return valid_irq(irq) ? (int32_t)NVIC->IPRIOR[irq] : XY_HAL_ERROR_INVALID_PARAM;
}
xy_hal_error_t xy_hal_sys_enable_irq_num(uint32_t irq) {
    if (!valid_irq(irq))
        return XY_HAL_ERROR_INVALID_PARAM;
    NVIC_EnableIRQ((IRQn_Type)irq);
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_sys_disable_irq_num(uint32_t irq) {
    if (!valid_irq(irq))
        return XY_HAL_ERROR_INVALID_PARAM;
    NVIC_DisableIRQ((IRQn_Type)irq);
    return XY_HAL_OK;
}
void xy_hal_sys_memory_barrier(void) {
    __sync_synchronize();
}
void xy_hal_sys_instruction_barrier(void) {
    __asm volatile("fence.i" ::: "memory");
}
uint32_t xy_hal_sys_get_cpu_freq(void) {
    return SystemCoreClock;
}
uint32_t xy_hal_sys_get_flash_size(void) {
    return 288U * 1024U;
}
uint32_t xy_hal_sys_get_ram_size(void) {
    return 32U * 1024U;
}
uint32_t xy_hal_sys_get_available_ram(void) {
    return 0U;
}
void xy_hal_sys_delay_us(uint32_t us) {
    uint32_t ticks = SystemCoreClock / 8000000U;
    while (us-- != 0U)
        for (volatile uint32_t i = 0U; i < ticks; ++i) {
        }
}
uint64_t xy_hal_sys_get_timestamp_us(void) {
    return (uint64_t)SysTick->CNT * 8000000ULL / SystemCoreClock;
}
uint64_t xy_hal_sys_get_timestamp_ms(void) {
    return xy_hal_sys_get_timestamp_us() / 1000U;
}
uint64_t xy_hal_sys_diff_us(uint64_t start, uint64_t end) {
    return end - start;
}

#endif /* MCU_CH32 */
