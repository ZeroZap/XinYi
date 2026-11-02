/**
 * @file test_at_urc_core.c
 * @brief AT URC 分发器 host 契约测试
 *
 * 覆盖 at_urc.h 的公开契约: 设备槽位生命周期、处理器注册/注销/启停、
 * 四种匹配类型的行语义、首个命中生效、内建 ATE/RDY 兜底与用户覆盖、
 * 统计、重入保护、超长行截断，以及 at_urc_poll() 经 at_readline() 的取行路径。
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "unity.h"

#include "at_client.h"
#include "at_urc.h"

/* 以字符串字面量喂一整块数据: 长度取真实 strlen，避免魔法数字越读结尾 NUL */
#define URC_FEED(dev, text) at_urc_process((dev), (text), (uint32_t)strlen(text))

/* 设备槽位是进程级静态表，容量与 at_client 一致；每个用例前后都必须归还 */
static at_device_t g_dev;
static at_device_t g_dev_b;
static at_device_t g_dev_c;
static at_device_t g_dev_d;
static at_device_t g_dev_e;

void setUp(void)
{
    at_urc_deinit(&g_dev);
    at_urc_deinit(&g_dev_b);
    at_urc_deinit(&g_dev_c);
    at_urc_deinit(&g_dev_d);
    at_urc_deinit(&g_dev_e);

    memset(&g_dev, 0, sizeof(g_dev));
    memset(&g_dev_b, 0, sizeof(g_dev_b));
    memset(&g_dev_c, 0, sizeof(g_dev_c));
    memset(&g_dev_d, 0, sizeof(g_dev_d));
    memset(&g_dev_e, 0, sizeof(g_dev_e));
}

void tearDown(void)
{
}

/* --- 回调夹具 ------------------------------------------------------------- */

static int g_hit_a;
static int g_hit_b;
static char g_last_line[512];
static size_t g_last_line_len;

static void handler_a(at_device_t *dev, const char *data, void *user_data)
{
    (void)dev;
    (void)user_data;
    g_hit_a++;
    strncpy(g_last_line, data, sizeof(g_last_line) - 1);
    g_last_line_len = strlen(data);
}

static void handler_b(at_device_t *dev, const char *data, void *user_data)
{
    (void)dev;
    (void)data;
    (void)user_data;
    g_hit_b++;
}

static bool g_nested_result;
static void handler_reentrant(at_device_t *dev, const char *data,
                              void *user_data)
{
    (void)data;
    (void)user_data;
    g_hit_a++;
    /* 处理中再次进入必须被拒绝，且不能破坏链表遍历 */
    g_nested_result = URC_FEED(dev, "+CREG: 9,9\r\n");
}

static void fixtures_reset(void)
{
    g_hit_a         = 0;
    g_hit_b         = 0;
    g_last_line[0]  = '\0';
    g_last_line_len = 0;
    g_nested_result = true;
}

/* --- 生命周期 ------------------------------------------------------------- */

static void test_init_rejects_null(void)
{
    TEST_ASSERT_EQUAL_INT(-1, at_urc_init(NULL));
}

static void test_register_requires_init(void)
{
    fixtures_reset();

    TEST_ASSERT_EQUAL_INT(
        -1, at_urc_register(&g_dev, "+CREG:", URC_MATCH_PREFIX, handler_a, NULL));
}

static void test_init_is_idempotent(void)
{
    fixtures_reset();

    TEST_ASSERT_EQUAL_INT(0, at_urc_init(&g_dev));
    TEST_ASSERT_EQUAL_INT(
        0, at_urc_register(&g_dev, "+CREG:", URC_MATCH_PREFIX, handler_a, NULL));

    /* 二次初始化不清空已有处理器，也不重置 ID 序列 */
    TEST_ASSERT_EQUAL_INT(0, at_urc_init(&g_dev));
    TEST_ASSERT_EQUAL_INT(
        1, at_urc_register(&g_dev, "+CGREG:", URC_MATCH_PREFIX, handler_b, NULL));

    TEST_ASSERT_TRUE(URC_FEED(&g_dev, "+CREG: 1,1\r\n"));
    TEST_ASSERT_EQUAL_INT(1, g_hit_a);
}

