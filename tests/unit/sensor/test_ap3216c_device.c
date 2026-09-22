#include "unity.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "xy_ap3216c.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
    uint8_t reg;
    uint8_t data[6];
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
    TEST_ASSERT_EQUAL_UINT16(XY_AP3216C_DEFAULT_ADDRESS, address);
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

xy_error_t xy_i2c_device_read_reg(xy_i2c_device_t *dev, uint8_t reg, uint8_t *data, size_t len)
{
    io_step_t *step;

    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_LESS_THAN_UINT(g_read_count, g_read_index);
    step = &g_reads[g_read_index++];
    TEST_ASSERT_EQUAL_UINT8(step->reg, reg);
    TEST_ASSERT_EQUAL_UINT(step->len, len);
    if (step->result == XY_DEVICE_OK) {
        memcpy(data, step->data, len);
    }
    return step->result;
}

xy_error_t xy_i2c_device_write_reg(xy_i2c_device_t *dev, uint8_t reg, const uint8_t *data,
                                    size_t len)
{
    io_step_t *step;

    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_LESS_THAN_UINT(g_write_count, g_write_index);
    step = &g_writes[g_write_index++];
    TEST_ASSERT_EQUAL_UINT8(step->reg, reg);
    TEST_ASSERT_EQUAL_UINT(step->len, len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(step->data, data, len);
    return step->result;
}

void xy_hal_delay_ms(uint32_t ms)
{
    g_delay_ms += ms;
}

static void queue_read(uint8_t reg, const uint8_t *data, size_t len, xy_error_t result)
{
    io_step_t *step = &g_reads[g_read_count++];
    step->reg = reg;
    step->len = len;
    step->result = result;
    if (data != NULL) {
        memcpy(step->data, data, len);
    }
}

static void queue_write(uint8_t reg, uint8_t value, xy_error_t result)
{
    io_step_t *step = &g_writes[g_write_count++];
    step->reg = reg;
    step->data[0] = value;
    step->len = 1U;
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

static void init_device(xy_ap3216c_t *dev, int *bus)
{
    queue_write(XY_AP3216C_REG_SYSTEM_CONFIG, XY_AP3216C_MODE_RESET, XY_DEVICE_OK);
    queue_write(XY_AP3216C_REG_SYSTEM_CONFIG, XY_AP3216C_MODE_ALS_PS, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_ap3216c_init(dev, bus, 0U, XY_AP3216C_MODE_ALS_PS));
}

static void test_init_and_deinit_are_atomic(void)
{
    xy_ap3216c_t dev;
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_ap3216c_init(NULL, &bus, 0U, XY_AP3216C_MODE_ALS_PS));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_ap3216c_init(&dev, NULL, 0U, XY_AP3216C_MODE_ALS_PS));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_ap3216c_init(&dev, &bus, 0x39U, XY_AP3216C_MODE_ALS_PS));

    memset(&dev, 0xA5, sizeof(dev));
    g_init_result = XY_DEVICE_TIMEOUT;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT,
                          xy_ap3216c_init(&dev, &bus, 0U, XY_AP3216C_MODE_ALS_PS));
    TEST_ASSERT_EQUAL_MEMORY(&(xy_ap3216c_t){0}, &dev, sizeof(dev));

    setUp();
    queue_write(XY_AP3216C_REG_SYSTEM_CONFIG, XY_AP3216C_MODE_RESET, XY_DEVICE_IO_ERROR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR,
                          xy_ap3216c_init(&dev, &bus, 0U, XY_AP3216C_MODE_ALS_PS));
    TEST_ASSERT_EQUAL_MEMORY(&(xy_ap3216c_t){0}, &dev, sizeof(dev));
    TEST_ASSERT_EQUAL_UINT32(0U, g_delay_ms);

    setUp();
    init_device(&dev, &bus);
    TEST_ASSERT_TRUE(dev.initialized);
    TEST_ASSERT_EQUAL_UINT8(XY_AP3216C_MODE_ALS_PS, dev.mode);
    TEST_ASSERT_EQUAL_UINT32(100U, g_delay_ms);

    queue_write(XY_AP3216C_REG_SYSTEM_CONFIG, XY_AP3216C_MODE_POWER_DOWN,
                XY_DEVICE_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_ap3216c_deinit(&dev));
    TEST_ASSERT_TRUE(dev.initialized);
    queue_write(XY_AP3216C_REG_SYSTEM_CONFIG, XY_AP3216C_MODE_POWER_DOWN, XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ap3216c_deinit(&dev));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
}

