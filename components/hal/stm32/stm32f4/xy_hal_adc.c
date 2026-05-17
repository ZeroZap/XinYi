/**
 * @file xy_hal_adc.c
 * @brief ADC HAL STM32F4 Implementation
 */

#include "../../inc/xy_hal_adc.h"

#ifdef STM32_HAL_ENABLED

#include "stm32_hal.h"
#include <string.h>

typedef struct {
    ADC_HandleTypeDef *hadc;
    xy_hal_adc_callback_t callback;
    void *arg;
    uint8_t initialized;
} adc_ctx_t;

#define MAX_ADC_INSTANCES 3
static adc_ctx_t g_adc_ctx[MAX_ADC_INSTANCES];

static adc_ctx_t *find_adc_ctx(void *adc)
{
    for (int i = 0; i < MAX_ADC_INSTANCES; i++) {
        if (g_adc_ctx[i].hadc == adc) {
            return &g_adc_ctx[i];
        }
    }
    return NULL;
}

static adc_ctx_t *alloc_adc_ctx(void)
{
    for (int i = 0; i < MAX_ADC_INSTANCES; i++) {
        if (g_adc_ctx[i].hadc == NULL) {
            return &g_adc_ctx[i];
        }
    }
    return NULL;
}

static uint32_t xy_to_stm32_resolution(xy_hal_adc_resolution_t res)
{
    switch (res) {
    case XY_HAL_ADC_RESOLUTION_6B:  return ADC_RESOLUTION_6B;
    case XY_HAL_ADC_RESOLUTION_8B:  return ADC_RESOLUTION_8B;
    case XY_HAL_ADC_RESOLUTION_10B: return ADC_RESOLUTION_10B;
    case XY_HAL_ADC_RESOLUTION_12B:
    default:                         return ADC_RESOLUTION_12B;
    }
}

