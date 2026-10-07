/**
 * @file xy_hal_rng.c
 * @brief WCH CH32V30x hardware RNG HAL implementation
 */

#include "xy_hal_rng.h"

#ifdef MCU_CH32

#include "ch32v30x.h"
#include "ch32v30x_rng.h"

static xy_hal_rng_callback_t s_callbacks[2];
static void* s_callback_args[2];
static uint8_t s_initialized;
static uint8_t s_enabled;

static int valid_rng(void* rng) {
    return rng == RNG;
}

static int has_error(void) {
    return RNG_GetFlagStatus(RNG_FLAG_CECS) == SET || RNG_GetFlagStatus(RNG_FLAG_SECS) == SET;
}

xy_hal_error_t xy_hal_rng_init(void* rng, const xy_hal_rng_config_t* config) {
    if (!valid_rng(rng) || config == NULL || config->clock_enable > 1U ||
        config->interrupt_enable > 1U)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (!config->clock_enable)
        return XY_HAL_ERROR_NOT_SUPPORTED;
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_RNG, ENABLE);
    RNG_ITConfig(config->interrupt_enable ? ENABLE : DISABLE);
    RNG_Cmd(ENABLE);
    s_initialized = 1U;
    s_enabled = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rng_deinit(void* rng) {
    if (!valid_rng(rng))
        return XY_HAL_ERROR_INVALID_PARAM;
    RNG_ITConfig(DISABLE);
    RNG_Cmd(DISABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_RNG, DISABLE);
    s_initialized = 0U;
    s_enabled = 0U;
    s_callbacks[0] = NULL;
    s_callbacks[1] = NULL;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rng_enable(void* rng) {
    if (!valid_rng(rng) || !s_initialized)
        return XY_HAL_ERROR_INVALID_PARAM;
    RNG_Cmd(ENABLE);
    s_enabled = 1U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rng_disable(void* rng) {
    if (!valid_rng(rng) || !s_initialized)
        return XY_HAL_ERROR_INVALID_PARAM;
    RNG_Cmd(DISABLE);
    s_enabled = 0U;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rng_get_random_nb(void* rng, uint32_t* value) {
    if (!valid_rng(rng) || value == NULL || !s_initialized || !s_enabled)
        return XY_HAL_ERROR_INVALID_PARAM;
    if (has_error())
        return XY_HAL_ERROR_FAIL;
    if (RNG_GetFlagStatus(RNG_FLAG_DRDY) == RESET)
        return XY_HAL_ERROR_BUSY;
    *value = RNG_GetRandomNumber();
    return XY_HAL_OK;
}

int32_t xy_hal_rng_get_random(void* rng, uint32_t timeout) {
    uint32_t value;
    if (!valid_rng(rng) || !s_initialized || !s_enabled)
        return (int32_t)XY_HAL_ERROR_INVALID_PARAM;
    for (uint32_t elapsed = 0U; elapsed <= timeout * 1000U; ++elapsed) {
        xy_hal_error_t status = xy_hal_rng_get_random_nb(rng, &value);
        if (status == XY_HAL_OK)
            return (int32_t)(value & 0x7FFFFFFFU);
        if (status != XY_HAL_ERROR_BUSY)
            return (int32_t)status;
    }
    return (int32_t)XY_HAL_ERROR_TIMEOUT;
}

xy_hal_error_t xy_hal_rng_get_buffer(void* rng, uint32_t* buffer, size_t count, uint32_t timeout) {
    if (!valid_rng(rng) || (buffer == NULL && count != 0U))
        return XY_HAL_ERROR_INVALID_PARAM;
    for (size_t i = 0U; i < count; ++i) {
        uint32_t elapsed;
        xy_hal_error_t status = XY_HAL_ERROR_BUSY;
        for (elapsed = 0U; elapsed <= timeout * 1000U && status == XY_HAL_ERROR_BUSY; ++elapsed)
            status = xy_hal_rng_get_random_nb(rng, &buffer[i]);
        if (status == XY_HAL_ERROR_BUSY)
            return XY_HAL_ERROR_TIMEOUT;
        if (status != XY_HAL_OK)
            return status;
    }
    return XY_HAL_OK;
}

int32_t xy_hal_rng_get_random_range(void* rng, int32_t min, int32_t max) {
    uint32_t value;
    uint32_t span;
    if (min >= max || xy_hal_rng_get_random_nb(rng, &value) != XY_HAL_OK)
        return (int32_t)XY_HAL_ERROR_INVALID_PARAM;
    span = (uint32_t)((int64_t)max - (int64_t)min);
    return min + (int32_t)(value % span);
}

xy_hal_error_t xy_hal_rng_register_callback(void* rng, xy_hal_rng_event_t event,
                                            xy_hal_rng_callback_t callback, void* arg) {
    if (!valid_rng(rng) || event > XY_HAL_RNG_EVENT_ERROR || callback == NULL)
        return XY_HAL_ERROR_INVALID_PARAM;
    s_callbacks[event] = callback;
    s_callback_args[event] = arg;
    return XY_HAL_OK;
}

int xy_hal_rng_is_ready(void* rng) {
    if (!valid_rng(rng) || !s_initialized || !s_enabled)
        return (int)XY_HAL_ERROR_INVALID_PARAM;
    if (has_error())
        return (int)XY_HAL_ERROR_FAIL;
    return RNG_GetFlagStatus(RNG_FLAG_DRDY) == SET;
}

xy_hal_error_t xy_hal_rng_clear_error(void* rng) {
    if (!valid_rng(rng) || !s_initialized)
        return XY_HAL_ERROR_INVALID_PARAM;
    RNG_ClearFlag(RNG_FLAG_CECS | RNG_FLAG_SECS);
    return XY_HAL_OK;
}

int xy_hal_rng_get_error_code(void* rng) {
    int error = 0;
    if (!valid_rng(rng) || !s_initialized)
        return (int)XY_HAL_ERROR_INVALID_PARAM;
    if (RNG_GetFlagStatus(RNG_FLAG_CECS) == SET)
        error |= RNG_FLAG_CECS;
    if (RNG_GetFlagStatus(RNG_FLAG_SECS) == SET)
        error |= RNG_FLAG_SECS;
    return error;
}

uint32_t xy_hal_rng_soft_random(uint32_t seed) {
    if (seed == 0U) {
        uint32_t value;
        if (xy_hal_rng_get_random_nb(RNG, &value) == XY_HAL_OK)
            return value;
        seed = 0x6D2B79F5U;
    }
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed;
}

void xy_hal_rng_soft_buffer(uint32_t* buffer, size_t count, uint32_t seed) {
    if (buffer == NULL)
        return;
    for (size_t i = 0U; i < count; ++i) {
        seed = xy_hal_rng_soft_random(seed);
        buffer[i] = seed;
    }
}

void RNG_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void RNG_IRQHandler(void) {
    if (has_error()) {
        if (s_callbacks[XY_HAL_RNG_EVENT_ERROR] != NULL)
            s_callbacks[XY_HAL_RNG_EVENT_ERROR](XY_HAL_RNG_EVENT_ERROR,
                                                s_callback_args[XY_HAL_RNG_EVENT_ERROR]);
        RNG_ClearITPendingBit(RNG_IT_CEI | RNG_IT_SEI);
    } else if (s_callbacks[XY_HAL_RNG_EVENT_READY] != NULL) {
        s_callbacks[XY_HAL_RNG_EVENT_READY](XY_HAL_RNG_EVENT_READY,
                                            s_callback_args[XY_HAL_RNG_EVENT_READY]);
    }
}

#endif /* MCU_CH32 */
