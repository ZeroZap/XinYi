/**
 * @file stm32u5xx_hal_conf.h
 * @brief HAL configuration file for STM32U5
 * @version 1.0.0
 * @date 2026-03-14
 */

#ifndef STM32U5XX_HAL_CONF_H
#define STM32U5XX_HAL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/* ########################## Module Selection ############################## */
#define HAL_MODULE_ENABLED
#define HAL_ADC_MODULE_ENABLED
#define HAL_CRC_MODULE_ENABLED
#define HAL_CRS_MODULE_ENABLED
#define HAL_DAC_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_EXTI_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_I2C_MODULE_ENABLED
#define HAL_I2S_MODULE_ENABLED
#define HAL_IRDA_MODULE_ENABLED
#define HAL_IWDG_MODULE_ENABLED
#define HAL_LPTIM_MODULE_ENABLED
#define HAL_NAND_MODULE_ENABLED
#define HAL_NOR_MODULE_ENABLED
#define HAL_OPAMP_MODULE_ENABLED
#define HAL_PCD_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_RNG_MODULE_ENABLED
#define HAL_RTC_MODULE_ENABLED
#define HAL_SD_MODULE_ENABLED
#define HAL_SMARTCARD_MODULE_ENABLED
#define HAL_SMBUS_MODULE_ENABLED
#define HAL_SPI_MODULE_ENABLED
#define HAL_SRAM_MODULE_ENABLED
#define HAL_TIM_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED
#define HAL_USART_MODULE_ENABLED
#define HAL_WWDG_MODULE_ENABLED

/* ########################## Oscillator Values adaptation ################## */
#if !defined(HSE_VALUE)
#define HSE_VALUE    8000000U  /*!< Value of the External oscillator in Hz */
#endif

#if !defined(HSE_STARTUP_TIMEOUT)
#define HSE_STARTUP_TIMEOUT    100U   /*!< Time out for HSE start up, in ms */
#endif

#if !defined(HSI_VALUE)
#define HSI_VALUE    16000000U /*!< Value of the Internal oscillator in Hz */
#endif

#if !defined(MSI_VALUE)
#define MSI_VALUE    4000000U /*!< Value of the Internal oscillator in Hz */
#endif

#if !defined(LSE_VALUE)
#define LSE_VALUE    32768U   /*!< Value of the External Low Speed oscillator in Hz */
#endif

#if !defined(LSE_STARTUP_TIMEOUT)
#define LSE_STARTUP_TIMEOUT    5000U   /*!< Time out for LSE start up, in ms */
#endif

#if !defined(LSI_VALUE)
#define LSI_VALUE    32000U   /*!< Value of the Internal Low Speed oscillator in Hz */
#endif

#if !defined(CLOCK_SWINGING)
#define CLOCK_SWINGING    0U   /*!< Clock spreading */
#endif

#if !defined(EXTERNAL_CLOCK_VALUE)
#define EXTERNAL_CLOCK_VALUE    16000000U /*!< Value of the External clock in Hz */
#endif

/* ########################### System Configuration ######################### */
#define  VDD_VALUE                    3300U /*!< Value of VDD in mv */
#define  TICK_INT_PRIORITY            15U   /*!< tick interrupt priority */
#define  USE_RTOS                     0U
#define  PREFETCH_ENABLE              0U
#define  INSTRUCTION_CACHE_ENABLE     1U
#define  DATA_CACHE_ENABLE            1U

/* ########################## Assert Selection ############################## */
/* #define USE_FULL_ASSERT    1U */

/* ################## SPI peripheral configuration ########################## */
#define USE_SPI_CRC                   0U

/* ################## UART peripheral configuration ########################## */
#define USE_UART_FRACTIONAL_DIVIDER   0U

/* Includes ------------------------------------------------------------------*/
#ifdef HAL_RCC_MODULE_ENABLED
#include "stm32u5xx_hal_rcc.h"
#endif

#ifdef HAL_GPIO_MODULE_ENABLED
#include "stm32u5xx_hal_gpio.h"
#endif

#ifdef HAL_DMA_MODULE_ENABLED
#include "stm32u5xx_hal_dma.h"
#endif

#ifdef HAL_CORTEX_MODULE_ENABLED
#include "stm32u5xx_hal_cortex.h"
#endif

#ifdef HAL_FLASH_MODULE_ENABLED
#include "stm32u5xx_hal_flash.h"
#endif

#ifdef HAL_PWR_MODULE_ENABLED
#include "stm32u5xx_hal_pwr.h"
#endif

#ifdef HAL_EXTI_MODULE_ENABLED
#include "stm32u5xx_hal_exti.h"
#endif

