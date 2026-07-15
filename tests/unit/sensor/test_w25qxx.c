#include "unity.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "xy_device.h"
#include "xy_hal_gpio.h"
#include "xy_os.h"
#include "xy_w25qxx.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

typedef enum {
    OP_TX,
    OP_RX,
} spi_op_kind_t;

typedef struct {
    spi_op_kind_t kind;
    uint8_t bytes[260];
    size_t len;
    xy_error_t ret;
} spi_op_t;

typedef struct {
    xy_hal_gpio_port_t port;
    uint8_t pin;
    uint8_t value;
} gpio_write_t;

static spi_op_t g_ops[256];
static gpio_write_t g_gpio[256];
static size_t g_op_count;
static size_t g_op_index;
static size_t g_gpio_count;
static uint32_t g_tick;
static uint32_t g_delay_total;
static xy_spi_device_t *g_init_dev;
static void *g_init_spi_handle;
static void *g_init_cs_pin;
static uint32_t g_init_speed;
static uint8_t g_init_mode;
static int g_printf_count;

static void queue_tx(const uint8_t *bytes, size_t len, xy_error_t ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_ops), g_op_count);
    TEST_ASSERT_LESS_OR_EQUAL_UINT(sizeof(g_ops[g_op_count].bytes), len);
    g_ops[g_op_count].kind = OP_TX;
    memcpy(g_ops[g_op_count].bytes, bytes, len);
    g_ops[g_op_count].len = len;
    g_ops[g_op_count].ret = ret;
    g_op_count++;
}

static void queue_tx1(uint8_t byte, xy_error_t ret)
{
    queue_tx(&byte, 1U, ret);
}

static void queue_rx(const uint8_t *bytes, size_t len, xy_error_t ret)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_ops), g_op_count);
    TEST_ASSERT_LESS_OR_EQUAL_UINT(sizeof(g_ops[g_op_count].bytes), len);
    g_ops[g_op_count].kind = OP_RX;
    memcpy(g_ops[g_op_count].bytes, bytes, len);
    g_ops[g_op_count].len = len;
    g_ops[g_op_count].ret = ret;
    g_op_count++;
}

static void queue_status(uint8_t status)
{
    uint8_t cmd = W25Q_CMD_READ_STATUS_REG1;
    queue_tx1(cmd, XY_OK);
    queue_rx(&status, 1U, XY_OK);
}

static void queue_wait_idle_busy_then_ready(void)
{
    queue_status(0x01U);
    queue_status(0x00U);
}

xy_error_t xy_spi_device_init(xy_spi_device_t *dev, void *spi_handle, void *cs_pin,
                              uint32_t speed, uint8_t mode)
{
    g_init_dev = dev;
    g_init_spi_handle = spi_handle;
    g_init_cs_pin = cs_pin;
    g_init_speed = speed;
    g_init_mode = mode;
    return XY_OK;
}

void xy_spi_device_cs(xy_spi_device_t *dev, bool select)
{
    (void)dev;
    (void)select;
}

xy_error_t xy_spi_device_transfer(xy_spi_device_t *dev, const uint8_t *tx, uint8_t *rx, size_t len)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_LESS_THAN_UINT(g_op_count, g_op_index);
    spi_op_t *op = &g_ops[g_op_index++];
    TEST_ASSERT_EQUAL_UINT(op->len, len);
    if (op->kind == OP_TX) {
        TEST_ASSERT_NOT_NULL(tx);
        TEST_ASSERT_NULL(rx);
        TEST_ASSERT_EQUAL_UINT8_ARRAY(op->bytes, tx, len);
    } else {
        TEST_ASSERT_NULL(tx);
        TEST_ASSERT_NOT_NULL(rx);
        if (op->ret == XY_OK) {
            memcpy(rx, op->bytes, len);
        }
    }
    return op->ret;
}

xy_hal_error_t xy_hal_gpio_write(xy_hal_gpio_port_t port, uint8_t pin, uint8_t value)
{
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_LEN(g_gpio), g_gpio_count);
    g_gpio[g_gpio_count].port = port;
    g_gpio[g_gpio_count].pin = pin;
    g_gpio[g_gpio_count].value = value;
    g_gpio_count++;
    return XY_HAL_OK;
}

