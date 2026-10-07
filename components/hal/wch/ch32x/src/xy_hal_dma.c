/**
 * @file xy_hal_dma.c
 * @brief WCH CH32V30x DMA HAL implementation
 */

#include "xy_hal_dma.h"

#ifdef MCU_CH32

#include "ch32v30x.h"
#include <string.h>

#define WCH_DMA_CONTEXTS 18U

typedef struct {
    DMA_Channel_TypeDef* channel;
    xy_hal_dma_config_t config;
    xy_hal_dma_callback_t callbacks[3];
    void* args[3];
    uint32_t complete_flag;
    uint32_t half_flag;
    uint32_t error_flag;
    uint8_t initialized;
} wch_dma_context_t;

static wch_dma_context_t contexts[WCH_DMA_CONTEXTS];

static const DMA_Channel_TypeDef* const channels[WCH_DMA_CONTEXTS] = {
    DMA1_Channel1, DMA1_Channel2, DMA1_Channel3, DMA1_Channel4, DMA1_Channel5,  DMA1_Channel6,
    DMA1_Channel7, DMA2_Channel1, DMA2_Channel2, DMA2_Channel3, DMA2_Channel4,  DMA2_Channel5,
    DMA2_Channel6, DMA2_Channel7, DMA2_Channel8, DMA2_Channel9, DMA2_Channel10, DMA2_Channel11,
};

static const uint32_t complete_flags[WCH_DMA_CONTEXTS] = {
    DMA1_FLAG_TC1, DMA1_FLAG_TC2, DMA1_FLAG_TC3, DMA1_FLAG_TC4, DMA1_FLAG_TC5,  DMA1_FLAG_TC6,
    DMA1_FLAG_TC7, DMA2_FLAG_TC1, DMA2_FLAG_TC2, DMA2_FLAG_TC3, DMA2_FLAG_TC4,  DMA2_FLAG_TC5,
    DMA2_FLAG_TC6, DMA2_FLAG_TC7, DMA2_FLAG_TC8, DMA2_FLAG_TC9, DMA2_FLAG_TC10, DMA2_FLAG_TC11,
};
static const uint32_t half_flags[WCH_DMA_CONTEXTS] = {
    DMA1_FLAG_HT1, DMA1_FLAG_HT2, DMA1_FLAG_HT3, DMA1_FLAG_HT4, DMA1_FLAG_HT5,  DMA1_FLAG_HT6,
    DMA1_FLAG_HT7, DMA2_FLAG_HT1, DMA2_FLAG_HT2, DMA2_FLAG_HT3, DMA2_FLAG_HT4,  DMA2_FLAG_HT5,
    DMA2_FLAG_HT6, DMA2_FLAG_HT7, DMA2_FLAG_HT8, DMA2_FLAG_HT9, DMA2_FLAG_HT10, DMA2_FLAG_HT11,
};
static const uint32_t error_flags[WCH_DMA_CONTEXTS] = {
    DMA1_FLAG_TE1, DMA1_FLAG_TE2, DMA1_FLAG_TE3, DMA1_FLAG_TE4, DMA1_FLAG_TE5,  DMA1_FLAG_TE6,
    DMA1_FLAG_TE7, DMA2_FLAG_TE1, DMA2_FLAG_TE2, DMA2_FLAG_TE3, DMA2_FLAG_TE4,  DMA2_FLAG_TE5,
    DMA2_FLAG_TE6, DMA2_FLAG_TE7, DMA2_FLAG_TE8, DMA2_FLAG_TE9, DMA2_FLAG_TE10, DMA2_FLAG_TE11,
};

static uint32_t dma_tick_ms(void) {
    return SysTick->CNT / (SystemCoreClock / 1000U);
}

static int channel_index(const void* dma) {
    size_t index;
    for (index = 0U; index < WCH_DMA_CONTEXTS; ++index) {
        if (channels[index] == dma)
            return (int)index;
    }
    return -1;
}

