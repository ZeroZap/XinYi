#include "unity.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "xy_aht10.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
    uint8_t data[8];
    size_t len;
    xy_error_t result;
} io_step_t;

static io_step_t g_reads[8];
static io_step_t g_writes[8];
static size_t g_read_count;
static size_t g_read_index;
static size_t g_write_count;
static size_t g_write_index;
static xy_error_t g_init_result;
static size_t g_init_count;
static uint32_t g_delay_ms;

xy_error_t xy_i2c_device_init(xy_i2c_device_t *dev, void *handle, uint16_t address,
                              uint32_t timeout)
{
    g_init_count++;
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_NOT_NULL(handle);
    TEST_ASSERT_EQUAL_UINT16(XY_AHT10_DEFAULT_ADDRESS, address);
    TEST_ASSERT_EQUAL_UINT32(100U, timeout);
    if (g_init_result != XY_DEVICE_OK) {
        return g_init_result;
    }
    memset(dev, 0, sizeof(*dev));
    dev->base.initialized = 1U;
    dev->i2c_handle = handle;
    dev->dev_addr = address;
    dev->timeout = timeout;
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_write(xy_i2c_device_t *dev, const uint8_t *data, size_t len)
{
    io_step_t *step;

    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_SIZE(g_writes), g_write_index);
    step = &g_writes[g_write_index++];
    TEST_ASSERT_EQUAL_UINT(step->len, len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(step->data, data, len);
    return step->result;
}

xy_error_t xy_i2c_device_read(xy_i2c_device_t *dev, uint8_t *data, size_t len)
{
    io_step_t *step;

    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_LESS_THAN_UINT(ARRAY_SIZE(g_reads), g_read_index);
    step = &g_reads[g_read_index++];
    TEST_ASSERT_EQUAL_UINT(step->len, len);
    if (step->result == XY_DEVICE_OK) {
        memcpy(data, step->data, len);
    }
    return step->result;
}

void xy_hal_delay_ms(uint32_t ms)
{
    g_delay_ms += ms;
}

static void queue_write(const uint8_t *data, size_t len, xy_error_t result)
{
    io_step_t *step = &g_writes[g_write_count++];
    memcpy(step->data, data, len);
    step->len = len;
    step->result = result;
}

static void queue_read(const uint8_t *data, size_t len, xy_error_t result)
{
    io_step_t *step = &g_reads[g_read_count++];
    if (data != NULL) {
        memcpy(step->data, data, len);
    }
    step->len = len;
    step->result = result;
}

void setUp(void)
{
    memset(g_reads, 0, sizeof(g_reads));
    memset(g_writes, 0, sizeof(g_writes));
    g_read_count = 0U;
    g_read_index = 0U;
    g_write_count = 0U;
    g_write_index = 0U;
    g_init_result = XY_DEVICE_OK;
    g_init_count = 0U;
    g_delay_ms = 0U;
}

void tearDown(void)
{
}

static void test_init_configures_device_and_fails_atomically(void)
{
    static const uint8_t init_command[] = {0xE1U, 0x08U, 0x00U};
    xy_aht10_t dev;
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_aht10_init(NULL, &bus, 0U));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_aht10_init(&dev, NULL, 0U));

    memset(&dev, 0xA5, sizeof(dev));
    g_init_result = XY_DEVICE_TIMEOUT;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_aht10_init(&dev, &bus, 0U));
    TEST_ASSERT_EQUAL_MEMORY(&(xy_aht10_t){0}, &dev, sizeof(dev));
    TEST_ASSERT_EQUAL_UINT(1U, g_init_count);
    TEST_ASSERT_EQUAL_UINT(0U, g_write_index);

    setUp();
    queue_write(init_command, sizeof(init_command), XY_DEVICE_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_aht10_init(&dev, &bus, 0U));
    TEST_ASSERT_EQUAL_MEMORY(&(xy_aht10_t){0}, &dev, sizeof(dev));
    TEST_ASSERT_EQUAL_UINT32(0U, g_delay_ms);

    setUp();
    queue_write(init_command, sizeof(init_command), XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_aht10_init(&dev, &bus, 0U));
    TEST_ASSERT_TRUE(dev.initialized);
    TEST_ASSERT_TRUE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_EQUAL_UINT16(XY_AHT10_DEFAULT_ADDRESS, dev.i2c_dev.dev_addr);
    TEST_ASSERT_EQUAL_UINT32(10U, g_delay_ms);
}