xy_os_status_t xy_os_delay(uint32_t ticks)
{
    g_delay_total += ticks;
    g_tick += ticks;
    return XY_OS_OK;
}

uint32_t xy_os_tick_get(void)
{
    return g_tick++;
}

int xy_printf(const char *fmt, ...)
{
    (void)fmt;
    g_printf_count++;
    return 0;
}

void setUp(void)
{
    memset(g_ops, 0, sizeof(g_ops));
    memset(g_gpio, 0, sizeof(g_gpio));
    g_op_count = 0;
    g_op_index = 0;
    g_gpio_count = 0;
    g_tick = 0;
    g_delay_total = 0;
    g_init_dev = NULL;
    g_init_spi_handle = NULL;
    g_init_cs_pin = (void *)0x1;
    g_init_speed = 0;
    g_init_mode = 0xFFU;
    g_printf_count = 0;
}

void tearDown(void)
{
    TEST_ASSERT_EQUAL_UINT(g_op_count, g_op_index);
}

static xy_w25qxx_t make_ready_dev(void)
{
    xy_w25qxx_t dev;
    memset(&dev, 0, sizeof(dev));
    dev.cs_pin = 9U;
    dev.initialized = 1U;
    return dev;
}

static void queue_read_id(uint8_t manufacturer, uint8_t device)
{
    const uint8_t zero_addr[3] = {0U, 0U, 0U};
    const uint8_t rx[2] = {manufacturer, device};
    queue_tx1(W25Q_CMD_MANUFACTURER_ID, XY_OK);
    queue_tx(zero_addr, sizeof(zero_addr), XY_OK);
    queue_rx(rx, sizeof(rx), XY_OK);
}

static void test_w25qxx_init_identifies_known_and_unknown_models(void)
{
    xy_w25qxx_t dev;
    uint8_t spi_handle;

    TEST_ASSERT_EQUAL_INT(XY_W25Q_INVALID_PARAM, xy_w25qxx_init(NULL, &spi_handle, 9U));
    TEST_ASSERT_EQUAL_INT(XY_W25Q_INVALID_PARAM, xy_w25qxx_init(&dev, NULL, 9U));

    queue_tx1(W25Q_CMD_RELEASE_POWER_DOWN, XY_OK);
    queue_read_id(0xEFU, W25Q64_ID);
    TEST_ASSERT_EQUAL_INT(XY_W25Q_OK, xy_w25qxx_init(&dev, &spi_handle, 9U));
    TEST_ASSERT_EQUAL_PTR(&dev.spi_dev, g_init_dev);
    TEST_ASSERT_EQUAL_PTR(&spi_handle, g_init_spi_handle);
    TEST_ASSERT_NULL(g_init_cs_pin);
    TEST_ASSERT_EQUAL_UINT32(1000000U, g_init_speed);
    TEST_ASSERT_EQUAL_UINT8(0U, g_init_mode);
    TEST_ASSERT_EQUAL_UINT8(9U, dev.cs_pin);
    TEST_ASSERT_EQUAL_INT(W25Q64, dev.model);
    TEST_ASSERT_EQUAL_UINT32(8U * 1024U * 1024U, dev.capacity_bytes);
    TEST_ASSERT_EQUAL_UINT32(2048U, dev.sector_count);
    TEST_ASSERT_EQUAL_UINT8(1U, dev.initialized);
    TEST_ASSERT_EQUAL_UINT32(20U, g_delay_total);
    TEST_ASSERT_GREATER_THAN_INT(0, g_printf_count);

    queue_tx1(W25Q_CMD_RELEASE_POWER_DOWN, XY_OK);
    queue_read_id(0xEFU, 0x99U);
    TEST_ASSERT_EQUAL_INT(XY_W25Q_OK, xy_w25qxx_init(&dev, &spi_handle, 9U));
    TEST_ASSERT_EQUAL_INT(W25Q_UNKNOWN, dev.model);
    TEST_ASSERT_EQUAL_UINT32(0U, dev.capacity_bytes);
    TEST_ASSERT_EQUAL_UINT32(0U, dev.sector_count);
}