static uint32_t dma_width(xy_hal_dma_width_t width, int memory) {
    if (memory) {
        if (width == XY_HAL_DMA_WIDTH_HALFWORD)
            return DMA_MemoryDataSize_HalfWord;
        if (width == XY_HAL_DMA_WIDTH_WORD)
            return DMA_MemoryDataSize_Word;
        return DMA_MemoryDataSize_Byte;
    }
    if (width == XY_HAL_DMA_WIDTH_HALFWORD)
        return DMA_PeripheralDataSize_HalfWord;
    if (width == XY_HAL_DMA_WIDTH_WORD)
        return DMA_PeripheralDataSize_Word;
    return DMA_PeripheralDataSize_Byte;
}

static uint32_t dma_priority(xy_hal_dma_priority_t priority) {
    if (priority == XY_HAL_DMA_PRIORITY_MEDIUM)
        return DMA_Priority_Medium;
    if (priority == XY_HAL_DMA_PRIORITY_HIGH)
        return DMA_Priority_High;
    if (priority == XY_HAL_DMA_PRIORITY_VERY_HIGH)
        return DMA_Priority_VeryHigh;
    return DMA_Priority_Low;
}

xy_hal_error_t xy_hal_dma_init(void* dma, const xy_hal_dma_config_t* config) {
    int index = channel_index(dma);
    if (index < 0 || config == NULL || config->direction > XY_HAL_DMA_DIR_MEM_TO_MEM ||
        config->mode > XY_HAL_DMA_MODE_CIRCULAR ||
        config->priority > XY_HAL_DMA_PRIORITY_VERY_HIGH ||
        config->periph_width > XY_HAL_DMA_WIDTH_WORD || config->mem_width > XY_HAL_DMA_WIDTH_WORD ||
        config->periph_incr > XY_HAL_DMA_INCR_ENABLE || config->mem_incr > XY_HAL_DMA_INCR_ENABLE ||
        (config->direction == XY_HAL_DMA_DIR_MEM_TO_MEM &&
         config->mode == XY_HAL_DMA_MODE_CIRCULAR)) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (contexts[index].initialized)
        return XY_HAL_ERROR_ALREADY_INIT;
    RCC_AHBPeriphClockCmd(index < 7 ? RCC_AHBPeriph_DMA1 : RCC_AHBPeriph_DMA2, ENABLE);
    DMA_DeInit((DMA_Channel_TypeDef*)dma);
    memset(&contexts[index], 0, sizeof(contexts[index]));
    contexts[index].channel = dma;
    contexts[index].config = *config;
    contexts[index].complete_flag = complete_flags[index];
    contexts[index].half_flag = half_flags[index];
    contexts[index].error_flag = error_flags[index];
    contexts[index].initialized = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dma_deinit(void* dma) {
    int index = channel_index(dma);
    if (index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[index].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    DMA_Cmd(dma, DISABLE);
    DMA_DeInit(dma);
    memset(&contexts[index], 0, sizeof(contexts[index]));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dma_start(void* dma, uint32_t src_addr, uint32_t dst_addr, size_t data_len) {
    DMA_InitTypeDef init = {0};
    int index = channel_index(dma);
    const xy_hal_dma_config_t* config;
    if (index < 0 || src_addr == 0U || dst_addr == 0U || data_len == 0U || data_len > 0xFFFFU) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    if (!contexts[index].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    config = &contexts[index].config;
    DMA_Cmd(dma, DISABLE);
    DMA_DeInit(dma);
    if (config->direction == XY_HAL_DMA_DIR_PERIPH_TO_MEM) {
        init.DMA_PeripheralBaseAddr = src_addr;
        init.DMA_MemoryBaseAddr = dst_addr;
        init.DMA_DIR = DMA_DIR_PeripheralSRC;
    } else {
        init.DMA_PeripheralBaseAddr = dst_addr;
        init.DMA_MemoryBaseAddr = src_addr;
        init.DMA_DIR = DMA_DIR_PeripheralDST;
    }
    init.DMA_BufferSize = (uint32_t)data_len;
    init.DMA_PeripheralInc =
        config->periph_incr ? DMA_PeripheralInc_Enable : DMA_PeripheralInc_Disable;
    init.DMA_MemoryInc = config->mem_incr ? DMA_MemoryInc_Enable : DMA_MemoryInc_Disable;
    init.DMA_PeripheralDataSize = dma_width(config->periph_width, 0);
    init.DMA_MemoryDataSize = dma_width(config->mem_width, 1);
    init.DMA_Mode = config->mode == XY_HAL_DMA_MODE_CIRCULAR ? DMA_Mode_Circular : DMA_Mode_Normal;
    init.DMA_Priority = dma_priority(config->priority);
    init.DMA_M2M =
        config->direction == XY_HAL_DMA_DIR_MEM_TO_MEM ? DMA_M2M_Enable : DMA_M2M_Disable;
    DMA_Init(dma, &init);
    DMA_ClearFlag(contexts[index].complete_flag | contexts[index].half_flag |
                  contexts[index].error_flag);
    if (contexts[index].callbacks[XY_HAL_DMA_EVENT_COMPLETE])
        DMA_ITConfig(dma, DMA_IT_TC, ENABLE);
    if (contexts[index].callbacks[XY_HAL_DMA_EVENT_HALF_COMPLETE])
        DMA_ITConfig(dma, DMA_IT_HT, ENABLE);
    if (contexts[index].callbacks[XY_HAL_DMA_EVENT_ERROR])
        DMA_ITConfig(dma, DMA_IT_TE, ENABLE);
    DMA_Cmd(dma, ENABLE);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dma_stop(void* dma) {
    int index = channel_index(dma);
    if (index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[index].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    DMA_Cmd(dma, DISABLE);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dma_register_callback(void* dma, xy_hal_dma_event_t event,
                                            xy_hal_dma_callback_t callback, void* arg) {
    int index = channel_index(dma);
    if (index < 0 || event > XY_HAL_DMA_EVENT_ERROR)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[index].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    contexts[index].callbacks[event] = callback;
    contexts[index].args[event] = arg;
    return XY_HAL_OK;
}

int xy_hal_dma_get_counter(void* dma) {
    int index = channel_index(dma);
    if (index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[index].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    return DMA_GetCurrDataCounter(dma);
}

xy_hal_error_t xy_hal_dma_poll_complete(void* dma, uint32_t timeout) {
    int index = channel_index(dma);
    uint32_t start;
    if (index < 0)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!contexts[index].initialized)
        return XY_HAL_ERROR_NOT_INIT;
    start = dma_tick_ms();
    while (DMA_GetFlagStatus(contexts[index].complete_flag) == RESET) {
        if (DMA_GetFlagStatus(contexts[index].error_flag) == SET) {
            DMA_ClearFlag(contexts[index].error_flag);
            return XY_HAL_ERROR_IO;
        }
        if (timeout != 0U && (dma_tick_ms() - start) >= timeout)
            return XY_HAL_ERROR_TIMEOUT;
    }
    DMA_ClearFlag(contexts[index].complete_flag);
    return XY_HAL_OK;
}

void xy_hal_dma_irq_handler(void* dma) {
    int index = channel_index(dma);
    size_t event;
    uint32_t flags[3];
    if (index < 0 || !contexts[index].initialized)
        return;
    flags[0] = contexts[index].complete_flag;
    flags[1] = contexts[index].half_flag;
    flags[2] = contexts[index].error_flag;
    for (event = 0U; event < 3U; ++event) {
        if (DMA_GetFlagStatus(flags[event]) == SET) {
            DMA_ClearFlag(flags[event]);
            if (contexts[index].callbacks[event] != NULL) {
                contexts[index].callbacks[event](dma, (xy_hal_dma_event_t)event,
                                                 contexts[index].args[event]);
            }
        }
    }
}

#else
#error "WCH DMA backend requires MCU_CH32"
#endif