static void test_channel_reads_decode_and_commit_cache(void)
{
    static const uint8_t als[] = {0x34U, 0x12U};
    static const uint8_t ps[] = {0x8AU, 0x21U};
    static const uint8_t ir[] = {0xAAU, 0x03U};
    xy_ap3216c_t dev;
    uint32_t lux_milli = 0U;
    uint16_t proximity = 0U;
    uint16_t infrared = 0U;
    int bus;

    init_device(&dev, &bus);
    queue_read(XY_AP3216C_REG_ALS_DATA_L, als, sizeof(als), XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ap3216c_read_light(&dev, &lux_milli));
    TEST_ASSERT_EQUAL_UINT32(0x1234U * 350U, lux_milli);
    queue_read(XY_AP3216C_REG_PS_DATA_L, ps, sizeof(ps), XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ap3216c_read_proximity(&dev, &proximity));
    TEST_ASSERT_EQUAL_UINT16(0x21AU, proximity);
    queue_read(XY_AP3216C_REG_IR_DATA_L, ir, sizeof(ir), XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_ap3216c_read_ir(&dev, &infrared));
    TEST_ASSERT_EQUAL_UINT16(0x3AAU, infrared);
    TEST_ASSERT_EQUAL_UINT32(lux_milli, dev.data.illuminance_millilux);
    TEST_ASSERT_EQUAL_UINT16(proximity, dev.data.proximity_raw);
    TEST_ASSERT_EQUAL_UINT16(infrared, dev.data.infrared_raw);
}

static void test_failures_preserve_outputs_cache_and_stop_io(void)
{
    static const uint8_t overflow[] = {0x40U, 0x00U};
    xy_ap3216c_t dev;
    xy_ap3216c_data_t cache;
    uint32_t light = 123U;
    uint16_t proximity = 456U;
    int bus;

    init_device(&dev, &bus);
    dev.data.illuminance_millilux = 11U;
    dev.data.proximity_raw = 22U;
    dev.data.infrared_raw = 33U;
    cache = dev.data;

    queue_read(XY_AP3216C_REG_ALS_DATA_L, NULL, 2U, XY_DEVICE_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_ap3216c_read_light(&dev, &light));
    TEST_ASSERT_EQUAL_UINT32(123U, light);
    TEST_ASSERT_EQUAL_MEMORY(&cache, &dev.data, sizeof(cache));

    queue_read(XY_AP3216C_REG_PS_DATA_L, overflow, sizeof(overflow), XY_DEVICE_OK);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_IO_ERROR,
                          xy_ap3216c_read_proximity(&dev, &proximity));
    TEST_ASSERT_EQUAL_UINT16(456U, proximity);
    TEST_ASSERT_EQUAL_MEMORY(&cache, &dev.data, sizeof(cache));
}

static void test_public_reads_require_outer_and_nested_lifecycle(void)
{
    xy_ap3216c_t dev;
    uint32_t light = 1U;
    uint16_t raw = 2U;
    int bus;

    memset(&dev, 0, sizeof(dev));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_ap3216c_read_light(NULL, &light));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_ap3216c_read_light(&dev, &light));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_ap3216c_read_proximity(&dev, &raw));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_ap3216c_read_ir(&dev, &raw));

    init_device(&dev, &bus);
    dev.i2c_dev.base.initialized = 0U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_ap3216c_read_light(&dev, &light));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_ap3216c_read_proximity(&dev, &raw));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_ap3216c_read_ir(&dev, &raw));
    TEST_ASSERT_EQUAL_UINT(g_read_count, g_read_index);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_and_deinit_are_atomic);
    RUN_TEST(test_channel_reads_decode_and_commit_cache);
    RUN_TEST(test_failures_preserve_outputs_cache_and_stop_io);
    RUN_TEST(test_public_reads_require_outer_and_nested_lifecycle);
    return UNITY_END();
}