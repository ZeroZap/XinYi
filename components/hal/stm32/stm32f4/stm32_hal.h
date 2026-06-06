#pragma once
/* Shim used by the STM32 wrapper sources.  Most wrappers live under the
 * stm32f4 directory today, but some platforms (for example STM32L4) reuse those
 * sources while supplying their own vendor HAL include paths. */
#if defined(STM32L4) || defined(STM32L4xx)
#include "stm32l4xx_hal.h"
#else
#include "stm32f4xx_hal.h"
#endif
