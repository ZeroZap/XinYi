/**
 * @file xy_fota_secure.c
 * @brief Secure FOTA Implementation
 * @version 2.0.0
 * @date 2026-03-02
 */

#include "xy_fota_secure.h"
#include "xy_log.h"
#include "xy_ecdsa.h"
#include "xy_chacha20poly1305.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/**
 * @brief ECDSA 验证实现
 */
static int xy_fota_ecdsa_verify_impl(const uint8_t *pub_key,
                                     const uint8_t *message,
                                     uint32_t msg_size,
                                     const uint8_t *signature)
{
    /* 使用 ECDSA P-256 验证 */
    return xy_ecdsa_verify_simple(pub_key, message, msg_size, signature);
}

/* ChaCha20 流加密核心 - 修复 TODO */
static void xy_fota_chacha20_block(const uint8_t *key,
                                   const uint8_t *nonce,
                                   uint32_t counter,
                                   uint8_t *keystream)
{
    /* 使用 xy_chacha20poly1305 库实现 */
    xy_chacha20_ctx_t ctx;
    xy_chacha20_init(&ctx, key, nonce);
    
    /* 设置计数器 */
    ctx.state[12] = counter;
    
    /* 生成 64 字节 keystream */
    memset(keystream, 0, 64);
    xy_chacha20_encrypt(&ctx, keystream, keystream, 64);
}

int xy_fota_secure_init(xy_fota_secure_t *fota,
                        const xy_fota_secure_config_t *config,
                        xy_fota_flash_ops_t *flash)
{
    if (!fota || !config || !flash) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    memset(fota, 0, sizeof(*fota));
    memcpy(&fota->config, config, sizeof(xy_fota_secure_config_t));
    fota->flash = flash;
    
    /* 验证配置 */
    if (!config->pub_key) {
        xy_log_e("Public key is NULL\n");
        return XY_FOTA_INVALID_PARAM;
    }
    
    if (config->slot_size < 32 * 1024) {
        xy_log_e("Slot size too small (%d bytes)\n", config->slot_size);
        return XY_FOTA_INVALID_PARAM;
    }
    
    /* 初始化 Flash */
    if (flash->init) {
        int ret = flash->init();
        if (ret < 0) {
            return XY_FOTA_FLASH_ERROR;
        }
    }
    
    fota->initialized = true;
    xy_log_i("Secure FOTA initialized (Slot0=0x%08X, Slot1=0x%08X)\n",
             config->slot0_addr, config->slot1_addr);
    
    return XY_FOTA_OK;
}

int xy_fota_secure_deinit(xy_fota_secure_t *fota)
{
    if (!fota) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    /* 清除敏感数据 */
    memset(fota->chacha_key, 0, sizeof(fota->chacha_key));
    
    if (fota->flash && fota->flash->deinit) {
        fota->flash->deinit();
    }
    
    fota->initialized = false;
    return XY_FOTA_OK;
}

int xy_fota_secure_verify(xy_fota_secure_t *fota,
                          const uint8_t *fw_pkg,
                          uint32_t pkg_size)
{
    if (!fota || !fw_pkg || pkg_size < sizeof(xy_fota_secure_header_t)) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    /* 解析固件包头 */
    memcpy(&fota->header, fw_pkg, sizeof(xy_fota_secure_header_t));
    
    /* 验证魔数 */
    if (fota->header.magic != XY_FOTA_SECURE_MAGIC) {
        xy_log_e("Invalid magic: 0x%08X\n", fota->header.magic);
        return XY_FOTA_ERROR;
    }
    
    /* 验证固件大小 */
    if (fota->header.fw_size > fota->config.slot_size) {
        xy_log_e("Firmware too large (%d > %d)\n", 
                 fota->header.fw_size, fota->config.slot_size);
        return XY_FOTA_ERROR;
    }
    
    /* 验证 ECDSA 签名 */
    const uint8_t *message = fw_pkg + sizeof(xy_fota_secure_header_t);
    uint32_t msg_size = fota->header.fw_size + XY_FOTA_POLY1305_TAG_SIZE;
    
    int ret = xy_fota_ecdsa_verify_impl(fota->config.pub_key,
                                        message,
                                        msg_size,
                                        fota->header.ecdsa_sig);
    if (ret != 0) {
        xy_log_e("ECDSA signature verification failed\n");
        return XY_FOTA_AUTH_ERROR;
    }
    
    fota->verified = true;
    fota->total_size = fota->header.fw_size;
    fota->decrypted_size = 0;
    
    xy_log_i("Firmware verified (version=%d, size=%d)\n",
             fota->header.version, fota->header.fw_size);
    
    return XY_FOTA_OK;
}

int xy_fota_chacha20_decrypt(const uint8_t *key,
                             const uint8_t *nonce,
                             const uint8_t *ciphertext,
                             uint32_t ct_len,
                             uint8_t *plaintext,
                             const uint8_t *tag)
{
    /* TODO: 实现完整的 ChaCha20-Poly1305 解密 */
    /* 包括 Poly1305 MAC 验证 */
    (void)key;
    (void)nonce;
    (void)ciphertext;
    (void)ct_len;
    (void)plaintext;
    (void)tag;
    
    /* 模拟解密 */
    if (plaintext != ciphertext) {
        memcpy(plaintext, ciphertext, ct_len);
    }
    
    return 0;
}

int xy_fota_secure_decrypt_and_write(xy_fota_secure_t *fota,
                                     const uint8_t *encrypted_data,
                                     uint32_t data_size,
                                     uint32_t offset)
{
    if (!fota || !encrypted_data || !fota->verified) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    if (offset + data_size > fota->total_size) {
        xy_log_e("Data size exceeds firmware size\n");
        return XY_FOTA_ERROR;
    }
    
    /* 分配解密缓冲区 */
    uint8_t *decrypted = malloc(data_size);
    if (!decrypted) {
        return XY_FOTA_NO_MEM;
    }
    
    /* 解密数据 */
    int ret = xy_fota_chacha20_decrypt(fota->chacha_key,
                                       fota->header.chacha_nonce,
                                       encrypted_data,
                                       data_size,
                                       decrypted,
                                       NULL);  /* TODO: 添加 tag 验证 */
    
    if (ret != 0) {
        free(decrypted);
        return XY_FOTA_ERROR;
    }
    
    /* 写入 Flash */
    uint32_t flash_addr = fota->config.slot1_addr + offset;
    ret = fota->flash->write(flash_addr, decrypted, data_size);
    
    free(decrypted);
    
    if (ret != 0) {
        return XY_FOTA_FLASH_ERROR;
    }
    
    fota->decrypted_size += data_size;
    
    xy_log_d("Decrypted %d/%d bytes (%d%%)\n",
             fota->decrypted_size, fota->total_size,
             (fota->decrypted_size * 100) / fota->total_size);
    
    return XY_FOTA_OK;
}

int xy_fota_secure_swap(xy_fota_secure_t *fota)
{
    if (!fota || !fota->config.dual_bank) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    /* 使用双 Bank 交换逻辑 */
    return xy_fota_bank_swap(NULL);
}

int xy_fota_secure_mark_valid(xy_fota_secure_t *fota, uint8_t slot)
{
    if (!fota || slot > 1) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    /* 标记 Slot 有效 */
    return xy_fota_bank_mark_valid(NULL, slot);
}

int xy_fota_secure_is_valid(xy_fota_secure_t *fota, uint8_t slot, bool *valid)
{
    if (!fota || !valid || slot > 1) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    /* 检查 Slot 有效性 */
    return xy_fota_bank_is_valid(NULL, slot, valid);
}
