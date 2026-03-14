/**
 * @file hal_pc_wrapper.c
 * @brief HAL PC Wrapper - Provides HAL functions for PC build
 * @version 1.0.0
 * @date 2026-03-14
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

/* ==================== HAL Core ==================== */

typedef enum {
    XY_HAL_OK = 0,
    XY_HAL_ERROR = -1,
    XY_HAL_ERROR_INVALID_PARAM = -2,
} xy_hal_status_t;

xy_hal_status_t xy_hal_init(void)
{
    printf("[HAL] PC initialized\n");
    return XY_HAL_OK;
}

uint32_t xy_hal_get_tick_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

void xy_hal_delay_ms(uint32_t ms)
{
    usleep(ms * 1000);
}

/* ==================== Device Registry ==================== */

void xy_device_registry_init(void)
{
    /* PC simulation */
}

/* ==================== Sensor ==================== */

/* Placeholder - actual implementation in sensor component */

/* ==================== Crypto ==================== */

typedef enum {
    XY_CRYPTO_OK = 0,
    XY_CRYPTO_ERROR = -1,
} xy_crypto_status_t;

xy_crypto_status_t xy_aes_init(void)
{
    return XY_CRYPTO_OK;
}

xy_crypto_status_t xy_sha256_hash(const char *input, size_t len, uint8_t *output)
{
    if (!input || !output) {
        return XY_CRYPTO_ERROR;
    }
    
    /* Simple dummy hash for PC test */
    memset(output, 0, 32);
    for (size_t i = 0; i < len && i < 32; i++) {
        output[i] = (uint8_t)input[i];
    }
    
    return XY_CRYPTO_OK;
}

/* ==================== Power Management ==================== */

typedef enum {
    XY_PM_MODE_ACTIVE = 0,
    XY_PM_MODE_SLEEP,
    XY_PM_MODE_STANDBY,
    XY_PM_MODE_SHUTDOWN,
} xy_pm_mode_t;

void xy_pm_init(void)
{
    /* PC simulation */
}

xy_pm_mode_t xy_pm_get_mode(void)
{
    return XY_PM_MODE_ACTIVE;
}

/* ==================== IPC ==================== */

void xy_ipc_init(void)
{
    /* PC simulation */
}

/* ==================== Data Manager ==================== */

void xy_dm_init(void)
{
    /* PC simulation */
}

/* ==================== FOTA ==================== */

void xy_fota_init(void)
{
    /* PC simulation */
}

/* ==================== Trace ==================== */

void xy_trace_init(void)
{
    /* PC simulation */
}

/* ==================== State Machine ==================== */

typedef enum {
    XY_SM_OK = 0,
    XY_SM_ERROR = -1,
} xy_sm_status_t;

typedef struct {
    char name[32];
    void *user_data;
} xy_sm_t;

xy_sm_status_t xy_sm_init(xy_sm_t *sm, const char *name, void *user_data, size_t user_data_size)
{
    if (!sm || !name) {
        return XY_SM_ERROR;
    }
    
    strncpy(sm->name, name, sizeof(sm->name) - 1);
    sm->user_data = user_data;
    
    return XY_SM_OK;
}

/* ==================== Drivers ==================== */

typedef enum {
    XY_KEY_OK = 0,
    XY_KEY_ERROR = -1,
    XY_KEY_ERROR_INVALID_PARAM = -2,
} xy_key_status_t;

typedef struct {
    int pin;
    bool active_low;
} xy_key_config_t;

typedef struct {
    xy_key_config_t config;
    bool pressed;
} xy_key_t;

xy_key_status_t xy_key_init(xy_key_t *key, const xy_key_config_t *config)
{
    if (!key || !config) {
        return XY_KEY_ERROR_INVALID_PARAM;
    }
    
    key->config = *config;
    key->pressed = false;
    
    return XY_KEY_OK;
}

void xy_led_init(void)
{
    /* PC simulation */
}

/* ==================== NET ==================== */

void xy_net_init(void)
{
    /* PC simulation */
}
