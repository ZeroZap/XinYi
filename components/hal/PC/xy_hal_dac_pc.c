/**
 * @file xy_hal_dac_pc.c
 * @brief PC/Linux simulation layer for the DAC HAL
 *
 * A host drives no analog output, so what is modelled is the state machine a caller can observe: the
 * value a channel was given is the value it reads back, a value outside the configured resolution is
 * refused instead of truncated, the alignment must be the one the channel was configured with, and a
 * software trigger advances the waveform generator rather than doing nothing.
 *
 * The waveform model is the generator, not the vendor's encoding. A triangle climbs from 0 to the
 * programmed amplitude and turns around; noise is a deterministic sequence bounded by the mask. The
 * host does not model conversion time, so a "step" here is one software trigger rather than one
 * update event.
 *
 * Where this backend is stricter than a hardware port in this tree, it is because the stricter answer
 * is the one a caller can act on: `xy_hal_dac_software_trigger()` refuses a channel that was never
 * enabled, and refuses a converter configured for a timer trigger. The F4 and U5 wrappers currently
 * write SWTRIGR without checking either.
 */

#include "xy_hal_dac.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define PC_DAC_SLOTS 2U
#define PC_DAC_CHANNELS 2U
#define PC_DAC_NOISE_MAX_MASK 11U
#define PC_DAC_NOISE_SEED 0x2545F491U

typedef struct {
    void *handle;
    uint8_t used;
    uint8_t initialized;
    xy_hal_dac_config_t config;
    uint32_t values[PC_DAC_CHANNELS];
    uint32_t wave_param[PC_DAC_CHANNELS];
    uint32_t noise_state[PC_DAC_CHANNELS];
    uint8_t enabled[PC_DAC_CHANNELS];
    uint8_t wave_down[PC_DAC_CHANNELS];
    xy_hal_dac_callback_t callbacks[PC_DAC_CHANNELS];
    void *callback_args[PC_DAC_CHANNELS];
} pc_dac_ctx_t;

static pc_dac_ctx_t g_dac_ctx[PC_DAC_SLOTS];

static pc_dac_ctx_t *pc_dac_find(const void *handle)
{
    unsigned int index;

    if (handle == NULL) {
        return NULL;
    }
    for (index = 0U; index < PC_DAC_SLOTS; index++) {
        if (g_dac_ctx[index].used != 0U && g_dac_ctx[index].handle == handle) {
            return &g_dac_ctx[index];
        }
    }
    return NULL;
}

static pc_dac_ctx_t *pc_dac_slot(void)
{
    unsigned int index;

    for (index = 0U; index < PC_DAC_SLOTS; index++) {
        if (g_dac_ctx[index].used == 0U) {
            g_dac_ctx[index].used = 1U;
            return &g_dac_ctx[index];
        }
    }
    return NULL;
}

static int pc_dac_channel_index(xy_hal_dac_channel_t channel)
{
    return channel <= XY_HAL_DAC_CHANNEL_2 ? (int)channel : -1;
}

static uint32_t pc_dac_max_value(xy_hal_dac_resolution_t resolution)
{
    return resolution == XY_HAL_DAC_RESOLUTION_8B ? 255U : 4095U;
}

static int pc_dac_config_valid(const xy_hal_dac_config_t *config)
{
    if (config->resolution > XY_HAL_DAC_RESOLUTION_12B) return 0;
    if (config->align > XY_HAL_DAC_DATAALIGN_LEFT) return 0;
    if (config->trigger_src > XY_HAL_DAC_TRIGGER_EXTI) return 0;
    if (config->wave > XY_HAL_DAC_WAVE_TRIANGLE) return 0;
    if (config->buffer_enable > 1U) return 0;
    /* An 8-bit channel is right-aligned on every part this tree ports to; a left-aligned 8-bit
     * request is a configuration the hardware does not have, so it is refused rather than bent. */
    if (config->resolution == XY_HAL_DAC_RESOLUTION_8B &&
        config->align != XY_HAL_DAC_DATAALIGN_RIGHT) {
        return 0;
    }
    return 1;
}