static void test_init_does_not_touch_user_data(void)
{
    int bus_handle = 42;

    g_dev.user_data = &bus_handle;

    TEST_ASSERT_EQUAL_INT(0, at_urc_init(&g_dev));

    /* dev->user_data 归调用方所有(总线/环形缓冲区句柄)，不能被 URC 管理器占用 */
    TEST_ASSERT_EQUAL_PTR(&bus_handle, g_dev.user_data);
    TEST_ASSERT_EQUAL_INT(42, *(int *)g_dev.user_data);

    /* 初始化后仍然可以正常注册并分发 */
    fixtures_reset();
    TEST_ASSERT_TRUE(
        at_urc_register(&g_dev, "+CSQ:", URC_MATCH_PREFIX, handler_a, NULL) >= 0);
    TEST_ASSERT_TRUE(URC_FEED(&g_dev, "+CSQ: 20,99\r\n"));
    TEST_ASSERT_EQUAL_INT(1, g_hit_a);
}

static void test_device_slot_capacity(void)
{
    /* 容量与 at_client 一致: AT_MAX_DEVICES 个设备，多出的 fail closed */
    TEST_ASSERT_EQUAL_INT(0, at_urc_init(&g_dev));
    TEST_ASSERT_EQUAL_INT(0, at_urc_init(&g_dev_b));
    TEST_ASSERT_EQUAL_INT(0, at_urc_init(&g_dev_c));
    TEST_ASSERT_EQUAL_INT(0, at_urc_init(&g_dev_d));

    TEST_ASSERT_EQUAL_INT(AT_MAX_DEVICES, 4);
    TEST_ASSERT_EQUAL_INT(-1, at_urc_init(&g_dev_e));
    TEST_ASSERT_EQUAL_INT(
        -1, at_urc_register(&g_dev_e, "X", URC_MATCH_EXACT, handler_a, NULL));

    /* 归还一个槽位后即可再用 */
    at_urc_deinit(&g_dev_b);
    TEST_ASSERT_EQUAL_INT(0, at_urc_init(&g_dev_e));
}

static void test_deinit_releases_slot_and_handlers(void)
{
    fixtures_reset();

    TEST_ASSERT_EQUAL_INT(0, at_urc_init(&g_dev));
    TEST_ASSERT_EQUAL_INT(
        0, at_urc_register(&g_dev, "+CREG:", URC_MATCH_PREFIX, handler_a, NULL));

    at_urc_deinit(&g_dev);

    /* 释放后设备回到未初始化状态: 既不分发也不再接受注册 */
    TEST_ASSERT_FALSE(URC_FEED(&g_dev, "+CREG: 1,1\r\n"));
    TEST_ASSERT_EQUAL_INT(
        -1, at_urc_register(&g_dev, "+CREG:", URC_MATCH_PREFIX, handler_a, NULL));
    TEST_ASSERT_EQUAL_INT(0, g_hit_a);

    /* 重复释放是空操作 */
    at_urc_deinit(&g_dev);
    at_urc_deinit(NULL);
}

/* --- 注册参数校验 --------------------------------------------------------- */

static void test_register_rejects_invalid_args(void)
{
    TEST_ASSERT_EQUAL_INT(0, at_urc_init(&g_dev));

    TEST_ASSERT_EQUAL_INT(-1, at_urc_register(NULL, "X", URC_MATCH_EXACT,
                                              handler_a, NULL));
    TEST_ASSERT_EQUAL_INT(-1, at_urc_register(&g_dev, NULL, URC_MATCH_EXACT,
                                              handler_a, NULL));
    TEST_ASSERT_EQUAL_INT(-1, at_urc_register(&g_dev, "X", URC_MATCH_EXACT,
                                              NULL, NULL));

    /* 失败注册不占用 ID */
    TEST_ASSERT_EQUAL_INT(
        0, at_urc_register(&g_dev, "X", URC_MATCH_EXACT, handler_a, NULL));
}