xy_hal_error_t xy_hal_adc_init(void *adc, const xy_hal_adc_config_t *config)
{
    if (!adc || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    adc_ctx_t *ctx = find_adc_ctx(adc);
    if (ctx != NULL && ctx->initialized) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }

    if (ctx == NULL) {
        ctx = alloc_adc_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
    }

    ADC_HandleTypeDef *hadc = (ADC_HandleTypeDef *)adc;
    hadc->Init.Resolution           = xy_to_stm32_resolution(config->resolution);
    hadc->Init.DataAlign            = (config->align == XY_HAL_ADC_DATAALIGN_LEFT)
                                          ? ADC_DATAALIGN_LEFT : ADC_DATAALIGN_RIGHT;
    hadc->Init.ScanConvMode         = (config->scan_mode == XY_HAL_ADC_SCAN_ENABLE)
                                          ? ENABLE : DISABLE;
    hadc->Init.ContinuousConvMode   = (config->continuous == XY_HAL_ADC_CONTINUOUS_ENABLE)
                                          ? ENABLE : DISABLE;
    hadc->Init.DiscontinuousConvMode = DISABLE;
    hadc->Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc->Init.ExternalTrigConv     = ADC_SOFTWARE_START;
    hadc->Init.DMAContinuousRequests = DISABLE;
    hadc->Init.EOCSelection         = ADC_EOC_SINGLE_CONV;
    hadc->Init.NbrOfConversion      = 1;

    if (HAL_ADC_Init(hadc) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    ctx->hadc        = hadc;
    ctx->callback    = NULL;
    ctx->arg         = NULL;
    ctx->initialized = 1;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_deinit(void *adc)
{
    if (!adc) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    adc_ctx_t *ctx = find_adc_ctx(adc);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_ADC_DeInit((ADC_HandleTypeDef *)adc) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    ctx->initialized = 0;
    ctx->hadc        = NULL;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_config_channels(void *adc,
                                           const xy_hal_adc_channel_config_t *channels,
                                           size_t count)
{
    if (!adc || !channels || count == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    ADC_HandleTypeDef *hadc = (ADC_HandleTypeDef *)adc;

    for (uint8_t i = 0; i < count; i++) {
        if (!channels[i].enabled) {
            continue;
        }

        ADC_ChannelConfTypeDef sConfig = { 0 };
        sConfig.Channel      = channels[i].channel;
        sConfig.Rank         = channels[i].rank;
        sConfig.SamplingTime = channels[i].sampling_time
                                   ? channels[i].sampling_time
                                   : ADC_SAMPLETIME_3CYCLES;

        if (HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK) {
            return XY_HAL_ERROR_FAIL;
        }
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_start(void *adc)
{
    if (!adc) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    adc_ctx_t *ctx = find_adc_ctx(adc);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_ADC_Start((ADC_HandleTypeDef *)adc) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_stop(void *adc)
{
    if (!adc) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    adc_ctx_t *ctx = find_adc_ctx(adc);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_ADC_Stop((ADC_HandleTypeDef *)adc) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

int32_t xy_hal_adc_read(void *adc, uint8_t channel, uint32_t timeout)
{
    if (!adc) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    adc_ctx_t *ctx = find_adc_ctx(adc);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    XY_UNUSED(channel); /* channel is pre-configured via config_channels */

    ADC_HandleTypeDef *hadc = (ADC_HandleTypeDef *)adc;
    HAL_ADC_Start(hadc);

    if (HAL_ADC_PollForConversion(hadc, timeout) != HAL_OK) {
        return XY_HAL_ERROR_TIMEOUT;
    }

    return (int32_t)HAL_ADC_GetValue(hadc);
}

int32_t xy_hal_adc_read_nb(void *adc, uint8_t channel)
{
    XY_UNUSED(channel);
    if (!adc) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    adc_ctx_t *ctx = find_adc_ctx(adc);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    ADC_HandleTypeDef *hadc = (ADC_HandleTypeDef *)adc;
    if (HAL_ADC_GetState(hadc) & HAL_ADC_STATE_REG_EOC) {
        return (int32_t)HAL_ADC_GetValue(hadc);
    }

    return XY_HAL_ERROR_BUSY;
}

int32_t xy_hal_adc_read_multi(void *adc, const uint8_t *channels,
                               uint32_t *values, size_t count, uint32_t timeout)
{
    if (!adc || !channels || !values || count == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    for (size_t i = 0; i < count; i++) {
        int32_t r = xy_hal_adc_read(adc, channels[i], timeout);
        if (r < 0) {
            return r;
        }
        values[i] = (uint32_t)r;
    }

    return (int32_t)count;
}

xy_hal_error_t xy_hal_adc_read_dma(void *adc, uint32_t *buffer, size_t count)
{
    if (!adc || !buffer || count == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    adc_ctx_t *ctx = find_adc_ctx(adc);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_ADC_Start_DMA((ADC_HandleTypeDef *)adc, buffer,
                          (uint32_t)count) != HAL_OK) {
        return XY_HAL_ERROR_BUSY;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_enable_vrefint(void)
{
    ADC->CCR |= ADC_CCR_TSVREFE;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_disable_vrefint(void)
{
    ADC->CCR &= ~ADC_CCR_TSVREFE;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_enable_vbat(void)
{
    ADC->CCR |= ADC_CCR_VBATE;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_disable_vbat(void)
{
    ADC->CCR &= ~ADC_CCR_VBATE;
    return XY_HAL_OK;
}

int32_t xy_hal_adc_get_vbat_value(void)
{
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int32_t xy_hal_adc_get_temp_value(void)
{
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_adc_set_window_threshold(void *adc, uint32_t low_threshold,
                                                uint32_t high_threshold)
{
    XY_UNUSED(adc);
    XY_UNUSED(low_threshold);
    XY_UNUSED(high_threshold);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_adc_get_window_threshold(void *adc, uint32_t *low_threshold,
                                                uint32_t *high_threshold)
{
    XY_UNUSED(adc);
    XY_UNUSED(low_threshold);
    XY_UNUSED(high_threshold);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_adc_enable_window_watchdog(void *adc, uint8_t channel)
{
    XY_UNUSED(adc);
    XY_UNUSED(channel);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_adc_disable_window_watchdog(void *adc, uint8_t channel)
{
    XY_UNUSED(adc);
    XY_UNUSED(channel);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_adc_register_callback(void *adc,
                                             xy_hal_adc_callback_t callback,
                                             void *arg)
{
    if (!adc) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    adc_ctx_t *ctx = find_adc_ctx(adc);
    if (ctx == NULL) {
        ctx = alloc_adc_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
        ctx->hadc = (ADC_HandleTypeDef *)adc;
    }

    ctx->callback = callback;
    ctx->arg      = arg;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_control(void *adc, uint32_t cmd, void *args)
{
    XY_UNUSED(adc);
    XY_UNUSED(cmd);
    XY_UNUSED(args);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int32_t xy_hal_adc_get_state(void *adc)
{
    if (!adc) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    return (int32_t)HAL_ADC_GetState((ADC_HandleTypeDef *)adc);
}

xy_hal_error_t xy_hal_adc_calibrate(void *adc)
{
    XY_UNUSED(adc);
    /* STM32F4 ADC does not require calibration */
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_set_prescaler(void *adc, uint8_t prescaler)
{
    XY_UNUSED(adc);
    XY_UNUSED(prescaler);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int32_t xy_hal_adc_get_prescaler(void *adc)
{
    XY_UNUSED(adc);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_adc_set_oversampling(void *adc, uint8_t oversampling)
{
    XY_UNUSED(adc);
    XY_UNUSED(oversampling);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int32_t xy_hal_adc_get_oversampling(void *adc)
{
    XY_UNUSED(adc);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

void xy_hal_adc_event_handler(void *adc, xy_hal_adc_evt_t event, void *arg)
{
    XY_UNUSED(adc);
    XY_UNUSED(event);
    XY_UNUSED(arg);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    adc_ctx_t *ctx = find_adc_ctx(hadc);
    if (ctx && ctx->callback) {
        ctx->callback(hadc, XY_HAL_ADC_EVENT_EOC, ctx->arg);
    }
}

void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc)
{
    adc_ctx_t *ctx = find_adc_ctx(hadc);
    if (ctx && ctx->callback) {
        ctx->callback(hadc, XY_HAL_ADC_EVENT_ERROR, ctx->arg);
    }
}

#endif /* STM32_HAL_ENABLED */