static void pc_dac_notify(pc_dac_ctx_t *ctx, xy_hal_dac_channel_t channel,
                          xy_hal_dac_event_t event)
{
    int index = pc_dac_channel_index(channel);

    if (index < 0 || ctx->callbacks[index] == NULL) {
        return;
    }
    ctx->callbacks[index](ctx->handle, channel, event, ctx->callback_args[index]);
}

/* One software trigger advances the generator one step. */
static void pc_dac_advance(pc_dac_ctx_t *ctx, int index)
{
    uint32_t amplitude = ctx->wave_param[index];
    uint32_t value = ctx->values[index];

    if (ctx->config.wave == XY_HAL_DAC_WAVE_TRIANGLE) {
        if (amplitude == 0U) {
            return; /* no amplitude was ever programmed: there is no ramp to advance */
        }
        if (ctx->wave_down[index] == 0U) {
            if (value >= amplitude) {
                ctx->wave_down[index] = 1U;
                value = value > 0U ? value - 1U : 0U;
            } else {
                value++;
            }
        } else if (value == 0U) {
            ctx->wave_down[index] = 0U;
            value = 1U;
        } else {
            value--;
        }
        ctx->values[index] = value;
        return;
    }
    if (ctx->config.wave == XY_HAL_DAC_WAVE_NOISE) {
        uint32_t width = amplitude + 1U; /* the mask is a width in bits, as on the parts */

        ctx->noise_state[index] = ctx->noise_state[index] * 1664525U + 1013904223U;
        value = (ctx->noise_state[index] >> 16U) & ((1U << width) - 1U);
        if (value > pc_dac_max_value(ctx->config.resolution)) {
            value = pc_dac_max_value(ctx->config.resolution);
        }
        ctx->values[index] = value;
    }
}

