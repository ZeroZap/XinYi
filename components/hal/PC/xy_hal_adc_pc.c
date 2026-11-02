/**
 * @file xy_hal_adc_pc.c
 * @brief PC/Linux simulation layer for the ADC HAL
 *
 * A host has no converter, so what is modelled here is the part of the contract a caller can
 * observe rather than a register map: a channel converts the voltage a test applied through
 * xy_hal_adc_pc_set_input(), the full scale follows the configured resolution, and the events a
 * vendor backend raises from its interrupt handler arrive through xy_hal_adc_event_handler().
 *
 * Three vendor behaviours are kept deliberately, because they are the ones that cost real debugging
 * time on hardware and a host backend that diverges from them produces host-green / board-red:
 *
 *  - read_nb() reports BUSY until a conversion has completed and its flag has been consumed; it does
 *    not quietly convert again. Calibration aborts a pending conversion, as it does on the part.
 *  - the global internal-sensor reads (temperature / VREFINT / VBAT) refuse when that source was
 *    never enabled. A plausible constant is what an unconnected or disabled channel looks like, and
 *    it is unreadable as evidence.
 *  - a foreign or deinitialised handle is NOT_INIT rather than a zero reading.
 *
 * Two places are deliberately more general than any single vendor port, and are documented where
 * they differ: a channel count of 32 (the public type is uint8_t; the parts this tree targets carry
 * 18 or fewer), and a prescaler that accepts any non-zero value (a host has no clock domain to
 * divide). Validation that the vendor performs against its register encoding is not reproduced -
 * what is reproduced is the failure the caller can see.
 */

#include "xy_hal_adc.h"
#include "xy_hal_pc.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define PC_ADC_SLOTS 4U
#define PC_ADC_CHANNELS 32U
#define PC_ADC_DEFAULT_VREF_MV 3300U
#define PC_ADC_MAX_RANK 16U

typedef struct {
    void *handle;
    uint8_t used;
    uint8_t initialized;
    xy_hal_adc_config_t config;
    uint32_t sampling_time[PC_ADC_CHANNELS];
    uint32_t input_mv[PC_ADC_CHANNELS];
    uint32_t vref_mv;
    uint32_t window_low;
    uint32_t window_high;
    uint32_t last_value;
    uint8_t window_channel;
    uint8_t window_enabled;
    uint8_t started;
    uint8_t eoc;
    uint8_t oversampling;
    xy_hal_adc_callback_t callback;
    void *callback_arg;
} pc_adc_ctx_t;

/* The internal-sensor entries take no instance handle, so their state belongs to the backend rather
 * than to one converter - the same shape the public header has. */
typedef struct {
    uint8_t temp_enabled;
    uint8_t vrefint_enabled;
    uint8_t vbat_enabled;
    int32_t temp_centi;
    uint32_t vrefint_mv;
    uint32_t vbat_mv;
} pc_adc_sources_t;

static pc_adc_ctx_t g_adc_ctx[PC_ADC_SLOTS];
static pc_adc_sources_t g_adc_sources = {
    0U, 0U, 0U, 2500, 1200U, 3000U
};

static pc_adc_ctx_t *pc_adc_find(const void *handle)
{
    unsigned int index;

    if (handle == NULL) {
        return NULL;
    }
    for (index = 0U; index < PC_ADC_SLOTS; index++) {
        if (g_adc_ctx[index].used != 0U && g_adc_ctx[index].handle == handle) {
            return &g_adc_ctx[index];
        }
    }
    return NULL;
}

static pc_adc_ctx_t *pc_adc_slot(void)
{
    unsigned int index;

    for (index = 0U; index < PC_ADC_SLOTS; index++) {
        if (g_adc_ctx[index].used == 0U) {
            g_adc_ctx[index].used = 1U;
            return &g_adc_ctx[index];
        }
    }
    return NULL;
}

/* The first configured converter. The internal-sensor reads have no handle to key on, so they use
 * whichever converter is running; with none configured there is no conversion to model. */
