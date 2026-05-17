/**
 * @file xy_hal_can.c
 * @brief CAN HAL — STM32U5 implementation backed by FDCAN.
 *
 * STM32U5 has no classic bxCAN; FDCAN is the only on-chip CAN controller.
 * We expose the xy_hal_can.h interface but drive FDCAN in Classic-CAN mode
 * so the rest of the framework behaves as before.
 *
 * Caller responsibilities:
 *  - Provide an `FDCAN_HandleTypeDef` (not the legacy CAN_HandleTypeDef)
 *    as the `void *can` argument.
 *  - Configure GPIO and FDCAN kernel clock externally (this layer does
 *    not touch RCC).  Default timing math assumes FDCAN kernel clock =
 *    16 MHz (HSI); override by writing hfdcan->Init.NominalPrescaler
 *    before xy_hal_can_init.
 */

#include "../../inc/xy_hal_can.h"

#if defined(STM32U5) || defined(STM32U5xx)

#include "stm32u5xx_hal.h"
#include <string.h>

typedef struct {
    FDCAN_HandleTypeDef   *hcan;
    xy_hal_can_callback_t  callback;
    void                  *arg;
    uint8_t                initialized;
} can_ctx_t;

#define MAX_CAN_INSTANCES 2
#define FDCAN_KERNEL_CLOCK_HZ 16000000U  /* HSI default; override via Init.NominalPrescaler */

static can_ctx_t g_can_ctx[MAX_CAN_INSTANCES];

static can_ctx_t *find_can_ctx(const void *can)
{
    for (size_t i = 0; i < MAX_CAN_INSTANCES; i++) {
        if (g_can_ctx[i].hcan == can) {
            return &g_can_ctx[i];
        }
    }
    return NULL;
}

static can_ctx_t *alloc_can_ctx(void)
{
    for (size_t i = 0; i < MAX_CAN_INSTANCES; i++) {
        if (g_can_ctx[i].hcan == NULL) {
            return &g_can_ctx[i];
        }
    }
    return NULL;
}

static uint32_t to_fdcan_mode(xy_hal_can_mode_t mode)
{
    switch (mode) {
    case XY_HAL_CAN_MODE_LOOPBACK:
    case XY_HAL_CAN_MODE_SILENT_LOOPBACK: return FDCAN_MODE_INTERNAL_LOOPBACK;
    case XY_HAL_CAN_MODE_SILENT:          return FDCAN_MODE_BUS_MONITORING;
    case XY_HAL_CAN_MODE_NORMAL:
    default:                              return FDCAN_MODE_NORMAL;
    }
}

/* DLC field on FDCAN is the byte count for the 0..8 range. */
static uint32_t to_fdcan_dlc(uint8_t dlc)
{
    return (uint32_t)(dlc & 0x0FU);
}

