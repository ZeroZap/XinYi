/**
 * @file xy_hal_dac.c
 * @brief DAC HAL STM32F4 Implementation
 */

#include "../../inc/xy_hal_dac.h"

#ifdef STM32_HAL_ENABLED

#include "stm32_hal.h"
#include <string.h>

typedef struct {
    DAC_HandleTypeDef *hdac;
    xy_hal_dac_callback_t callback;
    void *arg;
    uint8_t initialized;
} dac_ctx_t;

static dac_ctx_t g_dac_ctx = { 0 };

static uint32_t xy_to_stm32_channel(xy_hal_dac_channel_t channel)
{
    return (channel == XY_HAL_DAC_CHANNEL_2) ? DAC_CHANNEL_2 : DAC_CHANNEL_1;
}

static uint32_t xy_to_stm32_align(xy_hal_dac_align_t align)
{
    switch (align) {
    case XY_HAL_DAC_DATAALIGN_LEFT:  return DAC_ALIGN_12B_L;
    case XY_HAL_DAC_DATAALIGN_RIGHT:
    default:                          return DAC_ALIGN_12B_R;
    }
}

xy_hal_error_t xy_hal_dac_init(void *dac, const xy_hal_dac_config_t *config)
{
    if (!dac) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (g_dac_ctx.initialized) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }

    XY_UNUSED(config);

    DAC_HandleTypeDef *hdac = (DAC_HandleTypeDef *)dac;

    if (HAL_DAC_Init(hdac) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    g_dac_ctx.hdac        = hdac;
    g_dac_ctx.callback    = NULL;
    g_dac_ctx.arg         = NULL;
    g_dac_ctx.initialized = 1;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dac_deinit(void *dac)
{
    if (!dac) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_dac_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_DAC_DeInit((DAC_HandleTypeDef *)dac) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    g_dac_ctx.initialized = 0;
    g_dac_ctx.hdac        = NULL;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dac_enable_channel(void *dac, xy_hal_dac_channel_t channel)
{
    if (!dac) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_dac_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    uint32_t stm_channel = xy_to_stm32_channel(channel);

    DAC_ChannelConfTypeDef sConfig = { 0 };
    sConfig.DAC_Trigger      = DAC_TRIGGER_SOFTWARE;
    sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;

    if (HAL_DAC_ConfigChannel((DAC_HandleTypeDef *)dac, &sConfig,
                               stm_channel) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dac_disable_channel(void *dac, xy_hal_dac_channel_t channel)
{
    XY_UNUSED(channel);
    if (!dac) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_dac_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dac_set_value(void *dac, xy_hal_dac_channel_t channel,
                                     uint32_t value, xy_hal_dac_align_t align)
{
    if (!dac) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_dac_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    uint32_t stm_channel = xy_to_stm32_channel(channel);
    uint32_t stm_align   = xy_to_stm32_align(align);

    if (HAL_DAC_SetValue((DAC_HandleTypeDef *)dac, stm_channel,
                          stm_align, value) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

int xy_hal_dac_get_value(void *dac, xy_hal_dac_channel_t channel)
{
    if (!dac) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_dac_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    uint32_t stm_channel = xy_to_stm32_channel(channel);
    return (int)HAL_DAC_GetValue((DAC_HandleTypeDef *)dac, stm_channel);
}

xy_hal_error_t xy_hal_dac_start_output(void *dac, xy_hal_dac_channel_t channel)
{
    if (!dac) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_dac_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    uint32_t stm_channel = xy_to_stm32_channel(channel);
    if (HAL_DAC_Start((DAC_HandleTypeDef *)dac, stm_channel) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dac_stop_output(void *dac, xy_hal_dac_channel_t channel)
{
    if (!dac) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_dac_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    uint32_t stm_channel = xy_to_stm32_channel(channel);
    if (HAL_DAC_Stop((DAC_HandleTypeDef *)dac, stm_channel) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dac_software_trigger(void *dac, xy_hal_dac_channel_t channel)
{
    if (!dac) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_dac_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    uint32_t stm_channel = xy_to_stm32_channel(channel);
    uint32_t swtrig = (stm_channel == DAC_CHANNEL_1) ? 0x1U : 0x2U;
    ((DAC_HandleTypeDef *)dac)->Instance->SWTRIGR = swtrig;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dac_output_dma(void *dac, xy_hal_dac_channel_t channel,
                                      const uint32_t *data, size_t count)
{
    if (!dac || !data || count == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_dac_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    uint32_t stm_channel = xy_to_stm32_channel(channel);

    if (HAL_DAC_Start_DMA((DAC_HandleTypeDef *)dac, stm_channel,
                           (uint32_t *)data, (uint32_t)count,
                           DAC_ALIGN_12B_R) != HAL_OK) {
        return XY_HAL_ERROR_BUSY;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dac_generate_triangle(void *dac, xy_hal_dac_channel_t channel,
                                             uint32_t amplitude)
{
    if (!dac) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    uint32_t stm_channel = xy_to_stm32_channel(channel);
    if (HAL_DACEx_TriangleWaveGenerate((DAC_HandleTypeDef *)dac,
                                        stm_channel, amplitude) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dac_generate_noise(void *dac, xy_hal_dac_channel_t channel,
                                          uint32_t amplitude)
{
    if (!dac) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    uint32_t stm_channel = xy_to_stm32_channel(channel);
    if (HAL_DACEx_NoiseWaveGenerate((DAC_HandleTypeDef *)dac,
                                     stm_channel, amplitude) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dac_register_callback(void *dac, xy_hal_dac_channel_t channel,
                                             xy_hal_dac_callback_t callback,
                                             void *arg)
{
    XY_UNUSED(channel);
    if (!dac) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_dac_ctx.initialized) {
        g_dac_ctx.hdac        = (DAC_HandleTypeDef *)dac;
        g_dac_ctx.initialized = 1;
    }

    g_dac_ctx.callback = callback;
    g_dac_ctx.arg      = arg;
    return XY_HAL_OK;
}

uint32_t xy_hal_dac_value_to_mv(void *dac, uint32_t value, uint32_t vref)
{
    XY_UNUSED(dac);
    return (value * vref) / 4095U;
}

void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef *hdac)
{
    if (g_dac_ctx.hdac == hdac && g_dac_ctx.callback) {
        g_dac_ctx.callback(hdac, XY_HAL_DAC_CHANNEL_1, XY_HAL_DAC_EVENT_READY, g_dac_ctx.arg);
    }
}

void HAL_DACEx_ConvCpltCallbackCh2(DAC_HandleTypeDef *hdac)
{
    if (g_dac_ctx.hdac == hdac && g_dac_ctx.callback) {
        g_dac_ctx.callback(hdac, XY_HAL_DAC_CHANNEL_2, XY_HAL_DAC_EVENT_READY, g_dac_ctx.arg);
    }
}

#endif /* STM32_HAL_ENABLED */