/* --- 匹配语义 ------------------------------------------------------------- */

static void test_match_prefix(void)
{
    fixtures_reset();
    TEST_ASSERT_EQUAL_INT(0, at_urc_init(&g_dev));
    TEST_ASSERT_EQUAL_INT(
        0, at_urc_register(&g_dev, "+CREG:", URC_MATCH_PREFIX, handler_a, NULL));

    TEST_ASSERT_TRUE(URC_FEED(&g_dev, "+CREG: 1,1\r\n"));
    TEST_ASSERT_EQUAL_INT(1, g_hit_a);
    TEST_ASSERT_EQUAL_STRING("+CREG: 1,1", g_last_line);

    /* 前缀必须从行首开始 */
    TEST_ASSERT_FALSE(URC_FEED(&g_dev, "X+CREG: 1,1\r\n"));
    TEST_ASSERT_EQUAL_INT(1, g_hit_a);
}

static void test_match_suffix(void)
{
    fixtures_reset();
    TEST_ASSERT_EQUAL_INT(0, at_urc_init(&g_dev));
    TEST_ASSERT_EQUAL_INT(
        0, at_urc_register(&g_dev, "OK", URC_MATCH_SUFFIX, handler_a, NULL));

    TEST_ASSERT_TRUE(URC_FEED(&g_dev, "+CSQ: 20,99,OK\r\n"));
    TEST_ASSERT_FALSE(URC_FEED(&g_dev, "OK,+CSQ\r\n"));
    TEST_ASSERT_EQUAL_INT(1, g_hit_a);

    /* 模式比行更长时不能越界比较 */
    TEST_ASSERT_FALSE(URC_FEED(&g_dev, "K\r\n"));
    TEST_ASSERT_EQUAL_INT(1, g_hit_a);
}

static void test_match_contain(void)
{
    fixtures_reset();
    TEST_ASSERT_EQUAL_INT(0, at_urc_init(&g_dev));
    TEST_ASSERT_EQUAL_INT(
        0, at_urc_register(&g_dev, "RING", URC_MATCH_CONTAIN, handler_a, NULL));

    TEST_ASSERT_TRUE(URC_FEED(&g_dev, "XYZRINGXYZ\r\n"));
    TEST_ASSERT_FALSE(URC_FEED(&g_dev, "XYZ\r\n"));
    TEST_ASSERT_EQUAL_INT(1, g_hit_a);
}

static void test_match_exact(void)
{
    fixtures_reset();
    TEST_ASSERT_EQUAL_INT(0, at_urc_init(&g_dev));
    TEST_ASSERT_EQUAL_INT(
        0, at_urc_register(&g_dev, "+CPIN: READY", URC_MATCH_EXACT, handler_a, NULL));

    TEST_ASSERT_TRUE(URC_FEED(&g_dev, "+CPIN: READY\r\n"));
    TEST_ASSERT_FALSE(URC_FEED(&g_dev, "+CPIN: READY NOW\r\n"));
    TEST_ASSERT_FALSE(URC_FEED(&g_dev, "+CPIN: READ\r\n"));
    TEST_ASSERT_EQUAL_INT(1, g_hit_a);
}

#ifndef USE_REGEX
static void test_regex_type_never_matches_without_use_regex(void)
{
    fixtures_reset();
    TEST_ASSERT_EQUAL_INT(0, at_urc_init(&g_dev));
    TEST_ASSERT_EQUAL_INT(0, at_urc_register(&g_dev, "^\\\\+CREG:.*$",
                                             URC_MATCH_REGEX, handler_a, NULL));

    /* 未编译正则支持时不静默降级成前缀匹配 */
    TEST_ASSERT_FALSE(URC_FEED(&g_dev, "+CREG: 1,1\r\n"));
    TEST_ASSERT_EQUAL_INT(0, g_hit_a);
}
#endif

/* --- 优先级与内建兜底 ----------------------------------------------------- */

