#include "unity.h"
#include "xy_ads1115.h"
#include "xy_os.h"
#include <string.h>

static uint8_t regs[4][2];
static int init_result;
static int read_result;
static unsigned reads;
static unsigned delays;

int xy_i2c_device_init(xy_i2c_device_t *dev, void *handle, uint16_t addr, uint32_t timeout)
{
    memset(dev, 0, sizeof(*dev));
    dev->i2c_handle = handle;
    dev->dev_addr = addr;
    dev->timeout = timeout;
    dev->base.initialized = init_result == XY_DEVICE_OK;
    return init_result;
}

int xy_i2c_device_read_reg(xy_i2c_device_t *dev, uint8_t reg, uint8_t *data, size_t length)
{
    (void)dev;
    TEST_ASSERT_EQUAL_UINT(2U, length);
    reads++;
    if (read_result != XY_DEVICE_OK) {
        return read_result;
    }
    memcpy(data, regs[reg], 2U);
    return XY_DEVICE_OK;
}

int xy_i2c_device_write_reg(xy_i2c_device_t *dev, uint8_t reg, const uint8_t *data,
                            size_t length)
{
    (void)dev;
    TEST_ASSERT_EQUAL_UINT(3U, length);
    memcpy(regs[reg], &data[1], 2U);
    return XY_DEVICE_OK;
}

xy_os_status_t xy_os_delay(uint32_t ticks)
{
    delays += ticks;
    return XY_OS_OK;
}

void setUp(void)
{
    memset(regs, 0, sizeof(regs));
    init_result = XY_DEVICE_OK;
    read_result = XY_DEVICE_OK;
    reads = 0U;
    delays = 0U;
}

void tearDown(void) {}

static void init_ok(xy_ads1115_t *dev)
{
    int bus;
    TEST_ASSERT_EQUAL_INT(XY_ADS1115_OK, xy_ads1115_init(dev, &bus, ADS1115_ADDR_GND));
}

static void test_init_and_read_single(void)
{
    xy_ads1115_t dev;
    int16_t value = 0;

    regs[ADS1115_REG_CONFIG][0] = 0x85U;
    regs[ADS1115_REG_CONFIG][1] = 0x83U;
    init_ok(&dev);
    regs[ADS1115_REG_CONVERT][0] = 0x12U;
    regs[ADS1115_REG_CONVERT][1] = 0x34U;

    TEST_ASSERT_EQUAL_INT(XY_ADS1115_OK, xy_ads1115_read_single(&dev, 0U, &value));
    TEST_ASSERT_EQUAL_INT16(0x1234, value);
    TEST_ASSERT_EQUAL_UINT32(10U, delays);
    TEST_ASSERT_EQUAL_INT16(0x1234, dev.last_value);
}

static void test_failed_reinit_preserves_live_owner(void)
{
    xy_ads1115_t dev;
    xy_ads1115_t snapshot;
    int bus;

    init_ok(&dev);
    dev.last_value = 321;
    snapshot = dev;
    read_result = XY_DEVICE_TIMEOUT;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT,
                          xy_ads1115_init(&dev, &bus, ADS1115_ADDR_VDD));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &dev, sizeof(dev));
    TEST_ASSERT_EQUAL_UINT(2U, reads);
}

static void test_initialization_failure_clears_owner(void)
{
    xy_ads1115_t dev;
    int bus;

    memset(&dev, 0, sizeof(dev));
    init_result = XY_DEVICE_TIMEOUT;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_TIMEOUT,
                          xy_ads1115_init(&dev, &bus, ADS1115_ADDR_GND));
    TEST_ASSERT_EQUAL_MEMORY(&(xy_ads1115_t){0}, &dev, sizeof(dev));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_and_read_single);
    RUN_TEST(test_failed_reinit_preserves_live_owner);
    RUN_TEST(test_initialization_failure_clears_owner);
    return UNITY_END();
}
