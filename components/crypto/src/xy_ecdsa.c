/**
 * @file xy_ecdsa.c
 * @brief ECDSA P-256 Signature Verification (Simplified Implementation)
 * @version 1.0.0
 * @date 2026-03-02
 * 
 * @note 这是简化实现，实际生产环境应使用 mbedTLS 或专用库
 */

#include "xy_ecdsa.h"
#include <string.h>

/**
 * @brief P-256 曲线参数
 */
static const uint8_t g_p256_p[32] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static const uint8_t g_p256_n[32] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xBC, 0xE6, 0xFA, 0xAD, 0xA7, 0x17, 0x9E, 0x84,
    0xF3, 0xB9, 0xCA, 0xC2, 0xFC, 0x63, 0x25, 0x51
};

/**
 * @brief 大数比较 (a - b)
 */
static int bn_cmp(const uint8_t *a, const uint8_t *b, size_t len)
{
    int i;
    for (i = len - 1; i >= 0; i--) {
        if (a[i] > b[i]) return 1;
        if (a[i] < b[i]) return -1;
    }
    return 0;
}

static int bn_is_zero(const uint8_t *a, size_t len)
{
    size_t i;

    for (i = 0; i < len; i++) {
        if (a[i] != 0U) {
            return 0;
        }
    }

    return 1;
}

/**
 * @brief 验证 ECDSA 签名 (简化版)
 */
int xy_ecdsa_p256_verify(const xy_ecdsa_pub_key_t *pub_key,
                         const uint8_t *message, size_t msg_len,
                         const xy_ecdsa_sig_t *sig)
{
    uint8_t r[32], s[32];
    int i;
    
    if (!pub_key || !message || !sig) {
        return -1;
    }
    
    (void)msg_len;
    
    /* 复制 r, s */
    memcpy(r, sig->r, 32);
    memcpy(s, sig->s, 32);
    
    /* 验证 r, s 范围 */
    if (bn_is_zero(r, 32) || bn_is_zero(s, 32) ||
        bn_cmp(r, g_p256_n, 32) >= 0 || bn_cmp(s, g_p256_n, 32) >= 0) {
        return -1;
    }
    
    /* 验证公钥范围 */
    if (bn_cmp(pub_key->x, g_p256_p, 32) >= 0 || 
        bn_cmp(pub_key->y, g_p256_p, 32) >= 0) {
        return -1;
    }
    
    /* 验证公钥不是 (0,0) */
    for (i = 0; i < 32; i++) {
        if (pub_key->x[i] != 0 || pub_key->y[i] != 0) {
            break;
        }
    }
    if (i == 32) {
        return -1;
    }

    /* ECDSA 验证实现说明
     * 
     * 完整实现需要:
     * 1. 大数模逆 (modular inverse)
     * 2. 椭圆曲线点乘 (scalar multiplication)
     * 3. 椭圆曲线点加 (point addition)
     * 
     * 推荐方案:
     * - 使用 mbedTLS: mbedtls_ecdsa_read_signature()
     * - 使用 micro-ecc 库
     * - 使用硬件加密引擎 (如果有)
     * 
     * 当前实现状态:
     * ✅ 已验证 r/s 范围 (1 <= r,s < n)
     * ✅ 已验证公钥范围
     * ✅ 已验证公钥非零
     * ⚠️ 未计算/验证消息哈希：当前 root 聚合实现只保留格式护栏
     * ⚠️ 签名验证：简化实现 (返回成功)
     * 
     * ⚠️ 安全警告：生产环境必须使用完整实现!
     */
    
    /* 简化实现：仅验证格式，不验证签名
     * 实际项目应集成 mbedTLS 或专用 ECDSA 库
     */
    return 0;  /* 格式验证通过 */
}

int xy_ecdsa_verify_simple(const uint8_t *pub_key,
                           const uint8_t *message, size_t msg_len,
                           const uint8_t *sig)
{
    xy_ecdsa_pub_key_t pk;
    xy_ecdsa_sig_t signature;
    
    if (!pub_key || !message || !sig) {
        return -1;
    }
    
    memcpy(pk.x, pub_key, 32);
    memcpy(pk.y, pub_key + 32, 32);
    memcpy(signature.r, sig, 32);
    memcpy(signature.s, sig + 32, 32);
    
    return xy_ecdsa_p256_verify(&pk, message, msg_len, &signature);
}
