/**
 * @file xy_hal_adc.c
 * @brief ADC HAL — STM32U5 implementation.
 *
 * Covers the subset of xy_hal_adc.h that ports cleanly to the U5 HAL:
 * init/deinit/config/start/stop, blocking + non-blocking single-channel read,
 * DMA read, resolution helpers, callback registration.
 *
 * Functions in xy_hal_adc.h that are not implemented here (vbat, vrefint,
 * window watchdog, oversampling getters/setters, etc.) are intentionally
 * left out — the underlying U5 API surface diverges enough that a wrapper
 * would just be a misleading stub.  Call HAL_ADC_* directly when needed.
 */

#include "../../inc/xy_hal_adc.h"

#if defined(STM32U5) || defined(STM32U5xx)

#include "stm32u5xx_hal.h"
#include <string.h>

typedef struct {
    ADC_HandleTypeDef        *hadc;
    xy_hal_adc_callback_t     callback;
    void                     *arg;
    uint8_t                   initialized;
    xy_hal_adc_resolution_t   resolution;
} adc_ctx_t;

#define MAX_ADC_INSTANCES 4
static adc_ctx_t g_adc_ctx[MAX_ADC_INSTANCES];

static adc_ctx_t *find_adc_ctx(const void *adc)
{
    for (size_t i = 0; i < MAX_ADC_INSTANCES; i++) {
        if (g_adc_ctx[i].hadc == adc) {
            return &g_adc_ctx[i];
        }
    }
    return NULL;
}

static adc_ctx_t *alloc_adc_ctx(void)
{
    for (size_t i = 0; i < MAX_ADC_INSTANCES; i++) {
        if (g_adc_ctx[i].hadc == NULL) {
            return &g_adc_ctx[i];
        }
    }
    return NULL;
}

/* U5 ADC1/2 tops out at 14 bits; ADC4 tops out at 12 bits.  16B is not
 * a real STM32U5 mode, so we silently clamp to 14B and let the caller
 * notice via xy_hal_adc_get_resolution. */
static uint32_t xy_to_stm32_resolution(xy_hal_adc_resolution_t r)
{
    switch (r) {
    case XY_HAL_ADC_RESOLUTION_6B:  return ADC_RESOLUTION_6B;
    case XY_HAL_ADC_RESOLUTION_8B:  return ADC_RESOLUTION_8B;
    case XY_HAL_ADC_RESOLUTION_10B: return ADC_RESOLUTION_10B;
    case XY_HAL_ADC_RESOLUTION_12B: return ADC_RESOLUTION_12B;
    case XY_HAL_ADC_RESOLUTION_14B: return ADC_RESOLUTION_14B;
    case XY_HAL_ADC_RESOLUTION_16B: return ADC_RESOLUTION_14B;
    default:                        return ADC_RESOLUTION_12B;
    }
}

static uint32_t xy_to_stm32_align(xy_hal_adc_align_t align)
{
    return (align == XY_HAL_ADC_DATAALIGN_LEFT)
               ? ADC_DATAALIGN_LEFT
               : ADC_DATAALIGN_RIGHT;
}

static uint32_t xy_to_stm32_scan(xy_hal_adc_scan_t scan)
{
    return (scan == XY_HAL_ADC_SCAN_ENABLE)
               ? ADC_SCAN_ENABLE
               : ADC_SCAN_DISABLE;
}

