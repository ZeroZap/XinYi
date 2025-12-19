#include "norflash.h"
#include <string.h>

// ===================== 片上参数查表示例 =====================
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
    bool support_qspi;
} norflash_param_t;

static const norflash_param_t norflash_param_table[] = {
    // id,      capacity,      erase, page, read, write, 4k, 32k, 64k, chip,
    // qspi
    { 0xEF4017, 8 * 1024 * 1024, 64 * 1024, 256, 0x03, 0x02, 0x20, 0x52, 0xD8,
      0xC7, true }, // W25Q64
    { 0xC84016, 4 * 1024 * 1024, 64 * 1024, 256, 0x03, 0x02, 0x20, 0x52, 0xD8,
      0x60, false }, // GD25Q32
    // ...可扩展更多型号
};
#define PARAM_TABLE_SIZE \
    (sizeof(norflash_param_table) / sizeof(norflash_param_table[0]))

// ====================== 辅助函数 ======================
static uint32_t norflash_read_id(norflash_t *flash)
{
    uint8_t id_buf[3] = { 0 };
    flash->send_cmd(flash->user_ctx, 0x9F, 0, NULL, id_buf, 3);
    return (id_buf[0] << 16) | (id_buf[1] << 8) | id_buf[2];
}

// SFDP读取与解析（这里只演示基础部分）
static int norflash_sfdp_read(norflash_t *flash, norflash_info_t *info)
{
    uint8_t sfdp_header[8] = { 0 };
    flash->send_cmd(flash->user_ctx, 0x5A, 0x000000, NULL, sfdp_header, 8);
    if (memcmp(sfdp_header, "SFDP", 4) != 0)
        return -1; // 无SFDP
    info->sfdp_major_rev = sfdp_header[6];
    info->sfdp_minor_rev = sfdp_header[5];
    // 基础参数表
    uint32_t param_tbl_addr =
        (sfdp_header[7] << 16) | (sfdp_header[4] << 8) | sfdp_header[3];
    uint8_t param_tbl[64] = { 0 };
    flash->send_cmd(flash->user_ctx, 0x5A, param_tbl_addr, NULL, param_tbl, 64);

    // 容量（JESD216 BFP Table: DWORD2 [bit 0~31]，bit单位）
    uint32_t density = param_tbl[7] | (param_tbl[8] << 8) | (param_tbl[9] << 16)
                       | (param_tbl[10] << 24);
    if (density & 0x80000000)
        info->capacity = 1 << ((density & 0x7FFFFFFF) - 3);
    else
        info->capacity = (density + 1) / 8;
    info->cmd_read          = 0x03;
    info->cmd_write         = 0x02;
    info->cmd_erase_4k      = param_tbl[28];
    info->cmd_erase_32k     = param_tbl[29];
    info->cmd_erase_64k     = param_tbl[30];
    info->cmd_chip_erase    = param_tbl[31];
    info->erase_size        = 4096;
    info->page_size         = 256;
    info->support_4k_erase  = (info->cmd_erase_4k != 0);
    info->support_32k_erase = (info->cmd_erase_32k != 0);
    info->support_64k_erase = (info->cmd_erase_64k != 0);
    info->support_qspi      = false; // 如需进一步解析可扩展
    info->support_sfdp      = true;
    return 0;
}

static int norflash_lookup_table(uint32_t id, norflash_info_t *info)
{
    for (size_t i = 0; i < PARAM_TABLE_SIZE; i++) {
        if (norflash_param_table[i].id == id) {
            info->id                = id;
            info->capacity          = norflash_param_table[i].capacity;
            info->erase_size        = norflash_param_table[i].erase_size;
            info->page_size         = norflash_param_table[i].page_size;
            info->cmd_read          = norflash_param_table[i].cmd_read;
            info->cmd_write         = norflash_param_table[i].cmd_write;
            info->cmd_erase_4k      = norflash_param_table[i].cmd_erase_4k;
            info->cmd_erase_32k     = norflash_param_table[i].cmd_erase_32k;
            info->cmd_erase_64k     = norflash_param_table[i].cmd_erase_64k;
            info->cmd_chip_erase    = norflash_param_table[i].cmd_chip_erase;
            info->support_4k_erase  = (info->cmd_erase_4k != 0);
            info->support_32k_erase = (info->cmd_erase_32k != 0);
            info->support_64k_erase = (info->cmd_erase_64k != 0);
            info->support_qspi      = norflash_param_table[i].support_qspi;
            info->support_sfdp      = false;
            return 0;
        }
    }
    return -1;
}

