/**
 * @file xy_hal_adc.c
 * @brief WCH CH32V30x ADC HAL implementation
 */

#include "xy_hal_adc.h"

#ifdef MCU_CH32

#include "ch32v30x.h"
#include <string.h>

#define WCH_ADC_COUNT 2U
#define WCH_ADC_CHANNEL_COUNT 18U

typedef struct {
    ADC_TypeDef* adc;
    xy_hal_adc_config_t config;
    uint8_t sample_times[WCH_ADC_CHANNEL_COUNT];
    uint16_t low_threshold;
    uint16_t high_threshold;
    xy_hal_adc_callback_t callback;
    void* callback_arg;
    uint8_t initialized;
    uint8_t started;
} wch_adc_context_t;

static wch_adc_context_t contexts[WCH_ADC_COUNT];

static int adc_index(const void* adc) {
    if (adc == ADC1)
        return 0;
    if (adc == ADC2)
        return 1;

    return -1;
}

static uint32_t adc_tick_ms(void) {
    return SysTick->CNT / (SystemCoreClock / 1000U);
}

static uint8_t sample_time(uint32_t value) {
    return value <= ADC_SampleTime_239Cycles5 ? (uint8_t)value : ADC_SampleTime_55Cycles5;
}

static void enable_clock(int index) {
    static const uint32_t clocks[WCH_ADC_COUNT] = {RCC_APB2Periph_ADC1, RCC_APB2Periph_ADC2};
    RCC_APB2PeriphClockCmd(clocks[index], ENABLE);
}