#ifdef HAL_RTC_MODULE_ENABLED
#include "stm32u5xx_hal_rtc.h"
#endif

#ifdef HAL_TIM_MODULE_ENABLED
#include "stm32u5xx_hal_tim.h"
#endif

#ifdef HAL_UART_MODULE_ENABLED
#include "stm32u5xx_hal_uart.h"
#endif

#ifdef HAL_I2C_MODULE_ENABLED
#include "stm32u5xx_hal_i2c.h"
#endif

#ifdef HAL_SPI_MODULE_ENABLED
#include "stm32u5xx_hal_spi.h"
#endif

#ifdef HAL_ADC_MODULE_ENABLED
#include "stm32u5xx_hal_adc.h"
#endif

#ifdef HAL_DAC_MODULE_ENABLED
#include "stm32u5xx_hal_dac.h"
#endif

#ifdef HAL_RNG_MODULE_ENABLED
#include "stm32u5xx_hal_rng.h"
#endif

#ifdef HAL_WWDG_MODULE_ENABLED
#include "stm32u5xx_hal_wwdg.h"
#endif

#ifdef HAL_IWDG_MODULE_ENABLED
#include "stm32u5xx_hal_iwdg.h"
#endif

#ifdef HAL_OPAMP_MODULE_ENABLED
#include "stm32u5xx_hal_opamp.h"
#endif

#ifdef HAL_COMP_MODULE_ENABLED
#include "stm32u5xx_hal_comp.h"
#endif

#ifdef HAL_CRC_MODULE_ENABLED
#include "stm32u5xx_hal_crc.h"
#endif

#ifdef HAL_LPTIM_MODULE_ENABLED
#include "stm32u5xx_hal_lptim.h"
#endif

#ifdef HAL_PKA_MODULE_ENABLED
#include "stm32u5xx_hal_pka.h"
#endif

#ifdef HAL_HASH_MODULE_ENABLED
#include "stm32u5xx_hal_hash.h"
#endif

#ifdef HAL_SAI_MODULE_ENABLED
#include "stm32u5xx_hal_sai.h"
#endif

#ifdef HAL_SD_MODULE_ENABLED
#include "stm32u5xx_hal_sd.h"
#endif

#ifdef HAL_SMARTCARD_MODULE_ENABLED
#include "stm32u5xx_hal_smartcard.h"
#endif

#ifdef HAL_IRDA_MODULE_ENABLED
#include "stm32u5xx_hal_irda.h"
#endif

#ifdef HAL_SRAM_MODULE_ENABLED
#include "stm32u5xx_hal_sram.h"
#endif

#ifdef HAL_NOR_MODULE_ENABLED
#include "stm32u5xx_hal_nor.h"
#endif

#ifdef HAL_NAND_MODULE_ENABLED
#include "stm32u5xx_hal_nand.h"
#endif

#ifdef HAL_I2S_MODULE_ENABLED
#include "stm32u5xx_hal_i2s.h"
#endif

#ifdef HAL_SMBUS_MODULE_ENABLED
#include "stm32u5xx_hal_smbus.h"
#endif

#ifdef HAL_USART_MODULE_ENABLED
#include "stm32u5xx_hal_usart.h"
#endif

#ifdef HAL_CRS_MODULE_ENABLED
#include "stm32u5xx_hal_crs.h"
#endif

#ifdef HAL_PCD_MODULE_ENABLED
#include "stm32u5xx_hal_pcd.h"
#endif

#ifdef HAL_ICACHE_MODULE_ENABLED
#include "stm32u5xx_hal_icache.h"
#endif

#ifdef HAL_OSPI_MODULE_ENABLED
#include "stm32u5xx_hal_ospi.h"
#endif

#ifdef HAL_OSPI_MODULE_ENABLED
#include "stm32u5xx_hal_ospi.h"
#endif

#ifdef HAL_XSPI_MODULE_ENABLED
#include "stm32u5xx_hal_xspi.h"
#endif

#ifdef HAL_GFXMMU_MODULE_ENABLED
#include "stm32u5xx_hal_gfxmmu.h"
#endif

#ifdef HAL_LTDC_MODULE_ENABLED
#include "stm32u5xx_hal_ltdc.h"
#endif

#ifdef HAL_DSI_MODULE_ENABLED
#include "stm32u5xx_hal_dsi.h"
#endif

#ifdef HAL_DCMIPP_MODULE_ENABLED
#include "stm32u5xx_hal_dcmipp.h"
#endif

#ifdef HAL_PSSI_MODULE_ENABLED
#include "stm32u5xx_hal_pssi.h"
#endif

