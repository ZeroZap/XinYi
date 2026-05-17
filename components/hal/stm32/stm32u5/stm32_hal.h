#pragma once
/* Shim: redirect stm32_hal.h → STM32U5xx vendor HAL
 * Include path to stm32u5xx_hal.h is supplied by cmake/platform/STM32U5.cmake */
#include "stm32u5xx_hal.h"

/* Minimal ADC stubs for xy_pm_adc.c when HAL_ADC_MODULE_ENABLED is not set */
#ifndef HAL_ADC_MODULE_ENABLED
typedef struct { void *Instance; uint32_t _reserved[64]; } ADC_HandleTypeDef;
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *hadc);
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef *hadc);
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *hadc, uint32_t Timeout);
uint32_t HAL_ADC_GetValue(const ADC_HandleTypeDef *hadc);
#endif
