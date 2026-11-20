#include "unity.h"
#include "xy_apds9960.h"

#include <string.h>

static xy_error_t g_init_error;
static xy_error_t g_read_error;
static xy_error_t g_write_error;
static int g_init_establish_transport;
static uint32_t g_read_count;
static uint32_t g_write_count;
static uint8_t g_rgb[8] = {0x78, 0x56, 0x34, 0x12, 0xcd, 0xab, 0xef, 0xbe};
static uint8_t g_id_value = 0xabU;
static uint8_t g_gesture_status;
static uint8_t g_fifo_level;
static uint8_t g_fifo_data[XY_APDS9960_MAX_GESTURE_DATA];
static uint32_t g_fifo_read_length;
static uint8_t g_fail_read_reg = 0xffU;
static uint8_t g_last_write_reg;
static uint8_t g_last_write_value;

xy_error_t xy_i2c_device_init(xy_i2c_device_t *dev, void *handle, uint16_t address,
                              uint32_t timeout)
{
    memset(dev, 0, sizeof(*dev));
    dev->base.initialized = g_init_establish_transport;
    dev->i2c_handle = g_init_establish_transport ? handle : NULL;
    dev->dev_addr = address;
    dev->timeout = timeout;
    return g_init_error;
}

xy_error_t xy_i2c_device_read_reg(xy_i2c_device_t *dev, uint8_t reg, uint8_t *buffer,
                                  size_t length)
{
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(dev->i2c_handle);
    g_read_count++;
    if (g_read_error != XY_DEVICE_OK &&
        (g_fail_read_reg == 0xffU || reg == g_fail_read_reg)) {
        return g_read_error;
    }
    if (reg == XY_APDS9960_REG_ID) {
        *buffer = g_id_value;
    } else if (reg == XY_APDS9960_REG_CDATAL) {
        memcpy(buffer, g_rgb, length);
    } else if (reg == XY_APDS9960_REG_PDATA) {
        *buffer = 42U;
    } else if (reg == XY_APDS9960_REG_GSTATUS) {
        *buffer = g_gesture_status;
    } else if (reg == XY_APDS9960_REG_GFLVL) {
        *buffer = g_fifo_level;
    } else if (reg == XY_APDS9960_REG_GFIFO_U) {
        g_fifo_read_length = (uint32_t)length;
        size_t copied = length;
        if (copied > sizeof(g_fifo_data)) {
            copied = sizeof(g_fifo_data);
        }
        memcpy(buffer, g_fifo_data, copied);
    }
    return XY_DEVICE_OK;
}

xy_error_t xy_i2c_device_write_reg(xy_i2c_device_t *dev, uint8_t reg, const uint8_t *buffer,
                                   size_t length)
{
    (void)length;
    TEST_ASSERT_TRUE(dev->base.initialized);
    TEST_ASSERT_NOT_NULL(dev->i2c_handle);
    g_write_count++;
    g_last_write_reg = reg;
    g_last_write_value = buffer[0];
    return g_write_error;
}

uint32_t xy_hal_sys_get_tick_count(void)
{
    return 77U;
}

void setUp(void)
{
    g_init_error = XY_DEVICE_OK;
    g_read_error = XY_DEVICE_OK;
    g_write_error = XY_DEVICE_OK;
    g_init_establish_transport = 1;
    g_read_count = 0U;
    g_write_count = 0U;
    g_id_value = 0xabU;
    g_gesture_status = 0U;
    g_fifo_level = 0U;
    g_fifo_read_length = 0U;
    g_fail_read_reg = 0xffU;
    g_last_write_reg = 0U;
    g_last_write_value = 0U;
    memset(g_fifo_data, 0, sizeof(g_fifo_data));
}

void tearDown(void)
{
}

static void test_apds9960_init_read_and_deinit(void)
{
    xy_apds9960_t dev;
    xy_apds9960_rgb_t rgb;
    xy_apds9960_proximity_t proximity;
    xy_apds9960_gesture_fifo_t gesture;
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_apds9960_init(&dev, &bus));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_apds9960_read_rgb(&dev, &rgb));
    TEST_ASSERT_EQUAL_UINT16(0x5678U, rgb.clear);
    TEST_ASSERT_EQUAL_UINT16(0x1234U, rgb.red);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_apds9960_read_proximity(&dev, &proximity));
    TEST_ASSERT_EQUAL_UINT8(42U, proximity.proximity);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_apds9960_read_gesture_fifo(&dev, &gesture));
    TEST_ASSERT_EQUAL_UINT32(77U, gesture.timestamp);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_apds9960_deinit(&dev));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
}

