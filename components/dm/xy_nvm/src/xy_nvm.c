/**
 * @file xy_nvm.c
 * @brief NVM Key-Value Storage Implementation
 * @version 1.0.0
 * @date 2026-03-05
 */

#include "xy_nvm.h"
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG
#include "xy_log.h"

/* KV 头结构 */
typedef struct {
    uint32_t head;      // 头标志 (0xAA55AA55)
    uint8_t key_id;     // 键 ID
    uint8_t is_en;      // 使能标志 (0xFF=有效)
    uint16_t len;       // 数据长度
    uint8_t sum;        // 校验和
} kv_header_t;

#define KV_HEAD_MAGIC   0xAA55AA55
#define KV_HEAD_SIZE    ((uintptr_t)sizeof(kv_header_t))

/**
 * @brief 计算校验和
 */
static uint8_t calc_checksum(const kv_header_t *hdr, const uint8_t *data)
{
    uint16_t sum = hdr->key_id + hdr->is_en + hdr->len;
    for (uint16_t i = 0; i < hdr->len; i++) {
        sum += data[i];
    }
    return (uint8_t)(sum & 0xFF);
}

/**
 * @brief Flash 读取 (模拟)
 */
static xy_nvm_status_t storage_read(const xy_nvm_t *nvm, uintptr_t addr, void *buf, size_t len)
{
    if (nvm->config.storage_ops) {
        size_t offset = (size_t)(addr - (uintptr_t)nvm->config.flash_base);
        return nvm->config.storage_ops->read(nvm->config.storage_context, offset, buf, len);
    }
    /* 实际实现需要调用底层 Flash 读取 */
    memcpy(buf, (const void *)addr, len);
    return XY_NVM_OK;
}

/**
 * @brief Flash 写入 (模拟)
 */
static xy_nvm_status_t storage_write(const xy_nvm_t *nvm, uintptr_t addr, const void *buf,
                                     size_t len)
{
    if (nvm->config.storage_ops) {
        size_t offset = (size_t)(addr - (uintptr_t)nvm->config.flash_base);
        return nvm->config.storage_ops->write(nvm->config.storage_context, offset, buf, len);
    }
    /* 实际实现需要调用底层 Flash 写入 */
    memcpy((void *)addr, buf, len);
    return XY_NVM_OK;
}

/**
 * @brief Flash 擦除 (模拟)
 */
static xy_nvm_status_t storage_erase(const xy_nvm_t *nvm, uintptr_t addr, size_t len)
{
    if (nvm->config.storage_ops) {
        size_t offset = (size_t)(addr - (uintptr_t)nvm->config.flash_base);
        return nvm->config.storage_ops->erase(nvm->config.storage_context, offset, len);
    }
    /* 实际实现需要调用底层 Flash 擦除 */
    memset((void *)addr, 0xFF, len);
    return XY_NVM_OK;
}

/**
 * @brief 初始化 NVM
 */
xy_nvm_status_t xy_nvm_init(xy_nvm_t *nvm, const xy_nvm_config_t *cfg)
{
    if (!nvm || !cfg || !cfg->flash_base || cfg->page_size == 0U || cfg->num_pages == 0U ||
        (cfg->storage_ops && (!cfg->storage_ops->read || !cfg->storage_ops->write ||
                             !cfg->storage_ops->erase))) {
        return XY_NVM_ERROR_INVALID_PARAM;
    }
    
    memset(nvm, 0, sizeof(*nvm));
    nvm->config = *cfg;
    nvm->initialized = true;
    
    xy_log_i("NVM initialized: pages=%d\n", cfg->num_pages);
    
    return XY_NVM_OK;
}

/**
 * @brief 反初始化 NVM
 */
xy_nvm_status_t xy_nvm_deinit(xy_nvm_t *nvm)
{
    if (!nvm || !nvm->initialized) {
        return XY_NVM_ERROR_INVALID_PARAM;
    }
    
    nvm->initialized = false;
    return XY_NVM_OK;
}

