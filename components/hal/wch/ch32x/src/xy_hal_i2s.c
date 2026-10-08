/**
 * @file xy_hal_i2s.c
 * @brief WCH CH32V30x I2S HAL implementation
 */
#include "xy_hal_i2s.h"
#if defined(MCU_CH32) || defined(CH32V30x)
#include "ch32v30x_spi.h"
#include <string.h>

typedef struct {
    SPI_TypeDef* instance;
    xy_hal_i2s_config_t config;
    xy_hal_i2s_callback_t callback;
    void* callback_arg;
    int32_t error;
    uint8_t initialized;
    uint8_t started;
    uint8_t sleeping;
    uint8_t low_power;
} i2s_context_t;
static i2s_context_t contexts[2];

static int instance_index(const void* i2s) {
    if (i2s == SPI2)
        return 0;
    if (i2s == SPI3)
        return 1;
    return -1;
}
static uint32_t tick_ms(void) {
    uint32_t divisor = SystemCoreClock / 8U / 1000U;
    return divisor == 0U ? 0U : (uint32_t)(SysTick->CNT / divisor);
}
static int valid_frequency(uint32_t frequency) {
    static const uint32_t values[] = {8000U,  11025U, 16000U, 22050U, 32000U,
                                      44100U, 48000U, 96000U, 192000U};
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); ++i)
        if (frequency == values[i])
            return 1;
    return 0;
}
static uint16_t mode_value(xy_hal_i2s_mode_t mode) {
    static const uint16_t values[] = {I2S_Mode_SlaveTx, I2S_Mode_SlaveRx, I2S_Mode_MasterTx,
                                      I2S_Mode_MasterRx};
    return values[mode];
}
static uint16_t standard_value(xy_hal_i2s_standard_t standard) {
    static const uint16_t values[] = {I2S_Standard_Phillips, I2S_Standard_MSB, I2S_Standard_LSB,
                                      I2S_Standard_PCMShort, I2S_Standard_PCMLong};
    return values[standard];
}
static uint16_t format_value(xy_hal_i2s_dataformat_t format) {
    static const uint16_t values[] = {I2S_DataFormat_16b, I2S_DataFormat_16bextended,
                                      I2S_DataFormat_24b, I2S_DataFormat_32b};
    return values[format];
}
static void enable_instance(int index) {
    RCC_APB1PeriphClockCmd(index == 0 ? RCC_APB1Periph_SPI2 : RCC_APB1Periph_SPI3, ENABLE);
}
static xy_hal_error_t apply(i2s_context_t* context) {
    I2S_InitTypeDef init = {0};
    if (context->config.mode > XY_HAL_I2S_MODE_MASTER_RX)
        return XY_HAL_ERROR_NOT_SUPPORTED;
    init.I2S_Mode = mode_value(context->config.mode);
    init.I2S_Standard = standard_value(context->config.standard);
    init.I2S_DataFormat = format_value(context->config.data_format);
    init.I2S_MCLKOutput =
        context->config.mck_output ? I2S_MCLKOutput_Enable : I2S_MCLKOutput_Disable;
    init.I2S_AudioFreq = context->config.audio_freq;
    init.I2S_CPOL = context->config.cpol ? I2S_CPOL_High : I2S_CPOL_Low;
    I2S_Init(context->instance, &init);
    return XY_HAL_OK;
}
static xy_hal_error_t reconfigure(i2s_context_t* context) {
    uint8_t started = context->started;
    I2S_Cmd(context->instance, DISABLE);
    xy_hal_error_t status = apply(context);
    if (status == XY_HAL_OK && started)
        I2S_Cmd(context->instance, ENABLE);
    return status;
}
static i2s_context_t* ready_context(void* i2s) {
    int index = instance_index(i2s);
    return index >= 0 && contexts[index].initialized ? &contexts[index] : NULL;
}

