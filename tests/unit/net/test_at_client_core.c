#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "unity.h"
#include "fff.h"

#include "at_client.h"

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(int, mock_read_byte, at_device_t *);
FAKE_VOID_FUNC(mock_write, at_device_t *, const uint8_t *, uint32_t);
FAKE_VALUE_FUNC(at_tick_t, mock_get_tick);
FAKE_VALUE_FUNC(bool, custom_resp_handler, at_device_t *, const char *, void *, at_resp_type_t *);

static int mock_read_byte_impl(at_device_t *dev);
static void mock_write_impl(at_device_t *dev, const uint8_t *data, uint32_t len);
static at_tick_t mock_get_tick_impl(void);
static bool custom_resp_handler_impl(at_device_t *dev, const char *resp_line,
                                     void *user_data, at_resp_type_t *type);

static const char *g_rx_data;
static size_t g_rx_len;
static size_t g_rx_pos;
static uint8_t g_tx_data[512];
static uint32_t g_tx_len;
static uint32_t g_tick;

static void mock_feed(const char *data)
{
    g_rx_data = data;
    g_rx_len = strlen(data);
    g_rx_pos = 0;
}

static int mock_read_byte_impl(at_device_t *dev)
{
    (void)dev;
    if (g_rx_pos < g_rx_len) {
        return (uint8_t)g_rx_data[g_rx_pos++];
    }
    return -1;
}

static void mock_write_impl(at_device_t *dev, const uint8_t *data, uint32_t len)
{
    (void)dev;
    TEST_ASSERT_LESS_OR_EQUAL_size_t(sizeof(g_tx_data), g_tx_len + len);
    memcpy(&g_tx_data[g_tx_len], data, len);
    g_tx_len += len;
}

void setUp(void)
{
}

void tearDown(void)
{
    /* at_device_register() 里的设备是 malloc 出来的；不放回去，每一条用例都会把
       它留在单例里。tearDown 里归还，测试二进制在 ASan/LSan 下才是干净的。 */
    at_client_deinit();
}

static at_tick_t mock_get_tick_impl(void)
{
    return g_tick++;
}

static void reset_io(void)
{
    g_rx_data = "";
    g_rx_len = 0;
    g_rx_pos = 0;
    g_tx_len = 0;
    g_tick = 0;
    memset(g_tx_data, 0, sizeof(g_tx_data));

    RESET_FAKE(mock_read_byte);
    RESET_FAKE(mock_write);
    RESET_FAKE(mock_get_tick);
    RESET_FAKE(custom_resp_handler);
    FFF_RESET_HISTORY();

    mock_read_byte_fake.custom_fake = mock_read_byte_impl;
    mock_write_fake.custom_fake = mock_write_impl;
    mock_get_tick_fake.custom_fake = mock_get_tick_impl;
    custom_resp_handler_fake.custom_fake = custom_resp_handler_impl;
}

static bool custom_resp_handler_impl(at_device_t *dev, const char *resp_line,
                                     void *user_data, at_resp_type_t *type)
{
    (void)dev;
    int *seen = (int *)user_data;
    if (strstr(resp_line, "+READY")) {
        (*seen)++;
        *type = AT_RESP_CUSTOM;
        return true;
    }
    return false;
}