/**
 * @brief 查找 KV 地址
 */
static uintptr_t find_kv_addr(xy_nvm_t *nvm, uint8_t key_id)
{
    uintptr_t addr = (uintptr_t)nvm->config.flash_base;
    uintptr_t end_addr = addr + (uintptr_t)nvm->config.page_size * nvm->config.num_pages;
    uintptr_t latest_addr = 0;
    
    while (addr < end_addr) {
        kv_header_t hdr;
        if (storage_read(nvm, addr, &hdr, sizeof(hdr)) != XY_NVM_OK) {
            return 0;
        }

        /* 检查头标志 */
        if (hdr.head != KV_HEAD_MAGIC) {
            addr += 4;
            continue;
        }
        
        if (hdr.len > XY_NVM_MAX_DATA_LEN || addr + KV_HEAD_SIZE + hdr.len > end_addr) {
            addr += KV_HEAD_SIZE;
            continue;
        }

        /* 检查校验和 */
        const uint8_t *data = (const uint8_t *)(addr + KV_HEAD_SIZE);
        if (calc_checksum(&hdr, data) != hdr.sum) {
            addr += KV_HEAD_SIZE;
            continue;
        }
        
        /* 检查键 ID 和使能 */
        if (hdr.key_id == key_id && hdr.is_en == 0xFF) {
            latest_addr = addr;
        }
        
        /* 移动到下一个 KV */
        addr += KV_HEAD_SIZE + hdr.len;
        if (addr % 4 != 0) {
            addr += 4 - (addr % 4);
        }
    }
    
    return latest_addr;
}

/**
 * @brief 读取 KV (返回实体)
 */
xy_nvm_result_t xy_nvm_get(xy_nvm_t *nvm, uint8_t key_id)
{
    xy_nvm_result_t result = {0};
    
    if (!nvm || !nvm->initialized) {
        result.status = XY_NVM_ERROR_INVALID_PARAM;
        return result;
    }
    
    /* 查找地址 */
    uintptr_t addr = find_kv_addr(nvm, key_id);
    if (addr == 0) {
        result.status = XY_NVM_ERROR_NOT_FOUND;
        return result;
    }
    
    /* 读取头 */
    kv_header_t hdr;
    if (storage_read(nvm, addr, &hdr, sizeof(hdr)) != XY_NVM_OK) {
        result.status = XY_NVM_ERROR;
        return result;
    }
    
    /* 读取数据部分 */
    uintptr_t data_addr = addr + KV_HEAD_SIZE;
    if (storage_read(nvm, data_addr, result.data, hdr.len) != XY_NVM_OK) {
        result.status = XY_NVM_ERROR;
        return result;
    }
    
    result.len = hdr.len;
    result.status = XY_NVM_OK;
    
    return result;
}

/**
 * @brief 写入 KV
 */
xy_nvm_status_t xy_nvm_set(xy_nvm_t *nvm, uint8_t key_id, 
                           const uint8_t *data, uint16_t len)
{
    if (!nvm || !nvm->initialized || !data || len > XY_NVM_MAX_DATA_LEN) {
        return XY_NVM_ERROR_INVALID_PARAM;
    }
    
    /* 构建 KV 头 */
    kv_header_t hdr;
    hdr.head = KV_HEAD_MAGIC;
    hdr.key_id = key_id;
    hdr.is_en = 0xFF;
    hdr.len = len;
    hdr.sum = calc_checksum(&hdr, data);
    
    /* 查找写入位置 */
    uintptr_t addr = (uintptr_t)nvm->config.flash_base;
    uintptr_t end_addr = addr + (uintptr_t)nvm->config.page_size * nvm->config.num_pages;
    
    while (addr < end_addr) {
        kv_header_t existing;
        xy_nvm_status_t status = storage_read(nvm, addr, &existing, sizeof(existing));
        if (status != XY_NVM_OK) {
            return status;
        }
        
        /* 找到空闲空间 */
        if (existing.head == 0xFFFFFFFF) {
            if (addr + KV_HEAD_SIZE + len > end_addr) {
                return XY_NVM_ERROR_FULL;
            }

            /* 写入头 */
            status = storage_write(nvm, addr, &hdr, sizeof(hdr));
            if (status != XY_NVM_OK) {
                return status;
            }

            /* 写入数据部分 */
            status = storage_write(nvm, addr + KV_HEAD_SIZE, data, len);
            if (status != XY_NVM_OK) {
                return status;
            }

            return XY_NVM_OK;
        }

        /* 跳过完整记录；按 4 字节扫描无法解释的 torn header。 */
        if (existing.head != KV_HEAD_MAGIC) {
            addr += 4U;
            continue;
        }
        if (existing.len > XY_NVM_MAX_DATA_LEN || addr + KV_HEAD_SIZE + existing.len > end_addr) {
            addr += KV_HEAD_SIZE;
            continue;
        }

        addr += KV_HEAD_SIZE + existing.len;
        /* 对齐到 4 字节 */
        if (addr % 4 != 0) {
            addr += 4 - (addr % 4);
        }
    }
    
    return XY_NVM_ERROR_FULL;
}