static pc_adc_ctx_t *pc_adc_primary(void)
{
    unsigned int index;

    for (index = 0U; index < PC_ADC_SLOTS; index++) {
        if (g_adc_ctx[index].used != 0U && g_adc_ctx[index].initialized != 0U) {
            return &g_adc_ctx[index];
        }
    }
    return NULL;
}

static uint32_t pc_adc_bits(xy_hal_adc_resolution_t resolution)
{
    switch (resolution) {
    case XY_HAL_ADC_RESOLUTION_6B:
        return 6U;
    case XY_HAL_ADC_RESOLUTION_8B:
        return 8U;
    case XY_HAL_ADC_RESOLUTION_10B:
        return 10U;
    case XY_HAL_ADC_RESOLUTION_12B:
        return 12U;
    case XY_HAL_ADC_RESOLUTION_14B:
        return 14U;
    case XY_HAL_ADC_RESOLUTION_16B:
        return 16U;
    default:
        return 0U;
    }
}

static uint32_t pc_adc_max_value(xy_hal_adc_resolution_t resolution)
{
    uint32_t bits = pc_adc_bits(resolution);

    return bits == 0U ? 0U : (((uint32_t)1U << bits) - 1U);
}

/* Voltage in, count out, on the configured full scale. Saturating rather than wrapping: a caller
 * that applies more than the reference can measure should see full scale, not the remainder. */
static uint32_t pc_adc_raw_from_mv(const pc_adc_ctx_t *ctx, uint32_t millivolts)
{
    uint32_t maximum = pc_adc_max_value(ctx->config.resolution);
    uint32_t vref = ctx->vref_mv == 0U ? PC_ADC_DEFAULT_VREF_MV : ctx->vref_mv;
    uint32_t raw;

    if (maximum == 0U) {
        return 0U;
    }
    raw = (uint32_t)(((uint64_t)millivolts * (uint64_t)maximum) / (uint64_t)vref);
    return raw > maximum ? maximum : raw;
}

static void pc_adc_notify(pc_adc_ctx_t *ctx, xy_hal_adc_evt_t event)
{
    if (ctx->callback != NULL) {
        ctx->callback(ctx->handle, event, ctx->callback_arg);
    }
}

static int pc_adc_config_valid(const xy_hal_adc_config_t *config)
{
    if (config->resolution > XY_HAL_ADC_RESOLUTION_16B) return 0;
    if (config->align > XY_HAL_ADC_DATAALIGN_LEFT) return 0;
    if (config->scan_mode > XY_HAL_ADC_SCAN_ENABLE) return 0;
    if (config->continuous > XY_HAL_ADC_CONTINUOUS_ENABLE) return 0;
    if (config->trigger_src > XY_HAL_ADC_TRIGGER_EXTI11) return 0;
    if (config->enable_vrefint > 1U || config->enable_temp_sensor > 1U) return 0;
    if (config->enable_vbat > 1U) return 0;
    return 1;
}

