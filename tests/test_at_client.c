/**
 * @file test_at_client.c
 * @brief AT Client 完整测试
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#define XY_PLATFORM_PC 1
#define XY_LOG_LEVEL 3

#include "at_client.h"

// ==================== Mock ====================
static char rx_buf[256];
static size_t rx_len = 0;
static size_t rx_pos = 0;
static int tx_count = 0;

static uint32_t mock_get_tick(void)
{
    // 返回递增的 tick 值
    static uint32_t tick = 0;
    return tick++;
}

static void mock_feed(const char *data)
{
    rx_len = strlen(data);
    rx_pos = 0;
    if (rx_len < sizeof(rx_buf)) {
        memcpy(rx_buf, data, rx_len);
    }
}

static int mock_read_byte(at_device_t *dev)
{
    (void)dev;
    if (rx_pos < rx_len) {
        return (unsigned char)rx_buf[rx_pos++];
    }
    return -1;
}

static void mock_write(at_device_t *dev, const uint8_t *data, uint32_t len)
{
    (void)dev;
    tx_count++;
    printf("[TX #%d] %.*s\n", tx_count, (int)len, (const char*)data);
}

int main(void)
{
    int passed = 0, failed = 0;
    
    printf("\n╔══════════════════════════════════════════╗\n");
    printf("║       AT Client Test Suite              ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");
    
    // 初始化
    at_client_t *client = at_client_init();
    // 注意: 提供 get_tick 函数
    at_device_t *dev = at_device_register("test", mock_read_byte, mock_write, mock_get_tick, NULL);
    
    if (!client || !dev) {
        printf("❌ Init failed\n");
        return 1;
    }
    printf("✅ Init: OK\n");
    passed++;
    
    // 测试1: 空命令
    printf("\n--- Test: NULL command ---\n");
    tx_count = 0;
    at_resp_type_t resp = at_send_command(dev, NULL, NULL, NULL, 1000);
    if (resp == AT_RESP_ERROR && tx_count == 0) {
        printf("✅ NULL command rejected\n");
        passed++;
    } else {
        printf("❌ NULL command should be rejected\n");
        failed++;
    }
    
    // 测试2: 忙碌状态
    printf("\n--- Test: Busy state ---\n");
    dev->is_busy = true;
    tx_count = 0;
    resp = at_send_command(dev, "AT", NULL, NULL, 1000);
    dev->is_busy = false;
    if (resp == AT_RESP_ERROR && tx_count == 0) {
        printf("✅ Busy state handled\n");
        passed++;
    } else {
        printf("❌ Busy state not handled\n");
        failed++;
    }
    
    // 测试3: OK 响应
    printf("\n--- Test: OK response ---\n");
    mock_feed("OK\r\n");
    tx_count = 0;
    resp = at_send_command(dev, "AT", NULL, NULL, 1000);
    if (resp == AT_RESP_OK && tx_count > 0) {
        printf("✅ OK response: %d bytes sent\n", tx_count);
        passed++;
    } else {
        printf("❌ OK response failed (resp=%d, tx=%d)\n", resp, tx_count);
        failed++;
    }
    
    // 测试4: ERROR 响应
    printf("\n--- Test: ERROR response ---\n");
    mock_feed("ERROR\r\n");
    resp = at_send_command(dev, "AT+INVALID", NULL, NULL, 1000);
    if (resp == AT_RESP_ERROR) {
        printf("✅ ERROR response handled\n");
        passed++;
    } else {
        printf("❌ ERROR response failed (resp=%d)\n", resp);
        failed++;
    }
    
    // 测试5: 带参数命令
    printf("\n--- Test: Command with args ---\n");
    mock_feed("OK\r\n");
    resp = at_send_command(dev, "AT+BAUD=115200", NULL, NULL, 1000);
    if (resp == AT_RESP_OK) {
        printf("✅ Command with args\n");
        passed++;
    } else {
        printf("❌ Command with args failed\n");
        failed++;
    }
    
    // 测试6: 统计信息
    printf("\n--- Test: Statistics ---\n");
    uint32_t tx_before = dev->tx_count;
    mock_feed("OK\r\n");
    at_send_command(dev, "AT", NULL, NULL, 1000);
    if (dev->tx_count > tx_before) {
        printf("✅ Statistics: %u bytes sent\n", dev->tx_count);
        passed++;
    } else {
        printf("❌ Statistics not updated\n");
        failed++;
    }
    
    // 总结
    printf("\n╔══════════════════════════════════════════╗\n");
    printf("║            Results                       ║\n");
    printf("╠══════════════════════════════════════════╣\n");
    printf("║  ✅ Passed: %d                             ║\n", passed);
    printf("║  ❌ Failed: %d                             ║\n", failed);
    printf("║  📊 Total:  %d                             ║\n", passed + failed);
    printf("╚══════════════════════════════════════════╝\n");
    
    return failed > 0 ? 1 : 0;
}
