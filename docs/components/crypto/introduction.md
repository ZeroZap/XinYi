# Crypto 组件 - 密码学库

**状态**: ✅ 完善 | **测试**: 28 用例 | **版本**: 1.0

---

## 📖 简介

XinYi Crypto 是一个轻量级的 C 语言加密算法库，提供常用的加密、哈希和编码功能。

### 核心特性

- ✅ **对称加密** - AES-128/192/256 (ECB/CBC/CTR)
- ✅ **哈希算法** - MD5、SHA-256、HMAC
- ✅ **编码算法** - Base64、Hex
- ✅ **校验算法** - CRC32
- ✅ **随机数生成** - 真随机/伪随机
- ✅ **零依赖** - 纯 C 实现，无外部依赖

### 算法列表

| 类别 | 算法 | 说明 |
|------|------|------|
| **对称加密** | AES | ECB/CBC/CTR 模式 |
| **哈希** | MD5 | 128 位哈希 |
| **哈希** | SHA-256 | 256 位哈希 |
| **认证** | HMAC | 基于哈希的消息认证 |
| **编码** | Base64 | 二进制转文本 |
| **编码** | Hex | 十六进制编码 |
| **校验** | CRC32 | 32 位循环冗余校验 |

---

## 🚀 快速开始

### 1. 包含头文件

```c
#include "xy_tiny_crypto.h"
```

### 2. AES 加密示例

```c
#include "xy_tiny_crypto.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    // AES-128 密钥
    const uint8_t key[16] = {
        0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
    };
    
    // 明文
    const uint8_t plaintext[16] = "Hello, AES!     ";
    uint8_t ciphertext[16];
    uint8_t decrypted[16];
    
    // 初始化 AES
    xy_aes_ctx_t aes_ctx;
    xy_aes_init(&aes_ctx, key, XY_AES_KEY_SIZE_128);
    
    // 加密
    xy_aes_encrypt_block(&aes_ctx, plaintext, ciphertext);
    
    // 解密
    xy_aes_decrypt_block(&aes_ctx, ciphertext, decrypted);
    
    printf("加密/解密成功！\n");
    return 0;
}
```

### 3. MD5 哈希示例

```c
#include "xy_tiny_crypto.h"
#include <stdio.h>

int main(void) {
    const char *data = "Hello, World!";
    uint8_t digest[XY_MD5_DIGEST_SIZE];
    
    // 计算 MD5
    xy_md5_hash((const uint8_t *)data, strlen(data), digest);
    
    // 打印哈希值
    printf("MD5: ");
    for (int i = 0; i < XY_MD5_DIGEST_SIZE; i++) {
        printf("%02x", digest[i]);
    }
    printf("\n");
    
    return 0;
}
```

### 4. Base64 编码示例

```c
#include "xy_tiny_crypto.h"
#include <stdio.h>

int main(void) {
    const char *data = "Hello, Base64!";
    char encoded[64];
    
    // Base64 编码
    xy_base64_encode((const uint8_t *)data, strlen(data), encoded, sizeof(encoded));
    
    printf("Base64: %s\n", encoded);
    
    return 0;
}
```

---

## 📋 API 参考

### AES

| 函数 | 说明 | 参数 |
|------|------|------|
| `xy_aes_init()` | 初始化 AES | ctx, key, key_size |
| `xy_aes_encrypt_block()` | 加密块 | ctx, plaintext, ciphertext |
| `xy_aes_decrypt_block()` | 解密块 | ctx, ciphertext, plaintext |
| `xy_aes_cbc_encrypt()` | CBC 加密 | ctx, iv, plaintext, len, ciphertext |
| `xy_aes_cbc_decrypt()` | CBC 解密 | ctx, iv, ciphertext, len, plaintext |

### MD5

| 函数 | 说明 | 参数 |
|------|------|------|
| `xy_md5_hash()` | 单次哈希 | data, len, digest |
| `xy_md5_init()` | 初始化上下文 | ctx |
| `xy_md5_update()` | 更新数据 | ctx, data, len |
| `xy_md5_final()` | 完成哈希 | ctx, digest |

### SHA-256

| 函数 | 说明 | 参数 |
|------|------|------|
| `xy_sha256_hash()` | 单次哈希 | data, len, digest |
| `xy_sha256_init()` | 初始化上下文 | ctx |
| `xy_sha256_update()` | 更新数据 | ctx, data, len |
| `xy_sha256_final()` | 完成哈希 | ctx, digest |

