/* STM32L4 HAL configuration for XinYi.
 *
 * Keep this file in the project include path before the STM32CubeL4 driver
 * include directory.  The upstream HAL requires applications to provide a
 * family-specific stm32l4xx_hal_conf.h.
 */
#ifndef STM32L4xx_HAL_CONF_H
#define STM32L4xx_HAL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_MODULE_ENABLED
#define HAL_ADC_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DAC_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_EXTI_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_I2C_MODULE_ENABLED
#define HAL_IWDG_MODULE_ENABLED
#define HAL_LPTIM_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_RNG_MODULE_ENABLED
#define HAL_RTC_MODULE_ENABLED
#define HAL_SPI_MODULE_ENABLED
#define HAL_TIM_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED
#define HAL_WWDG_MODULE_ENABLED

#if !defined(HSE_VALUE)
#define HSE_VALUE 8000000U
#endif
#if !defined(HSE_STARTUP_TIMEOUT)
#define HSE_STARTUP_TIMEOUT 100U
#endif
#if !defined(MSI_VALUE)
#define MSI_VALUE 4000000U
#endif
#if !defined(HSI_VALUE)
#define HSI_VALUE 16000000U
#endif
#if !defined(HSI48_VALUE)
#define HSI48_VALUE 48000000U
#endif
#if !defined(LSI_VALUE)
#define LSI_VALUE 32000U
#endif
#if !defined(LSE_VALUE)
#define LSE_VALUE 32768U
#endif
#if !defined(LSE_STARTUP_TIMEOUT)
#define LSE_STARTUP_TIMEOUT 5000U
#endif
#if !defined(EXTERNAL_SAI1_CLOCK_VALUE)
#define EXTERNAL_SAI1_CLOCK_VALUE 2097000U
#endif
#if !defined(EXTERNAL_SAI2_CLOCK_VALUE)
#define EXTERNAL_SAI2_CLOCK_VALUE 2097000U
#endif

#define VDD_VALUE 3300U
#define TICK_INT_PRIORITY 0x0FU
#define USE_RTOS 0U
#define PREFETCH_ENABLE 0U
#define INSTRUCTION_CACHE_ENABLE 1U
#define DATA_CACHE_ENABLE 1U

#define USE_HAL_ADC_REGISTER_CALLBACKS 0U
#define USE_HAL_DAC_REGISTER_CALLBACKS 0U
#define USE_HAL_I2C_REGISTER_CALLBACKS 0U
#define USE_HAL_LPTIM_REGISTER_CALLBACKS 0U
#define USE_HAL_RNG_REGISTER_CALLBACKS 0U
#define USE_HAL_RTC_REGISTER_CALLBACKS 0U
#define USE_HAL_SPI_REGISTER_CALLBACKS 0U
#define USE_HAL_TIM_REGISTER_CALLBACKS 0U
#define USE_HAL_UART_REGISTER_CALLBACKS 0U
#define USE_HAL_WWDG_REGISTER_CALLBACKS 0U

#define USE_SPI_CRC 1U

#ifdef HAL_RCC_MODULE_ENABLED
#include "stm32l4xx_hal_rcc.h"
#endif
#ifdef HAL_GPIO_MODULE_ENABLED
#include "stm32l4xx_hal_gpio.h"
#endif
#ifdef HAL_DMA_MODULE_ENABLED
#include "stm32l4xx_hal_dma.h"
#endif
#ifdef HAL_CORTEX_MODULE_ENABLED
#include "stm32l4xx_hal_cortex.h"
#endif
#ifdef HAL_ADC_MODULE_ENABLED
#include "stm32l4xx_hal_adc.h"
#endif
#ifdef HAL_DAC_MODULE_ENABLED
#include "stm32l4xx_hal_dac.h"
#endif
#ifdef HAL_FLASH_MODULE_ENABLED
#include "stm32l4xx_hal_flash.h"
#endif
#ifdef HAL_I2C_MODULE_ENABLED
#include "stm32l4xx_hal_i2c.h"
#endif
#ifdef HAL_IWDG_MODULE_ENABLED
#include "stm32l4xx_hal_iwdg.h"
#endif
#ifdef HAL_LPTIM_MODULE_ENABLED
#include "stm32l4xx_hal_lptim.h"
#endif
#ifdef HAL_PWR_MODULE_ENABLED
#include "stm32l4xx_hal_pwr.h"
#endif
#ifdef HAL_RNG_MODULE_ENABLED
#include "stm32l4xx_hal_rng.h"
#endif
#ifdef HAL_RTC_MODULE_ENABLED
#include "stm32l4xx_hal_rtc.h"
#endif
#ifdef HAL_SPI_MODULE_ENABLED
#include "stm32l4xx_hal_spi.h"
#endif
#ifdef HAL_TIM_MODULE_ENABLED
#include "stm32l4xx_hal_tim.h"
#endif
#ifdef HAL_UART_MODULE_ENABLED
#include "stm32l4xx_hal_uart.h"
#endif
#ifdef HAL_WWDG_MODULE_ENABLED
#include "stm32l4xx_hal_wwdg.h"
#endif
#ifdef HAL_EXTI_MODULE_ENABLED
#include "stm32l4xx_hal_exti.h"
#endif

#ifdef USE_FULL_ASSERT
#include <stdint.h>
void assert_failed(uint8_t *file, uint32_t line);
#endif

#ifdef __cplusplus
}
#endif

#endif /* STM32L4xx_HAL_CONF_H */