static void test_read_converts_both_channels_and_commits_atomically(void)
{
    static const uint8_t init_command[] = {0xE1U, 0x08U, 0x00U};
    static const uint8_t measure_command[] = {0xACU, 0x33U, 0x00U};
    static const uint8_t frame[] = {0x00U, 0x80U, 0x00U, 0x08U, 0x00U, 0x00U};
    xy_aht10_data_t output = {0};
    xy_aht10_t dev;
    int bus;

    queue_write(init_command, sizeof(init_command), XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_aht10_init(&dev, &bus, 0U));
    queue_write(measure_command, sizeof(measure_command), XY_DEVICE_OK);
    queue_read(frame, sizeof(frame), XY_DEVICE_OK);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_aht10_read(&dev, &output));
    TEST_ASSERT_EQUAL_UINT32(5000U, output.humidity_centi_pct);
    TEST_ASSERT_EQUAL_INT32(5000, output.temperature_centi_c);
    TEST_ASSERT_EQUAL_MEMORY(&output, &dev.data, sizeof(output));
    TEST_ASSERT_EQUAL_UINT32(90U, g_delay_ms);
}

static void test_read_failures_preserve_output_cache_and_stop_io(void)
{
    static const uint8_t init_command[] = {0xE1U, 0x08U, 0x00U};
    static const uint8_t measure_command[] = {0xACU, 0x33U, 0x00U};
    static const uint8_t busy_frame[] = {0x80U, 0U, 0U, 0U, 0U, 0U};
    xy_aht10_data_t output = {.temperature_centi_c = -123, .humidity_centi_pct = 456U};
    xy_aht10_data_t before = output;
    xy_aht10_t dev;
    int bus;

    queue_write(init_command, sizeof(init_command), XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_aht10_init(&dev, &bus, 0U));
    dev.data.temperature_centi_c = 321;
    dev.data.humidity_centi_pct = 654U;

    queue_write(measure_command, sizeof(measure_command), XY_DEVICE_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_aht10_read(&dev, &output));
    TEST_ASSERT_EQUAL_MEMORY(&before, &output, sizeof(output));
    TEST_ASSERT_EQUAL_INT32(321, dev.data.temperature_centi_c);
    TEST_ASSERT_EQUAL_UINT32(654U, dev.data.humidity_centi_pct);
    TEST_ASSERT_EQUAL_UINT(0U, g_read_index);

    queue_write(measure_command, sizeof(measure_command), XY_DEVICE_OK);
    queue_read(NULL, 6U, XY_DEVICE_IO_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR, xy_aht10_read(&dev, &output));
    TEST_ASSERT_EQUAL_MEMORY(&before, &output, sizeof(output));

    queue_write(measure_command, sizeof(measure_command), XY_DEVICE_OK);
    queue_read(busy_frame, sizeof(busy_frame), XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_BUSY, xy_aht10_read(&dev, &output));
    TEST_ASSERT_EQUAL_MEMORY(&before, &output, sizeof(output));
}

static void test_public_ops_require_both_lifecycle_layers(void)
{
    static const uint8_t init_command[] = {0xE1U, 0x08U, 0x00U};
    xy_aht10_data_t output = {.temperature_centi_c = 1, .humidity_centi_pct = 2U};
    xy_aht10_t dev;
    int bus;

    memset(&dev, 0, sizeof(dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_aht10_read(NULL, &output));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_aht10_read(&dev, &output));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_aht10_deinit(&dev));

    queue_write(init_command, sizeof(init_command), XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_aht10_init(&dev, &bus, 0U));
    dev.i2c_dev.base.initialized = 0U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_aht10_read(&dev, &output));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_aht10_deinit(&dev));
    TEST_ASSERT_EQUAL_UINT(1U, g_write_index);
    TEST_ASSERT_EQUAL_UINT(0U, g_read_index);

    dev.i2c_dev.base.initialized = 1U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_aht10_deinit(&dev));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_aht10_deinit(&dev));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_configures_device_and_fails_atomically);
    RUN_TEST(test_read_converts_both_channels_and_commits_atomically);
    RUN_TEST(test_read_failures_preserve_output_cache_and_stop_io);
    RUN_TEST(test_public_ops_require_both_lifecycle_layers);
    return UNITY_END();
}