static void test_first_match_wins(void)
{
    fixtures_reset();
    TEST_ASSERT_EQUAL_INT(0, at_urc_init(&g_dev));
    TEST_ASSERT_EQUAL_INT(
        0, at_urc_register(&g_dev, "+CREG:", URC_MATCH_PREFIX, handler_a, NULL));
    TEST_ASSERT_EQUAL_INT(
        1, at_urc_register(&g_dev, "+CREG:", URC_MATCH_PREFIX, handler_b, NULL));

    TEST_ASSERT_TRUE(URC_FEED(&g_dev, "+CREG: 1,1\r\n"));

    /* 首个命中者生效，后续处理器不再尝试 */
    TEST_ASSERT_EQUAL_INT(1, g_hit_a);
    TEST_ASSERT_EQUAL_INT(0, g_hit_b);
}

static void test_user_handler_overrides_builtin_rdy(void)
{
    fixtures_reset();
    TEST_ASSERT_EQUAL_INT(0, at_urc_init(&g_dev));
    TEST_ASSERT_EQUAL_INT(
        0, at_urc_register(&g_dev, "RDY", URC_MATCH_EXACT, handler_a, NULL));

    /* 内建 RDY 兜底不能让用户注册的 RDY 处理器永远收不到行 */
    TEST_ASSERT_TRUE(URC_FEED(&g_dev, "RDY\r\n"));
    TEST_ASSERT_EQUAL_INT(1, g_hit_a);
}

static void test_builtin_lines_are_swallowed(void)
{
    fixtures_reset();
    TEST_ASSERT_EQUAL_INT(0, at_urc_init(&g_dev));

    /* 没有用户处理器时，ATE 前缀与 RDY 精确行仍算已处理(静默) */
    TEST_ASSERT_TRUE(URC_FEED(&g_dev, "ATE0\r\n"));
    TEST_ASSERT_TRUE(URC_FEED(&g_dev, "RDY\r\n"));
    TEST_ASSERT_FALSE(URC_FEED(&g_dev, "RDYX\r\n"));
    TEST_ASSERT_FALSE(URC_FEED(&g_dev, "+CREG: 1,1\r\n"));

    uint32_t total   = 0;
    uint32_t handled = 0;
    at_urc_get_stats(&g_dev, &total, &handled);
    TEST_ASSERT_EQUAL_UINT(4, total);
    TEST_ASSERT_EQUAL_UINT(2, handled);
}

/* --- 启停与注销 ----------------------------------------------------------- */

static void test_set_enable_toggles_dispatch(void)
{
    fixtures_reset();
    TEST_ASSERT_EQUAL_INT(0, at_urc_init(&g_dev));
    int id = at_urc_register(&g_dev, "+CREG:", URC_MATCH_PREFIX, handler_a, NULL);
    TEST_ASSERT_EQUAL_INT(0, id);

    TEST_ASSERT_EQUAL_INT(0, at_urc_set_enable(&g_dev, id, false));
    TEST_ASSERT_FALSE(URC_FEED(&g_dev, "+CREG: 1,1\r\n"));
    TEST_ASSERT_EQUAL_INT(0, g_hit_a);

    /* 禁用不移除处理器，也不改变 ID */
    TEST_ASSERT_EQUAL_INT(0, at_urc_set_enable(&g_dev, id, true));
    TEST_ASSERT_TRUE(URC_FEED(&g_dev, "+CREG: 1,1\r\n"));
    TEST_ASSERT_EQUAL_INT(1, g_hit_a);

    TEST_ASSERT_EQUAL_INT(-1, at_urc_set_enable(&g_dev, 99, true));
    TEST_ASSERT_EQUAL_INT(-1, at_urc_set_enable(NULL, id, true));
    TEST_ASSERT_EQUAL_INT(-1, at_urc_set_enable(&g_dev_b, id, true));
}

