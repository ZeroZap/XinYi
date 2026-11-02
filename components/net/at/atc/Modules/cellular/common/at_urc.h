/**
 * @file at_urc.h
 * @brief AT客户端动态URC解析扩展
 */

#ifndef AT_URC_H
#define AT_URC_H

#include "at_client.h"

/* URC匹配模式类型 */
typedef enum {
    URC_MATCH_PREFIX = 0, /* 前缀匹配（默认） */
    URC_MATCH_SUFFIX,     /* 后缀匹配 */
    URC_MATCH_CONTAIN,    /* 包含匹配 */
    URC_MATCH_EXACT,      /* 精确匹配 */
    URC_MATCH_REGEX       /* 正则表达式匹配 */
} urc_match_type_t;

/* URC回调函数类型 */
typedef void (*urc_callback_t)(at_device_t *dev, const char *data,
                               void *user_data);

/* URC处理项 */
typedef struct at_urc_handler {
    int id;                      /* 处理器ID(注册时分配)；注销/启停用它定位 */
    char *pattern;               /* 匹配模式 */
    urc_match_type_t match_type; /* 匹配类型 */
    urc_callback_t callback;     /* 回调函数 */
    void *user_data;             /* 用户数据 */
    bool enabled;                /* 是否启用 */
    uint32_t hit_count;          /* 命中次数 */
    struct at_urc_handler *next; /* 下一个 */
} at_urc_handler_t;

/* URC管理器 */
typedef struct {
    at_urc_handler_t *handlers; /* URC处理器链表 */
    uint16_t handler_count;     /* 处理器数量 */
    uint32_t urc_total;         /* 收到的行数 */
    uint32_t urc_handled;       /* 命中的行数 */
    int next_id;                /* 下一个处理器ID */
    bool processing;            /* 是否正在处理(重入保护) */
} at_urc_manager_t;

/* API函数声明 */

/**
 * @brief 初始化URC管理器
 *
 * 每个设备占一个内部槽位，设备指针即句柄。设备数量上限与 at_client 一致
 * (AT_MAX_DEVICES)。重复初始化是幂等的。
 *
 * 注意：本模块不占用 dev->user_data，该字段由调用方持有(串口/环形缓冲区
 * 句柄)，at_client 的设备读写回调依赖它。
 */
int at_urc_init(at_device_t *dev);

/**
 * @brief 释放URC管理器及该设备占用的槽位
 *
 * 清空所有处理器并让设备可被重新 at_urc_init()。传 NULL 或无槽位时为空操作。
 */
void at_urc_deinit(at_device_t *dev);

/**
 * @brief 注册URC处理器
 *
 * 处理器按注册顺序串行匹配，首个命中者生效(命中后不再尝试后续处理器)。
 * 内建静默 URC(ATE 前缀、RDY 精确)只在所有用户处理器都不命中时才生效，
 * 因此用户总是可以覆盖内建的 ATE/RDY 判定。
 * @param dev AT设备(必须已 at_urc_init())
 * @param pattern 匹配模式
 * @param match_type 匹配类型
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return 处理器ID(从0起，单调分配)，参数非法/未初始化/内存不足返回-1
 */
int at_urc_register(at_device_t *dev, const char *pattern,
                    urc_match_type_t match_type, urc_callback_t callback,
                    void *user_data);

/**
 * @brief 注销URC处理器
 * @return 0成功；ID不存在或设备未初始化返回-1
 */
int at_urc_unregister(at_device_t *dev, int handler_id);

/**
 * @brief 启用/禁用URC处理器
 *
 * 禁用后处理器仍保留在链表中(保持相对顺序与ID)，但不参与匹配。
 * @return 0成功；ID不存在或设备未初始化返回-1
 */
int at_urc_set_enable(at_device_t *dev, int handler_id, bool enable);

/**
 * @brief 处理URC数据
 *
 * 按 '\r'/'\n' 切行，空行忽略；末尾无换行的残行同样处理。单行超过255字节
 * 时截断(超出部分丢弃)，不会越界也不会把一行拆成多行。
 * 处理期间回调内再次调用本函数会直接被拒(false)，避免链表中途被重入遍历。
 * @param dev AT设备
 * @param data 接收到的数据
 * @param len 数据长度
 * @return 是否有至少一行被处理
 */
bool at_urc_process(at_device_t *dev, const char *data, uint32_t len);

/**
 * @brief URC后台处理任务（RTOS环境）
 */
void at_urc_task(void *param);

/**
 * @brief URC轮询处理（裸机环境）
 *
 * 从设备读一行并交给 at_urc_process()；无数据时不做任何事。
 */
void at_urc_poll(at_device_t *dev);

/**
 * @brief 获取URC统计信息
 * @param total 收到的行数(可为NULL)
 * @param handled 命中处理的行数(可为NULL)
 */
void at_urc_get_stats(at_device_t *dev, uint32_t *total, uint32_t *handled);

/**
 * @brief 清除所有URC处理器(保留设备槽位与统计)
 */
void at_urc_clear(at_device_t *dev);

#endif /* AT_URC_H */