// ===================== 主初始化流程 =====================
int norflash_init(norflash_t *flash)
{
    if (!flash || !flash->send_cmd)
        return -1;

    uint32_t id    = norflash_read_id(flash);
    flash->info.id = id;

    // 尝试SFDP
    if (norflash_sfdp_read(flash, &flash->info) == 0) {
        flash->inited = true;
        return 0;
    }
    // 查表
    if (norflash_lookup_table(id, &flash->info) == 0) {
        flash->inited = true;
        return 0;
    }
    // 失败
    flash->inited = false;
    return -1;
}

// ===================== 基础操作实现 =====================
int norflash_read(norflash_t *flash, uint32_t addr, uint8_t *buf, uint32_t len)
{
    if (!flash || !flash->inited)
        return -1;
    uint8_t cmd = flash->info.cmd_read;
    while (len > 0) {
        uint32_t chunk = (len > 256) ? 256 : len;
        flash->send_cmd(flash->user_ctx, cmd, addr, NULL, buf, chunk);
        addr += chunk;
        buf += chunk;
        len -= chunk;
    }
    return 0;
}

int norflash_write(norflash_t *flash, uint32_t addr, const uint8_t *buf,
                   uint32_t len)
{
    if (!flash || !flash->inited)
        return -1;
    uint8_t cmd        = flash->info.cmd_write;
    uint32_t page_size = flash->info.page_size;
    while (len > 0) {
        uint32_t page_offset = addr % page_size;
        uint32_t chunk       = page_size - page_offset;
        if (chunk > len)
            chunk = len;
        flash->write_enable(flash->user_ctx);
        flash->send_cmd(flash->user_ctx, cmd, addr, buf, NULL, chunk);
        flash->wait_ready(flash->user_ctx);
        addr += chunk;
        buf += chunk;
        len -= chunk;
    }
    return 0;
}

int norflash_erase(norflash_t *flash, uint32_t addr, uint32_t len)
{
    if (!flash || !flash->inited)
        return -1;
    uint32_t remain     = len;
    uint32_t erase_size = 0;
    uint8_t erase_cmd   = 0;

    if (flash->info.support_4k_erase) {
        erase_size = 4096;
        erase_cmd  = flash->info.cmd_erase_4k;
    } else if (flash->info.support_64k_erase) {
        erase_size = 64 * 1024;
        erase_cmd  = flash->info.cmd_erase_64k;
    } else if (flash->info.support_32k_erase) {
        erase_size = 32 * 1024;
        erase_cmd  = flash->info.cmd_erase_32k;
    } else {
        return -1;
    }

    while (remain > 0) {
        flash->write_enable(flash->user_ctx);
        flash->send_cmd(flash->user_ctx, erase_cmd, addr, NULL, NULL, 0);
        flash->wait_ready(flash->user_ctx);
        addr += erase_size;
        remain -= erase_size;
    }
    return 0;
}

int norflash_chip_erase(norflash_t *flash)
{
    if (!flash || !flash->inited)
        return -1;
    if (flash->info.cmd_chip_erase == 0)
        return -1;
    flash->write_enable(flash->user_ctx);
    flash->send_cmd(
        flash->user_ctx, flash->info.cmd_chip_erase, 0, NULL, NULL, 0);
    flash->wait_ready(flash->user_ctx);
    return 0;
}

int norflash_sleep(norflash_t *flash)
{
    if (!flash || !flash->inited)
        return -1;
    // Deep Power Down 指令（0xB9）
    flash->send_cmd(flash->user_ctx, 0xB9, 0, NULL, NULL, 0);
    // 通常无需等待 ready
    return 0;
}

int norflash_wakeup(norflash_t *flash)
{
    if (!flash || !flash->inited)
        return -1;
    // Release from Deep Power Down 指令（0xAB）
    // 有的芯片唤醒后需要等待~30us
    flash->send_cmd(flash->user_ctx, 0xAB, 0, NULL, NULL, 0);
    // 可选：延时一段时间，具体见数据手册
    return 0;
}