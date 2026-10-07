/**
 * @file xy_hal_can.c
 * @brief WCH CH32V30x CAN HAL implementation
 */
#include "xy_hal_can.h"
#ifdef MCU_CH32
#include "ch32v30x.h"
#include <string.h>
typedef struct {
    xy_hal_can_config_t config;
    xy_hal_can_callback_t callback;
    void* arg;
    uint8_t initialized;
    uint8_t started;
} can_ctx_t;
static can_ctx_t contexts[2];
static int can_index(const void* can) {
    if (can == CAN1)
        return 0;
    if (can == CAN2)
        return 1;
    return -1;
}
static uint32_t tick_ms(void) {
    return SysTick->CNT / (SystemCoreClock / 1000U);
}
static uint8_t mode_hw(xy_hal_can_mode_t m) {
    static const uint8_t v[] = {CAN_Mode_Normal, CAN_Mode_LoopBack, CAN_Mode_Silent,
                                CAN_Mode_Silent_LoopBack};
    return v[m];
}
static xy_hal_error_t apply_config(void* can, const xy_hal_can_config_t* cfg) {
    CAN_InitTypeDef init = {0};
    uint32_t tq;
    uint32_t pclk = SystemCoreClock;
    RCC_ClocksTypeDef clocks;
    if (cfg->baudrate != XY_HAL_CAN_BAUD_125K && cfg->baudrate != XY_HAL_CAN_BAUD_250K &&
        cfg->baudrate != XY_HAL_CAN_BAUD_500K && cfg->baudrate != XY_HAL_CAN_BAUD_1M)
        return XY_HAL_ERROR_NOT_SUPPORTED;
    if (cfg->mode > XY_HAL_CAN_MODE_SILENT_LOOPBACK || cfg->sjw < 1U || cfg->sjw > 4U ||
        cfg->bs1 < 1U || cfg->bs1 > 16U || cfg->bs2 < 1U || cfg->bs2 > 8U ||
        cfg->auto_bus_off > 1U || cfg->auto_wake_up > 1U || cfg->auto_retrans > 1U ||
        cfg->rx_fifo_locked > 1U || cfg->tx_fifo_priority > 1U)
        return XY_HAL_ERROR_INVALID_PARAM;
    tq = 1U + cfg->bs1 + cfg->bs2;
    RCC_GetClocksFreq(&clocks);
    pclk = clocks.PCLK1_Frequency;
    if ((pclk % ((uint32_t)cfg->baudrate * tq)) != 0U)
        return XY_HAL_ERROR_INVALID_PARAM;
    init.CAN_Prescaler = (uint16_t)(pclk / ((uint32_t)cfg->baudrate * tq));
    init.CAN_Mode = mode_hw(cfg->mode);
    init.CAN_SJW = (uint8_t)(cfg->sjw - 1U);
    init.CAN_BS1 = (uint8_t)(cfg->bs1 - 1U);
    init.CAN_BS2 = (uint8_t)(cfg->bs2 - 1U);
    init.CAN_TTCM = DISABLE;
    init.CAN_ABOM = cfg->auto_bus_off ? ENABLE : DISABLE;
    init.CAN_AWUM = cfg->auto_wake_up ? ENABLE : DISABLE;
    init.CAN_NART = cfg->auto_retrans ? DISABLE : ENABLE;
    init.CAN_RFLM = cfg->rx_fifo_locked ? ENABLE : DISABLE;
    init.CAN_TXFP = cfg->tx_fifo_priority ? ENABLE : DISABLE;
    return CAN_Init(can, &init) == CAN_InitStatus_Success ? XY_HAL_OK : XY_HAL_ERROR_FAIL;
}
xy_hal_error_t xy_hal_can_init(void* can, const xy_hal_can_config_t* cfg) {
    int i = can_index(can);
    xy_hal_error_t e;
    if (i < 0 || !cfg)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (contexts[i].initialized)
        return XY_HAL_ERROR_ALREADY_INIT;
    RCC_APB1PeriphClockCmd(i == 0 ? RCC_APB1Periph_CAN1 : RCC_APB1Periph_CAN2, ENABLE);
    CAN_DeInit(can);
    e = apply_config(can, cfg);
    if (e != XY_HAL_OK)
        return e;
    memset(&contexts[i], 0, sizeof(contexts[i]));
    contexts[i].config = *cfg;
    contexts[i].initialized = 1U;
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_can_deinit(void* can) {
    int i = can_index(can);
    if (i < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[i].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    CAN_DeInit(can);
    memset(&contexts[i], 0, sizeof(contexts[i]));
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_can_start(void* can) {
    int i = can_index(can);
    if (i < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[i].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    if (CAN_OperatingModeRequest(can, CAN_OperatingMode_Normal) != CAN_ModeStatus_Success)
        return XY_HAL_ERROR_FAIL;
    contexts[i].started = 1U;
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_can_stop(void* can) {
    int i = can_index(can);
    if (i < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[i].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    if (CAN_OperatingModeRequest(can, CAN_OperatingMode_Initialization) != CAN_ModeStatus_Success)
        return XY_HAL_ERROR_FAIL;
    contexts[i].started = 0U;
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_can_send(void* can, const xy_hal_can_msg_t* msg, uint32_t timeout) {
    CanTxMsg tx = {0};
    uint8_t mb, status;
    uint32_t start;
    int i = can_index(can);
    if (i < 0 || !msg || msg->dlc > 8U || msg->frame_type > XY_HAL_CAN_FRAME_EXT ||
        msg->data_type > XY_HAL_CAN_REMOTE_FRAME ||
        (msg->frame_type == XY_HAL_CAN_FRAME_STD && msg->id > XY_HAL_CAN_STD_ID_MAX) ||
        (msg->frame_type == XY_HAL_CAN_FRAME_EXT && msg->id > XY_HAL_CAN_EXT_ID_MAX))
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[i].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    if (!contexts[i].started)
        return XY_HAL_ERROR_BUSY;
    tx.IDE = msg->frame_type == XY_HAL_CAN_FRAME_STD ? CAN_ID_STD : CAN_ID_EXT;
    tx.RTR = msg->data_type == XY_HAL_CAN_DATA_FRAME ? CAN_RTR_DATA : CAN_RTR_REMOTE;
    tx.StdId = msg->id;
    tx.ExtId = msg->id;
    tx.DLC = msg->dlc;
    memcpy(tx.Data, msg->data, msg->dlc);
    mb = CAN_Transmit(can, &tx);
    if (mb == CAN_TxStatus_NoMailBox)
        return XY_HAL_ERROR_BUSY;
    start = tick_ms();
    do {
        status = CAN_TransmitStatus(can, mb);
        if (status == CAN_TxStatus_Ok)
            return XY_HAL_OK;
        if (status == CAN_TxStatus_Failed)
            return XY_HAL_ERROR_IO;
        if (timeout && tick_ms() - start >= timeout) {
            CAN_CancelTransmit(can, mb);
            return XY_HAL_ERROR_TIMEOUT;
        }
    } while (1);
}
xy_hal_error_t xy_hal_can_receive(void* can, xy_hal_can_msg_t* msg, xy_hal_can_fifo_t fifo,
                                  uint32_t timeout) {
    CanRxMsg rx = {0};
    uint32_t start;
    int i = can_index(can);
    uint8_t f = (uint8_t)fifo;
    if (i < 0 || !msg || fifo > XY_HAL_CAN_FIFO_1)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[i].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    start = tick_ms();
    while (CAN_MessagePending(can, f) == 0U)
        if (timeout && tick_ms() - start >= timeout)
            return XY_HAL_ERROR_TIMEOUT;
    CAN_Receive(can, f, &rx);
    msg->id = rx.IDE == CAN_ID_STD ? rx.StdId : rx.ExtId;
    msg->frame_type = rx.IDE == CAN_ID_STD ? XY_HAL_CAN_FRAME_STD : XY_HAL_CAN_FRAME_EXT;
    msg->data_type = rx.RTR == CAN_RTR_DATA ? XY_HAL_CAN_DATA_FRAME : XY_HAL_CAN_REMOTE_FRAME;
    msg->dlc = rx.DLC;
    msg->fifo = f;
    msg->timestamp = tick_ms();
    memcpy(msg->data, rx.Data, rx.DLC);
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_can_config_filter(void* can, const xy_hal_can_filter_config_t* cfg) {
    CAN_FilterInitTypeDef f = {0};
    int i = can_index(can);
    if (i < 0 || !cfg || cfg->bank_number > 27U || cfg->fifo_assignment > 1U ||
        cfg->filter_mode > XY_HAL_CAN_FILTERMODE_IDLIST ||
        cfg->filter_scale > XY_HAL_CAN_FILTERSCALE_32BIT || cfg->activation > 1U)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[i].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    f.CAN_FilterNumber = (uint8_t)cfg->bank_number;
    f.CAN_FilterMode = cfg->filter_mode == XY_HAL_CAN_FILTERMODE_IDMASK ? CAN_FilterMode_IdMask
                                                                        : CAN_FilterMode_IdList;
    f.CAN_FilterScale = cfg->filter_scale == XY_HAL_CAN_FILTERSCALE_32BIT ? CAN_FilterScale_32bit
                                                                          : CAN_FilterScale_16bit;
    f.CAN_FilterIdHigh = (uint16_t)(cfg->filter_id >> 16);
    f.CAN_FilterIdLow = (uint16_t)cfg->filter_id;
    f.CAN_FilterMaskIdHigh = (uint16_t)(cfg->filter_mask >> 16);
    f.CAN_FilterMaskIdLow = (uint16_t)cfg->filter_mask;
    f.CAN_FilterFIFOAssignment = cfg->fifo_assignment;
    f.CAN_FilterActivation = cfg->activation ? ENABLE : DISABLE;
    CAN_FilterInit(&f);
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_can_config_filters(void* can, const xy_hal_can_filter_config_t* f, size_t n) {
    size_t k;
    if (!f || !n)
        return XY_HAL_ERROR_INVALID_PARAM;
    for (k = 0; k < n; k++) {
        xy_hal_error_t e = xy_hal_can_config_filter(can, &f[k]);
        if (e != XY_HAL_OK)
            return e;
    }
    return XY_HAL_OK;
}
static uint32_t irq_value(xy_hal_can_it_t it) {
    static const uint32_t v[] = {CAN_IT_TME, CAN_IT_FMP0, CAN_IT_FOV0, CAN_IT_WKU, CAN_IT_EWG,
                                 CAN_IT_EPV, CAN_IT_BOF,  CAN_IT_LEC,  CAN_IT_ERR};
    return it <= XY_HAL_CAN_IT_ERROR ? v[it] : 0U;
}
xy_hal_error_t xy_hal_can_enable_irq(void* can, xy_hal_can_it_t it) {
    uint32_t v = irq_value(it);
    if (can_index(can) < 0 || !v)
        return XY_HAL_ERROR_INVALID_PARAM;
    CAN_ITConfig(can, v, ENABLE);
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_can_disable_irq(void* can, xy_hal_can_it_t it) {
    uint32_t v = irq_value(it);
    if (can_index(can) < 0 || !v)
        return XY_HAL_ERROR_INVALID_PARAM;
    CAN_ITConfig(can, v, DISABLE);
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_can_get_state(void* can, xy_hal_can_state_t* s) {
    if (can_index(can) < 0 || !s)
        return XY_HAL_ERROR_INVALID_PARAM;
    memset(s, 0, sizeof(*s));
    s->error_code = (((CAN_TypeDef*)can)->ERRSR & CAN_ERRSR_LEC) >> 4;
    s->error_state =
        (((CAN_TypeDef*)can)->ERRSR & (CAN_ERRSR_EWGF | CAN_ERRSR_EPVF | CAN_ERRSR_BOFF)) ? 1U : 0U;
    s->rx0e = (((CAN_TypeDef*)can)->RFIFO0 & 0x10U) ? 1U : 0U;
    s->rx1e = (((CAN_TypeDef*)can)->RFIFO1 & 0x10U) ? 1U : 0U;
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_can_get_error_count(void* can, uint8_t* tx, uint8_t* rx) {
    if (can_index(can) < 0 || !tx || !rx)
        return XY_HAL_ERROR_INVALID_PARAM;
    *tx = (uint8_t)(((CAN_TypeDef*)can)->ERRSR >> 16);
    *rx = (uint8_t)(((CAN_TypeDef*)can)->ERRSR >> 24);
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_can_reset_error_count(void* can) {
    XY_UNUSED(can);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
int32_t xy_hal_can_is_ready(void* can) {
    int i = can_index(can);
    return i < 0 ? XY_HAL_ERROR_INVALID_PARAM : contexts[i].initialized && contexts[i].started;
}
int32_t xy_hal_can_is_error(void* can) {
    return can_index(can) < 0 ? XY_HAL_ERROR_INVALID_PARAM
                              : ((((CAN_TypeDef*)can)->ERRSR & 0x77U) != 0U);
}
int32_t xy_hal_can_is_bus_off(void* can) {
    return can_index(can) < 0 ? XY_HAL_ERROR_INVALID_PARAM
                              : ((((CAN_TypeDef*)can)->ERRSR & CAN_ERRSR_BOFF) != 0U);
}
int32_t xy_hal_can_is_error_passive(void* can) {
    return can_index(can) < 0 ? XY_HAL_ERROR_INVALID_PARAM
                              : ((((CAN_TypeDef*)can)->ERRSR & CAN_ERRSR_EPVF) != 0U);
}
int32_t xy_hal_can_is_error_warning(void* can) {
    return can_index(can) < 0 ? XY_HAL_ERROR_INVALID_PARAM
                              : ((((CAN_TypeDef*)can)->ERRSR & CAN_ERRSR_EWGF) != 0U);
}
xy_hal_error_t xy_hal_can_set_mode(void* can, xy_hal_can_mode_t m) {
    int i = can_index(can);
    xy_hal_can_config_t candidate;
    xy_hal_error_t error;
    if (i < 0 || m > XY_HAL_CAN_MODE_SILENT_LOOPBACK)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[i].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    candidate = contexts[i].config;
    candidate.mode = m;
    error = apply_config(can, &candidate);
    if (error == XY_HAL_OK)
        contexts[i].config = candidate;
    return error;
}
int32_t xy_hal_can_get_mode(void* can) {
    int i = can_index(can);
    return i < 0                     ? XY_HAL_ERROR_INVALID_PARAM
           : contexts[i].initialized ? (int32_t)contexts[i].config.mode
                                     : XY_HAL_ERROR_NOT_INIT;
}
xy_hal_error_t xy_hal_can_set_baudrate(void* can, xy_hal_can_baud_t b) {
    int i = can_index(can);
    xy_hal_can_config_t candidate;
    xy_hal_error_t error;
    if (i < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[i].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    candidate = contexts[i].config;
    candidate.baudrate = b;
    error = apply_config(can, &candidate);
    if (error == XY_HAL_OK)
        contexts[i].config = candidate;
    return error;
}
int32_t xy_hal_can_get_baudrate(void* can) {
    int i = can_index(can);
    return i < 0                     ? XY_HAL_ERROR_INVALID_PARAM
           : contexts[i].initialized ? (int32_t)contexts[i].config.baudrate
                                     : XY_HAL_ERROR_NOT_INIT;
}
xy_hal_error_t xy_hal_can_register_callback(void* can, xy_hal_can_callback_t cb, void* arg) {
    int i = can_index(can);
    if (i < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[i].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    contexts[i].callback = cb;
    contexts[i].arg = arg;
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_can_control(void* can, uint32_t cmd, void* args) {
    XY_UNUSED(can);
    XY_UNUSED(cmd);
    XY_UNUSED(args);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
int32_t xy_hal_can_get_fifo_count(void* can, xy_hal_can_fifo_t f) {
    return can_index(can) < 0 || f > 1 ? XY_HAL_ERROR_INVALID_PARAM
                                       : CAN_MessagePending(can, (uint8_t)f);
}
xy_hal_error_t xy_hal_can_release_fifo(void* can, xy_hal_can_fifo_t f) {
    if (can_index(can) < 0 || f > 1)
        return XY_HAL_ERROR_INVALID_PARAM;
    CAN_FIFORelease(can, (uint8_t)f);
    return XY_HAL_OK;
}
int32_t xy_hal_can_get_tx_mailbox_state(void* can, uint8_t m) {
    if (can_index(can) < 0 || m > 2)
        return XY_HAL_ERROR_INVALID_PARAM;
    return CAN_TransmitStatus(can, m) != CAN_TxStatus_Pending;
}
xy_hal_error_t xy_hal_can_abort_tx_request(void* can, uint8_t m) {
    if (can_index(can) < 0 || m > 2)
        return XY_HAL_ERROR_INVALID_PARAM;
    CAN_CancelTransmit(can, m);
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_can_send_remote_frame(void* can, uint32_t id, xy_hal_can_frame_type_t t,
                                            uint8_t fifo, uint32_t timeout) {
    xy_hal_can_msg_t m = {0};
    XY_UNUSED(fifo);
    m.id = id;
    m.frame_type = t;
    m.data_type = XY_HAL_CAN_REMOTE_FRAME;
    return xy_hal_can_send(can, &m, timeout);
}
uint32_t xy_hal_can_get_timestamp(void* can) {
    XY_UNUSED(can);
    return tick_ms();
}
xy_hal_error_t xy_hal_can_enable_timestamp(void* can) {
    XY_UNUSED(can);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
xy_hal_error_t xy_hal_can_disable_timestamp(void* can) {
    XY_UNUSED(can);
    return XY_HAL_ERROR_NOT_SUPPORTED;
}
int32_t xy_hal_can_get_error_code(void* can) {
    return can_index(can) < 0 ? XY_HAL_ERROR_INVALID_PARAM
                              : (int32_t)((((CAN_TypeDef*)can)->ERRSR & CAN_ERRSR_LEC) >> 4);
}
xy_hal_error_t xy_hal_can_clear_error_flags(void* can) {
    if (can_index(can) < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    CAN_ClearFlag(can, CAN_FLAG_LEC | CAN_FLAG_EWG | CAN_FLAG_EPV | CAN_FLAG_BOF);
    return XY_HAL_OK;
}
xy_hal_error_t xy_hal_can_enable_autoretrans(void* can, uint8_t e) {
    int i = can_index(can);
    xy_hal_can_config_t candidate;
    xy_hal_error_t error;
    if (i < 0 || e > 1)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[i].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    candidate = contexts[i].config;
    candidate.auto_retrans = e;
    error = apply_config(can, &candidate);
    if (error == XY_HAL_OK)
        contexts[i].config = candidate;
    return error;
}
xy_hal_error_t xy_hal_can_enable_autowakeup(void* can, uint8_t e) {
    int i = can_index(can);
    xy_hal_can_config_t candidate;
    xy_hal_error_t error;
    if (i < 0 || e > 1)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[i].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    candidate = contexts[i].config;
    candidate.auto_wake_up = e;
    error = apply_config(can, &candidate);
    if (error == XY_HAL_OK)
        contexts[i].config = candidate;
    return error;
}
xy_hal_error_t xy_hal_can_enable_autobusoff(void* can, uint8_t e) {
    int i = can_index(can);
    xy_hal_can_config_t candidate;
    xy_hal_error_t error;
    if (i < 0 || e > 1)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[i].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    candidate = contexts[i].config;
    candidate.auto_bus_off = e;
    error = apply_config(can, &candidate);
    if (error == XY_HAL_OK)
        contexts[i].config = candidate;
    return error;
}
void xy_hal_can_event_handler(void* can, xy_hal_can_evt_t event, void* arg) {
    int i = can_index(can);
    XY_UNUSED(arg);
    if (i >= 0 && contexts[i].callback)
        contexts[i].callback(can, event, contexts[i].arg);
}
#else
#error "WCH CAN backend requires MCU_CH32"
#endif