static void test_device_registration_and_default_command(void)
{
    reset_io();
    at_client_t *client = at_client_init();
    TEST_ASSERT_NOT_NULL(client);
    TEST_ASSERT_EQUAL_UINT8(0, client->device_count);

    TEST_ASSERT_NULL(at_device_register(NULL, mock_read_byte, mock_write, mock_get_tick, NULL));
    TEST_ASSERT_NULL(at_device_register("no-read", NULL, mock_write, mock_get_tick, NULL));
    TEST_ASSERT_NULL(at_device_register("no-write", mock_read_byte, NULL, mock_get_tick, NULL));
    TEST_ASSERT_NULL(at_device_register("no-tick", mock_read_byte, mock_write, NULL, NULL));
    TEST_ASSERT_EQUAL_UINT8(0, client->device_count);

    at_device_t *dev = at_device_register("modem", mock_read_byte, mock_write, mock_get_tick, NULL);
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_EQUAL_UINT8(0, dev->id);
    TEST_ASSERT_EQUAL_STRING("modem", dev->name);
    TEST_ASSERT_EQUAL_UINT8(1, client->device_count);
    TEST_ASSERT_EQUAL_UINT8(0, client->default_device);

    mock_feed("OK\r\n");
    TEST_ASSERT_EQUAL(AT_RESP_OK, at_send_cmd("AT", NULL, NULL, 100));
    TEST_ASSERT_EQUAL_UINT32(4, g_tx_len);
    TEST_ASSERT_EQUAL_MEMORY("AT\r\n", g_tx_data, 4);
    TEST_ASSERT_EQUAL_UINT32(4, dev->tx_count);
    TEST_ASSERT_EQUAL_UINT32(2, dev->rx_count);
    TEST_ASSERT_EQUAL_UINT(2U, mock_write_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(dev, mock_write_fake.arg0_history[0]);
    TEST_ASSERT_EQUAL_MEMORY("AT", mock_write_fake.arg1_history[0], 2);
    TEST_ASSERT_EQUAL_UINT32(2U, mock_write_fake.arg2_history[0]);
    TEST_ASSERT_EQUAL_MEMORY("\r\n", mock_write_fake.arg1_history[1], 2);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT(4U, mock_read_byte_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(dev, mock_read_byte_fake.arg0_val);
    TEST_ASSERT_GREATER_THAN_UINT(0U, mock_get_tick_fake.call_count);
}

static void test_default_device_selection_guards_and_switches(void)
{
    reset_io();
    at_client_t *client = at_client_init();
    TEST_ASSERT_NOT_NULL(client);

    TEST_ASSERT_EQUAL_INT(-1, at_set_default_device(NULL));
    TEST_ASSERT_EQUAL(AT_RESP_ERROR, at_send_cmd("AT", NULL, NULL, 100));

    at_device_t orphan = {0};
    TEST_ASSERT_EQUAL_INT(-1, at_set_default_device(&orphan));
    TEST_ASSERT_EQUAL_UINT8(0, client->device_count);

    at_device_t *first = at_device_register("first", mock_read_byte, mock_write, mock_get_tick, NULL);
    at_device_t *second = at_device_register("second", mock_read_byte, mock_write, mock_get_tick, NULL);
    TEST_ASSERT_NOT_NULL(first);
    TEST_ASSERT_NOT_NULL(second);
    TEST_ASSERT_EQUAL_UINT8(0, client->default_device);

    mock_feed("OK\r\n");
    TEST_ASSERT_EQUAL(AT_RESP_OK, at_send_cmd("AT", NULL, NULL, 100));
    TEST_ASSERT_EQUAL_UINT32(4, first->tx_count);
    TEST_ASSERT_EQUAL_UINT32(0, second->tx_count);

    reset_io();
    TEST_ASSERT_EQUAL_INT(0, at_set_default_device(second));
    TEST_ASSERT_EQUAL_UINT8(1, client->default_device);
    mock_feed("OK\r\n");
    TEST_ASSERT_EQUAL(AT_RESP_OK, at_send_cmd("AT+SECOND", NULL, NULL, 100));
    TEST_ASSERT_EQUAL_UINT32(4, first->tx_count);
    TEST_ASSERT_EQUAL_UINT32(11, second->tx_count);
}

static void test_send_command_validation_and_busy_state(void)
{
    reset_io();
    at_client_init();
    at_device_t *dev = at_device_register("modem", mock_read_byte, mock_write, mock_get_tick, NULL);
    TEST_ASSERT_NOT_NULL(dev);

    TEST_ASSERT_EQUAL(AT_RESP_ERROR, at_send_command(NULL, "AT", NULL, NULL, 100));
    TEST_ASSERT_EQUAL(AT_RESP_ERROR, at_send_command(dev, NULL, NULL, NULL, 100));

    dev->is_busy = true;
    TEST_ASSERT_EQUAL(AT_RESP_ERROR, at_send_command(dev, "AT", NULL, NULL, 100));
    TEST_ASSERT_EQUAL_UINT32(0, g_tx_len);
    dev->is_busy = false;
}

static void test_ok_error_timeout_and_custom_response(void)
{
    reset_io();
    at_client_init();
    at_device_t *dev = at_device_register("modem", mock_read_byte, mock_write, mock_get_tick, NULL);
    TEST_ASSERT_NOT_NULL(dev);

    mock_feed("OK\r\n");
    TEST_ASSERT_EQUAL(AT_RESP_OK, at_send_command(dev, "AT", NULL, NULL, 100));

    reset_io();
    mock_feed("ERROR\r\n");
    TEST_ASSERT_EQUAL(AT_RESP_ERROR, at_send_command(dev, "AT+BAD", NULL, NULL, 100));
    TEST_ASSERT_EQUAL_UINT(2U, mock_write_fake.call_count);
    TEST_ASSERT_EQUAL_MEMORY("AT+BAD", mock_write_fake.arg1_history[0], 6);

    reset_io();
    mock_feed("");
    TEST_ASSERT_EQUAL(AT_RESP_TIMEOUT, at_send_command(dev, "AT", NULL, NULL, 3));
    TEST_ASSERT_EQUAL_UINT(2U, mock_write_fake.call_count);
    TEST_ASSERT_GREATER_THAN_UINT(0U, mock_get_tick_fake.call_count);
    TEST_ASSERT_GREATER_THAN_UINT(0U, mock_read_byte_fake.call_count);

    reset_io();
    int seen = 0;
    mock_feed("+READY\r\n");
    TEST_ASSERT_EQUAL(AT_RESP_CUSTOM, at_send_command(dev, "AT+READY", custom_resp_handler, &seen, 100));
    TEST_ASSERT_EQUAL_INT(1, seen);
    TEST_ASSERT_EQUAL_UINT(1U, custom_resp_handler_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(dev, custom_resp_handler_fake.arg0_val);
    TEST_ASSERT_EQUAL_STRING("+READY", custom_resp_handler_fake.arg1_val);
    TEST_ASSERT_EQUAL_PTR(&seen, custom_resp_handler_fake.arg2_val);
    TEST_ASSERT_NOT_NULL(custom_resp_handler_fake.arg3_val);
}

static void test_raw_readline_expect_and_stats(void)
{
    reset_io();
    at_client_init();
    at_device_t *dev = at_device_register("modem", mock_read_byte, mock_write, mock_get_tick, NULL);
    TEST_ASSERT_NOT_NULL(dev);

    const uint8_t raw[] = {0x01, 0x02, 0x03};
    TEST_ASSERT_EQUAL_INT(-1, at_send_raw(NULL, raw, sizeof(raw)));
    TEST_ASSERT_EQUAL_INT(-1, at_send_raw(dev, NULL, sizeof(raw)));
    TEST_ASSERT_EQUAL_INT(-1, at_send_raw(dev, raw, 0));
    TEST_ASSERT_EQUAL_INT((int)sizeof(raw), at_send_raw(dev, raw, sizeof(raw)));
    TEST_ASSERT_EQUAL_UINT(1U, mock_write_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(dev, mock_write_fake.arg0_val);
    TEST_ASSERT_EQUAL_PTR(raw, mock_write_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT32(sizeof(raw), mock_write_fake.arg2_val);

    char line[16];
    mock_feed("VALUE\r\n");
    TEST_ASSERT_EQUAL_INT(5, at_readline(dev, line, sizeof(line)));
    TEST_ASSERT_EQUAL_STRING("VALUE", line);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT(7U, mock_read_byte_fake.call_count);

    reset_io();
    mock_feed("NOISE\r\nTARGET\r\n");
    TEST_ASSERT_TRUE(at_expect(dev, "TARGET", 100));
    TEST_ASSERT_GREATER_THAN_UINT(0U, mock_get_tick_fake.call_count);
    TEST_ASSERT_GREATER_THAN_UINT(0U, mock_read_byte_fake.call_count);

    uint32_t tx = 0, rx = 0, err = 0;
    at_get_stats(dev, &tx, &rx, &err);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(sizeof(raw), tx);
}

static void test_expect_refuses_invalid_input(void)
{
    reset_io();
    at_client_init();

    /* NULL 设备过去在 at_expect() 里直接 dev->get_tick() —— 实测 SIGSEGV（rc=139）。
       公共 API 的无效入参一律返回 false，且不碰总线。 */
    TEST_ASSERT_FALSE(at_expect(NULL, "OK", 10));
    TEST_ASSERT_EQUAL_UINT32(0, g_tx_len);
    TEST_ASSERT_EQUAL_UINT(0U, mock_read_byte_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(0U, mock_get_tick_fake.call_count);

    at_device_t *dev = at_device_register("modem", mock_read_byte, mock_write, mock_get_tick, NULL);
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_FALSE(at_expect(dev, NULL, 10));

    /* 手搭出来的设备（没经过 register）可能缺回调；同样拒绝而不是解引用。 */
    at_device_t bare = {0};
    bare.get_tick = mock_get_tick;
    TEST_ASSERT_FALSE(at_expect(&bare, "OK", 10));
    bare.read_byte = mock_read_byte;
    bare.get_tick = NULL;
    TEST_ASSERT_FALSE(at_expect(&bare, "OK", 10));
}

static void test_error_and_timeout_are_counted(void)
{
    reset_io();
    at_client_init();
    at_device_t *dev = at_device_register("modem", mock_read_byte, mock_write, mock_get_tick, NULL);
    TEST_ASSERT_NOT_NULL(dev);

    uint32_t tx = 0, rx = 0, err = 0;

    mock_feed("OK\r\n");
    TEST_ASSERT_EQUAL(AT_RESP_OK, at_send_command(dev, "AT", NULL, NULL, 100));
    at_get_stats(dev, &tx, &rx, &err);
    TEST_ASSERT_EQUAL_UINT32(0, err);

    /* error_count 此前没有任何写入点：at_get_stats() 的第三个输出永远是 0。 */
    reset_io();
    mock_feed("ERROR\r\n");
    TEST_ASSERT_EQUAL(AT_RESP_ERROR, at_send_command(dev, "AT+BAD", NULL, NULL, 100));
    at_get_stats(dev, &tx, &rx, &err);
    TEST_ASSERT_EQUAL_UINT32(1, err);

    reset_io();
    mock_feed("");
    TEST_ASSERT_EQUAL(AT_RESP_TIMEOUT, at_send_command(dev, "AT", NULL, NULL, 3));
    at_get_stats(dev, &tx, &rx, &err);
    TEST_ASSERT_EQUAL_UINT32(2, err);

    /* 成功的一条不计数。 */
    reset_io();
    mock_feed("OK\r\n");
    TEST_ASSERT_EQUAL(AT_RESP_OK, at_send_command(dev, "AT", NULL, NULL, 100));
    at_get_stats(dev, &tx, &rx, &err);
    TEST_ASSERT_EQUAL_UINT32(2, err);
}

static void test_deinit_releases_the_registry(void)
{
    reset_io();
    at_client_t *client = at_client_init();
    TEST_ASSERT_NOT_NULL(client);

    at_device_t *dev = at_device_register("modem", mock_read_byte, mock_write, mock_get_tick, NULL);
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_EQUAL_UINT8(1, client->device_count);

    at_client_deinit();
    TEST_ASSERT_EQUAL_UINT8(0, client->device_count);
    TEST_ASSERT_EQUAL_UINT8(0, client->default_device);
    TEST_ASSERT_NULL(client->devices[0]);

    /* 注册表空了：没有默认设备可发，旧指针也不再被认作注册成员。 */
    TEST_ASSERT_EQUAL(AT_RESP_ERROR, at_send_cmd("AT", NULL, NULL, 100));
    TEST_ASSERT_EQUAL_INT(-1, at_set_default_device(dev));

    /* 重新注册从 id 0 开始，说明计数确实被清过，而不是只把条目指空。 */
    at_device_t *again = at_device_register("again", mock_read_byte, mock_write, mock_get_tick, NULL);
    TEST_ASSERT_NOT_NULL(again);
    TEST_ASSERT_EQUAL_UINT8(0, again->id);
    TEST_ASSERT_EQUAL_UINT8(1, client->device_count);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_device_registration_and_default_command);
    RUN_TEST(test_default_device_selection_guards_and_switches);
    RUN_TEST(test_send_command_validation_and_busy_state);
    RUN_TEST(test_ok_error_timeout_and_custom_response);
    RUN_TEST(test_raw_readline_expect_and_stats);
    RUN_TEST(test_expect_refuses_invalid_input);
    RUN_TEST(test_error_and_timeout_are_counted);
    RUN_TEST(test_deinit_releases_the_registry);
    return UNITY_END();
}