xy_hal_error_t xy_hal_adc_init(void *adc, const xy_hal_adc_config_t *config)
{
    pc_adc_ctx_t *ctx;
    unsigned int index;

    if (adc == NULL || config == NULL || !pc_adc_config_valid(config)) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_adc_find(adc);
    if (ctx != NULL && ctx->initialized != 0U) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }
    if (ctx == NULL) {
        ctx = pc_adc_slot();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
        ctx->handle = adc;
    }
    memset(ctx->sampling_time, 0, sizeof(ctx->sampling_time));
    memset(ctx->input_mv, 0, sizeof(ctx->input_mv));
    for (index = 0U; index < PC_ADC_CHANNELS; index++) {
        ctx->sampling_time[index] = config->sampling_time;
    }
    ctx->config = *config;
    ctx->vref_mv = PC_ADC_DEFAULT_VREF_MV;
    ctx->oversampling = 0U;
    ctx->window_low = 0U;
    ctx->window_high = 0U;
    ctx->window_channel = 0U;
    ctx->window_enabled = 0U;
    ctx->last_value = 0U;
    ctx->started = 0U;
    ctx->eoc = 0U;
    ctx->callback = NULL;
    ctx->callback_arg = NULL;
    ctx->initialized = 1U;
    /* The config can switch the shared internal sources on, exactly as ADC_TempSensorVrefintCmd()
     * does on the parts this backend stands in for. */
    if (config->enable_temp_sensor != 0U) {
        g_adc_sources.temp_enabled = 1U;
    }
    if (config->enable_vrefint != 0U) {
        g_adc_sources.vrefint_enabled = 1U;
    }
    if (config->enable_vbat != 0U) {
        g_adc_sources.vbat_enabled = 1U;
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_deinit(void *adc)
{
    pc_adc_ctx_t *ctx;

    if (adc == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_adc_find(adc);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    memset(ctx, 0, sizeof(*ctx));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_config_channels(void *adc,
                                         const xy_hal_adc_channel_config_t *channels,
                                         size_t count)
{
    pc_adc_ctx_t *ctx;
    size_t index;

    if (adc == NULL || channels == NULL || count == 0U || count > PC_ADC_CHANNELS) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_adc_find(adc);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    /* The whole request is validated before anything is applied. A vendor loop returns in the middle
     * of exactly this sequence, which leaves a converter whose configured rank disagrees with the
     * array the caller holds; a caller cannot repair that because it was told the call failed. */
    for (index = 0U; index < count; index++) {
        if (channels[index].channel >= PC_ADC_CHANNELS || channels[index].rank == 0U ||
            channels[index].rank > PC_ADC_MAX_RANK) {
            return XY_HAL_ERROR_INVALID_PARAM;
        }
    }
    for (index = 0U; index < count; index++) {
        if (channels[index].enabled != 0U) {
            ctx->sampling_time[channels[index].channel] = channels[index].sampling_time;
        }
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_start(void *adc)
{
    pc_adc_ctx_t *ctx = pc_adc_find(adc);

    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    ctx->started = 1U;
    ctx->eoc = 0U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_stop(void *adc)
{
    pc_adc_ctx_t *ctx = pc_adc_find(adc);

    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    ctx->started = 0U;
    return XY_HAL_OK;
}

int32_t xy_hal_adc_read(void *adc, uint8_t channel, uint32_t timeout)
{
    pc_adc_ctx_t *ctx;
    uint32_t value;

    if (adc == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_adc_find(adc);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (channel >= PC_ADC_CHANNELS) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    /* A host conversion takes no time, so no finite timeout is ever reached and the zero sentinel the
     * vendor backends read as "wait forever" is satisfied as well. Modelling a delay here would make
     * host code depend on the simulator's clock instead of its own. */
    (void)timeout;
    value = pc_adc_raw_from_mv(ctx, ctx->input_mv[channel]);
    ctx->last_value = value;
    ctx->eoc = 1U;
    pc_adc_notify(ctx, XY_HAL_ADC_EVENT_EOC);
    if (ctx->window_enabled != 0U && ctx->window_channel == channel &&
        (value < ctx->window_low || value > ctx->window_high)) {
        pc_adc_notify(ctx, XY_HAL_ADC_EVENT_AWD);
    }
    return (int32_t)value;
}

int32_t xy_hal_adc_read_nb(void *adc, uint8_t channel)
{
    pc_adc_ctx_t *ctx;

    if (adc == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_adc_find(adc);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (channel >= PC_ADC_CHANNELS) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (ctx->eoc == 0U) {
        return XY_HAL_ERROR_BUSY;
    }
    ctx->eoc = 0U;
    return (int32_t)ctx->last_value;
}

int32_t xy_hal_adc_read_multi(void *adc, const uint8_t *channels,
                              uint32_t *values, size_t count, uint32_t timeout)
{
    size_t index;

    if (adc == NULL || channels == NULL || values == NULL || count == 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    for (index = 0U; index < count; index++) {
        int32_t value = xy_hal_adc_read(adc, channels[index], timeout);

        if (value < 0) {
            return value;
        }
        values[index] = (uint32_t)value;
    }
    return (int32_t)count;
}

xy_hal_error_t xy_hal_adc_read_dma(void *adc, uint32_t *buffer, size_t count)
{
    pc_adc_ctx_t *ctx;
    size_t index;

    if (adc == NULL || buffer == NULL || count == 0U || count > PC_ADC_CHANNELS) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_adc_find(adc);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    /* There is no transfer to wait for: the host fills the buffer from the applied inputs and reports
     * completion. This is a model of a completed DMA round, not of the transfer itself, so a host
     * result says nothing about DMA ordering or memory visibility on a part. */
    for (index = 0U; index < count; index++) {
        buffer[index] = pc_adc_raw_from_mv(ctx, ctx->input_mv[index]);
    }
    ctx->last_value = buffer[count - 1U];
    ctx->eoc = 1U;
    pc_adc_notify(ctx, XY_HAL_ADC_EVENT_EOC);
    return XY_HAL_OK;
}

int32_t xy_hal_adc_get_resolution(void *adc)
{
    pc_adc_ctx_t *ctx;

    if (adc == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_adc_find(adc);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return (int32_t)pc_adc_bits(ctx->config.resolution);
}

uint32_t xy_hal_adc_get_max_value(void *adc)
{
    pc_adc_ctx_t *ctx;

    if (adc == NULL) {
        return 0U;
    }
    ctx = pc_adc_find(adc);
    if (ctx == NULL || ctx->initialized == 0U) {
        return 0U;
    }
    return pc_adc_max_value(ctx->config.resolution);
}

uint32_t xy_hal_adc_value_to_mv(void *adc, uint32_t value, uint32_t vref)
{
    pc_adc_ctx_t *ctx;
    uint32_t maximum;

    if (adc == NULL || vref == 0U) {
        return 0U;
    }
    ctx = pc_adc_find(adc);
    if (ctx == NULL || ctx->initialized == 0U) {
        return 0U;
    }
    maximum = pc_adc_max_value(ctx->config.resolution);
    /* A count above full scale is not a voltage this part could have produced, so it converts to
     * nothing rather than to a value the caller would report as a measurement. */
    if (maximum == 0U || value > maximum) {
        return 0U;
    }
    return (uint32_t)(((uint64_t)value * (uint64_t)vref) / (uint64_t)maximum);
}

xy_hal_error_t xy_hal_adc_set_sampling_time(void *adc, uint8_t channel,
                                           uint32_t sampling_time)
{
    pc_adc_ctx_t *ctx;

    if (adc == NULL || channel >= PC_ADC_CHANNELS) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_adc_find(adc);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    ctx->sampling_time[channel] = sampling_time;
    return XY_HAL_OK;
}

int32_t xy_hal_adc_get_sampling_time(void *adc, uint8_t channel)
{
    pc_adc_ctx_t *ctx;

    if (adc == NULL || channel >= PC_ADC_CHANNELS) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_adc_find(adc);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return (int32_t)ctx->sampling_time[channel];
}

xy_hal_error_t xy_hal_adc_set_trigger(void *adc, xy_hal_adc_trigger_src_t trigger_src)
{
    pc_adc_ctx_t *ctx;

    if (adc == NULL || trigger_src > XY_HAL_ADC_TRIGGER_EXTI11) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_adc_find(adc);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    ctx->config.trigger_src = trigger_src;
    return XY_HAL_OK;
}

int32_t xy_hal_adc_get_trigger(void *adc)
{
    pc_adc_ctx_t *ctx;

    if (adc == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_adc_find(adc);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return (int32_t)ctx->config.trigger_src;
}

xy_hal_error_t xy_hal_adc_set_continuous(void *adc, xy_hal_adc_continuous_t continuous)
{
    pc_adc_ctx_t *ctx;

    if (adc == NULL || continuous > XY_HAL_ADC_CONTINUOUS_ENABLE) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_adc_find(adc);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    ctx->config.continuous = continuous;
    return XY_HAL_OK;
}

int32_t xy_hal_adc_get_continuous(void *adc)
{
    pc_adc_ctx_t *ctx;

    if (adc == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_adc_find(adc);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return (int32_t)ctx->config.continuous;
}

xy_hal_error_t xy_hal_adc_enable_temp_sensor(void)
{
    g_adc_sources.temp_enabled = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_disable_temp_sensor(void)
{
    g_adc_sources.temp_enabled = 0U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_enable_vrefint(void)
{
    g_adc_sources.vrefint_enabled = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_disable_vrefint(void)
{
    g_adc_sources.vrefint_enabled = 0U;
    return XY_HAL_OK;
}

int32_t xy_hal_adc_get_vrefint_value(void)
{
    pc_adc_ctx_t *ctx = pc_adc_primary();

    if (ctx == NULL) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    /* A source that was never switched on is not a reading. Returning the nominal 1.2 V here would
     * make a caller believe it measured the reference. */
    if (g_adc_sources.vrefint_enabled == 0U) {
        return XY_HAL_ERROR_NOT_SUPPORTED;
    }
    return (int32_t)pc_adc_raw_from_mv(ctx, g_adc_sources.vrefint_mv);
}

xy_hal_error_t xy_hal_adc_enable_vbat(void)
{
    g_adc_sources.vbat_enabled = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_disable_vbat(void)
{
    g_adc_sources.vbat_enabled = 0U;
    return XY_HAL_OK;
}

int32_t xy_hal_adc_get_vbat_value(void)
{
    pc_adc_ctx_t *ctx = pc_adc_primary();

    if (ctx == NULL) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (g_adc_sources.vbat_enabled == 0U) {
        return XY_HAL_ERROR_NOT_SUPPORTED;
    }
    /* The battery voltage is the one presented to the converter; an on-board divider is a board fact,
     * not a HAL one, so it is not modelled. */
    return (int32_t)pc_adc_raw_from_mv(ctx, g_adc_sources.vbat_mv);
}

int32_t xy_hal_adc_get_temp_value(void)
{
    pc_adc_ctx_t *ctx = pc_adc_primary();

    if (ctx == NULL) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (g_adc_sources.temp_enabled == 0U) {
        return XY_HAL_ERROR_NOT_SUPPORTED;
    }
    return g_adc_sources.temp_centi;
}

xy_hal_error_t xy_hal_adc_set_window_threshold(void *adc, uint32_t low_threshold,
                                              uint32_t high_threshold)
{
    pc_adc_ctx_t *ctx;
    uint32_t maximum;

    if (adc == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_adc_find(adc);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    maximum = pc_adc_max_value(ctx->config.resolution);
    /* A window that is inverted or reaches past full scale cannot be watched, and reporting success
     * would leave a caller believing a conversion outside it will be flagged. */
    if (low_threshold > high_threshold || high_threshold > maximum) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx->window_low = low_threshold;
    ctx->window_high = high_threshold;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_get_window_threshold(void *adc, uint32_t *low_threshold,
                                              uint32_t *high_threshold)
{
    pc_adc_ctx_t *ctx;

    if (adc == NULL || low_threshold == NULL || high_threshold == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_adc_find(adc);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    *low_threshold = ctx->window_low;
    *high_threshold = ctx->window_high;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_enable_window_watchdog(void *adc, uint8_t channel)
{
    pc_adc_ctx_t *ctx;

    if (adc == NULL || channel >= PC_ADC_CHANNELS) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_adc_find(adc);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    ctx->window_channel = channel;
    ctx->window_enabled = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_disable_window_watchdog(void *adc, uint8_t channel)
{
    pc_adc_ctx_t *ctx;

    if (adc == NULL || channel >= PC_ADC_CHANNELS) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_adc_find(adc);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    /* The parts this stands in for have one watchdog and one watched channel per converter, so
     * disarming it clears the selection rather than remembering a per-channel state. */
    ctx->window_enabled = 0U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_register_callback(void *adc,
                                           xy_hal_adc_callback_t callback,
                                           void *arg)
{
    pc_adc_ctx_t *ctx;

    if (adc == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_adc_find(adc);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    ctx->callback = callback;
    ctx->callback_arg = arg;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_control(void *adc, uint32_t cmd, void *args)
{
    /* No backend in this tree defines a command set for this entry, so a host that accepted one would
     * be describing a protocol nothing else implements. Refusing is the only answer that stays true
     * on every platform. */
    (void)adc;
    (void)cmd;
    (void)args;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int32_t xy_hal_adc_get_state(void *adc)
{
    pc_adc_ctx_t *ctx;

    if (adc == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_adc_find(adc);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return (int32_t)ctx->started;
}

xy_hal_error_t xy_hal_adc_calibrate(void *adc)
{
    pc_adc_ctx_t *ctx = pc_adc_find(adc);

    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    /* Calibration on the part aborts a conversion in flight; the observable half of that is that a
     * pending conversion flag is gone afterwards. There is no gain/offset to adjust on a host, so no
     * correction is applied to later readings and none is claimed. */
    ctx->eoc = 0U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_set_prescaler(void *adc, uint8_t prescaler)
{
    pc_adc_ctx_t *ctx;

    if (adc == NULL || prescaler == 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_adc_find(adc);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    ctx->config.clock_div = prescaler;
    return XY_HAL_OK;
}

int32_t xy_hal_adc_get_prescaler(void *adc)
{
    pc_adc_ctx_t *ctx;

    if (adc == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_adc_find(adc);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return (int32_t)ctx->config.clock_div;
}

xy_hal_error_t xy_hal_adc_set_oversampling(void *adc, uint8_t oversampling)
{
    pc_adc_ctx_t *ctx;

    if (adc == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_adc_find(adc);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    ctx->oversampling = oversampling;
    return XY_HAL_OK;
}

int32_t xy_hal_adc_get_oversampling(void *adc)
{
    pc_adc_ctx_t *ctx;

    if (adc == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_adc_find(adc);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return (int32_t)ctx->oversampling;
}

/**
 * @brief Deliver one ADC event on the host.
 *
 * A vendor backend calls this from its interrupt handler; the host equivalent is a test or a driver
 * calling it directly. The event reaches the callback that was registered for this converter, and
 * nothing is delivered when no handler is registered.
 */
void xy_hal_adc_event_handler(void *adc, xy_hal_adc_evt_t event, void *arg)
{
    pc_adc_ctx_t *ctx = pc_adc_find(adc);

    (void)arg;
    if (ctx == NULL || ctx->initialized == 0U) {
        return;
    }
    pc_adc_notify(ctx, event);
}

/* ==================== Test hooks ==================== */

xy_hal_error_t xy_hal_adc_pc_set_input(void *adc, uint8_t channel, uint32_t millivolts)
{
    pc_adc_ctx_t *ctx;

    if (adc == NULL || channel >= PC_ADC_CHANNELS) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_adc_find(adc);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    ctx->input_mv[channel] = millivolts;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_pc_set_vref(void *adc, uint32_t millivolts)
{
    pc_adc_ctx_t *ctx;

    if (adc == NULL || millivolts == 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_adc_find(adc);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    ctx->vref_mv = millivolts;
    return XY_HAL_OK;
}

void xy_hal_adc_pc_set_temperature(int32_t centidegrees)
{
    g_adc_sources.temp_centi = centidegrees;
}

void xy_hal_adc_pc_set_vrefint(uint32_t millivolts)
{
    g_adc_sources.vrefint_mv = millivolts;
}

void xy_hal_adc_pc_set_vbat(uint32_t millivolts)
{
    g_adc_sources.vbat_mv = millivolts;
}