static void test_apds9960_init_failure_clears_partial_transport(void)
{
    xy_apds9960_t dev;
    int bus;

    memset(&dev, 0xa5, sizeof(dev));
    g_init_error = XY_DEVICE_TIMEOUT;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_apds9960_init(&dev, &bus));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_UINT32(0U, g_read_count);
    TEST_ASSERT_EQUAL_UINT32(0U, g_write_count);
}

static void test_apds9960_init_rejects_incomplete_nested_transport(void)
{
    xy_apds9960_t dev;
    int bus;

    memset(&dev, 0xa5, sizeof(dev));
    g_init_establish_transport = 0;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_apds9960_init(&dev, &bus));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_UINT32(0U, g_read_count);
    TEST_ASSERT_EQUAL_UINT32(0U, g_write_count);
}

static void test_apds9960_rejects_lost_nested_transport_without_io(void)
{
    xy_apds9960_t dev;
    xy_apds9960_rgb_t rgb = {1U, 2U, 3U, 4U, 5U};
    xy_apds9960_proximity_t proximity = {6U, 7U};
    xy_apds9960_gesture_fifo_t gesture;
    uint32_t reads_before;
    uint32_t writes_before;
    int bus;

    memset(&gesture, 0x5a, sizeof(gesture));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_apds9960_init(&dev, &bus));
    dev.rgb = rgb;
    dev.proximity = proximity;
    dev.gesture = gesture;
    reads_before = g_read_count;
    writes_before = g_write_count;

    dev.i2c_dev.base.initialized = 0U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_apds9960_read_rgb(&dev, &rgb));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_apds9960_read_proximity(&dev, &proximity));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_apds9960_read_gesture_fifo(&dev, &gesture));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_apds9960_deinit(&dev));
    TEST_ASSERT_EQUAL_UINT32(reads_before, g_read_count);
    TEST_ASSERT_EQUAL_UINT32(writes_before, g_write_count);
    TEST_ASSERT_EQUAL_UINT16(1U, rgb.clear);
    TEST_ASSERT_EQUAL_UINT8(6U, proximity.proximity);
    TEST_ASSERT_EQUAL_UINT8(0x5aU, gesture.level);

    dev.i2c_dev.base.initialized = 1U;
    dev.i2c_dev.i2c_handle = NULL;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_apds9960_read_rgb(&dev, &rgb));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_apds9960_deinit(&dev));
    TEST_ASSERT_EQUAL_UINT32(reads_before, g_read_count);
    TEST_ASSERT_EQUAL_UINT32(writes_before, g_write_count);
}

static void test_apds9960_transport_failures_preserve_state(void)
{
    xy_apds9960_t dev;
    xy_apds9960_rgb_t rgb = {1U, 2U, 3U, 4U, 5U};
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_apds9960_init(&dev, &bus));
    dev.rgb = rgb;
    g_read_error = XY_DEVICE_TIMEOUT;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_apds9960_read_rgb(&dev, &rgb));
    TEST_ASSERT_EQUAL_UINT16(1U, rgb.clear);
    TEST_ASSERT_EQUAL_UINT16(1U, dev.rgb.clear);

    g_read_error = XY_DEVICE_OK;
    g_write_error = XY_DEVICE_TIMEOUT;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_apds9960_deinit(&dev));
    TEST_ASSERT_TRUE(dev.initialized);
    TEST_ASSERT_TRUE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NOT_NULL(dev.i2c_dev.i2c_handle);
}