xy_hal_error_t xy_hal_adc_init(void *adc, const xy_hal_adc_config_t *config)
{
    if (!adc || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    ADC_HandleTypeDef *hadc = (ADC_HandleTypeDef *)adc;

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

    hadc->Init.ClockPrescaler         = config->clock_div;
    hadc->Init.Resolution             = xy_to_stm32_resolution(config->resolution);
    hadc->Init.DataAlign              = xy_to_stm32_align(config->align);
    hadc->Init.ScanConvMode           = xy_to_stm32_scan(config->scan_mode);
    hadc->Init.EOCSelection           = ADC_EOC_SINGLE_CONV;
    /* U5: ContinuousConvMode / DiscontinuousConvMode are FunctionalState. */
    hadc->Init.ContinuousConvMode     =
        (config->continuous == XY_HAL_ADC_CONTINUOUS_ENABLE) ? ENABLE : DISABLE;
    hadc->Init.DiscontinuousConvMode  = DISABLE;
    hadc->Init.ExternalTrigConv       = ADC_SOFTWARE_START;
    /* U5 replaces DMAContinuousRequests with ConversionDataManagement. */
    hadc->Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;
    hadc->Init.NbrOfConversion        = 1;
    hadc->Init.Overrun                = ADC_OVR_DATA_OVERWRITTEN;
    hadc->Init.LeftBitShift           = ADC_LEFTBITSHIFT_NONE;

    if (HAL_ADC_Init(hadc) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    ctx->hadc        = hadc;
    ctx->callback    = NULL;
    ctx->arg         = NULL;
    ctx->initialized = 1;
    ctx->resolution  = config->resolution;
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
    memset(ctx, 0, sizeof(*ctx));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_config_channels(void *adc,
                                          const xy_hal_adc_channel_config_t *channels,
                                          size_t count)
{
    if (!adc || !channels || count == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    adc_ctx_t *ctx = find_adc_ctx(adc);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    for (size_t i = 0; i < count; i++) {
        if (!channels[i].enabled) {
            continue;
        }
        ADC_ChannelConfTypeDef cfg = {
            .Channel      = channels[i].channel,
            .Rank         = channels[i].rank,
            .SamplingTime = channels[i].sampling_time,
            .SingleDiff   = ADC_SINGLE_ENDED,
            .OffsetNumber = ADC_OFFSET_NONE,
            .Offset       = 0,
        };
        if (HAL_ADC_ConfigChannel((ADC_HandleTypeDef *)adc, &cfg) != HAL_OK) {
            return XY_HAL_ERROR_FAIL;
        }
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_start(void *adc)
{
    adc_ctx_t *ctx = find_adc_ctx(adc);
    if (!adc || ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return (HAL_ADC_Start((ADC_HandleTypeDef *)adc) == HAL_OK)
               ? XY_HAL_OK : XY_HAL_ERROR_FAIL;
}

xy_hal_error_t xy_hal_adc_stop(void *adc)
{
    adc_ctx_t *ctx = find_adc_ctx(adc);
    if (!adc || ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return (HAL_ADC_Stop((ADC_HandleTypeDef *)adc) == HAL_OK)
               ? XY_HAL_OK : XY_HAL_ERROR_FAIL;
}

/* Configure-single-channel + start + poll + read.  Helper used by the
 * single-channel read variants. */
static int32_t read_single(ADC_HandleTypeDef *hadc, uint8_t channel,
                           uint32_t timeout)
{
    ADC_ChannelConfTypeDef cfg = {
        .Channel      = channel,
        .Rank         = ADC_REGULAR_RANK_1,
        .SamplingTime = ADC_SAMPLETIME_5CYCLES,
        .SingleDiff   = ADC_SINGLE_ENDED,
        .OffsetNumber = ADC_OFFSET_NONE,
        .Offset       = 0,
    };
    if (HAL_ADC_ConfigChannel(hadc, &cfg) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
    if (HAL_ADC_Start(hadc) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
    HAL_StatusTypeDef st = HAL_ADC_PollForConversion(hadc, timeout);
    if (st != HAL_OK) {
        HAL_ADC_Stop(hadc);
        return (timeout == 0) ? XY_HAL_ERROR_BUSY : XY_HAL_ERROR_TIMEOUT;
    }
    uint32_t v = HAL_ADC_GetValue(hadc);
    HAL_ADC_Stop(hadc);
    return (int32_t)v;
}

int32_t xy_hal_adc_read(void *adc, uint8_t channel, uint32_t timeout)
{
    adc_ctx_t *ctx = find_adc_ctx(adc);
    if (!adc || ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return read_single((ADC_HandleTypeDef *)adc, channel, timeout);
}

int32_t xy_hal_adc_read_nb(void *adc, uint8_t channel)
{
    adc_ctx_t *ctx = find_adc_ctx(adc);
    if (!adc || ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return read_single((ADC_HandleTypeDef *)adc, channel, 0);
}

xy_hal_error_t xy_hal_adc_read_dma(void *adc, uint32_t *buffer, size_t count)
{
    adc_ctx_t *ctx = find_adc_ctx(adc);
    if (!adc || !buffer || count == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return (HAL_ADC_Start_DMA((ADC_HandleTypeDef *)adc, buffer, (uint32_t)count)
            == HAL_OK) ? XY_HAL_OK : XY_HAL_ERROR_FAIL;
}

int32_t xy_hal_adc_get_resolution(void *adc)
{
    adc_ctx_t *ctx = find_adc_ctx(adc);
    if (!adc || ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    switch (ctx->resolution) {
    case XY_HAL_ADC_RESOLUTION_6B:  return 6;
    case XY_HAL_ADC_RESOLUTION_8B:  return 8;
    case XY_HAL_ADC_RESOLUTION_10B: return 10;
    case XY_HAL_ADC_RESOLUTION_12B: return 12;
    case XY_HAL_ADC_RESOLUTION_14B: return 14;
    case XY_HAL_ADC_RESOLUTION_16B: return 14;  /* clamped, U5 max */
    default:                        return 12;
    }
}

uint32_t xy_hal_adc_get_max_value(void *adc)
{
    int32_t bits = xy_hal_adc_get_resolution(adc);
    return (bits < 0) ? 0U : ((1U << bits) - 1U);
}

uint32_t xy_hal_adc_value_to_mv(void *adc, uint32_t value, uint32_t vref)
{
    uint32_t max = xy_hal_adc_get_max_value(adc);
    return (max == 0U) ? 0U : (value * vref) / max;
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

/* ==================== HAL callbacks ==================== */

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    adc_ctx_t *ctx = find_adc_ctx(hadc);
    if (ctx && ctx->callback) {
        ctx->callback(hadc, XY_HAL_ADC_EVENT_EOC, ctx->arg);
    }
}

void HAL_ADC_LevelOutOfWindowCallback(ADC_HandleTypeDef *hadc)
{
    adc_ctx_t *ctx = find_adc_ctx(hadc);
    if (ctx && ctx->callback) {
        ctx->callback(hadc, XY_HAL_ADC_EVENT_AWD, ctx->arg);
    }
}

void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc)
{
    adc_ctx_t *ctx = find_adc_ctx(hadc);
    if (ctx && ctx->callback) {
        ctx->callback(hadc, XY_HAL_ADC_EVENT_OVR, ctx->arg);
    }
}

#endif /* STM32U5 || STM32U5xx */