static void test_unregister_removes_only_target(void)
{
    fixtures_reset();
    TEST_ASSERT_EQUAL_INT(0, at_urc_init(&g_dev));
    int id_a =
        at_urc_register(&g_dev, "+CREG:", URC_MATCH_PREFIX, handler_a, NULL);
    int id_b =
        at_urc_register(&g_dev, "+CGREG:", URC_MATCH_PREFIX, handler_b, NULL);

    TEST_ASSERT_EQUAL_INT(0, at_urc_unregister(&g_dev, id_a));
    TEST_ASSERT_EQUAL_INT(-1, at_urc_unregister(&g_dev, id_a));
    TEST_ASSERT_EQUAL_INT(-1, at_urc_unregister(&g_dev, 1234));
    TEST_ASSERT_EQUAL_INT(-1, at_urc_unregister(NULL, id_b));
    TEST_ASSERT_EQUAL_INT(-1, at_urc_unregister(&g_dev_b, id_b));

    TEST_ASSERT_FALSE(URC_FEED(&g_dev, "+CREG: 1,1\r\n"));
    TEST_ASSERT_EQUAL_INT(0, g_hit_a);

    /* 邻接处理器保留原 ID，仍按注册顺序生效 */
    TEST_ASSERT_TRUE(URC_FEED(&g_dev, "+CGREG: 1,1\r\n"));
    TEST_ASSERT_EQUAL_INT(1, g_hit_b);
    TEST_ASSERT_EQUAL_INT(0, at_urc_set_enable(&g_dev, id_b, false));
}

static void test_clear_removes_all_handlers(void)
{
    fixtures_reset();
    TEST_ASSERT_EQUAL_INT(0, at_urc_init(&g_dev));
    TEST_ASSERT_TRUE(
        at_urc_register(&g_dev, "+CREG:", URC_MATCH_PREFIX, handler_a, NULL) >= 0);
    TEST_ASSERT_TRUE(
        at_urc_register(&g_dev, "+CGREG:", URC_MATCH_PREFIX, handler_b, NULL) >= 0);
    TEST_ASSERT_TRUE(URC_FEED(&g_dev, "+CREG: 1,1\r\n"));

    at_urc_clear(&g_dev);

    TEST_ASSERT_FALSE(URC_FEED(&g_dev, "+CREG: 1,1\r\n"));
    TEST_ASSERT_FALSE(URC_FEED(&g_dev, "+CGREG: 1,1\r\n"));
    TEST_ASSERT_EQUAL_INT(1, g_hit_a);
    TEST_ASSERT_EQUAL_INT(0, g_hit_b);

    /* clear 保留设备槽位，设备可继续注册 */
    TEST_ASSERT_EQUAL_INT(
        2, at_urc_register(&g_dev, "+CSQ:", URC_MATCH_PREFIX, handler_a, NULL));

    at_urc_clear(NULL);
    at_urc_clear(&g_dev_b);
}

/* --- 行切分、统计、重入、截断 --------------------------------------------- */

static void test_multiline_and_partial_tail(void)
{
    fixtures_reset();
    TEST_ASSERT_EQUAL_INT(0, at_urc_init(&g_dev));
    TEST_ASSERT_TRUE(
        at_urc_register(&g_dev, "+CREG:", URC_MATCH_PREFIX, handler_a, NULL) >= 0);

    static const char *chunk = "garbage\r\n+CREG: 1,1\r\n+CREG: 5,1";
    size_t len               = strlen(chunk);

    TEST_ASSERT_TRUE(at_urc_process(&g_dev, chunk, (uint32_t)len));

    uint32_t total   = 0;
    uint32_t handled = 0;
    at_urc_get_stats(&g_dev, &total, &handled);
    TEST_ASSERT_EQUAL_UINT(3, total);   /* 三行: garbage / +CREG / 无换行残行 */
    TEST_ASSERT_EQUAL_UINT(2, handled);
    TEST_ASSERT_EQUAL_INT(2, g_hit_a);
    TEST_ASSERT_EQUAL_STRING("+CREG: 5,1", g_last_line);
}