xy_hal_error_t xy_hal_can_init(void *can, const xy_hal_can_config_t *config)
{
    if (!can || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    FDCAN_HandleTypeDef *hcan = (FDCAN_HandleTypeDef *)can;

    can_ctx_t *ctx = find_can_ctx(can);
    if (ctx != NULL && ctx->initialized) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }
    if (ctx == NULL) {
        ctx = alloc_can_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
    }

    /* Compute NominalPrescaler from baud + bit-time quanta unless the
     * caller has already filled it in. */
    uint8_t bs1 = config->bs1 ? config->bs1 : 13U;
    uint8_t bs2 = config->bs2 ? config->bs2 : 2U;
    uint8_t sjw = config->sjw ? config->sjw : 1U;
    uint32_t bit_tq = 1U + (uint32_t)bs1 + (uint32_t)bs2;
    if (hcan->Init.NominalPrescaler == 0U) {
        uint32_t presc = FDCAN_KERNEL_CLOCK_HZ /
                         ((uint32_t)config->baudrate * bit_tq);
        if (presc == 0U) presc = 1U;
        hcan->Init.NominalPrescaler = presc;
    }

    hcan->Init.ClockDivider        = FDCAN_CLOCK_DIV1;
    hcan->Init.FrameFormat         = FDCAN_FRAME_CLASSIC;
    hcan->Init.Mode                = to_fdcan_mode(config->mode);
    hcan->Init.AutoRetransmission  =
        config->auto_retrans ? ENABLE : DISABLE;
    hcan->Init.TransmitPause       = DISABLE;
    hcan->Init.ProtocolException   = DISABLE;
    hcan->Init.NominalSyncJumpWidth = sjw;
    hcan->Init.NominalTimeSeg1     = bs1;
    hcan->Init.NominalTimeSeg2     = bs2;
    /* Data-phase parameters are unused in classic mode but must be valid. */
    hcan->Init.DataPrescaler       = 1;
    hcan->Init.DataSyncJumpWidth   = 1;
    hcan->Init.DataTimeSeg1        = 1;
    hcan->Init.DataTimeSeg2        = 1;
    hcan->Init.StdFiltersNbr       = 8;
    hcan->Init.ExtFiltersNbr       = 0;
    hcan->Init.TxFifoQueueMode     = FDCAN_TX_FIFO_OPERATION;

    if (HAL_FDCAN_Init(hcan) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    ctx->hcan        = hcan;
    ctx->callback    = NULL;
    ctx->arg         = NULL;
    ctx->initialized = 1;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_can_deinit(void *can)
{
    if (!can) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    can_ctx_t *ctx = find_can_ctx(can);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    if (HAL_FDCAN_DeInit((FDCAN_HandleTypeDef *)can) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
    memset(ctx, 0, sizeof(*ctx));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_can_start(void *can)
{
    can_ctx_t *ctx = find_can_ctx(can);
    if (!can || ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return (HAL_FDCAN_Start((FDCAN_HandleTypeDef *)can) == HAL_OK)
               ? XY_HAL_OK : XY_HAL_ERROR_FAIL;
}

xy_hal_error_t xy_hal_can_stop(void *can)
{
    can_ctx_t *ctx = find_can_ctx(can);
    if (!can || ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    return (HAL_FDCAN_Stop((FDCAN_HandleTypeDef *)can) == HAL_OK)
               ? XY_HAL_OK : XY_HAL_ERROR_FAIL;
}

xy_hal_error_t xy_hal_can_send(void *can, const xy_hal_can_msg_t *msg,
                               uint32_t timeout)
{
    if (!can || !msg) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    can_ctx_t *ctx = find_can_ctx(can);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    FDCAN_TxHeaderTypeDef hdr = {
        .Identifier          = msg->id,
        .IdType              = (msg->frame_type == XY_HAL_CAN_FRAME_EXT)
                                   ? FDCAN_EXTENDED_ID : FDCAN_STANDARD_ID,
        .TxFrameType         = (msg->data_type == XY_HAL_CAN_REMOTE_FRAME)
                                   ? FDCAN_REMOTE_FRAME : FDCAN_DATA_FRAME,
        .DataLength          = to_fdcan_dlc(msg->dlc),
        .ErrorStateIndicator = FDCAN_ESI_ACTIVE,
        .BitRateSwitch       = FDCAN_BRS_OFF,
        .FDFormat            = FDCAN_CLASSIC_CAN,
        .TxEventFifoControl  = FDCAN_NO_TX_EVENTS,
        .MessageMarker       = 0,
    };

    /* Wait until there is room in the Tx FIFO. */
    uint32_t start = HAL_GetTick();
    while (HAL_FDCAN_GetTxFifoFreeLevel((FDCAN_HandleTypeDef *)can) == 0U) {
        if ((HAL_GetTick() - start) >= timeout) {
            return XY_HAL_ERROR_TIMEOUT;
        }
    }

    if (HAL_FDCAN_AddMessageToTxFifoQ((FDCAN_HandleTypeDef *)can, &hdr,
                                      (uint8_t *)msg->data) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_can_receive(void *can, xy_hal_can_msg_t *msg,
                                  xy_hal_can_fifo_t fifo, uint32_t timeout)
{
    if (!can || !msg) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    can_ctx_t *ctx = find_can_ctx(can);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    /* The xy_hal_can_fifo_t parameter is accepted for API parity, but the
     * only RX FIFO wired up here is FIFO 0 — FDCAN_RX_FIFO1 would need
     * extra filter routing and Init.RxFifo1ElmtsNbr bookkeeping. */
    XY_UNUSED(fifo);

    uint32_t start = HAL_GetTick();
    while (HAL_FDCAN_GetRxFifoFillLevel((FDCAN_HandleTypeDef *)can,
                                        FDCAN_RX_FIFO0) == 0U) {
        if ((HAL_GetTick() - start) >= timeout) {
            return XY_HAL_ERROR_TIMEOUT;
        }
    }

    FDCAN_RxHeaderTypeDef hdr;
    if (HAL_FDCAN_GetRxMessage((FDCAN_HandleTypeDef *)can, FDCAN_RX_FIFO0,
                               &hdr, msg->data) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
    msg->id         = hdr.Identifier;
    msg->frame_type = (hdr.IdType == FDCAN_EXTENDED_ID)
                          ? XY_HAL_CAN_FRAME_EXT : XY_HAL_CAN_FRAME_STD;
    msg->data_type  = (hdr.RxFrameType == FDCAN_REMOTE_FRAME)
                          ? XY_HAL_CAN_REMOTE_FRAME : XY_HAL_CAN_DATA_FRAME;
    msg->dlc        = (uint8_t)(hdr.DataLength & 0x0FU);
    msg->fifo       = 0;
    msg->timestamp  = hdr.RxTimestamp;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_can_config_filter(void *can,
                                        const xy_hal_can_filter_config_t *fcfg)
{
    if (!can || !fcfg) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    can_ctx_t *ctx = find_can_ctx(can);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    FDCAN_FilterTypeDef f = {
        .IdType       = FDCAN_STANDARD_ID,
        .FilterIndex  = fcfg->bank_number,
        .FilterType   = (fcfg->filter_mode == XY_HAL_CAN_FILTERMODE_IDLIST)
                            ? FDCAN_FILTER_DUAL : FDCAN_FILTER_MASK,
        .FilterConfig = FDCAN_FILTER_TO_RXFIFO0,
        .FilterID1    = fcfg->filter_id,
        .FilterID2    = fcfg->filter_mask,
    };
    if (HAL_FDCAN_ConfigFilter((FDCAN_HandleTypeDef *)can, &f) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_can_register_callback(void *can,
                                            xy_hal_can_callback_t callback,
                                            void *arg)
{
    if (!can) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    can_ctx_t *ctx = find_can_ctx(can);
    if (ctx == NULL) {
        ctx = alloc_can_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
        ctx->hcan = (FDCAN_HandleTypeDef *)can;
    }
    ctx->callback = callback;
    ctx->arg      = arg;
    return XY_HAL_OK;
}

/* ==================== HAL FDCAN callbacks ==================== */

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hcan, uint32_t flags)
{
    XY_UNUSED(flags);
    can_ctx_t *ctx = find_can_ctx(hcan);
    if (ctx && ctx->callback) {
        ctx->callback(hcan, XY_HAL_CAN_EVENT_RX_PENDING, ctx->arg);
    }
}

void HAL_FDCAN_TxBufferCompleteCallback(FDCAN_HandleTypeDef *hcan,
                                        uint32_t indexes)
{
    XY_UNUSED(indexes);
    can_ctx_t *ctx = find_can_ctx(hcan);
    if (ctx && ctx->callback) {
        ctx->callback(hcan, XY_HAL_CAN_EVENT_TX_COMPLETE, ctx->arg);
    }
}

void HAL_FDCAN_ErrorCallback(FDCAN_HandleTypeDef *hcan)
{
    can_ctx_t *ctx = find_can_ctx(hcan);
    if (ctx && ctx->callback) {
        ctx->callback(hcan, XY_HAL_CAN_EVENT_ERROR, ctx->arg);
    }
}

#endif /* STM32U5 || STM32U5xx */