xy_hal_error_t xy_hal_i2s_init(void* i2s, const xy_hal_i2s_config_t* config) {
    int index = instance_index(i2s);
    if (index < 0 || config == NULL || config->mode > XY_HAL_I2S_MODE_SLAVE_FULLDUPLEX ||
        config->standard > XY_HAL_I2S_STANDARD_PCM_LONG ||
        config->data_format > XY_HAL_I2S_DATAFORMAT_32B || config->cpol > XY_HAL_I2S_CPOL_HIGH ||
        config->mck_output > XY_HAL_I2S_MCLK_ENABLE || !valid_frequency(config->audio_freq))
        return XY_HAL_ERROR_INVALID_PARAM;
    if (contexts[index].initialized)
        return XY_HAL_ERROR_ALREADY_INIT;
    if (config->mode > XY_HAL_I2S_MODE_MASTER_RX ||
        (config->comm_mode != XY_HAL_I2S_COMMUNICATION_TXONLY &&
         config->comm_mode != XY_HAL_I2S_COMMUNICATION_RXONLY))
        return XY_HAL_ERROR_NOT_SUPPORTED;
    memset(&contexts[index], 0, sizeof(contexts[index]));
    contexts[index].instance = i2s;
    contexts[index].config = *config;
    enable_instance(index);
    SPI_I2S_DeInit(i2s);
    xy_hal_error_t status = apply(&contexts[index]);
    if (status != XY_HAL_OK)
        return status;
    contexts[index].initialized = 1U;
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_i2s_deinit(void* i2s) {
    i2s_context_t* context = ready_context(i2s);
    if (instance_index(i2s) < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (context == NULL)
        return XY_HAL_ERROR_NOT_INIT;
    I2S_Cmd(i2s, DISABLE);
    SPI_I2S_DeInit(i2s);
    memset(context, 0, sizeof(*context));
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_i2s_start(void* i2s) {
    i2s_context_t* context = ready_context(i2s);
    if (instance_index(i2s) < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (context == NULL)
        return XY_HAL_ERROR_NOT_INIT;
    I2S_Cmd(i2s, ENABLE);
    context->started = 1U;
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_i2s_stop(void* i2s) {
    i2s_context_t* context = ready_context(i2s);
    if (instance_index(i2s) < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (context == NULL)
        return XY_HAL_ERROR_NOT_INIT;
    I2S_Cmd(i2s, DISABLE);
    context->started = 0U;
    return XY_HAL_OK;
}
static int wait_flag(SPI_TypeDef* i2s, uint16_t flag, uint32_t timeout) {
    uint32_t start = tick_ms();
    while (SPI_I2S_GetFlagStatus(i2s, flag) == RESET)
        if (timeout != 0U && tick_ms() - start >= timeout)
            return 0;
    return 1;
}
int32_t xy_hal_i2s_send(void* i2s, const void* data, size_t size, uint32_t timeout) {
    i2s_context_t* context = ready_context(i2s);
    const uint16_t* samples = data;
    if (instance_index(i2s) < 0 || data == NULL || size == 0U)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (context == NULL || !context->started)
        return XY_HAL_ERROR_NOT_INIT;
    if (context->config.mode == XY_HAL_I2S_MODE_SLAVE_RX ||
        context->config.mode == XY_HAL_I2S_MODE_MASTER_RX)
        return XY_HAL_ERROR_NOT_SUPPORTED;
    for (size_t i = 0; i < size; ++i) {
        if (!wait_flag(i2s, SPI_I2S_FLAG_TXE, timeout)) {
            context->error = XY_HAL_ERROR_TIMEOUT;
            return XY_HAL_ERROR_TIMEOUT;
        }
        SPI_I2S_SendData(i2s, samples[i]);
    }
    if (context->callback != NULL)
        context->callback(i2s, XY_HAL_I2S_EVENT_TX_COMPLETE, context->callback_arg);
    return (int32_t)size;
}
int32_t xy_hal_i2s_receive(void* i2s, void* data, size_t size, uint32_t timeout) {
    i2s_context_t* context = ready_context(i2s);
    uint16_t* samples = data;
    if (instance_index(i2s) < 0 || data == NULL || size == 0U)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (context == NULL || !context->started)
        return XY_HAL_ERROR_NOT_INIT;
    if (context->config.mode == XY_HAL_I2S_MODE_SLAVE_TX ||
        context->config.mode == XY_HAL_I2S_MODE_MASTER_TX)
        return XY_HAL_ERROR_NOT_SUPPORTED;
    for (size_t i = 0; i < size; ++i) {
        if (!wait_flag(i2s, SPI_I2S_FLAG_RXNE, timeout)) {
            context->error = XY_HAL_ERROR_TIMEOUT;
            return XY_HAL_ERROR_TIMEOUT;
        }
        samples[i] = SPI_I2S_ReceiveData(i2s);
    }
    if (context->callback != NULL)
        context->callback(i2s, XY_HAL_I2S_EVENT_RX_COMPLETE, context->callback_arg);
    return (int32_t)size;
}
#define UNSUPPORTED3(name, t1, t2, t3)                                                             \
    xy_hal_error_t name(t1 a, t2 b, t3 c) {                                                        \
        (void)a;                                                                                   \
        (void)b;                                                                                   \
        (void)c;                                                                                   \
        return XY_HAL_ERROR_NOT_SUPPORTED;                                                         \
    }
UNSUPPORTED3(xy_hal_i2s_send_dma, void*, const void*, size_t)
UNSUPPORTED3(xy_hal_i2s_receive_dma, void*, void*, size_t)
UNSUPPORTED3(xy_hal_i2s_send_it, void*, const void*, size_t)
UNSUPPORTED3(xy_hal_i2s_receive_it, void*, void*, size_t)

xy_hal_error_t xy_hal_i2s_set_audio_freq(void* i2s, uint32_t value) {
    i2s_context_t* c = ready_context(i2s);
    if (c == NULL)
        return XY_HAL_ERROR_NOT_INIT;
    if (!valid_frequency(value))
        return XY_HAL_ERROR_INVALID_PARAM;
    c->config.audio_freq = value;
    return reconfigure(c);
}
int32_t xy_hal_i2s_get_audio_freq(void* i2s) {
    i2s_context_t* c = ready_context(i2s);
    return c ? (int32_t)c->config.audio_freq : XY_HAL_ERROR_NOT_INIT;
}
xy_hal_error_t xy_hal_i2s_set_data_format(void* i2s, xy_hal_i2s_dataformat_t value) {
    i2s_context_t* c = ready_context(i2s);
    if (!c)
        return XY_HAL_ERROR_NOT_INIT;
    if (value > XY_HAL_I2S_DATAFORMAT_32B)
        return XY_HAL_ERROR_INVALID_PARAM;
    c->config.data_format = value;
    return reconfigure(c);
}
int32_t xy_hal_i2s_get_data_format(void* i2s) {
    i2s_context_t* c = ready_context(i2s);
    return c ? (int32_t)c->config.data_format : XY_HAL_ERROR_NOT_INIT;
}
xy_hal_error_t xy_hal_i2s_set_mode(void* i2s, xy_hal_i2s_mode_t value) {
    i2s_context_t* c = ready_context(i2s);
    if (!c)
        return XY_HAL_ERROR_NOT_INIT;
    if (value > XY_HAL_I2S_MODE_MASTER_RX)
        return XY_HAL_ERROR_NOT_SUPPORTED;
    c->config.mode = value;
    return reconfigure(c);
}
int32_t xy_hal_i2s_get_mode(void* i2s) {
    i2s_context_t* c = ready_context(i2s);
    return c ? (int32_t)c->config.mode : XY_HAL_ERROR_NOT_INIT;
}
xy_hal_error_t xy_hal_i2s_set_standard(void* i2s, xy_hal_i2s_standard_t value) {
    i2s_context_t* c = ready_context(i2s);
    if (!c)
        return XY_HAL_ERROR_NOT_INIT;
    if (value > XY_HAL_I2S_STANDARD_PCM_LONG)
        return XY_HAL_ERROR_INVALID_PARAM;
    c->config.standard = value;
    return reconfigure(c);
}
int32_t xy_hal_i2s_get_standard(void* i2s) {
    i2s_context_t* c = ready_context(i2s);
    return c ? (int32_t)c->config.standard : XY_HAL_ERROR_NOT_INIT;
}
xy_hal_error_t xy_hal_i2s_set_cpol(void* i2s, xy_hal_i2s_cpol_t value) {
    i2s_context_t* c = ready_context(i2s);
    if (!c)
        return XY_HAL_ERROR_NOT_INIT;
    if (value > XY_HAL_I2S_CPOL_HIGH)
        return XY_HAL_ERROR_INVALID_PARAM;
    c->config.cpol = value;
    return reconfigure(c);
}
int32_t xy_hal_i2s_get_cpol(void* i2s) {
    i2s_context_t* c = ready_context(i2s);
    return c ? (int32_t)c->config.cpol : XY_HAL_ERROR_NOT_INIT;
}
xy_hal_error_t xy_hal_i2s_enable_mck(void* i2s, uint8_t value) {
    i2s_context_t* c = ready_context(i2s);
    if (!c)
        return XY_HAL_ERROR_NOT_INIT;
    if (value > 1U)
        return XY_HAL_ERROR_INVALID_PARAM;
    c->config.mck_output = value ? XY_HAL_I2S_MCLK_ENABLE : XY_HAL_I2S_MCLK_DISABLE;
    return reconfigure(c);
}
int32_t xy_hal_i2s_is_mck_enabled(void* i2s) {
    i2s_context_t* c = ready_context(i2s);
    return c ? (int32_t)c->config.mck_output : XY_HAL_ERROR_NOT_INIT;
}
xy_hal_error_t xy_hal_i2s_register_callback(void* i2s, xy_hal_i2s_callback_t callback, void* arg) {
    i2s_context_t* c = ready_context(i2s);
    if (!c)
        return XY_HAL_ERROR_NOT_INIT;
    c->callback = callback;
    c->callback_arg = arg;
    return XY_HAL_OK;
}
int32_t xy_hal_i2s_get_state(void* i2s) {
    i2s_context_t* c = ready_context(i2s);
    return c ? (c->started ? 2 : 1) : XY_HAL_ERROR_NOT_INIT;
}
int32_t xy_hal_i2s_get_error(void* i2s) {
    i2s_context_t* c = ready_context(i2s);
    return c ? c->error : XY_HAL_ERROR_NOT_INIT;
}
xy_hal_error_t xy_hal_i2s_clear_error(void* i2s) {
    i2s_context_t* c = ready_context(i2s);
    if (!c)
        return XY_HAL_ERROR_NOT_INIT;
    c->error = 0;
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_i2s_enter_sleep(void* i2s) {
    i2s_context_t* c = ready_context(i2s);
    if (!c)
        return XY_HAL_ERROR_NOT_INIT;
    xy_hal_error_t s = xy_hal_i2s_stop(i2s);
    if (s == XY_HAL_OK)
        c->sleeping = 1U;
    return s;
}
xy_hal_error_t xy_hal_i2s_exit_sleep(void* i2s) {
    i2s_context_t* c = ready_context(i2s);
    if (!c)
        return XY_HAL_ERROR_NOT_INIT;
    c->sleeping = 0U;
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_i2s_set_low_power_mode(void* i2s, uint8_t value) {
    i2s_context_t* c = ready_context(i2s);
    if (!c)
        return XY_HAL_ERROR_NOT_INIT;
    if (value > 1U)
        return XY_HAL_ERROR_INVALID_PARAM;
    c->low_power = value;
    return XY_HAL_OK;
}
int32_t xy_hal_i2s_get_low_power_mode(void* i2s) {
    i2s_context_t* c = ready_context(i2s);
    return c ? c->low_power : XY_HAL_ERROR_NOT_INIT;
}
xy_hal_error_t xy_hal_i2s_control(void* i2s, uint32_t cmd, void* args) {
    (void)i2s;
    (void)cmd;
    (void)args;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
int32_t xy_hal_i2s_transmit_receive(void* i2s, const void* tx, void* rx, size_t size,
                                    uint32_t timeout) {
    (void)i2s;
    (void)tx;
    (void)rx;
    (void)size;
    (void)timeout;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
xy_hal_error_t xy_hal_i2s_transmit_receive_dma(void* i2s, const void* tx, void* rx, size_t size) {
    (void)i2s;
    (void)tx;
    (void)rx;
    (void)size;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
xy_hal_error_t xy_hal_i2s_config_clock(void* i2s, const void* config) {
    (void)i2s;
    (void)config;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
xy_hal_error_t xy_hal_i2s_enable_noise_suppression(void* i2s, uint8_t e) {
    (void)i2s;
    (void)e;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
xy_hal_error_t xy_hal_i2s_enable_echo_cancel(void* i2s, uint8_t e) {
    (void)i2s;
    (void)e;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
xy_hal_error_t xy_hal_i2s_enable_agc(void* i2s, uint8_t e) {
    (void)i2s;
    (void)e;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
xy_hal_error_t xy_hal_i2s_config_equalizer(void* i2s, const void* c) {
    (void)i2s;
    (void)c;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
xy_hal_error_t xy_hal_i2s_enable_audio_effect(void* i2s, uint8_t effect, uint8_t e) {
    (void)i2s;
    (void)effect;
    (void)e;
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
void xy_hal_i2s_event_handler(void* i2s, xy_hal_i2s_evt_t event, void* arg) {
    i2s_context_t* c = ready_context(i2s);
    (void)arg;
    if (c && c->callback)
        c->callback(i2s, event, c->callback_arg);
}
#endif