static void test_crlf_counts_as_single_line(void)
{
    fixtures_reset();
    TEST_ASSERT_EQUAL_INT(0, at_urc_init(&g_dev));
    TEST_ASSERT_TRUE(
        at_urc_register(&g_dev, "+CREG:", URC_MATCH_PREFIX, handler_a, NULL) >= 0);

    TEST_ASSERT_TRUE(URC_FEED(&g_dev, "+CREG: 1,1\r\n"));
    TEST_ASSERT_EQUAL_INT(1, g_hit_a);

    uint32_t total   = 0;
    uint32_t handled = 0;
    at_urc_get_stats(&g_dev, &total, &handled);
    TEST_ASSERT_EQUAL_UINT(1, total);
    TEST_ASSERT_EQUAL_UINT(1, handled);

    /* 连续空行不产生额外"行" */
    TEST_ASSERT_FALSE(URC_FEED(&g_dev, "\r\n\r\n"));
    at_urc_get_stats(&g_dev, &total, &handled);
    TEST_ASSERT_EQUAL_UINT(1, total);
}

static void test_reentrant_process_is_rejected(void)
{
    fixtures_reset();
    TEST_ASSERT_EQUAL_INT(0, at_urc_init(&g_dev));
    TEST_ASSERT_TRUE(at_urc_register(&g_dev, "+CREG:", URC_MATCH_PREFIX,
                                     handler_reentrant, NULL) >= 0);

    TEST_ASSERT_TRUE(URC_FEED(&g_dev, "+CREG: 1,1\r\n"));
    TEST_ASSERT_FALSE(g_nested_result); /* 回调内的再次进入被拒 */
    TEST_ASSERT_EQUAL_INT(1, g_hit_a);

    /* 外层次处理结束后，状态复位，可继续处理 */
    TEST_ASSERT_TRUE(URC_FEED(&g_dev, "+CREG: 2,1\r\n"));
    TEST_ASSERT_EQUAL_INT(2, g_hit_a);
}

static void test_long_line_is_truncated_not_split(void)
{
    char buf[401];
    size_t i;

    fixtures_reset();
    TEST_ASSERT_EQUAL_INT(0, at_urc_init(&g_dev));
    TEST_ASSERT_TRUE(
        at_urc_register(&g_dev, "LONG", URC_MATCH_PREFIX, handler_a, NULL) >= 0);

    memcpy(buf, "LONG", 4);
    for (i = 4; i < sizeof(buf) - 2; i++) {
        buf[i] = 'x';
    }
    buf[sizeof(buf) - 2] = '\r';
    buf[sizeof(buf) - 1] = '\n';

    /* 单行超过 255 字节按截断处理: 不越界、不拆成多行、按截断内容匹配 */
    TEST_ASSERT_TRUE(at_urc_process(&g_dev, buf, (uint32_t)sizeof(buf)));
    TEST_ASSERT_EQUAL_INT(1, g_hit_a);
    TEST_ASSERT_EQUAL_UINT(255, g_last_line_len);

    uint32_t total   = 0;
    uint32_t handled = 0;
    at_urc_get_stats(&g_dev, &total, &handled);
    TEST_ASSERT_EQUAL_UINT(1, total);
    TEST_ASSERT_EQUAL_UINT(1, handled);
}

static void test_process_rejects_invalid_args(void)
{
    fixtures_reset();
    TEST_ASSERT_FALSE(URC_FEED(NULL, "X\r\n"));
    TEST_ASSERT_FALSE(at_urc_process(&g_dev, NULL, 3));

    /* 未初始化设备 + 零长度数据均 fail closed */
    TEST_ASSERT_FALSE(URC_FEED(&g_dev, "X\r\n"));
    TEST_ASSERT_EQUAL_INT(0, at_urc_init(&g_dev));
    TEST_ASSERT_FALSE(at_urc_process(&g_dev, "X\r\n", 0));
}