xy_hal_error_t xy_hal_adc_init(void* adc, const xy_hal_adc_config_t* config) {
    ADC_InitTypeDef init = {0};
    int index = adc_index(adc);
    if (index < 0 || config == NULL || config->resolution != XY_HAL_ADC_RESOLUTION_12B ||
        config->align > XY_HAL_ADC_DATAALIGN_LEFT || config->scan_mode > XY_HAL_ADC_SCAN_ENABLE ||
        config->continuous > XY_HAL_ADC_CONTINUOUS_ENABLE ||
        config->trigger_src != XY_HAL_ADC_TRIGGER_SOFTWARE || config->enable_vbat != 0U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (contexts[index].initialized)
        return XY_HAL_ERROR_ALREADY_INIT;
    enable_clock(index);
    RCC_ADCCLKConfig(RCC_PCLK2_Div8);
    ADC_DeInit(adc);
    init.ADC_Mode = ADC_Mode_Independent;
    init.ADC_ScanConvMode = config->scan_mode ? ENABLE : DISABLE;
    init.ADC_ContinuousConvMode = config->continuous ? ENABLE : DISABLE;
    init.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    init.ADC_DataAlign =
        config->align == XY_HAL_ADC_DATAALIGN_LEFT ? ADC_DataAlign_Left : ADC_DataAlign_Right;
    init.ADC_NbrOfChannel = 1U;
    init.ADC_OutputBuffer = ADC_OutputBuffer_Enable;
    init.ADC_Pga = ADC_Pga_1;
    ADC_Init(adc, &init);
    ADC_Cmd(adc, ENABLE);
    ADC_ResetCalibration(adc);
    while (ADC_GetResetCalibrationStatus(adc) != RESET) {
    }
    ADC_StartCalibration(adc);
    while (ADC_GetCalibrationStatus(adc) != RESET) {
    }
    memset(&contexts[index], 0, sizeof(contexts[index]));
    contexts[index].adc = adc;
    contexts[index].config = *config;
    memset(contexts[index].sample_times, sample_time(config->sampling_time),
           sizeof(contexts[index].sample_times));
    contexts[index].initialized = 1U;
    if (config->enable_temp_sensor || config->enable_vrefint)
        ADC_TempSensorVrefintCmd(ENABLE);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_deinit(void* adc) {
    int index = adc_index(adc);
    if (index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[index].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    ADC_Cmd(adc, DISABLE);
    ADC_DeInit(adc);
    memset(&contexts[index], 0, sizeof(contexts[index]));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_config_channels(void* adc, const xy_hal_adc_channel_config_t* channels,
                                          size_t count) {
    int index = adc_index(adc);
    size_t i;
    if (index < 0 || channels == NULL || count == 0U || count > 16U) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (!contexts[index].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    for (i = 0U; i < count; ++i) {
        if (channels[i].channel >= WCH_ADC_CHANNEL_COUNT || channels[i].rank == 0U ||
            channels[i].rank > 16U)
            return XY_HAL_ERROR_INVALID_PARAM;
        if (channels[i].enabled) {
            contexts[index].sample_times[channels[i].channel] =
                sample_time(channels[i].sampling_time);
            ADC_RegularChannelConfig(adc, channels[i].channel, channels[i].rank,
                                     contexts[index].sample_times[channels[i].channel]);
        }
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_start(void* adc) {
    int index = adc_index(adc);
    if (index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[index].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    ADC_SoftwareStartConvCmd(adc, ENABLE);
    contexts[index].started = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_stop(void* adc) {
    int index = adc_index(adc);
    if (index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[index].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    ADC_SoftwareStartConvCmd(adc, DISABLE);
    contexts[index].started = 0U;
    return XY_HAL_OK;
}

int32_t xy_hal_adc_read(void* adc, uint8_t channel, uint32_t timeout) {
    int index = adc_index(adc);
    uint32_t start;
    if (index < 0 || channel >= WCH_ADC_CHANNEL_COUNT)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[index].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    ADC_RegularChannelConfig(adc, channel, 1U, contexts[index].sample_times[channel]);
    ADC_SoftwareStartConvCmd(adc, ENABLE);
    start = adc_tick_ms();
    while (ADC_GetFlagStatus(adc, ADC_FLAG_EOC) == RESET) {
        if (timeout != 0U && (adc_tick_ms() - start) >= timeout)
            return XY_HAL_ERROR_TIMEOUT;
    }
    ADC_ClearFlag(adc, ADC_FLAG_EOC);
    return (int32_t)ADC_GetConversionValue(adc);
}

int32_t xy_hal_adc_read_nb(void* adc, uint8_t channel) {
    int index = adc_index(adc);
    if (index < 0 || channel >= WCH_ADC_CHANNEL_COUNT)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[index].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    if (ADC_GetFlagStatus(adc, ADC_FLAG_EOC) == RESET)
        return XY_HAL_ERROR_BUSY;
    ADC_ClearFlag(adc, ADC_FLAG_EOC);
    return (int32_t)ADC_GetConversionValue(adc);
}

int32_t xy_hal_adc_read_multi(void* adc, const uint8_t* channels, uint32_t* values, size_t count,
                              uint32_t timeout) {
    size_t i;
    if (channels == NULL || values == NULL || count == 0U)
        return XY_HAL_ERROR_INVALID_PARAM;
    for (i = 0U; i < count; ++i) {
        int32_t value = xy_hal_adc_read(adc, channels[i], timeout);
        if (value < 0)
            return value;
        values[i] = (uint32_t)value;
    }
    return (int32_t)count;
}

xy_hal_error_t xy_hal_adc_read_dma(void* adc, uint32_t* buffer, size_t count) {
    XY_UNUSED(adc);
    XY_UNUSED(buffer);
    XY_UNUSED(count);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int32_t xy_hal_adc_get_resolution(void* adc) {
    int index = adc_index(adc);
    if (index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    return contexts[index].initialized ? 12 : XY_HAL_ERROR_NOT_INIT;
}

uint32_t xy_hal_adc_get_max_value(void* adc) {
    return adc_index(adc) >= 0 ? 4095U : 0U;
}

uint32_t xy_hal_adc_value_to_mv(void* adc, uint32_t value, uint32_t vref) {
    return adc_index(adc) >= 0 ? (value * vref) / 4095U : 0U;
}

xy_hal_error_t xy_hal_adc_set_sampling_time(void* adc, uint8_t channel, uint32_t sampling) {
    int index = adc_index(adc);
    if (index < 0 || channel >= WCH_ADC_CHANNEL_COUNT)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[index].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    contexts[index].sample_times[channel] = sample_time(sampling);
    return XY_HAL_OK;
}

int32_t xy_hal_adc_get_sampling_time(void* adc, uint8_t channel) {
    int index = adc_index(adc);
    if (index < 0 || channel >= WCH_ADC_CHANNEL_COUNT)
        return XY_HAL_ERROR_INVALID_PARAM;
    return contexts[index].initialized ? contexts[index].sample_times[channel]
                                       : XY_HAL_ERROR_NOT_INIT;
}

xy_hal_error_t xy_hal_adc_set_trigger(void* adc, xy_hal_adc_trigger_src_t trigger) {
    int index = adc_index(adc);
    if (index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[index].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    if (trigger != XY_HAL_ADC_TRIGGER_SOFTWARE)
        return XY_HAL_ERROR_NOT_SUPPORTED;
    contexts[index].config.trigger_src = trigger;
    return XY_HAL_OK;
}

int32_t xy_hal_adc_get_trigger(void* adc) {
    int index = adc_index(adc);
    if (index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    return contexts[index].initialized ? (int32_t)contexts[index].config.trigger_src
                                       : XY_HAL_ERROR_NOT_INIT;
}

xy_hal_error_t xy_hal_adc_set_continuous(void* adc, xy_hal_adc_continuous_t continuous) {
    int index = adc_index(adc);
    if (index < 0 || continuous > XY_HAL_ADC_CONTINUOUS_ENABLE)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[index].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    if (continuous == XY_HAL_ADC_CONTINUOUS_ENABLE)
        ((ADC_TypeDef*)adc)->CTLR2 |= ADC_CONT;
    else
        ((ADC_TypeDef*)adc)->CTLR2 &= ~ADC_CONT;
    contexts[index].config.continuous = continuous;
    return XY_HAL_OK;
}

int32_t xy_hal_adc_get_continuous(void* adc) {
    int index = adc_index(adc);
    if (index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    return contexts[index].initialized ? (int32_t)contexts[index].config.continuous
                                       : XY_HAL_ERROR_NOT_INIT;
}

xy_hal_error_t xy_hal_adc_enable_temp_sensor(void) {
    ADC_TempSensorVrefintCmd(ENABLE);
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_adc_disable_temp_sensor(void) {
    ADC_TempSensorVrefintCmd(DISABLE);
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_adc_enable_vrefint(void) {
    ADC_TempSensorVrefintCmd(ENABLE);
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_adc_disable_vrefint(void) {
    ADC_TempSensorVrefintCmd(DISABLE);
    return XY_HAL_OK;
}
int32_t xy_hal_adc_get_vrefint_value(void) {
    return xy_hal_adc_read(ADC1, ADC_Channel_Vrefint, 100U);
}
xy_hal_error_t xy_hal_adc_enable_vbat(void) {
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
xy_hal_error_t xy_hal_adc_disable_vbat(void) {
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
int32_t xy_hal_adc_get_vbat_value(void) {
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
int32_t xy_hal_adc_get_temp_value(void) {
    int32_t raw = xy_hal_adc_read(ADC1, ADC_Channel_TempSensor, 100U);
    return raw < 0 ? raw : TempSensor_Volt_To_Temper(raw) * 100;
}

xy_hal_error_t xy_hal_adc_set_window_threshold(void* adc, uint32_t low, uint32_t high) {
    int index = adc_index(adc);
    if (index < 0 || low > high || high > 4095U)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[index].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    ADC_AnalogWatchdogThresholdsConfig(adc, (uint16_t)high, (uint16_t)low);
    contexts[index].low_threshold = (uint16_t)low;
    contexts[index].high_threshold = (uint16_t)high;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_get_window_threshold(void* adc, uint32_t* low, uint32_t* high) {
    int index = adc_index(adc);
    if (index < 0 || low == NULL || high == NULL)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[index].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    *low = contexts[index].low_threshold;
    *high = contexts[index].high_threshold;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_enable_window_watchdog(void* adc, uint8_t channel) {
    int index = adc_index(adc);
    if (index < 0 || channel >= WCH_ADC_CHANNEL_COUNT)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[index].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    ADC_AnalogWatchdogSingleChannelConfig(adc, channel);
    ADC_AnalogWatchdogCmd(adc, ADC_AnalogWatchdog_SingleRegEnable);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_disable_window_watchdog(void* adc, uint8_t channel) {
    int index = adc_index(adc);
    XY_UNUSED(channel);
    if (index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[index].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    ADC_AnalogWatchdogCmd(adc, ADC_AnalogWatchdog_None);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_register_callback(void* adc, xy_hal_adc_callback_t callback, void* arg) {
    int index = adc_index(adc);
    if (index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[index].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    contexts[index].callback = callback;
    contexts[index].callback_arg = arg;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_control(void* adc, uint32_t cmd, void* args) {
    XY_UNUSED(adc);
    XY_UNUSED(cmd);
    XY_UNUSED(args);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int32_t xy_hal_adc_get_state(void* adc) {
    int index = adc_index(adc);
    if (index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[index].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    return contexts[index].started;
}

xy_hal_error_t xy_hal_adc_calibrate(void* adc) {
    int index = adc_index(adc);
    if (index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[index].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    ADC_ResetCalibration(adc);
    while (ADC_GetResetCalibrationStatus(adc) != RESET) {
    }
    ADC_StartCalibration(adc);
    while (ADC_GetCalibrationStatus(adc) != RESET) {
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_set_prescaler(void* adc, uint8_t prescaler) {
    int index = adc_index(adc);
    if (index < 0 || (prescaler != 2U && prescaler != 4U && prescaler != 6U && prescaler != 8U))
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[index].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    RCC_ADCCLKConfig(prescaler == 2U   ? RCC_PCLK2_Div2
                     : prescaler == 4U ? RCC_PCLK2_Div4
                     : prescaler == 6U ? RCC_PCLK2_Div6
                                       : RCC_PCLK2_Div8);
    contexts[index].config.clock_div = prescaler;
    return XY_HAL_OK;
}

int32_t xy_hal_adc_get_prescaler(void* adc) {
    int index = adc_index(adc);
    if (index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    return contexts[index].initialized ? (int32_t)contexts[index].config.clock_div
                                       : XY_HAL_ERROR_NOT_INIT;
}

xy_hal_error_t xy_hal_adc_set_oversampling(void* adc, uint8_t oversampling) {
    XY_UNUSED(adc);
    XY_UNUSED(oversampling);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
int32_t xy_hal_adc_get_oversampling(void* adc) {
    XY_UNUSED(adc);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

void xy_hal_adc_event_handler(void* adc, xy_hal_adc_evt_t event, void* arg) {
    int index = adc_index(adc);
    XY_UNUSED(arg);
    if (index >= 0 && contexts[index].initialized && contexts[index].callback != NULL)
        contexts[index].callback(adc, event, contexts[index].callback_arg);
}

#else
#error "WCH ADC backend requires MCU_CH32"
#endif
