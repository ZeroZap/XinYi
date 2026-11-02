/**
 * @file at_urc.c
 * @brief 动态URC解析实现
 *
 * 设计约束:
 * - 每个设备一个 URC 管理器，按设备指针在内部静态表中索引，容量与
 *   at_client 一致(AT_MAX_DEVICES)。
 * - 绝不改写 dev->user_data: 该字段由调用方持有(串口/环形缓冲区句柄)，
 *   at_client 的设备读写回调依赖它。
 * - 内建静默 URC(ATE/RDY)只在用户处理器全部不命中后才生效，用户优先。
 */

#include "at_urc.h"

#include <stdlib.h>
#include <string.h>

#ifdef USE_REGEX
#include <regex.h>
#endif

/* 单行最大长度(含结尾'\0')；超长行截断而不是拆分 */
#define AT_URC_LINE_MAX 256U

/* 内建静默 URC: 无用户处理器命中时兜底，不产生任何输出，仅计为已处理 */
#define AT_URC_BUILTIN_ECHO "ATE"
#define AT_URC_BUILTIN_READY "RDY"

/* 设备槽位: dev 为 NULL 表示空闲槽 */
typedef struct {
    at_device_t *dev;
    at_urc_manager_t manager;
} at_urc_slot_t;

static at_urc_slot_t g_urc_slots[AT_MAX_DEVICES];

/* 匹配函数 */
static bool urc_process_line(at_device_t *dev, const char *line,
                             at_urc_manager_t *mgr);
static bool urc_line_matches(const char *line, const at_urc_handler_t *handler);
static bool urc_match_prefix(const char *data, const char *pattern);
static bool urc_match_suffix(const char *data, const char *pattern);
static bool urc_match_contain(const char *data, const char *pattern);
static bool urc_match_exact(const char *data, const char *pattern);
#ifdef USE_REGEX
static bool urc_match_regex(const char *data, const char *pattern);
#endif

/* 可移植的字符串复制(C99 没有 strdup) */
static char *urc_strdup(const char *src)
{
    size_t len = strlen(src) + 1U;
    char *dst  = (char *)malloc(len);

    if (!dst) {
        return NULL;
    }

    memcpy(dst, src, len);
    return dst;
}

static at_urc_slot_t *urc_slot_find(const at_device_t *dev)
{
    if (!dev) {
        return NULL;
    }

    for (size_t i = 0; i < AT_MAX_DEVICES; i++) {
        if (g_urc_slots[i].dev == dev) {
            return &g_urc_slots[i];
        }
    }

    return NULL;
}

static at_urc_slot_t *urc_slot_claim(at_device_t *dev)
{
    at_urc_slot_t *slot = urc_slot_find(dev);

    if (slot) {
        return slot;
    }

    for (size_t i = 0; i < AT_MAX_DEVICES; i++) {
        if (!g_urc_slots[i].dev) {
            memset(&g_urc_slots[i], 0, sizeof(g_urc_slots[i]));
            g_urc_slots[i].dev = dev;
            return &g_urc_slots[i];
        }
    }

    return NULL;
}

static void urc_handlers_free(at_urc_manager_t *mgr)
{
    at_urc_handler_t *handler = mgr->handlers;

    while (handler) {
        at_urc_handler_t *next = handler->next;

        free(handler->pattern);
        free(handler);
        handler = next;
    }

    mgr->handlers      = NULL;
    mgr->handler_count = 0;
}

/* 初始化URC管理器(幂等；不占用 dev->user_data) */
int at_urc_init(at_device_t *dev)
{
    at_urc_slot_t *slot;

    if (!dev) {
        return -1;
    }

    if (urc_slot_find(dev)) {
        return 0; /* 已初始化: 保持既有处理器 */
    }

    slot = urc_slot_claim(dev);
    if (!slot) {
        return -1; /* 设备槽位已满 */
    }

    slot->manager.handlers      = NULL;
    slot->manager.handler_count = 0;
    slot->manager.urc_total     = 0;
    slot->manager.urc_handled   = 0;
    slot->manager.next_id       = 0;
    slot->manager.processing    = false;

    return 0;
}

