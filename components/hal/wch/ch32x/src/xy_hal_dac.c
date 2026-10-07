/**
 * @file xy_hal_dac.c
 * @brief WCH CH32V30x DAC HAL implementation
 */

#include "xy_hal_dac.h"

#ifdef MCU_CH32

#include "ch32v30x.h"
#include "ch32v30x_dac.h"
#include <string.h>

#define WCH_DAC_CHANNEL_COUNT 2U

typedef struct {
    xy_hal_dac_config_t config;
    xy_hal_dac_callback_t callbacks[WCH_DAC_CHANNEL_COUNT];
    void* callback_args[WCH_DAC_CHANNEL_COUNT];
    uint16_t values[WCH_DAC_CHANNEL_COUNT];
    uint8_t enabled[WCH_DAC_CHANNEL_COUNT];
    uint8_t initialized;
} wch_dac_context_t;

static wch_dac_context_t context;

static int channel_index(xy_hal_dac_channel_t channel) {
    return channel <= XY_HAL_DAC_CHANNEL_2 ? (int)channel : -1;
}

static uint32_t sdk_channel(xy_hal_dac_channel_t channel) {
    return channel == XY_HAL_DAC_CHANNEL_1 ? DAC_Channel_1 : DAC_Channel_2;
}

static uint32_t sdk_trigger(xy_hal_dac_trigger_src_t trigger) {
    if (trigger == XY_HAL_DAC_TRIGGER_SOFTWARE)
        return DAC_Trigger_Software;
    if (trigger == XY_HAL_DAC_TRIGGER_TIMER)
        return DAC_Trigger_T6_TRGO;
    return DAC_Trigger_Ext_IT9;
}

static uint32_t sdk_wave(xy_hal_dac_wave_t wave) {
    if (wave == XY_HAL_DAC_WAVE_NOISE)
        return DAC_WaveGeneration_Noise;
    if (wave == XY_HAL_DAC_WAVE_TRIANGLE)
        return DAC_WaveGeneration_Triangle;
    return DAC_WaveGeneration_None;
}

static uint32_t sdk_align(xy_hal_dac_align_t align) {
    if (context.config.resolution == XY_HAL_DAC_RESOLUTION_8B)
        return DAC_Align_8b_R;
    return align == XY_HAL_DAC_DATAALIGN_LEFT ? DAC_Align_12b_L : DAC_Align_12b_R;
}