/**
 * @brief 删除 KV
 */
xy_nvm_status_t xy_nvm_delete(xy_nvm_t *nvm, uint8_t key_id)
{
    if (!nvm || !nvm->initialized) {
        return XY_NVM_ERROR_INVALID_PARAM;
    }
    
    /* 查找地址 */
    uintptr_t addr = find_kv_addr(nvm, key_id);
    if (addr == 0) {
        return XY_NVM_ERROR_NOT_FOUND;
    }
    
    /* 标记为删除 (is_en = 0x00) */
    uint8_t is_en = 0x00;
    xy_nvm_status_t status =
        storage_write(nvm, addr + offsetof(kv_header_t, is_en), &is_en, 1);
    if (status != XY_NVM_OK) {
        return status;
    }
    
    return XY_NVM_OK;
}

/**
 * @brief 格式化 NVM
 */
xy_nvm_status_t xy_nvm_format(xy_nvm_t *nvm)
{
    if (!nvm || !nvm->initialized) {
        return XY_NVM_ERROR_INVALID_PARAM;
    }
    
    uintptr_t addr = (uintptr_t)nvm->config.flash_base;
    size_t size = (size_t)nvm->config.page_size * nvm->config.num_pages;
    
    /* 擦除所有区域 */
    xy_nvm_status_t status = storage_erase(nvm, addr, size);
    if (status != XY_NVM_OK) {
        return status;
    }
    
    xy_log_i("NVM formatted\n");
    return XY_NVM_OK;
}

/**
 * @brief 获取使用统计
 */
void xy_nvm_get_stats(xy_nvm_t *nvm, uint16_t *used, uint16_t *free)
{
    if (!nvm || !nvm->initialized) {
        return;
    }
    
    uintptr_t addr = (uintptr_t)nvm->config.flash_base;
    uintptr_t end_addr = addr + (uintptr_t)nvm->config.page_size * nvm->config.num_pages;
    uint16_t used_bytes = 0;
    
    while (addr < end_addr) {
        kv_header_t hdr;
        if (storage_read(nvm, addr, &hdr, sizeof(hdr)) != XY_NVM_OK) {
            return;
        }
        
        if (hdr.head == 0xFFFFFFFF) {
            break;
        }
        
        if (hdr.len > XY_NVM_MAX_DATA_LEN || addr + KV_HEAD_SIZE + hdr.len > end_addr) {
            break;
        }

        used_bytes += (uint16_t)(KV_HEAD_SIZE + hdr.len);
        
        /* 对齐到 4 字节 */
        addr += KV_HEAD_SIZE + hdr.len;
        if (addr % 4 != 0) {
            addr += 4 - (addr % 4);
        }
    }
    
    uint16_t total = (uint16_t)(nvm->config.page_size * nvm->config.num_pages);
    
    if (used) *used = used_bytes;
    if (free) *free = total - used_bytes;
}
