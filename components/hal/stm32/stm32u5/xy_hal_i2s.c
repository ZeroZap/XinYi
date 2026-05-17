/**
 * @file xy_hal_i2s.c
 * @brief I2S HAL — STM32U5 stub.
 *
 * STM32U5 ships no dedicated I2S HAL: there is no stm32u5xx_hal_i2s.h,
 * no I2S_HandleTypeDef, no HAL_I2S_Init.  The hardware uses the SPI
 * peripheral re-configured in I2S mode through the SPI extended registers
 * (CFG2.I2SCFG, I2SCFGR, etc.), and the official driver model is to drop
 * down to register access or pull in the LL SPI driver.
 *
 * Until that wrapper is written, this file provides stubs so any caller
 * using the xy_hal_i2s API compiles and links cleanly but gets a clear
 * NOT_SUPPORTED at runtime.  Implementations of the long tail of helpers
 * in xy_hal_i2s.h (set_audio_freq / config_equalizer / audio_effect / etc.)
 * are intentionally omitted — they would not be reached anyway.
 */

#include "../../inc/xy_hal_i2s.h"

#if defined(STM32U5) || defined(STM32U5xx)

#include <stddef.h>

xy_hal_error_t xy_hal_i2s_init(void *i2s, const xy_hal_i2s_config_t *config)
{
    XY_UNUSED(i2s);
    XY_UNUSED(config);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_i2s_deinit(void *i2s)
{
    XY_UNUSED(i2s);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_i2s_start(void *i2s)
{
    XY_UNUSED(i2s);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_i2s_stop(void *i2s)
{
    XY_UNUSED(i2s);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int32_t xy_hal_i2s_send(void *i2s, const void *data, size_t size, uint32_t timeout)
{
    XY_UNUSED(i2s); XY_UNUSED(data); XY_UNUSED(size); XY_UNUSED(timeout);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int32_t xy_hal_i2s_receive(void *i2s, void *data, size_t size, uint32_t timeout)
{
    XY_UNUSED(i2s); XY_UNUSED(data); XY_UNUSED(size); XY_UNUSED(timeout);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_i2s_send_dma(void *i2s, const void *data, size_t size)
{
    XY_UNUSED(i2s); XY_UNUSED(data); XY_UNUSED(size);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_i2s_receive_dma(void *i2s, void *data, size_t size)
{
    XY_UNUSED(i2s); XY_UNUSED(data); XY_UNUSED(size);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_i2s_send_it(void *i2s, const void *data, size_t size)
{
    XY_UNUSED(i2s); XY_UNUSED(data); XY_UNUSED(size);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_i2s_receive_it(void *i2s, void *data, size_t size)
{
    XY_UNUSED(i2s); XY_UNUSED(data); XY_UNUSED(size);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_i2s_register_callback(void *i2s,
                                            xy_hal_i2s_callback_t callback,
                                            void *arg)
{
    XY_UNUSED(i2s); XY_UNUSED(callback); XY_UNUSED(arg);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

#endif /* STM32U5 || STM32U5xx */