static void test_stats_guards_and_unknown_device(void)
{
    uint32_t total   = 7;
    uint32_t handled = 7;

    at_urc_get_stats(NULL, &total, &handled);
    TEST_ASSERT_EQUAL_UINT(0, total);
    TEST_ASSERT_EQUAL_UINT(0, handled);

    total   = 7;
    handled = 7;
    at_urc_get_stats(&g_dev, &total, &handled); /* 未初始化 */
    TEST_ASSERT_EQUAL_UINT(0, total);
    TEST_ASSERT_EQUAL_UINT(0, handled);

    /* 输出参数可为 NULL */
    TEST_ASSERT_EQUAL_INT(0, at_urc_init(&g_dev));
    at_urc_get_stats(&g_dev, NULL, NULL);
    at_urc_get_stats(&g_dev, &total, NULL);
    TEST_ASSERT_EQUAL_UINT(0, total);
}

/* --- 轮询取行路径 --------------------------------------------------------- */

static at_device_t g_poll_dev;
static const char *g_rx;
static size_t g_rx_pos;

static int poll_read_byte(at_device_t *dev)
{
    (void)dev;
    if (!g_rx || g_rx[g_rx_pos] == '\0') {
        return -1;
    }
    return (unsigned char)g_rx[g_rx_pos++];
}

static void test_poll_dispatches_readline(void)
{
    fixtures_reset();

    memset(&g_poll_dev, 0, sizeof(g_poll_dev));
    g_poll_dev.read_byte = poll_read_byte;
    at_urc_deinit(&g_poll_dev);

    TEST_ASSERT_EQUAL_INT(0, at_urc_init(&g_poll_dev));
    TEST_ASSERT_TRUE(at_urc_register(&g_poll_dev, "+CREG:", URC_MATCH_PREFIX,
                                     handler_a, NULL) >= 0);

    /* at_urc_poll 经 at_readline 取一行(CR/LF 已剥除)再分发 */
    g_rx     = "+CREG: 1,1\r\n";
    g_rx_pos = 0;
    at_urc_poll(&g_poll_dev);
    TEST_ASSERT_EQUAL_INT(1, g_hit_a);
    TEST_ASSERT_EQUAL_STRING("+CREG: 1,1", g_last_line);

    /* 无数据时不产生行，也不改动统计 */
    at_urc_poll(&g_poll_dev);
    TEST_ASSERT_EQUAL_INT(1, g_hit_a);

    uint32_t total   = 0;
    uint32_t handled = 0;
    at_urc_get_stats(&g_poll_dev, &total, &handled);
    TEST_ASSERT_EQUAL_UINT(1, total);
    TEST_ASSERT_EQUAL_UINT(1, handled);

    at_urc_poll(NULL); /* 空指针为空操作 */

    at_urc_deinit(&g_poll_dev);
    g_rx = NULL;
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_init_rejects_null);
    RUN_TEST(test_register_requires_init);
    RUN_TEST(test_init_is_idempotent);
    RUN_TEST(test_init_does_not_touch_user_data);
    RUN_TEST(test_device_slot_capacity);
    RUN_TEST(test_deinit_releases_slot_and_handlers);

    RUN_TEST(test_register_rejects_invalid_args);

    RUN_TEST(test_match_prefix);
    RUN_TEST(test_match_suffix);
    RUN_TEST(test_match_contain);
    RUN_TEST(test_match_exact);
#ifndef USE_REGEX
    RUN_TEST(test_regex_type_never_matches_without_use_regex);
#endif

    RUN_TEST(test_first_match_wins);
    RUN_TEST(test_user_handler_overrides_builtin_rdy);
    RUN_TEST(test_builtin_lines_are_swallowed);

    RUN_TEST(test_set_enable_toggles_dispatch);
    RUN_TEST(test_unregister_removes_only_target);
    RUN_TEST(test_clear_removes_all_handlers);

    RUN_TEST(test_multiline_and_partial_tail);
    RUN_TEST(test_crlf_counts_as_single_line);
    RUN_TEST(test_reentrant_process_is_rejected);
    RUN_TEST(test_long_line_is_truncated_not_split);
    RUN_TEST(test_process_rejects_invalid_args);
    RUN_TEST(test_stats_guards_and_unknown_device);

    RUN_TEST(test_poll_dispatches_readline);

    return UNITY_END();
}