xy_hal_error_t xy_hal_dac_init(void* dac, const xy_hal_dac_config_t* config) {
    DAC_InitTypeDef init = {0};
    GPIO_InitTypeDef gpio = {0};
    if (dac != DAC || config == NULL || config->resolution > XY_HAL_DAC_RESOLUTION_12B ||
        config->align > XY_HAL_DAC_DATAALIGN_LEFT ||
        config->trigger_src > XY_HAL_DAC_TRIGGER_EXTI || config->wave > XY_HAL_DAC_WAVE_TRIANGLE ||
        config->buffer_enable > 1U)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (context.initialized)
        return XY_HAL_ERROR_ALREADY_INIT;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
    gpio.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
    gpio.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &gpio);
    DAC_DeInit();

    init.DAC_Trigger = sdk_trigger(config->trigger_src);
    init.DAC_WaveGeneration = sdk_wave(config->wave);
    init.DAC_LFSRUnmask_TriangleAmplitude = DAC_LFSRUnmask_Bit0;
    init.DAC_OutputBuffer =
        config->buffer_enable ? DAC_OutputBuffer_Enable : DAC_OutputBuffer_Disable;
    DAC_Init(DAC_Channel_1, &init);
    DAC_Init(DAC_Channel_2, &init);

    memset(&context, 0, sizeof(context));
    context.config = *config;
    context.initialized = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dac_deinit(void* dac) {
    if (dac != DAC)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!context.initialized)
        return XY_HAL_ERROR_NOT_INIT;
    DAC_Cmd(DAC_Channel_1, DISABLE);
    DAC_Cmd(DAC_Channel_2, DISABLE);
    DAC_DeInit();
    memset(&context, 0, sizeof(context));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dac_enable_channel(void* dac, xy_hal_dac_channel_t channel) {
    int index = channel_index(channel);
    if (dac != DAC || index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!context.initialized)
        return XY_HAL_ERROR_NOT_INIT;
    DAC_Cmd(sdk_channel(channel), ENABLE);
    context.enabled[index] = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dac_disable_channel(void* dac, xy_hal_dac_channel_t channel) {
    int index = channel_index(channel);
    if (dac != DAC || index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!context.initialized)
        return XY_HAL_ERROR_NOT_INIT;
    DAC_Cmd(sdk_channel(channel), DISABLE);
    context.enabled[index] = 0U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dac_set_value(void* dac, xy_hal_dac_channel_t channel, uint32_t value,
                                    xy_hal_dac_align_t align) {
    int index = channel_index(channel);
    uint32_t maximum;
    if (dac != DAC || index < 0 || align > XY_HAL_DAC_DATAALIGN_LEFT)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!context.initialized)
        return XY_HAL_ERROR_NOT_INIT;
    maximum = context.config.resolution == XY_HAL_DAC_RESOLUTION_8B ? 255U : 4095U;
    if (value > maximum)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (channel == XY_HAL_DAC_CHANNEL_1)
        DAC_SetChannel1Data(sdk_align(align), (uint16_t)value);
    else
        DAC_SetChannel2Data(sdk_align(align), (uint16_t)value);
    context.values[index] = (uint16_t)value;
    return XY_HAL_OK;
}

int xy_hal_dac_get_value(void* dac, xy_hal_dac_channel_t channel) {
    int index = channel_index(channel);
    if (dac != DAC || index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!context.initialized)
        return XY_HAL_ERROR_NOT_INIT;
    return (int)context.values[index];
}

xy_hal_error_t xy_hal_dac_start_output(void* dac, xy_hal_dac_channel_t channel) {
    int index = channel_index(channel);
    xy_hal_error_t status = xy_hal_dac_enable_channel(dac, channel);
    if (status != XY_HAL_OK)
        return status;
    if (context.callbacks[index] != NULL)
        context.callbacks[index](dac, channel, XY_HAL_DAC_EVENT_READY,
                                 context.callback_args[index]);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dac_stop_output(void* dac, xy_hal_dac_channel_t channel) {
    return xy_hal_dac_disable_channel(dac, channel);
}

xy_hal_error_t xy_hal_dac_software_trigger(void* dac, xy_hal_dac_channel_t channel) {
    int index = channel_index(channel);
    if (dac != DAC || index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!context.initialized || !context.enabled[index])
        return XY_HAL_ERROR_NOT_INIT;
    if (context.config.trigger_src != XY_HAL_DAC_TRIGGER_SOFTWARE)
        return XY_HAL_ERROR_NOT_SUPPORTED;
    DAC_SoftwareTriggerCmd(sdk_channel(channel), ENABLE);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dac_output_dma(void* dac, xy_hal_dac_channel_t channel, const uint32_t* data,
                                     size_t count) {
    (void)dac;
    (void)channel;
    (void)data;
    (void)count;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_dac_generate_triangle(void* dac, xy_hal_dac_channel_t channel,
                                            uint32_t amplitude) {
    (void)dac;
    (void)channel;
    (void)amplitude;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_dac_generate_noise(void* dac, xy_hal_dac_channel_t channel, uint32_t mask) {
    (void)dac;
    (void)channel;
    (void)mask;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_dac_register_callback(void* dac, xy_hal_dac_channel_t channel,
                                            xy_hal_dac_callback_t callback, void* arg) {
    int index = channel_index(channel);
    if (dac != DAC || index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!context.initialized)
        return XY_HAL_ERROR_NOT_INIT;
    context.callbacks[index] = callback;
    context.callback_args[index] = arg;
    return XY_HAL_OK;
}

uint32_t xy_hal_dac_value_to_mv(void* dac, uint32_t value, uint32_t vref) {
    uint32_t maximum;
    if (dac != DAC || !context.initialized)
        return 0U;
    maximum = context.config.resolution == XY_HAL_DAC_RESOLUTION_8B ? 255U : 4095U;
    return value <= maximum ? (value * vref) / maximum : 0U;
}

#endif /* MCU_CH32 */