void at_urc_deinit(at_device_t *dev)
{
    at_urc_slot_t *slot = urc_slot_find(dev);

    if (!slot) {
        return;
    }

    urc_handlers_free(&slot->manager);

    /* 释放槽位，设备可再次 at_urc_init() */
    memset(slot, 0, sizeof(*slot));
}

/* 注册URC处理器 */
int at_urc_register(at_device_t *dev, const char *pattern,
                    urc_match_type_t match_type, urc_callback_t callback,
                    void *user_data)
{
    at_urc_slot_t *slot;
    at_urc_handler_t *handler;
    at_urc_handler_t *last;

    if (!dev || !pattern || !callback) {
        return -1;
    }

    slot = urc_slot_find(dev);
    if (!slot) {
        return -1; /* 必须先 at_urc_init() */
    }

    handler = (at_urc_handler_t *)calloc(1, sizeof(*handler));
    if (!handler) {
        return -1;
    }

    handler->pattern = urc_strdup(pattern);
    if (!handler->pattern) {
        free(handler);
        return -1;
    }

    handler->id         = slot->manager.next_id++;
    handler->match_type = match_type;
    handler->callback   = callback;
    handler->user_data  = user_data;
    handler->enabled    = true;
    handler->hit_count  = 0;
    handler->next       = NULL;

    /* 添加到链表尾部，保持注册顺序(即匹配优先级) */
    if (!slot->manager.handlers) {
        slot->manager.handlers = handler;
    } else {
        last = slot->manager.handlers;
        while (last->next) {
            last = last->next;
        }
        last->next = handler;
    }

    slot->manager.handler_count++;
    return handler->id;
}

int at_urc_unregister(at_device_t *dev, int handler_id)
{
    at_urc_slot_t *slot = urc_slot_find(dev);
    at_urc_handler_t **link;

    if (!slot) {
        return -1;
    }

    link = &slot->manager.handlers;
    while (*link) {
        if ((*link)->id == handler_id) {
            at_urc_handler_t *dead = *link;

            *link = dead->next;
            free(dead->pattern);
            free(dead);

            if (slot->manager.handler_count > 0) {
                slot->manager.handler_count--;
            }

            return 0;
        }
        link = &(*link)->next;
    }

    return -1;
}

int at_urc_set_enable(at_device_t *dev, int handler_id, bool enable)
{
    at_urc_slot_t *slot = urc_slot_find(dev);
    at_urc_handler_t *handler;

    if (!slot) {
        return -1;
    }

    for (handler = slot->manager.handlers; handler; handler = handler->next) {
        if (handler->id == handler_id) {
            handler->enabled = enable;
            return 0;
        }
    }

    return -1;
}

bool at_urc_process(at_device_t *dev, const char *data, uint32_t len)
{
    at_urc_slot_t *slot;
    at_urc_manager_t *mgr;
    char line[AT_URC_LINE_MAX];
    uint32_t line_idx = 0;
    bool processed    = false;

    if (!dev || !data || len == 0U) {
        return false;
    }

    slot = urc_slot_find(dev);
    if (!slot) {
        return false;
    }

    mgr = &slot->manager;
    if (mgr->processing) {
        return false; /* 重入保护: 回调内不再遍历链表 */
    }

    mgr->processing = true;

    /* 按行分割处理 */
    for (uint32_t i = 0; i < len; i++) {
        if (data[i] == '\r' || data[i] == '\n') {
            if (line_idx > 0U) {
                line[line_idx] = '\0';
                mgr->urc_total++;
                if (urc_process_line(dev, line, mgr)) {
                    mgr->urc_handled++;
                    processed = true;
                }
                line_idx = 0;
            }
        } else if (line_idx < (AT_URC_LINE_MAX - 1U)) {
            line[line_idx++] = data[i];
        }
        /* 超出缓冲区的字节丢弃: 该行按截断内容匹配 */
    }

    /* 处理末尾没有换行的残行 */
    if (line_idx > 0U) {
        line[line_idx] = '\0';
        mgr->urc_total++;
        if (urc_process_line(dev, line, mgr)) {
            mgr->urc_handled++;
            processed = true;
        }
    }

    mgr->processing = false;
    return processed;
}