static void test_apds9960_reinit_failure_preserves_live_owner(void)
{
    xy_apds9960_t dev;
    xy_apds9960_rgb_t rgb = {11U, 22U, 33U, 44U, 55U};
    xy_apds9960_proximity_t proximity = {66U, 77U};
    xy_apds9960_gesture_fifo_t gesture;
    xy_i2c_device_t old_i2c;
    int bus;

    memset(&gesture, 0x3c, sizeof(gesture));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_apds9960_init(&dev, &bus));
    dev.rgb = rgb;
    dev.proximity = proximity;
    dev.gesture = gesture;
    old_i2c = dev.i2c_dev;
    g_init_error = XY_DEVICE_TIMEOUT;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_apds9960_init(&dev, &bus));
    TEST_ASSERT_TRUE(dev.initialized);
    TEST_ASSERT_EQUAL_MEMORY(&old_i2c, &dev.i2c_dev, sizeof(old_i2c));
    TEST_ASSERT_EQUAL_MEMORY(&rgb, &dev.rgb, sizeof(rgb));
    TEST_ASSERT_EQUAL_MEMORY(&proximity, &dev.proximity, sizeof(proximity));
    TEST_ASSERT_EQUAL_MEMORY(&gesture, &dev.gesture, sizeof(gesture));

    g_init_error = XY_DEVICE_OK;
    g_read_error = XY_DEVICE_TIMEOUT;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_apds9960_init(&dev, &bus));
    TEST_ASSERT_TRUE(dev.initialized);
    TEST_ASSERT_EQUAL_MEMORY(&old_i2c, &dev.i2c_dev, sizeof(old_i2c));
    TEST_ASSERT_EQUAL_MEMORY(&rgb, &dev.rgb, sizeof(rgb));

    g_read_error = XY_DEVICE_OK;
    g_write_error = XY_DEVICE_TIMEOUT;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT, xy_apds9960_init(&dev, &bus));
    TEST_ASSERT_TRUE(dev.initialized);
    TEST_ASSERT_EQUAL_MEMORY(&old_i2c, &dev.i2c_dev, sizeof(old_i2c));
    TEST_ASSERT_EQUAL_MEMORY(&rgb, &dev.rgb, sizeof(rgb));
}

static void test_apds9960_init_accepts_both_documented_id_values(void)
{
    xy_apds9960_t dev = {0};
    int bus;

    g_id_value = 0xabU;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_apds9960_init(&dev, &bus));

    memset(&dev, 0, sizeof(dev));
    g_id_value = 0x9cU;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_apds9960_init(&dev, &bus));
}

static void test_apds9960_init_rejects_unknown_id_without_writing_enable(void)
{
    xy_apds9960_t dev = {0};
    int bus;

    g_id_value = 0x42U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_NOT_FOUND, xy_apds9960_init(&dev, &bus));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_FALSE(dev.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(dev.i2c_dev.i2c_handle);
    TEST_ASSERT_EQUAL_UINT32(1U, g_read_count);
    TEST_ASSERT_EQUAL_UINT32(0U, g_write_count);
}

static void test_apds9960_init_and_deinit_write_the_documented_enable_profile(void)
{
    xy_apds9960_t dev = {0};
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_apds9960_init(&dev, &bus));
    TEST_ASSERT_EQUAL_UINT8(XY_APDS9960_REG_ENABLE, g_last_write_reg);
    TEST_ASSERT_EQUAL_UINT8(XY_APDS9960_ENABLE_PON_AEN_PEN_GEN, g_last_write_value);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_apds9960_deinit(&dev));
    TEST_ASSERT_EQUAL_UINT8(XY_APDS9960_REG_ENABLE, g_last_write_reg);
    TEST_ASSERT_EQUAL_UINT8(0U, g_last_write_value);
}

static void test_apds9960_rgb_decodes_all_four_channels(void)
{
    xy_apds9960_t dev = {0};
    xy_apds9960_rgb_t rgb;
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_apds9960_init(&dev, &bus));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_apds9960_read_rgb(&dev, &rgb));
    TEST_ASSERT_EQUAL_UINT16(0x5678U, rgb.clear);
    TEST_ASSERT_EQUAL_UINT16(0x1234U, rgb.red);
    TEST_ASSERT_EQUAL_UINT16(0xabcdU, rgb.green);
    TEST_ASSERT_EQUAL_UINT16(0xbeefU, rgb.blue);
}

