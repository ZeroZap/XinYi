#ifndef FEE_H
#define FEE_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @file fee.h
 * @brief Flash EEPROM Emulation - 使用Flash模拟EEPROM
 * @version 1.0
 * @date 2024
 *
 * @note 设计约束：
 *       - write_granularity >= 8 (保证至少4字节数据)
 *       - FEE Page Header对齐到write_granularity
 *       - 每条Record = 1个write_granularity
 *       - Cache是虚拟EEPROM的完整镜像
 */

/* ============ 配置常量 ============ */

#define FEE_MIN_GRANULARITY 8   /**< 最小写入颗粒（字节） */
#define FEE_MAX_GRANULARITY 128 /**< 最大写入颗粒（字节） */

/* ============ 页状态定义 ============ */

#define FEE_PAGE_STATE_ERASED    0xFFFFFFFF /**< 擦除态 */
#define FEE_PAGE_STATE_ACTIVE    0x46454541 /**< "FEEA" - 活动页 */
#define FEE_PAGE_STATE_RECEIVING 0x46454552 /**< "FEER" - 接收中（GC） */
#define FEE_PAGE_STATE_INVALID   0x46454549 /**< "FEEI" - 已失效 */

/* ============ 数据结构 ============ */

/**
 * @brief FEE Page头部（基础8字节，实际占用会对齐到write_granularity）
 */
typedef struct __attribute__((packed)) {
    uint32_t page_state;  /**< 页状态（magic + state合一） */
    uint16_t erase_count; /**< 擦除次数 */
    uint16_t header_crc;  /**< 头部CRC16校验 */
} fee_page_header_t;

#define FEE_PAGE_HEADER_BASE_SIZE 8

/**
 * @brief Record头部（固定4字节）
 */
typedef struct __attribute__((packed)) {
    uint16_t addr; /**< 逻辑地址 */
    uint16_t crc;  /**< 数据CRC16 */
    /* 后面紧跟 (write_granularity - 4) 字节数据 */
} fee_record_header_t;

#define FEE_RECORD_HEADER_SIZE 4

/**
 * @brief 判断Record是否为擦除态
 */
#define FEE_RECORD_IS_ERASED(rec) \
    ((rec)->addr == 0xFFFF && (rec)->crc == 0xFFFF)

/**
 * @brief Flash操作接口
 */
typedef struct {
    /**
     * @brief 擦除Flash页
     * @param addr Flash页起始地址
     * @return 0-成功，非0-失败
     */
    int (*erase)(uint32_t addr);

    /**
     * @brief 写入Flash
     * @param addr 写入地址
     * @param data 数据指针
     * @param len 数据长度（必须是write_granularity的整数倍）
     * @return 0-成功，非0-失败
     */
    int (*write)(uint32_t addr, const uint8_t *data, uint16_t len);

    /**
     * @brief 读取Flash
     * @param addr 读取地址
     * @param data 数据缓冲区
     * @param len 读取长度
     * @return 0-成功，非0-失败
     */
    int (*read)(uint32_t addr, uint8_t *data, uint16_t len);
} fee_flash_ops_t;

/**
 * @brief FEE配置参数
 */
typedef struct {
    uint8_t *flash_base;        /**< Flash基地址 */
    uint8_t pages_per_fee_page; /**< 每个FEE Page包含的Flash page数 */
    uint16_t flash_page_size;   /**< 单个Flash page大小（字节） */
    uint16_t cache_size;        /**< 虚拟EEPROM大小（字节） */
    uint8_t write_granularity;  /**< 写入颗粒（8/16/32/64/128） */
    uint16_t max_erase_count;   /**< 最大擦除次数 */
    const fee_flash_ops_t *flash_ops; /**< Flash操作接口 */
} fee_config_t;

/**
 * @brief FEE句柄（内部状态）
 */
typedef struct {
    /* Flash基础信息 */
    uint8_t *flash_base;
    uint16_t fee_page_size; /**< 单个FEE Page大小 */
    uint8_t pages_per_fee_page;
    uint8_t write_granularity;

    /* 布局信息 */
    uint8_t aligned_header_size; /**< Header对齐后的大小 */
    uint8_t record_data_size;    /**< 每条Record的数据大小 */
    uint16_t data_area_size;     /**< 数据区大小 */

    /* 运行状态 */
    uint16_t cache_size;
    uint16_t write_offset; /**< 当前写入偏移 */
    uint16_t max_erase_count;
    uint8_t active_page; /**< 活动FEE Page (0或1) */
    uint8_t flags;

    /* 缓冲区 */
    uint8_t *cache; /**< 指向虚拟EEPROM数组 */
} fee_handle_t;

#define FEE_FLAG_INIT  (1 << 0)
#define FEE_FLAG_VALID (1 << 1)