### Base64

| 函数 | 说明 | 参数 |
|------|------|------|
| `xy_base64_encode()` | 编码 | input, input_len, output, output_len |
| `xy_base64_decode()` | 解码 | input, input_len, output, output_len |
| `xy_base64_encode_len()` | 获取编码长度 | input_len |
| `xy_base64_decode_len()` | 获取解码长度 | input_len |

### CRC32

| 函数 | 说明 | 参数 |
|------|------|------|
| `xy_crc32()` | 计算 CRC32 | data, len |
| `xy_crc32_update()` | 更新 CRC | crc, data, len |

---

## 🔧 配置选项

```c
// xy_crypto_config.h

// 选择性启用算法 (节省代码空间)
#define XY_CRYPTO_ENABLE_AES    1
#define XY_CRYPTO_ENABLE_SHA256 1
#define XY_CRYPTO_ENABLE_MD5    1
#define XY_CRYPTO_ENABLE_HMAC   1
#define XY_CRYPTO_ENABLE_CRC    1
#define XY_CRYPTO_ENABLE_BASE64 1
#define XY_CRYPTO_ENABLE_HEX    1

// 平台选择
#define XY_CRYPTO_PLATFORM XY_CRYPTO_PLATFORM_SOFTWARE  // 纯软件
// #define XY_CRYPTO_PLATFORM XY_CRYPTO_PLATFORM_HAL    // HAL 加速
// #define XY_CRYPTO_PLATFORM XY_CRYPTO_PLATFORM_SDK    // SDK 实现
```

---

## 📝 使用示例

### 示例 1: HMAC-SHA256 消息认证

```c
#include "xy_tiny_crypto.h"

const char *key = "my_secret_key";
const char *data = "Important message";
uint8_t hmac[XY_SHA256_DIGEST_SIZE];

// 计算 HMAC
xy_hmac_sha256(
    (const uint8_t *)key, strlen(key),
    (const uint8_t *)data, strlen(data),
    hmac
);

// 验证时重新计算并比较
```

### 示例 2: 文件完整性校验

```c
#include "xy_tiny_crypto.h"

// 计算文件 CRC32
uint32_t crc = xy_crc32(file_data, file_size);

// 存储 CRC 用于后续验证
// ...

// 验证时重新计算并比较
uint32_t verify_crc = xy_crc32(verify_data, verify_size);
if (crc == verify_crc) {
    // 文件完整
}
```

### 示例 3: 增量哈希计算

```c
#include "xy_tiny_crypto.h"

xy_sha256_ctx_t ctx;
uint8_t digest[XY_SHA256_DIGEST_SIZE];

// 初始化
xy_sha256_init(&ctx);

// 分块更新
xy_sha256_update(&ctx, chunk1, chunk1_len);
xy_sha256_update(&ctx, chunk2, chunk2_len);
xy_sha256_update(&ctx, chunk3, chunk3_len);

// 完成
xy_sha256_final(&ctx, digest);
```

---

## 🧪 测试用例

Crypto 组件测试已收敛到仓库级 Unity + CTest 套件，覆盖 CRC、随机数、
Base64/Hex、MD5/SHA-256、AES/HMAC/SM3/SM4/ChaCha20 和 SM2 公共 API 契约。

运行测试：

```bash
make test-unit

# 或运行 Crypto focused tests
ctest --test-dir build/tests/unit -R '^crypto_(crc|random|encode|hash|cipher_hmac|sm2)$' --output-on-failure
```

---

## ⚠️ 注意事项

1. **安全警告**: 本库主要用于学习和轻量级应用，不建议在高安全要求的生产环境中使用
2. **随机数**: 在无法访问系统熵源时会降级为伪随机数生成器
3. **AES 填充**: AES 实现未包含填充方案，需要用户自行处理数据对齐
4. **内存**: 所有函数都进行了基本的参数验证，但调用者需确保缓冲区足够大

---

## 📚 相关文档

- [算法详情](algorithms.md)
- [API 参考](api-reference.md)
- [使用示例](examples.md)
- [平台选择指南](platform-guide.md)

---

## 📞 获取帮助

- 📚 [API 文档](api-reference.md)
- ❓ [常见问题](../about/faq.md)
- 🐛 [报告问题](https://github.com/ZeroZap/XinYi/issues)

---

*最后更新：2026-02-28 | 维护者：XinYi Team*