static void test_apds9960_gesture_fifo_reads_level_before_the_payload(void)
{
    xy_apds9960_t dev = {0};
    xy_apds9960_gesture_fifo_t gesture;
    int bus;

    g_gesture_status = XY_APDS9960_GVALID;
    g_fifo_level = 3U;
    for (uint32_t index = 0U; index < sizeof(g_fifo_data); index++) {
        g_fifo_data[index] = (uint8_t)(index + 1U);
    }

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_apds9960_init(&dev, &bus));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_apds9960_read_gesture_fifo(&dev, &gesture));
    TEST_ASSERT_EQUAL_UINT8(3U, gesture.level);
    TEST_ASSERT_EQUAL_UINT32(12U, g_fifo_read_length);
    TEST_ASSERT_EQUAL_UINT8(1U, gesture.data[0]);
    TEST_ASSERT_EQUAL_UINT8(12U, gesture.data[11]);
    TEST_ASSERT_EQUAL_UINT32(77U, gesture.timestamp);
}

static void test_apds9960_gesture_fifo_rejects_a_level_above_the_local_buffer(void)
{
    xy_apds9960_t dev = {0};
    xy_apds9960_gesture_fifo_t gesture = {0x5aU, {0x5aU}, 0x5aU};
    int bus;

    g_gesture_status = XY_APDS9960_GVALID;
    g_fifo_level = 33U;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_apds9960_init(&dev, &bus));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_apds9960_read_gesture_fifo(&dev, &gesture));
    TEST_ASSERT_EQUAL_UINT32(0U, g_fifo_read_length);
    TEST_ASSERT_EQUAL_UINT8(0x5aU, gesture.level);
    TEST_ASSERT_EQUAL_UINT32(0x5aU, gesture.timestamp);
}

static void test_apds9960_gesture_fifo_propagates_the_level_read_failure(void)
{
    xy_apds9960_t dev = {0};
    xy_apds9960_gesture_fifo_t gesture = {0x5aU, {0x5aU}, 0x5aU};
    int bus;

    g_gesture_status = XY_APDS9960_GVALID;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_apds9960_init(&dev, &bus));

    g_read_error = XY_DEVICE_TIMEOUT;
    g_fail_read_reg = XY_APDS9960_REG_GFLVL;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT,
                          xy_apds9960_read_gesture_fifo(&dev, &gesture));
    TEST_ASSERT_EQUAL_UINT32(0U, g_fifo_read_length);
    TEST_ASSERT_EQUAL_UINT8(0x5aU, gesture.level);
    TEST_ASSERT_EQUAL_UINT32(0x5aU, gesture.timestamp);
}

static void test_apds9960_null_arguments_are_rejected_without_bus_io(void)
{
    xy_apds9960_t dev = {0};
    int bus;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_apds9960_init(NULL, &bus));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_apds9960_init(&dev, NULL));
    TEST_ASSERT_EQUAL_UINT32(0U, g_read_count);
    TEST_ASSERT_EQUAL_UINT32(0U, g_write_count);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_apds9960_init(&dev, &bus));
    g_read_count = 0U;
    g_write_count = 0U;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_apds9960_read_rgb(&dev, NULL));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_apds9960_read_proximity(&dev, NULL));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_apds9960_read_gesture_fifo(&dev, NULL));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_apds9960_deinit(NULL));
    TEST_ASSERT_EQUAL_UINT32(0U, g_read_count);
    TEST_ASSERT_EQUAL_UINT32(0U, g_write_count);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_apds9960_init_read_and_deinit);
    RUN_TEST(test_apds9960_init_accepts_both_documented_id_values);
    RUN_TEST(test_apds9960_init_rejects_unknown_id_without_writing_enable);
    RUN_TEST(test_apds9960_init_and_deinit_write_the_documented_enable_profile);
    RUN_TEST(test_apds9960_rgb_decodes_all_four_channels);
    RUN_TEST(test_apds9960_gesture_fifo_reads_level_before_the_payload);
    RUN_TEST(test_apds9960_gesture_fifo_rejects_a_level_above_the_local_buffer);
    RUN_TEST(test_apds9960_gesture_fifo_propagates_the_level_read_failure);
    RUN_TEST(test_apds9960_null_arguments_are_rejected_without_bus_io);
    RUN_TEST(test_apds9960_init_failure_clears_partial_transport);
    RUN_TEST(test_apds9960_init_rejects_incomplete_nested_transport);
    RUN_TEST(test_apds9960_rejects_lost_nested_transport_without_io);
    RUN_TEST(test_apds9960_transport_failures_preserve_state);
    RUN_TEST(test_apds9960_reinit_failure_preserves_live_owner);
    return UNITY_END();
}