xy_hal_error_t xy_hal_dac_init(void *dac, const xy_hal_dac_config_t *config)
{
    pc_dac_ctx_t *ctx;
    unsigned int index;

    if (dac == NULL || config == NULL || !pc_dac_config_valid(config)) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_dac_find(dac);
    if (ctx != NULL && ctx->initialized != 0U) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }
    if (ctx == NULL) {
        ctx = pc_dac_slot();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
        ctx->handle = dac;
    }
    memset(ctx->values, 0, sizeof(ctx->values));
    memset(ctx->wave_param, 0, sizeof(ctx->wave_param));
    memset(ctx->enabled, 0, sizeof(ctx->enabled));
    memset(ctx->wave_down, 0, sizeof(ctx->wave_down));
    memset(ctx->callbacks, 0, sizeof(ctx->callbacks));
    memset(ctx->callback_args, 0, sizeof(ctx->callback_args));
    for (index = 0U; index < PC_DAC_CHANNELS; index++) {
        ctx->noise_state[index] = PC_DAC_NOISE_SEED + index;
    }
    ctx->config = *config;
    ctx->initialized = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dac_deinit(void *dac)
{
    pc_dac_ctx_t *ctx;

    if (dac == NULL) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx = pc_dac_find(dac);
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    memset(ctx, 0, sizeof(*ctx));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dac_enable_channel(void *dac, xy_hal_dac_channel_t channel)
{
    pc_dac_ctx_t *ctx = pc_dac_find(dac);
    int index = pc_dac_channel_index(channel);

    if (index < 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    ctx->enabled[index] = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dac_disable_channel(void *dac, xy_hal_dac_channel_t channel)
{
    pc_dac_ctx_t *ctx = pc_dac_find(dac);
    int index = pc_dac_channel_index(channel);

    if (index < 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    ctx->enabled[index] = 0U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dac_set_value(void *dac, xy_hal_dac_channel_t channel,
                                    uint32_t value, xy_hal_dac_align_t align)
{
    pc_dac_ctx_t *ctx = pc_dac_find(dac);
    int index = pc_dac_channel_index(channel);

    if (index < 0 || align > XY_HAL_DAC_DATAALIGN_LEFT) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    /* The channel was configured with one alignment; a value written with another would be shifted
     * on the part instead of refused, which reads as an attenuator rather than an error. */
    if (align != ctx->config.align) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (value > pc_dac_max_value(ctx->config.resolution)) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx->values[index] = value;
    return XY_HAL_OK;
}

int xy_hal_dac_get_value(void *dac, xy_hal_dac_channel_t channel)
{
    pc_dac_ctx_t *ctx = pc_dac_find(dac);
    int index = pc_dac_channel_index(channel);

    if (index < 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return (int)ctx->values[index];
}

xy_hal_error_t xy_hal_dac_start_output(void *dac, xy_hal_dac_channel_t channel)
{
    xy_hal_error_t status = xy_hal_dac_enable_channel(dac, channel);
    pc_dac_ctx_t *ctx;

    if (status != XY_HAL_OK) {
        return status;
    }
    ctx = pc_dac_find(dac);
    pc_dac_notify(ctx, channel, XY_HAL_DAC_EVENT_READY);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dac_stop_output(void *dac, xy_hal_dac_channel_t channel)
{
    return xy_hal_dac_disable_channel(dac, channel);
}

xy_hal_error_t xy_hal_dac_software_trigger(void *dac, xy_hal_dac_channel_t channel)
{
    pc_dac_ctx_t *ctx = pc_dac_find(dac);
    int index = pc_dac_channel_index(channel);

    if (index < 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (ctx->enabled[index] == 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (ctx->config.trigger_src != XY_HAL_DAC_TRIGGER_SOFTWARE) {
        return XY_HAL_ERROR_NOT_SUPPORTED;
    }
    pc_dac_advance(ctx, index);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dac_output_dma(void *dac, xy_hal_dac_channel_t channel,
                                     const uint32_t *data, size_t count)
{
    pc_dac_ctx_t *ctx = pc_dac_find(dac);
    int index = pc_dac_channel_index(channel);
    size_t sample;
    uint32_t maximum;

    if (index < 0 || data == NULL || count == 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    maximum = pc_dac_max_value(ctx->config.resolution);
    /* The whole buffer is checked first: a transfer that emits the samples that fit and rejects the
     * rest would leave a waveform nobody asked for on the pin. */
    for (sample = 0U; sample < count; sample++) {
        if (data[sample] > maximum) {
            return XY_HAL_ERROR_INVALID_PARAM;
        }
    }
    /* There is no stream to keep fed on a host, so the transfer completes immediately and the latched
     * value is the last sample - a model of a completed DMA round, not of its timing. */
    ctx->values[index] = data[count - 1U];
    pc_dac_notify(ctx, channel, XY_HAL_DAC_EVENT_READY);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dac_generate_triangle(void *dac, xy_hal_dac_channel_t channel,
                                            uint32_t amplitude)
{
    pc_dac_ctx_t *ctx = pc_dac_find(dac);
    int index = pc_dac_channel_index(channel);
    uint32_t maximum;

    if (index < 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    maximum = pc_dac_max_value(ctx->config.resolution);
    if (amplitude == 0U || amplitude > maximum) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ctx->config.wave = XY_HAL_DAC_WAVE_TRIANGLE;
    ctx->wave_param[index] = amplitude;
    ctx->wave_down[index] = 0U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dac_generate_noise(void *dac, xy_hal_dac_channel_t channel,
                                         uint32_t mask)
{
    pc_dac_ctx_t *ctx = pc_dac_find(dac);
    int index = pc_dac_channel_index(channel);

    if (index < 0 || mask > PC_DAC_NOISE_MAX_MASK) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    ctx->config.wave = XY_HAL_DAC_WAVE_NOISE;
    ctx->wave_param[index] = mask;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dac_register_callback(void *dac, xy_hal_dac_channel_t channel,
                                            xy_hal_dac_callback_t callback, void *arg)
{
    pc_dac_ctx_t *ctx = pc_dac_find(dac);
    int index = pc_dac_channel_index(channel);

    if (index < 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (ctx == NULL || ctx->initialized == 0U) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    ctx->callbacks[index] = callback;
    ctx->callback_args[index] = arg;
    return XY_HAL_OK;
}

uint32_t xy_hal_dac_value_to_mv(void *dac, uint32_t value, uint32_t vref)
{
    pc_dac_ctx_t *ctx = pc_dac_find(dac);
    uint32_t maximum;

    if (ctx == NULL || ctx->initialized == 0U) {
        return 0U;
    }
    maximum = pc_dac_max_value(ctx->config.resolution);
    /* A value the channel could not have been given is not a voltage. */
    if (value > maximum) {
        return 0U;
    }
    return (uint32_t)(((uint64_t)value * (uint64_t)vref) / (uint64_t)maximum);
}