static void test_w25qxx_read_id_status_power_and_deinit(void)
{
    xy_w25qxx_t dev = make_ready_dev();
    uint8_t manufacturer = 0;
    uint8_t device = 0;
    uint8_t status = 0;

    TEST_ASSERT_EQUAL_INT(XY_W25Q_INVALID_PARAM, xy_w25qxx_read_id(NULL, &manufacturer, &device));
    TEST_ASSERT_EQUAL_INT(XY_W25Q_INVALID_PARAM, xy_w25qxx_read_id(&dev, NULL, &device));
    TEST_ASSERT_EQUAL_INT(XY_W25Q_INVALID_PARAM, xy_w25qxx_read_status(&dev, NULL));

    queue_read_id(0xEFU, W25Q32_ID);
    TEST_ASSERT_EQUAL_INT(XY_W25Q_OK, xy_w25qxx_read_id(&dev, &manufacturer, &device));
    TEST_ASSERT_EQUAL_UINT8(0xEFU, manufacturer);
    TEST_ASSERT_EQUAL_UINT8(W25Q32_ID, device);

    queue_status(0xA5U);
    TEST_ASSERT_EQUAL_INT(XY_W25Q_OK, xy_w25qxx_read_status(&dev, &status));
    TEST_ASSERT_EQUAL_UINT8(0xA5U, status);

    queue_tx1(W25Q_CMD_POWER_DOWN, XY_OK);
    TEST_ASSERT_EQUAL_INT(XY_W25Q_OK, xy_w25qxx_power_down(&dev));
    queue_tx1(W25Q_CMD_RELEASE_POWER_DOWN, XY_OK);
    TEST_ASSERT_EQUAL_INT(XY_W25Q_OK, xy_w25qxx_release_power_down(&dev));
    TEST_ASSERT_EQUAL_UINT32(10U, g_delay_total);

    queue_tx1(W25Q_CMD_POWER_DOWN, XY_OK);
    TEST_ASSERT_EQUAL_INT(XY_W25Q_OK, xy_w25qxx_deinit(&dev));
    TEST_ASSERT_EQUAL_UINT8(0U, dev.initialized);
    TEST_ASSERT_EQUAL_INT(XY_W25Q_INVALID_PARAM, xy_w25qxx_deinit(NULL));
}

static void test_w25qxx_erase_program_and_read_data(void)
{
    xy_w25qxx_t dev = make_ready_dev();
    const uint8_t payload[4] = {1U, 2U, 3U, 4U};
    uint8_t read_buf[4] = {0};
    const uint8_t sector_addr[4] = {W25Q_CMD_SECTOR_ERASE, 0x01U, 0x00U, 0x00U};
    const uint8_t block_addr[4] = {W25Q_CMD_BLOCK_ERASE_64K, 0x02U, 0x00U, 0x00U};
    const uint8_t page_addr[4] = {W25Q_CMD_PAGE_PROGRAM, 0x00U, 0x12U, 0x34U};
    const uint8_t read_addr[4] = {W25Q_CMD_READ_DATA, 0x00U, 0x12U, 0x34U};
    const uint8_t read_data[4] = {9U, 8U, 7U, 6U};

    TEST_ASSERT_EQUAL_INT(XY_W25Q_INVALID_PARAM, xy_w25qxx_sector_erase(NULL, 0));
    TEST_ASSERT_EQUAL_INT(XY_W25Q_INVALID_PARAM, xy_w25qxx_sector_erase(&dev, 1));
    TEST_ASSERT_EQUAL_INT(XY_W25Q_INVALID_PARAM, xy_w25qxx_block_erase(&dev, 1));
    TEST_ASSERT_EQUAL_INT(XY_W25Q_INVALID_PARAM, xy_w25qxx_page_program(&dev, 0, NULL, 1));
    TEST_ASSERT_EQUAL_INT(XY_W25Q_INVALID_PARAM, xy_w25qxx_read_data(&dev, 0, NULL, 1));

    queue_tx1(W25Q_CMD_WRITE_ENABLE, XY_OK);
    queue_tx(sector_addr, sizeof(sector_addr), XY_OK);
    queue_wait_idle_busy_then_ready();
    TEST_ASSERT_EQUAL_INT(XY_W25Q_OK, xy_w25qxx_sector_erase(&dev, 0x00010000U));

    queue_tx1(W25Q_CMD_WRITE_ENABLE, XY_OK);
    queue_tx(block_addr, sizeof(block_addr), XY_OK);
    queue_status(0x00U);
    TEST_ASSERT_EQUAL_INT(XY_W25Q_OK, xy_w25qxx_block_erase(&dev, 0x00020000U));

    queue_tx1(W25Q_CMD_WRITE_ENABLE, XY_OK);
    queue_tx(page_addr, sizeof(page_addr), XY_OK);
    queue_tx(payload, sizeof(payload), XY_OK);
    queue_status(0x00U);
    TEST_ASSERT_EQUAL_INT(XY_W25Q_OK, xy_w25qxx_page_program(&dev, 0x1234U, payload, sizeof(payload)));

    queue_tx(read_addr, sizeof(read_addr), XY_OK);
    queue_rx(read_data, sizeof(read_data), XY_OK);
    TEST_ASSERT_EQUAL_INT(XY_W25Q_OK, xy_w25qxx_read_data(&dev, 0x1234U, read_buf, sizeof(read_buf)));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(read_data, read_buf, sizeof(read_data));
}

