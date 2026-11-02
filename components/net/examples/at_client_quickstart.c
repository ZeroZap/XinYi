/**
 * @file at_client_quickstart.c
 * @brief `components/net/README.md` 的 AT 客户端快速上手，**编译并运行**的那一份
 *
 * README 的快速上手过去写的是 `xy_at_client_create()` / `xy_at_create_resp()` /
 * `xy_at_exec_cmd()` 一族。这些名字确实在 `atc/core/xy_at_client.h` 里有声明，
 * 但全仓**没有任何一个 `.c` 定义过它们**：按原样抄下来，`gcc -c` 先给
 * incompatible-pointer-types，链接器再点名 5 个 undefined reference。文档里
 * 教一个链不上的 API，而没有任何目标编译过那份样张。
 *
 * 现在快速上手按**真正被构建的**所有者 `at_client.c` / `at_client.h` 写，并且
 * 这份文件本身就是它的守卫（CTest `net_at_quickstart`）：README 里那段代码与
 * 这里保持同构，谁改坏了 `at_client` 的契约这条测试就红。
 *
 * 端口侧只需要提供三样东西：读一个字节、写一段数据、一个单调递增的 tick。
 * 下面是一个假模块，它按队列吐出响应行，并记录主机发出去的命令。
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "at_client.h"

/* ==================== 端口侧：一个假模块 ==================== */

static const char *g_rx;
static size_t g_rx_len;
static size_t g_rx_pos;
static char g_tx[128];
static size_t g_tx_len;
static uint32_t g_tick;

/** 把一段响应放进“模块 -> 主机”的方向。 */
static void modem_queue(const char *response)
{
    g_rx = response;
    g_rx_len = strlen(response);
    g_rx_pos = 0;
}

/** 主机读一个字节；没有数据时按契约返回负数。 */
static int modem_read_byte(at_device_t *dev)
{
    (void)dev;
    if (g_rx_pos >= g_rx_len) {
        return -1;
    }
    return (unsigned char)g_rx[g_rx_pos++];
}

/** 主机写一段数据；这里只是记录下来，好让样例能断言真的发出去过。 */
static void modem_write(at_device_t *dev, const uint8_t *data, uint32_t len)
{
    (void)dev;
    if (g_tx_len + len >= sizeof(g_tx)) {
        return;
    }
    memcpy(&g_tx[g_tx_len], data, len);
    g_tx_len += len;
}

/** 毫秒 tick。AT 客户端的超时判断就靠它前进。 */
static at_tick_t modem_tick(void)
{
    return g_tick++;
}

/* ==================== 样例本身 ==================== */

static int g_checks;
static int g_failures;

static void check(bool condition, const char *what)
{
    g_checks++;
    if (!condition) {
        g_failures++;
        printf("FAIL %s\n", what);
    }
}

int main(void)
{
    /* 1. 初始化客户端并注册设备：这是唯一“接线”的地方。 */
    at_client_t *client = at_client_init();
    check(client != NULL, "at_client_init() returns the client");

    at_device_t *dev = at_device_register("modem", modem_read_byte, modem_write,
                                          modem_tick, NULL);
    check(dev != NULL, "at_device_register() accepts a complete port");
    if (dev == NULL) {
        printf("AT_QUICKSTART_FAILED\n");
        return 1;
    }

    /* 2. 发一条命令，等 OK。README 里的 at_send_command() 就是这一条。 */
    modem_queue("\r\nOK\r\n");
    check(at_send_command(dev, "AT", NULL, NULL, 100) == AT_RESP_OK,
          "AT -> OK");
    check(g_tx_len == 4 && memcmp(g_tx, "AT\r\n", 4) == 0,
          "the command is terminated with CRLF");

    /* 3. at_send_cmd() 走默认设备，同一个事务。 */
    g_tx_len = 0;
    modem_queue("\r\nOK\r\n");
    check(at_send_cmd("AT+CSQ", NULL, NULL, 100) == AT_RESP_OK,
          "at_send_cmd() uses the default device");

    /* 4. 统计：tx 计的是字节，err 计的是 ERROR/超时。 */
    uint32_t tx = 0, rx = 0, err = 0;
    at_get_stats(dev, &tx, &rx, &err);
    check(tx == 4U + 6U + 2U, "tx counts the bytes the client wrote");
    check(rx > 0U, "rx counts the bytes the client read");
    check(err == 0U, "a clean command does not count an error");

    modem_queue("\r\nERROR\r\n");
    check(at_send_command(dev, "AT+BAD", NULL, NULL, 100) == AT_RESP_ERROR,
          "ERROR is reported as AT_RESP_ERROR");
    at_get_stats(dev, &tx, &rx, &err);
    check(err == 1U, "an ERROR response is counted");

    /* 5. at_expect(): 等一条特定响应，读到的噪声行不算命中。 */
    modem_queue("NOISE\r\nURC: 1,2\r\n");
    check(at_expect(dev, "URC: 1,2", 100), "at_expect() matches a later line");

    /* 6. 无效入参一律拒绝，不碰总线。NULL 设备过去会直接解引用（SIGSEGV）。 */
    g_tx_len = 0;
    check(!at_expect(NULL, "OK", 10), "at_expect(NULL) is refused");
    check(!at_expect(dev, NULL, 10), "at_expect(NULL expect) is refused");
    check(at_send_command(NULL, "AT", NULL, NULL, 10) == AT_RESP_ERROR,
          "at_send_command(NULL) is refused");
    check(g_tx_len == 0U, "a refused call writes nothing");

    /* 7. 收尾：释放注册表。重复 init 也不会再泄漏设备（ASan 实测过旧实现的
       1128 字节 direct leak）。 */
    at_client_deinit();
    check(at_send_cmd("AT", NULL, NULL, 10) == AT_RESP_ERROR,
          "after deinit there is no default device");

    printf("AT_QUICKSTART %d checks, %d failures\n", g_checks, g_failures);
    if (g_failures != 0) {
        printf("AT_QUICKSTART_FAILED\n");
        return 1;
    }
    printf("AT_QUICKSTART_OK\n");
    return 0;
}
