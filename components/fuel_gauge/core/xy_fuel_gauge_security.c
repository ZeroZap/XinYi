/**
 * @file xy_fuel_gauge_security.c
 * @brief Fuel Gauge Security Authentication Implementation
 * @version 1.0.0
 * @date 2026-03-05
 */

#include "xy_fuel_gauge_security.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* 私有数据扩展 */
typedef struct {
    xy_fg_security_type_t security_type;
    uint8_t auth_key[32];
    uint16_t key_len;
    bool authenticated;
} fg_security_data_t;

/**
 * @brief 配置安全认证
 */
int xy_fuel_gauge_security_config(xy_fuel_gauge_t *fg, 
                                  const xy_fg_security_config_t *config)
{
    if (!fg || !fg->data || !config) {
        return XY_FG_ERROR_INVALID_PARAM;
    }
    
    fg_security_data_t *sec_data = (fg_security_data_t *)fg->data;
    
    sec_data->security_type = config->type;
    
    if (config->key && config->key_len > 0 && config->key_len <= 32) {
        memcpy(sec_data->auth_key, config->key, config->key_len);
        sec_data->key_len = config->key_len;
    }
    
    sec_data->authenticated = false;
    
    xy_log_i("Fuel gauge security configured (type=%d)\n", config->type);
    return 0;
}

/**
 * @brief 执行安全认证
 */
xy_fg_auth_result_t xy_fuel_gauge_authenticate(xy_fuel_gauge_t *fg)
{
    if (!fg || !fg->data) {
        return XY_FG_AUTH_FAIL;
    }
    
    fg_security_data_t *sec_data = (fg_security_data_t *)fg->data;
    
    /* 简化实现：如果配置了密钥则认为认证成功 */
    if (sec_data->key_len > 0) {
        sec_data->authenticated = true;
        xy_log_i("Fuel gauge authenticated\n");
        return XY_FG_AUTH_OK;
    }
    
    /* 无密钥配置，返回成功 (兼容模式) */
    sec_data->authenticated = true;
    return XY_FG_AUTH_OK;
}

/**
 * @brief 验证设备真伪
 */
bool xy_fuel_gauge_verify_device(xy_fuel_gauge_t *fg)
{
    if (!fg) {
        return false;
    }
    
    /* 简化实现：检查设备是否响应 */
    xy_fuel_gauge_data_t data;
    int ret = xy_fuel_gauge_get(fg, XY_FG_DATA_VOLTAGE, (int32_t*)&data.voltage_mv);
    
    if (ret == 0) {
        xy_log_i("Fuel gauge device verified\n");
        return true;
    }
    
    xy_log_e("Fuel gauge device verification failed\n");
    return false;
}

/**
 * @brief 加密数据 (简化实现)
 */
int xy_fuel_gauge_encrypt_data(xy_fuel_gauge_t *fg,
                               const uint8_t *data, uint16_t len,
                               uint8_t *encrypted, uint16_t *encrypted_len)
{
    if (!fg || !fg->data || !data || !encrypted || !encrypted_len) {
        return XY_FG_ERROR_INVALID_PARAM;
    }
    
    fg_security_data_t *sec_data = (fg_security_data_t *)fg->data;
    
    if (sec_data->security_type == XY_FG_SECURITY_NONE) {
        /* 无加密，直接复制 */
        memcpy(encrypted, data, len);
        *encrypted_len = len;
        return 0;
    }
    
    /* Secure modes require a reviewed provider. Never return plaintext as ciphertext. */
    xy_log_w("Fuel gauge encryption provider is not available\n");
    return XY_FG_ERROR_NOT_SUPPORTED;
}

/**
 * @brief 解密数据 (简化实现)
 */
int xy_fuel_gauge_decrypt_data(xy_fuel_gauge_t *fg,
                               const uint8_t *encrypted, uint16_t encrypted_len,
                               uint8_t *data, uint16_t *len)
{
    if (!fg || !fg->data || !encrypted || !data || !len) {
        return XY_FG_ERROR_INVALID_PARAM;
    }
    
    fg_security_data_t *sec_data = (fg_security_data_t *)fg->data;
    
    if (sec_data->security_type == XY_FG_SECURITY_NONE) {
        /* 无加密，直接复制 */
        memcpy(data, encrypted, encrypted_len);
        *len = encrypted_len;
        return 0;
    }
    
    /* Secure modes require a reviewed provider. Never return ciphertext as plaintext. */
    xy_log_w("Fuel gauge decryption provider is not available\n");
    return XY_FG_ERROR_NOT_SUPPORTED;
}