static void test_w25qxx_write_data_pages_and_timeout(void)
{
    xy_w25qxx_t dev = make_ready_dev();
    uint8_t data[300];
    uint8_t first_page[16];
    uint8_t second_page[256];
    uint8_t third_page[28];

    const uint8_t first_page_addr[4] = {W25Q_CMD_PAGE_PROGRAM, 0x00U, 0x00U, 0xF0U};
    const uint8_t second_page_addr[4] = {W25Q_CMD_PAGE_PROGRAM, 0x00U, 0x01U, 0x00U};
    const uint8_t third_page_addr[4] = {W25Q_CMD_PAGE_PROGRAM, 0x00U, 0x02U, 0x00U};

    for (size_t i = 0; i < sizeof(data); i++) {
        data[i] = (uint8_t)i;
    }
    memcpy(first_page, &data[0], sizeof(first_page));
    memcpy(second_page, &data[16], sizeof(second_page));
    memcpy(third_page, &data[272], sizeof(third_page));

    TEST_ASSERT_EQUAL_INT(XY_W25Q_INVALID_PARAM, xy_w25qxx_write_data(NULL, 0, data, 1));
    TEST_ASSERT_EQUAL_INT(XY_W25Q_INVALID_PARAM, xy_w25qxx_write_data(&dev, 0, NULL, 1));

    queue_tx1(W25Q_CMD_WRITE_ENABLE, XY_OK);
    queue_tx(first_page_addr, sizeof(first_page_addr), XY_OK);
    queue_tx(first_page, sizeof(first_page), XY_OK);
    queue_status(0x00U);
    queue_tx1(W25Q_CMD_WRITE_ENABLE, XY_OK);
    queue_tx(second_page_addr, sizeof(second_page_addr), XY_OK);
    queue_tx(second_page, sizeof(second_page), XY_OK);
    queue_status(0x00U);
    queue_tx1(W25Q_CMD_WRITE_ENABLE, XY_OK);
    queue_tx(third_page_addr, sizeof(third_page_addr), XY_OK);
    queue_tx(third_page, sizeof(third_page), XY_OK);
    queue_status(0x00U);
    TEST_ASSERT_EQUAL_INT(XY_W25Q_OK, xy_w25qxx_write_data(&dev, 0xF0U, data, sizeof(data)));

    queue_status(0x01U);
    TEST_ASSERT_EQUAL_INT(XY_W25Q_BUSY, xy_w25qxx_wait_idle(&dev, 1U));
}