#ifdef HAL_FMAC_MODULE_ENABLED
#include "stm32u5xx_hal_fmac.h"
#endif

#ifdef HAL_CORDIC_MODULE_ENABLED
#include "stm32u5xx_hal_cordic.h"
#endif

#ifdef HAL_CRC_MODULE_ENABLED
#include "stm32u5xx_hal_crc.h"
#endif

#ifdef HAL_RAMCFG_MODULE_ENABLED
#include "stm32u5xx_hal_ramcfg.h"
#endif

#ifdef HAL_FDCAN_MODULE_ENABLED
#include "stm32u5xx_hal_fdcan.h"
#endif

#ifdef HAL_MDF_MODULE_ENABLED
#include "stm32u5xx_hal_mdf.h"
#endif

#ifdef HAL_ADF_MODULE_ENABLED
#include "stm32u5xx_hal_adf.h"
#endif

#ifdef HAL_LPGPIO_MODULE_ENABLED
#include "stm32u5xx_hal_lpgpio.h"
#endif

#ifdef HAL_TSC_MODULE_ENABLED
#include "stm32u5xx_hal_tsc.h"
#endif

#ifdef HAL_I3C_MODULE_ENABLED
#include "stm32u5xx_hal_i3c.h"
#endif

#ifdef HAL_UART_MODULE_ENABLED
#include "stm32u5xx_hal_uart.h"
#endif

#ifdef HAL_LPUART_MODULE_ENABLED
#include "stm32u5xx_hal_lpuart.h"
#endif

#ifdef HAL_I2C_MODULE_ENABLED
#include "stm32u5xx_hal_i2c.h"
#endif

#ifdef HAL_I2C_MODULE_ENABLED
#include "stm32u5xx_hal_i2c.h"
#endif

#ifdef HAL_I3C_MODULE_ENABLED
#include "stm32u5xx_hal_i3c.h"
#endif

#ifdef HAL_USART_MODULE_ENABLED
#include "stm32u5xx_hal_usart.h"
#endif

#ifdef HAL_LPUART_MODULE_ENABLED
#include "stm32u5xx_hal_lpuart.h"
#endif

#ifdef HAL_IRDA_MODULE_ENABLED
#include "stm32u5xx_hal_irda.h"
#endif

#ifdef HAL_SMARTCARD_MODULE_ENABLED
#include "stm32u5xx_hal_smartcard.h"
#endif

#ifdef HAL_WWDG_MODULE_ENABLED
#include "stm32u5xx_hal_wwdg.h"
#endif

#ifdef HAL_IWDG_MODULE_ENABLED
#include "stm32u5xx_hal_iwdg.h"
#endif

#ifdef HAL_DTS_MODULE_ENABLED
#include "stm32u5xx_hal_dts.h"
#endif

#ifdef HAL_PKA_MODULE_ENABLED
#include "stm32u5xx_hal_pka.h"
#endif

#ifdef HAL_AES_MODULE_ENABLED
#include "stm32u5xx_hal_aes.h"
#endif

#ifdef HAL_HASH_MODULE_ENABLED
#include "stm32u5xx_hal_hash.h"
#endif

#ifdef HAL_RNG_MODULE_ENABLED
#include "stm32u5xx_hal_rng.h"
#endif

#ifdef HAL_CRC_MODULE_ENABLED
#include "stm32u5xx_hal_crc.h"
#endif

#ifdef HAL_SAI_MODULE_ENABLED
#include "stm32u5xx_hal_sai.h"
#endif

#ifdef HAL_SDMMC_MODULE_ENABLED
#include "stm32u5xx_hal_sd.h"
#endif

#ifdef HAL_XSPI_MODULE_ENABLED
#include "stm32u5xx_hal_xspi.h"
#endif

#ifdef HAL_OSPI_MODULE_ENABLED
#include "stm32u5xx_hal_ospi.h"
#endif

#ifdef HAL_SPI_MODULE_ENABLED
#include "stm32u5xx_hal_spi.h"
#endif

#ifdef HAL_I2S_MODULE_ENABLED
#include "stm32u5xx_hal_i2s.h"
#endif

#ifdef HAL_IPCC_MODULE_ENABLED
#include "stm32u5xx_hal_ipcc.h"
#endif

#ifdef HAL_HSEM_MODULE_ENABLED
#include "stm32u5xx_hal_hsem.h"
#endif

#ifdef HAL_GFXMMU_MODULE_ENABLED
#include "stm32u5xx_hal_gfxmmu.h"
#endif

#ifdef HAL_LTDC_MODULE_ENABLED
#include "stm32u5xx_hal_ltdc.h"
#endif

