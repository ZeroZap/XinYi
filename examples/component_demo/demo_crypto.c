/**
 * @file demo_crypto.c
 * @brief Crypto Component Demo (Standalone)
 * @version 1.0.0
 * @date 2026-03-13
 */

#include <stdio.h>
#include <string.h>

#ifdef DEMO_CRYPTO

/* Simple CRC16 implementation for demo */
static unsigned short crc16(const unsigned char *data, int len)
{
    unsigned short crc = 0xFFFF;
    for (int i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
        }
    }
    return crc;
}

/* Simple hash for demo */
static void simple_hash(const char *input, unsigned char *output)
{
    unsigned int hash = 5381;
    int c;
    while ((c = *input++))
        hash = ((hash << 5) + hash) + c;
    
    for (int i = 0; i < 32; i++) {
        output[i] = (hash >> (i % 32)) & 0xFF;
    }
}

int demo_crypto_init(void)
{
    printf("  Crypto module initialized\n");
    return 0;
}

void demo_crypto_run(void)
{
    const char *test_data = "Hello, XinYi!";
    unsigned char hash[32];
    unsigned short crc;
    
    /* CRC16 演示 */
    printf("  CRC-16 Checksum Demo:\n");
    printf("    Input: \"%s\"\n", test_data);
    
    crc = crc16((const unsigned char *)test_data, strlen(test_data));
    printf("    CRC16: 0x%04X\n", crc);
    
    /* Hash 演示 */
    printf("  Hash Demo:\n");
    printf("    Input: \"%s\"\n", test_data);
    
    simple_hash(test_data, hash);
    printf("    Hash: %02x%02x%02x%02x...\n", hash[0], hash[1], hash[2], hash[3]);
    
    printf("  Crypto demo completed\n");
}

#endif /* DEMO_CRYPTO */
