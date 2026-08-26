/**
 * @file xy_fota_secure.c
 * @brief Secure FOTA Implementation
 * @version 2.0.0
 * @date 2026-03-02
 */

#include "xy_fota_secure.h"
#include "xy_log.h"
#include "xy_chacha20poly1305.h"
#include <stdlib.h>
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

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
    
    /* Secure updates must never fall back to the format-only ECDSA placeholder. */
    if (!config->signature_provider || !config->signature_provider->verify) {
        xy_log_e("Production signature provider is unavailable\n");
        return XY_FOTA_AUTH_ERROR;
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
    if (!fota || !fota->initialized || !fw_pkg ||
        pkg_size < sizeof(xy_fota_secure_header_t)) {
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

    if (fota->header.version < fota->config.min_version) {
        xy_log_e("Firmware version is below anti-rollback floor\n");
        return XY_FOTA_VERSION_ERROR;
    }

    if (pkg_size < sizeof(xy_fota_secure_header_t) + XY_FOTA_POLY1305_TAG_SIZE ||
        fota->header.fw_size >
            pkg_size - sizeof(xy_fota_secure_header_t) - XY_FOTA_POLY1305_TAG_SIZE) {
        xy_log_e("Firmware package is truncated\n");
        return XY_FOTA_ERROR;
    }
    
    /* Verify through the caller-supplied production provider. */
    const uint8_t *message = fw_pkg + sizeof(xy_fota_secure_header_t);
    uint32_t msg_size = fota->header.fw_size + XY_FOTA_POLY1305_TAG_SIZE;
    
    int ret = fota->config.signature_provider->verify(
        fota->config.signature_provider->context, fota->config.key_id, message, msg_size,
        fota->header.ecdsa_sig, XY_FOTA_ECDSA_P256_SIG_SIZE);
    if (ret != XY_FOTA_OK) {
        xy_log_e("Signature provider rejected firmware\n");
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
    /* 验证参数 */
    if (!key || !nonce || !ciphertext || !plaintext || ct_len < XY_FOTA_POLY1305_TAG_SIZE) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    /* 分离密文和 Tag */
    uint32_t actual_ct_len = ct_len - XY_FOTA_POLY1305_TAG_SIZE;
    const uint8_t *received_tag = ciphertext + actual_ct_len;
    
    /* 使用 ChaCha20-Poly1305 AEAD 解密 */
    size_t pt_len;
    int ret = xy_chacha20poly1305_decrypt(key, nonce,
                                          NULL, 0,  /* No AAD */
                                          ciphertext, ct_len,
                                          plaintext, &pt_len);
    
    if (ret != 0) {
        /* 解密失败 (可能是 MAC 验证失败) */
        return XY_FOTA_AUTH_ERROR;
    }
    
    /* 验证 Tag (如果提供了外部 tag) */
    if (tag != NULL) {
        if (memcmp(received_tag, tag, XY_FOTA_POLY1305_TAG_SIZE) != 0) {
            xy_log_e("Poly1305 tag mismatch!\n");
            return XY_FOTA_AUTH_ERROR;
        }
    }
    
    return XY_FOTA_OK;
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
    
    /* 解密数据 (包含 Poly1305 tag 验证) */
    int ret = xy_fota_chacha20_decrypt(fota->chacha_key,
                                       fota->header.chacha_nonce,
                                       encrypted_data,
                                       data_size,
                                       decrypted,
                                       NULL);  /* Tag 已包含在 encrypted_data 末尾 */
    
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
    if (!fota || !fota->verified) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    /* 双 Bank 模式：交换 Slot */
    if (fota->config.dual_bank) {
        xy_log_i("Performing dual-bank slot swap...\n");
        
        /* 读取当前启动 Slot (从 OTP/Flash 标志位) */
        uint8_t current_slot = 0;
        /* 实际实现应从以下位置读取:
         * - OTP: 读取 OTP 中的 boot_slot 标志
         * - Flash: 读取保留扇区的 slot 标志
         * - 寄存器：读取 RTC 备份寄存器
         * 示例：current_slot = read_boot_slot_from_otp();
         */
        uint8_t next_slot = (current_slot == 0) ? 1 : 0;
        
        /* 验证新 Slot 的固件 */
        bool valid = false;
        int ret = xy_fota_secure_is_valid(fota, next_slot, &valid);
        if (ret != XY_FOTA_OK || !valid) {
            xy_log_e("Next slot (%d) firmware invalid, aborting swap\n", next_slot);
            return XY_FOTA_AUTH_ERROR;
        }
        
        /* 执行交换 */
        ret = xy_fota_bank_swap(NULL);
        if (ret == XY_FOTA_OK) {
            xy_log_i("Slot swap successful, booting from Slot %d\n", next_slot);
        }
        return ret;
    }
    
    /* 单 Bank 模式：直接标记 Slot1 为有效 */
    xy_log_i("Single-bank mode, marking Slot1 as valid...\n");
    return xy_fota_secure_mark_valid(fota, 1);
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
