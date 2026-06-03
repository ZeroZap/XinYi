/**
 * @file test_mux_spi.c
 * @brief Focused host tests for MUX SPI helper contracts
 */

#include "xy_mux.h"
#include "xy_mux_spi.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 512U

static uint8_t g_last_write[256];
static size_t g_last_write_len;
static uint8_t g_read_pattern = 0xA5;
static int g_ioctl_count;

void xy_log_char(char ch)
{
    (void)ch;
}

static int32_t mock_spi_init(uint8_t channel, const void *config)
{
    (void)config;
    g_last_write_len = 0;
    g_ioctl_count = 0;
    printf("    [MOCK] SPI-%u init\n", (unsigned)channel);
    return XY_MUX_OK;
}

static int32_t mock_spi_deinit(uint8_t channel)
{
    printf("    [MOCK] SPI-%u deinit\n", (unsigned)channel);
    return XY_MUX_OK;
}

static int32_t mock_spi_write(uint8_t channel, const void *data, size_t len)
{
    (void)channel;
    if (!data || len == 0 || len > sizeof(g_last_write)) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }
    memcpy(g_last_write, data, len);
    g_last_write_len = len;
    return (int32_t)len;
}

static int32_t mock_spi_read(uint8_t channel, void *data, size_t len)
{
    (void)channel;
    if (!data || len == 0) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }
    memset(data, g_read_pattern, len);
    return (int32_t)len;
}

static int32_t mock_spi_ioctl(uint8_t channel, int cmd, void *arg)
{
    (void)channel;
    g_ioctl_count++;
    if (cmd == XY_MUX_SPI_CMD_SET_CONFIG) {
        assert(arg != NULL);
    }
    return XY_MUX_OK;
}

static xy_mux_manager_t make_mgr(uint8_t *tx, uint8_t *rx)
{
    xy_mux_manager_t mgr;
    assert(xy_mux_init(&mgr, tx, rx, BUFFER_SIZE) == XY_MUX_OK);
    return mgr;
}

static xy_mux_ops_t make_ops(void)
{
    xy_mux_ops_t ops = {
        .init = mock_spi_init,
        .deinit = mock_spi_deinit,
        .read = mock_spi_read,
        .write = mock_spi_write,
        .ioctl = mock_spi_ioctl,
    };
    return ops;
}

static void test_spi_register_and_config(void)
{
    uint8_t tx[BUFFER_SIZE];
    uint8_t rx[BUFFER_SIZE];
    xy_mux_manager_t mgr = make_mgr(tx, rx);
    xy_mux_ops_t ops = make_ops();

    assert(xy_mux_spi_register(&mgr, 0, &ops, NULL) == XY_MUX_OK);
    assert(xy_mux_spi_register(&mgr, 1, &ops, NULL) == XY_MUX_OK);
    assert(mgr.device_count == 2);

    xy_mux_spi_config_t cfg = {
        .speed = 8000000,
        .mode = XY_MUX_SPI_MODE3,
        .bits = 8,
        .cs_pin = 10,
    };
    assert(xy_mux_spi_config(&mgr, 0, &cfg) == XY_MUX_OK);
    assert(g_ioctl_count > 0);

    xy_mux_deinit(&mgr);
}

static void test_spi_write_header_and_payload(void)
{
    uint8_t tx[BUFFER_SIZE];
    uint8_t rx[BUFFER_SIZE];
    xy_mux_manager_t mgr = make_mgr(tx, rx);
    xy_mux_ops_t ops = make_ops();
    assert(xy_mux_spi_register(&mgr, 0, &ops, NULL) == XY_MUX_OK);

    uint8_t data[] = {0x11, 0x22, 0x33};
    assert(xy_mux_spi_write(&mgr, 0, data, sizeof(data)) == (int32_t)sizeof(data));
    assert(g_last_write_len == sizeof(data));
    assert(memcmp(g_last_write, data, sizeof(data)) == 0);

    xy_mux_deinit(&mgr);
}

static void test_spi_transfer_and_read(void)
{
    uint8_t tx_buf[BUFFER_SIZE];
    uint8_t rx_buf[BUFFER_SIZE];
    xy_mux_manager_t mgr = make_mgr(tx_buf, rx_buf);
    xy_mux_ops_t ops = make_ops();
    assert(xy_mux_spi_register(&mgr, 2, &ops, NULL) == XY_MUX_OK);

    uint8_t tx_data[] = {0x9A, 0xBC, 0xDE, 0xF0};
    uint8_t rx_data[sizeof(tx_data)] = {0};
    assert(xy_mux_spi_transfer(&mgr, 2, tx_data, rx_data, sizeof(tx_data)) == (int32_t)sizeof(tx_data));
    assert(memcmp(rx_data, (uint8_t[]){0xA5, 0xA5, 0xA5, 0xA5}, sizeof(rx_data)) == 0);

    memset(rx_data, 0, sizeof(rx_data));
    assert(xy_mux_spi_read(&mgr, 2, rx_data, sizeof(rx_data)) == (int32_t)sizeof(rx_data));
    assert(rx_data[0] == 0xA5);

    xy_mux_deinit(&mgr);
}

static void test_spi_error_paths(void)
{
    uint8_t tx[BUFFER_SIZE];
    uint8_t rx[BUFFER_SIZE];
    xy_mux_manager_t mgr = make_mgr(tx, rx);
    xy_mux_ops_t ops = make_ops();
    uint8_t data = 0x55;

    assert(xy_mux_spi_write(NULL, 0, &data, 1) == XY_MUX_ERROR_INVALID_PARAM);
    assert(xy_mux_spi_read(NULL, 0, &data, 1) == XY_MUX_ERROR_INVALID_PARAM);
    assert(xy_mux_spi_transfer(NULL, 0, &data, &data, 1) == XY_MUX_ERROR_INVALID_PARAM);
    assert(xy_mux_spi_transfer(&mgr, 0, &data, &data, 0) == XY_MUX_ERROR_INVALID_PARAM);

    assert(xy_mux_spi_register(&mgr, 0, &ops, NULL) == XY_MUX_OK);
    assert(xy_mux_spi_write(&mgr, 9, &data, 1) == XY_MUX_ERROR_NO_DEVICE);
    assert(xy_mux_spi_read(&mgr, 9, &data, 1) == XY_MUX_ERROR_NO_DEVICE);

    xy_mux_deinit(&mgr);
}

int main(void)
{
    test_spi_register_and_config();
    test_spi_write_header_and_payload();
    test_spi_transfer_and_read();
    test_spi_error_paths();
    puts("MUX SPI tests passed");
    return 0;
}