#ifdef HAL_DSI_MODULE_ENABLED
#include "stm32u5xx_hal_dsi.h"
#endif

#ifdef HAL_DCMIPP_MODULE_ENABLED
#include "stm32u5xx_hal_dcmipp.h"
#endif

#ifdef HAL_PSSI_MODULE_ENABLED
#include "stm32u5xx_hal_pssi.h"
#endif

#ifdef HAL_FMAC_MODULE_ENABLED
#include "stm32u5xx_hal_fmac.h"
#endif

#ifdef HAL_CORDIC_MODULE_ENABLED
#include "stm32u5xx_hal_cordic.h"
#endif

#ifdef HAL_DCACHE_MODULE_ENABLED
#include "stm32u5xx_hal_dcache.h"
#endif

#ifdef HAL_ICACHE_MODULE_ENABLED
#include "stm32u5xx_hal_icache.h"
#endif

#ifdef HAL_FDCAN_MODULE_ENABLED
#include "stm32u5xx_hal_fdcan.h"
#endif

#ifdef HAL_MDF_MODULE_ENABLED
#include "stm32u5xx_hal_mdf.h"
#endif

#ifdef HAL_ADF_MODULE_ENABLED
#include "stm32u5xx_hal_adf.h"
#endif

#ifdef HAL_LPGPIO_MODULE_ENABLED
#include "stm32u5xx_hal_lpgpio.h"
#endif

#ifdef HAL_TSC_MODULE_ENABLED
#include "stm32u5xx_hal_tsc.h"
#endif

#ifdef HAL_I3C_MODULE_ENABLED
#include "stm32u5xx_hal_i3c.h"
#endif

#ifdef HAL_UART_MODULE_ENABLED
#include "stm32u5xx_hal_uart.h"
#endif

#ifdef HAL_LPUART_MODULE_ENABLED
#include "stm32u5xx_hal_lpuart.h"
#endif

#ifdef HAL_USART_MODULE_ENABLED
#include "stm32u5xx_hal_usart.h"
#endif

#ifdef HAL_IRDA_MODULE_ENABLED
#include "stm32u5xx_hal_irda.h"
#endif

#ifdef HAL_SMARTCARD_MODULE_ENABLED
#include "stm32u5xx_hal_smartcard.h"
#endif

#ifdef HAL_WWDG_MODULE_ENABLED
#include "stm32u5xx_hal_wwdg.h"
#endif

#ifdef HAL_IWDG_MODULE_ENABLED
#include "stm32u5xx_hal_iwdg.h"
#endif

#ifdef HAL_DTS_MODULE_ENABLED
#include "stm32u5xx_hal_dts.h"
#endif

#ifdef HAL_PKA_MODULE_ENABLED
#include "stm32u5xx_hal_pka.h"
#endif

#ifdef HAL_AES_MODULE_ENABLED
#include "stm32u5xx_hal_aes.h"
#endif

#ifdef HAL_HASH_MODULE_ENABLED
#include "stm32u5xx_hal_hash.h"
#endif

#ifdef HAL_RNG_MODULE_ENABLED
#include "stm32u5xx_hal_rng.h"
#endif

#ifdef HAL_CRC_MODULE_ENABLED
#include "stm32u5xx_hal_crc.h"
#endif

#ifdef HAL_SAI_MODULE_ENABLED
#include "stm32u5xx_hal_sai.h"
#endif

#ifdef HAL_SDMMC_MODULE_ENABLED
#include "stm32u5xx_hal_sd.h"
#endif

#ifdef HAL_XSPI_MODULE_ENABLED
#include "stm32u5xx_hal_xspi.h"
#endif

#ifdef HAL_OSPI_MODULE_ENABLED
#include "stm32u5xx_hal_ospi.h"
#endif

#ifdef HAL_SPI_MODULE_ENABLED
#include "stm32u5xx_hal_spi.h"
#endif

#ifdef HAL_I2S_MODULE_ENABLED
#include "stm32u5xx_hal_i2s.h"
#endif

#ifdef HAL_IPCC_MODULE_ENABLED
#include "stm32u5xx_hal_ipcc.h"
#endif

#ifdef HAL_HSEM_MODULE_ENABLED
#include "stm32u5xx_hal_hsem.h"
#endif

/* Exported macro ------------------------------------------------------------*/
#ifdef USE_FULL_ASSERT
#define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
void assert_failed(uint8_t *file, uint32_t line);
#else
#define assert_param(expr) ((void)0U)
#endif

#ifdef __cplusplus
}
#endif

#endif /* STM32U5XX_HAL_CONF_H */
