#include "pandora_platform_startup.h"
#include "stm32l4xx_hal.h"
#include "xy_hal_delay.h"
#include "xy_hal_sys.h"
#include "xy_hal_uart.h"
#include "xy_tiny_crypto.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifndef XINYI_FIRMWARE_COMMIT
#error "XINYI_FIRMWARE_COMMIT must identify the source commit used for this image"
#endif

static UART_HandleTypeDef uart1;

void _init(void) {}
void _fini(void) {}
void SysTick_Handler(void) { xy_hal_sys_tick_irq_handler(); }

static void fail(void)
{
    __disable_irq();
    for (;;) {}
}

static void uart_text(const char *text)
{
    uint16_t length = 0U;

    while (text[length] != '\0') {
        ++length;
    }
    (void)xy_hal_uart_send(&uart1, (const uint8_t *)text, length, 100U);
}

static void uart_init(void)
{
    const xy_hal_gpio_config_t gpio = {XY_HAL_GPIO_MODE_AF, XY_HAL_GPIO_PULL_UP,
                                       XY_HAL_GPIO_OTYPE_PP, XY_HAL_GPIO_SPEED_VERY_HIGH,
                                       GPIO_AF7_USART1};
    const xy_hal_uart_config_t uart = {115200U, XY_HAL_UART_WORDLEN_8B,
                                       XY_HAL_UART_STOPBITS_1, XY_HAL_UART_PARITY_NONE,
                                       XY_HAL_UART_FLOWCTRL_NONE, XY_HAL_UART_MODE_TX_RX};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();
    if (xy_hal_gpio_init(GPIOA, 9U, &gpio) != XY_HAL_OK ||
        xy_hal_gpio_init(GPIOA, 10U, &gpio) != XY_HAL_OK) {
        fail();
    }
    uart1.Instance = USART1;
    if (xy_hal_uart_init(&uart1, &uart) != XY_HAL_OK) {
        fail();
    }
}

static int run_sha256_kat(void)
{
    static const uint8_t expected[XY_SHA256_DIGEST_SIZE] = {
        0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
        0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
        0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
        0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad,
    };
    uint8_t digest[XY_SHA256_DIGEST_SIZE];

    return xy_sha256_hash((const uint8_t *)"abc", 3U, digest) == XY_CRYPTO_SUCCESS &&
           memcmp(digest, expected, sizeof(expected)) == 0;
}

static int run_hmac_sha256_kat(void)
{
    static const uint8_t expected[XY_SHA256_DIGEST_SIZE] = {
        0xf7, 0xbc, 0x83, 0xf4, 0x30, 0x53, 0x84, 0x24,
        0xb1, 0x32, 0x98, 0xe6, 0xaa, 0x6f, 0xb1, 0x43,
        0xef, 0x4d, 0x59, 0xa1, 0x49, 0x46, 0x17, 0x59,
        0x97, 0x47, 0x9d, 0xbc, 0x2d, 0x1a, 0x3c, 0xd8,
    };
    static const uint8_t key[] = "key";
    static const uint8_t message[] = "The quick brown fox jumps over the lazy dog";
    uint8_t digest[XY_SHA256_DIGEST_SIZE];

    return xy_hmac_sha256(key, sizeof(key) - 1U, message, sizeof(message) - 1U, digest) ==
               XY_CRYPTO_SUCCESS &&
           memcmp(digest, expected, sizeof(expected)) == 0;
}

static int run_aes128_kat(void)
{
    static const uint8_t key[XY_AES_KEY_SIZE_128] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    };
    static const uint8_t plaintext[XY_AES_BLOCK_SIZE] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
    };
    static const uint8_t expected[XY_AES_BLOCK_SIZE] = {
        0x69, 0xc4, 0xe0, 0xd8, 0x6a, 0x7b, 0x04, 0x30,
        0xd8, 0xcd, 0xb7, 0x80, 0x70, 0xb4, 0xc5, 0x5a,
    };
    xy_aes_ctx_t context;
    uint8_t ciphertext[XY_AES_BLOCK_SIZE];
    uint8_t roundtrip[XY_AES_BLOCK_SIZE];

    return xy_aes_init(&context, key, XY_AES_KEY_SIZE_128) == XY_CRYPTO_SUCCESS &&
           xy_aes_encrypt_block(&context, plaintext, ciphertext) == XY_CRYPTO_SUCCESS &&
           memcmp(ciphertext, expected, sizeof(expected)) == 0 &&
           xy_aes_decrypt_block(&context, ciphertext, roundtrip) == XY_CRYPTO_SUCCESS &&
           memcmp(roundtrip, plaintext, sizeof(plaintext)) == 0;
}

int main(void)
{
    uint32_t iteration;

    if (xy_hal_sys_init() != XY_HAL_OK || pandora_platform_startup() != 0) {
        fail();
    }
    uart_init();
    uart_text("PANDORA STM32L475VE CRYPTO SOFTWARE READY\r\n");
    uart_text("FIRMWARE_COMMIT " XINYI_FIRMWARE_COMMIT "\r\n");

    if (!run_sha256_kat()) {
        uart_text("CRYPTO_SHA256_KAT_ERROR\r\n");
        fail();
    }
    uart_text("CRYPTO_SHA256_KAT_PASS\r\n");

    if (!run_hmac_sha256_kat()) {
        uart_text("CRYPTO_HMAC_SHA256_KAT_ERROR\r\n");
        fail();
    }
    uart_text("CRYPTO_HMAC_SHA256_KAT_PASS\r\n");

    if (!run_aes128_kat()) {
        uart_text("CRYPTO_AES128_KAT_ERROR\r\n");
        fail();
    }
    uart_text("CRYPTO_AES128_KAT_PASS\r\n");

    for (iteration = 0U; iteration < 1000U; ++iteration) {
        if (!run_sha256_kat() || !run_hmac_sha256_kat() || !run_aes128_kat()) {
            uart_text("CRYPTO_REPEAT_ERROR\r\n");
            fail();
        }
    }
    uart_text("CRYPTO_REPEAT_1000_PASS\r\n");
    uart_text("B1_CRYPTO_SOFTWARE_KAT_PASS\r\n");

    for (;;) {
        xy_hal_delay_ms(1000U);
    }
}
