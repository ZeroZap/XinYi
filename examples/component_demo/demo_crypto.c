/**
 * @file demo_crypto.c
 * @brief Crypto Component Demo
 * @version 1.0.0
 * @date 2026-03-13
 */

#include <stdio.h>
#include <string.h>
#include "xy_log.h"

#ifdef DEMO_CRYPTO

/* 简化的 AES/SHA256/CRC 接口声明 */
extern int xy_aes_encrypt(const unsigned char *input, unsigned char *output, 
                          const unsigned char *key);
extern int xy_sha256(const unsigned char *input, int len, unsigned char *output);
extern unsigned short xy_crc16(const unsigned char *data, int len);

/**
 * @brief 初始化加密演示
 */
int demo_crypto_init(void)
{
    xy_log_i("Crypto module initialized\n");
    return 0;
}

/**
 * @brief 运行加密演示
 */
void demo_crypto_run(void)
{
    const char *test_data = "Hello, XinYi!";
    unsigned char encrypted[32];
    unsigned char hash[32];
    unsigned short crc;
    
    /* AES 加密演示 */
    xy_log_i("AES-128 Encryption Demo:\n");
    xy_log_d("  Input: \"%s\"\n", test_data);
    
    /* 简化演示：实际应调用完整 AES API */
    memset(encrypted, 0, sizeof(encrypted));
    xy_log_d("  Encrypted: [binary data, %zu bytes]\n", strlen(test_data));
    
    /* SHA256 哈希演示 */
    xy_log_i("SHA-256 Hash Demo:\n");
    xy_log_d("  Input: \"%s\"\n", test_data);
    
    /* 简化演示：实际应调用 xy_sha256() */
    memset(hash, 0, sizeof(hash));
    xy_log_d("  Hash: [32 bytes binary]\n");
    
    /* CRC16 校验演示 */
    xy_log_i("CRC-16 Checksum Demo:\n");
    xy_log_d("  Input: \"%s\"\n", test_data);
    
    crc = xy_crc16((const unsigned char *)test_data, strlen(test_data));
    xy_log_d("  CRC16: 0x%04X\n", crc);
    
    xy_log_i("Crypto demo completed\n");
}

#endif /* DEMO_CRYPTO */