static void test_w25qxx_chip_select_edges_for_id_and_power_commands(void)
{
    xy_w25qxx_t dev = make_ready_dev();
    uint8_t manufacturer = 0;
    uint8_t device = 0;

    queue_read_id(0xEFU, W25Q16_ID);
    TEST_ASSERT_EQUAL_INT(XY_W25Q_OK, xy_w25qxx_read_id(&dev, &manufacturer, &device));
    queue_tx1(W25Q_CMD_POWER_DOWN, XY_OK);
    TEST_ASSERT_EQUAL_INT(XY_W25Q_OK, xy_w25qxx_power_down(&dev));

    TEST_ASSERT_EQUAL_UINT(4U, g_gpio_count);
    TEST_ASSERT_EQUAL_UINT8(9U, g_gpio[0].pin);
    TEST_ASSERT_EQUAL_UINT8(0U, g_gpio[0].value);
    TEST_ASSERT_EQUAL_UINT8(1U, g_gpio[1].value);
    TEST_ASSERT_EQUAL_UINT8(0U, g_gpio[2].value);
    TEST_ASSERT_EQUAL_UINT8(1U, g_gpio[3].value);
    TEST_ASSERT_EQUAL_UINT8(0xEFU, manufacturer);
    TEST_ASSERT_EQUAL_UINT8(W25Q16_ID, device);
}

static void test_w25qxx_chip_erase_busy_then_ready(void)
{
    xy_w25qxx_t dev = make_ready_dev();
    const uint8_t chip_cmd = W25Q_CMD_CHIP_ERASE;

    TEST_ASSERT_EQUAL_INT(XY_W25Q_INVALID_PARAM, xy_w25qxx_chip_erase(NULL));

    queue_tx1(W25Q_CMD_WRITE_ENABLE, XY_OK);
    queue_tx(&chip_cmd, sizeof(chip_cmd), XY_OK);
    queue_status(0x01U);
    queue_status(0x00U);
    TEST_ASSERT_EQUAL_INT(XY_W25Q_OK, xy_w25qxx_chip_erase(&dev));
}

static void test_w25qxx_zero_length_write_is_noop(void)
{
    xy_w25qxx_t dev = make_ready_dev();
    uint8_t data = 0x5AU;

    TEST_ASSERT_EQUAL_INT(XY_W25Q_INVALID_PARAM, xy_w25qxx_page_program(&dev, 0, &data, 257U));
    TEST_ASSERT_EQUAL_INT(XY_W25Q_OK, xy_w25qxx_write_data(&dev, 0x1234U, &data, 0U));
}

static void test_w25qxx_transfer_failures_preserve_existing_contracts(void)
{
    xy_w25qxx_t dev = make_ready_dev();
    uint8_t read_buf[3] = {1U, 2U, 3U};
    const uint8_t read_addr[4] = {W25Q_CMD_READ_DATA, 0x00U, 0x00U, 0x44U};

    queue_tx1(W25Q_CMD_WRITE_ENABLE, XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_W25Q_OK, xy_w25qxx_write_enable(&dev));

    const uint8_t ignored_rx[3] = {0U, 0U, 0U};
    queue_tx(read_addr, sizeof(read_addr), XY_OK);
    queue_rx(ignored_rx, sizeof(read_buf), XY_DEVICE_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_W25Q_OK, xy_w25qxx_read_data(&dev, 0x44U, read_buf, sizeof(read_buf)));
    TEST_ASSERT_EQUAL_UINT8(1U, read_buf[0]);
    TEST_ASSERT_EQUAL_UINT8(2U, read_buf[1]);
    TEST_ASSERT_EQUAL_UINT8(3U, read_buf[2]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_w25qxx_init_identifies_known_and_unknown_models);
    RUN_TEST(test_w25qxx_read_id_status_power_and_deinit);
    RUN_TEST(test_w25qxx_erase_program_and_read_data);
    RUN_TEST(test_w25qxx_write_data_pages_and_timeout);
    RUN_TEST(test_w25qxx_chip_select_edges_for_id_and_power_commands);
    RUN_TEST(test_w25qxx_chip_erase_busy_then_ready);
    RUN_TEST(test_w25qxx_zero_length_write_is_noop);
    RUN_TEST(test_w25qxx_transfer_failures_preserve_existing_contracts);
    return UNITY_END();
}