/* 处理单行URC数据: 首个命中的处理器生效，用户处理器优先于内建判定 */
static bool urc_process_line(at_device_t *dev, const char *line,
                             at_urc_manager_t *mgr)
{
    at_urc_handler_t *handler = mgr->handlers;

    while (handler) {
        if (handler->enabled && urc_line_matches(line, handler)) {
            handler->hit_count++;

            if (handler->callback) {
                handler->callback(dev, line, handler->user_data);
            }

            return true; /* 匹配成功后停止，避免重复处理 */
        }
        handler = handler->next;
    }

    /* 内建静默 URC: 仅在没有用户处理器命中时兜底，不产生任何输出 */
    if (urc_match_prefix(line, AT_URC_BUILTIN_ECHO) ||
        urc_match_exact(line, AT_URC_BUILTIN_READY)) {
        return true;
    }

    return false;
}

static bool urc_line_matches(const char *line, const at_urc_handler_t *handler)
{
    switch (handler->match_type) {
    case URC_MATCH_PREFIX:
        return urc_match_prefix(line, handler->pattern);
    case URC_MATCH_SUFFIX:
        return urc_match_suffix(line, handler->pattern);
    case URC_MATCH_CONTAIN:
        return urc_match_contain(line, handler->pattern);
    case URC_MATCH_EXACT:
        return urc_match_exact(line, handler->pattern);
#ifdef USE_REGEX
    case URC_MATCH_REGEX:
        return urc_match_regex(line, handler->pattern);
#endif
    default:
        /* 未启用 USE_REGEX 时 URC_MATCH_REGEX 不匹配任何行(不静默降级为前缀) */
        return false;
    }
}

/* 匹配函数实现 */
static bool urc_match_prefix(const char *data, const char *pattern)
{
    return strncmp(data, pattern, strlen(pattern)) == 0;
}

static bool urc_match_suffix(const char *data, const char *pattern)
{
    size_t data_len    = strlen(data);
    size_t pattern_len = strlen(pattern);

    if (pattern_len > data_len) {
        return false;
    }

    return strcmp(data + data_len - pattern_len, pattern) == 0;
}

static bool urc_match_contain(const char *data, const char *pattern)
{
    return strstr(data, pattern) != NULL;
}

static bool urc_match_exact(const char *data, const char *pattern)
{
    return strcmp(data, pattern) == 0;
}

#ifdef USE_REGEX
static bool urc_match_regex(const char *data, const char *pattern)
{
    regex_t regex;
    int ret;

    ret = regcomp(&regex, pattern, REG_EXTENDED | REG_NOSUB);
    if (ret) {
        return false;
    }

    ret = regexec(&regex, data, 0, NULL, 0);
    regfree(&regex);

    return ret == 0;
}
#endif

/* URC后台处理任务（RTOS） */
void at_urc_task(void *param)
{
    at_device_t *dev = (at_device_t *)param;

    while (1) {
        char buffer[AT_URC_LINE_MAX];
        int len = at_readline(dev, buffer, sizeof(buffer));

        if (len > 0) {
            at_urc_process(dev, buffer, (uint32_t)len);
        }

#ifdef USE_RTOS
        vTaskDelay(pdMS_TO_TICKS(10));
#else
        /* 裸机: 由调用方提供节拍/延迟 */
#endif
    }
}

/* URC轮询处理（裸机） */
void at_urc_poll(at_device_t *dev)
{
    char buffer[AT_URC_LINE_MAX];
    int len;

    if (!dev) {
        return;
    }

    len = at_readline(dev, buffer, sizeof(buffer));

    if (len > 0) {
        at_urc_process(dev, buffer, (uint32_t)len);
    }
}

void at_urc_get_stats(at_device_t *dev, uint32_t *total, uint32_t *handled)
{
    at_urc_slot_t *slot;

    if (total) {
        *total = 0;
    }
    if (handled) {
        *handled = 0;
    }

    slot = urc_slot_find(dev);
    if (!slot) {
        return;
    }

    if (total) {
        *total = slot->manager.urc_total;
    }
    if (handled) {
        *handled = slot->manager.urc_handled;
    }
}

void at_urc_clear(at_device_t *dev)
{
    at_urc_slot_t *slot = urc_slot_find(dev);

    if (!slot) {
        return;
    }

    urc_handlers_free(&slot->manager);
}
