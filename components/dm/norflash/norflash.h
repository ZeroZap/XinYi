#ifndef _NORFLASH_H_
#define _NORFLASH_H_

#include <stdint.h>
#include <stdbool.h>

// 芯片参数与特性结构体
typedef struct {
    uint32_t id;
    uint32_t capacity;
    uint32_t erase_size;
    uint32_t page_size;
    uint8_t cmd_read;
    uint8_t cmd_write;
    uint8_t cmd_erase_4k;
    uint8_t cmd_erase_32k;
    uint8_t cmd_erase_64k;
    uint8_t cmd_chip_erase;
    bool support_sfdp;
    bool support_qspi;
    bool support_4k_erase;
    bool support_32k_erase;
    bool support_64k_erase;
    uint8_t sfdp_major_rev;
    uint8_t sfdp_minor_rev;
} norflash_info_t;

// norflash 驱动对象，包含参数和底层操作
typedef struct {
    norflash_info_t info;

    // 底层硬件操作函数指针
    void (*write_enable)(void *user_ctx);
    void (*wait_ready)(void *user_ctx);
    int (*send_cmd)(void *user_ctx, uint8_t cmd, uint32_t addr,
                    const uint8_t *tx, uint8_t *rx, uint32_t len);
    void *user_ctx; // 硬件相关上下文（SPI句柄等）

    // 状态：是否已初始化
    bool inited;
} norflash_t;

// 自动识别芯片并初始化 norflash_info（需先配置好 send_cmd/user_ctx）
int norflash_init(norflash_t *flash);

// 基础操作接口
int norflash_read(norflash_t *flash, uint32_t addr, uint8_t *buf, uint32_t len);
int norflash_write(norflash_t *flash, uint32_t addr, const uint8_t *buf,
                   uint32_t len);
int norflash_erase(norflash_t *flash, uint32_t addr, uint32_t len);
int norflash_chip_erase(norflash_t *flash);

int norflash_sleep(norflash_t *flash);
int norflash_wakeup(norflash_t *flash);

#endif // _NORFLASH_H_