/**
 * @brief FEE状态码
 */
typedef enum {
    FEE_OK                = 0,  /**< 成功 */
    FEE_ERROR             = -1, /**< 通用错误 */
    FEE_ERROR_PARAM       = -2, /**< 参数错误 */
    FEE_ERROR_NOT_INIT    = -3, /**< 未初始化 */
    FEE_ERROR_ERASE_LIMIT = -4, /**< 达到擦除次数上限 */
    FEE_ERROR_FLASH       = -5, /**< Flash操作失败 */
    FEE_ERROR_FULL        = -6  /**< 空间已满 */
} fee_status_t;

/* ============ API函数 ============ */

/**
 * @brief 初始化FEE
 *
 * @param handle FEE句柄指针
 * @param config 配置参数
 * @param cache_buffer 虚拟EEPROM数组（用户提供，大小=config->cache_size）
 * @param work_buffer 工作缓冲区（用户提供，大小=FEE_WORK_SIZE(gran)）
 * @return FEE状态码
 *
 * @note 1. 如果Flash中存在有效数据，会自动恢复到cache_buffer
 *       2. 如果Flash为空，会格式化FEE Page 0
 *       3. cache_buffer就是虚拟EEPROM，读写操作直接访问它
 */
fee_status_t fee_init(fee_handle_t *handle, const fee_config_t *config,
                      uint8_t *cache_buffer, uint8_t *work_buffer);

/**
 * @brief 写入数据到虚拟EEPROM
 *
 * @param handle FEE句柄
 * @param addr 逻辑地址
 * @param data 数据指针
 * @param len 数据长度
 * @return FEE状态码
 *
 * @note 1. 会同时写入Flash和更新Cache
 *       2. 如果空间不足，自动触发GC
 *       3. 地址和长度无对齐要求
 */
fee_status_t fee_write(fee_handle_t *handle, uint16_t addr, const uint8_t *data,
                       uint16_t len);

/**
 * @brief 从虚拟EEPROM读取数据
 *
 * @param handle FEE句柄
 * @param addr 逻辑地址
 * @param data 数据缓冲区
 * @param len 读取长度
 * @return FEE状态码
 *
 * @note 直接从Cache读取，不访问Flash（速度极快）
 */
fee_status_t fee_read(fee_handle_t *handle, uint16_t addr, uint8_t *data,
                      uint16_t len);

/**
 * @brief 格式化FEE（擦除所有数据）
 *
 * @param handle FEE句柄
 * @return FEE状态码
 */
fee_status_t fee_format(fee_handle_t *handle);

/**
 * @brief 手动触发垃圾回收
 *
 * @param handle FEE句柄
 * @return FEE状态码
 *
 * @note 通常由fee_write()自动触发，无需手动调用
 */
fee_status_t fee_gc(fee_handle_t *handle);

/**
 * @brief 获取FEE运行信息
 *
 * @param handle FEE句柄
 * @param erase_count 擦除次数（可为NULL）
 * @param free_bytes 剩余空间（字节，可为NULL）
 * @param record_count 当前记录数（可为NULL）
 * @return FEE状态码
 */
fee_status_t fee_get_info(fee_handle_t *handle, uint16_t *erase_count,
                          uint16_t *free_bytes, uint16_t *record_count);

/* ============ 工具宏 ============ */

/**
 * @brief 计算对齐后的Header大小
 */
#define FEE_ALIGNED_HEADER_SIZE(gran) \
    (((FEE_PAGE_HEADER_BASE_SIZE + (gran) - 1) / (gran)) * (gran))

/**
 * @brief 计算每条Record的数据大小
 */
#define FEE_RECORD_DATA_SIZE(gran) ((gran) - FEE_RECORD_HEADER_SIZE)

/**
 * @brief 计算Record总大小（固定等于颗粒）
 */
#define FEE_RECORD_SIZE(gran) (gran)

/**
 * @brief 计算数据区大小
 */
#define FEE_DATA_AREA_SIZE(fee_page_size, gran) \
    ((fee_page_size) - FEE_ALIGNED_HEADER_SIZE(gran))

/**
 * @brief 计算最大Record数
 */
#define FEE_MAX_RECORDS(fee_page_size, gran) \
    (FEE_DATA_AREA_SIZE(fee_page_size, gran) / (gran))

/**
 * @brief 计算工作缓冲区大小
 */
#define FEE_WORK_SIZE(gran) ((gran) * 2)

/**
 * @brief 计算总RAM占用
 */
#define FEE_TOTAL_RAM(cache_size, gran) \
    (sizeof(fee_handle_t) + (cache_size) + FEE_WORK_SIZE(gran))

#endif /* FEE_H */