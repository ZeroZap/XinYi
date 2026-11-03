#include "unity.h"
#include "xy_guvas12sd.h"

#include <string.h>

static int32_t g_raw;
static unsigned g_reads;
static uint8_t g_last_channel;

int32_t xy_adc_dev_read(void *dev, uint8_t channel, uint32_t timeout)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_EQUAL_UINT32(XY_GUVAS12SD_DEFAULT_TIMEOUT_MS, timeout);
    g_last_channel = channel;
    g_reads++;
    return g_raw;
}

uint32_t xy_hal_sys_get_tick_count(void)
{
    return 121212U;
}

void setUp(void)
{
    g_raw = 123;
    g_reads = 0U;
    g_last_channel = 0U;
}

void tearDown(void) {}

static void test_init_deinit_and_null_guards(void)
{
    xy_guvas12sd_t dev;
    int adc;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_guvas12sd_init(NULL, &adc, 3U));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_guvas12sd_init(&dev, NULL, 3U));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_guvas12sd_init(&dev, &adc, 3U));
    TEST_ASSERT_TRUE(dev.initialized);
    TEST_ASSERT_EQUAL_PTR(&adc, dev.adc);
    TEST_ASSERT_EQUAL_UINT8(3U, dev.channel);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_guvas12sd_deinit(&dev));
    TEST_ASSERT_FALSE(dev.initialized);
    TEST_ASSERT_NULL(dev.adc);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_guvas12sd_deinit(&dev));
}

static void test_read_returns_raw_and_timestamp(void)
{
    xy_guvas12sd_t dev;
    xy_guvas12sd_sample_t sample;
    int adc;

    memset(&sample, 0xA5, sizeof(sample));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_guvas12sd_init(&dev, &adc, 7U));
    g_raw = 123;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_guvas12sd_read(&dev, &sample));
    TEST_ASSERT_EQUAL_UINT32(123U, sample.raw);
    TEST_ASSERT_EQUAL_UINT32(121212U, sample.timestamp);
    TEST_ASSERT_EQUAL_UINT(1U, g_reads);
    TEST_ASSERT_EQUAL_UINT8(7U, g_last_channel);
}

static void test_read_failure_is_atomic(void)
{
    xy_guvas12sd_t dev;
    xy_guvas12sd_sample_t sample;
    xy_guvas12sd_sample_t snapshot;
    int adc;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_guvas12sd_init(&dev, &adc, 4U));
    memset(&sample, 0x5A, sizeof(sample));
    snapshot = sample;
    g_raw = -1;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, xy_guvas12sd_read(&dev, &sample));
    TEST_ASSERT_EQUAL_MEMORY(&snapshot, &sample, sizeof(sample));
}

static void test_read_missing_handle_fails_closed(void)
{
    xy_guvas12sd_t dev;
    xy_guvas12sd_sample_t sample;
    unsigned before;
    int adc;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_guvas12sd_init(&dev, &adc, 2U));
    dev.adc = NULL;
    before = g_reads;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_guvas12sd_read(&dev, &sample));
    TEST_ASSERT_EQUAL_UINT(before, g_reads);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_deinit_and_null_guards);
    RUN_TEST(test_read_returns_raw_and_timestamp);
    RUN_TEST(test_read_failure_is_atomic);
    RUN_TEST(test_read_missing_handle_fails_closed);
    return UNITY_END();
}
