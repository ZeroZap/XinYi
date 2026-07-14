#include "unity.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "xy_sgp40.h"

static uint16_t g_command_queue[32];
static int g_command_ret_queue[32];
static size_t g_command_count;
static size_t g_command_index;

static uint8_t g_read_queue[32][9];
static uint16_t g_read_len_queue[32];
static int g_read_ret_queue[32];
static size_t g_read_count;
static size_t g_read_index;

static uint8_t g_write_data_queue[16][8];
static uint16_t g_write_data_len_queue[16];
static int g_write_data_ret_queue[16];
static size_t g_write_data_count;
static size_t g_write_data_index;

static uint32_t g_delay_total;
static size_t g_delay_count;

xy_ret_t xy_i2c_write_command(xy_i2c_dev_t *dev, uint16_t command)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_LESS_THAN_UINT(sizeof(g_command_queue) / sizeof(g_command_queue[0]), g_command_index);
    g_command_queue[g_command_count++] = command;
    return g_command_ret_queue[g_command_index++];
}

xy_ret_t xy_i2c_write_data(xy_i2c_dev_t *dev, const uint8_t *data, uint16_t len)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_LESS_THAN_UINT(sizeof(g_write_data_queue) / sizeof(g_write_data_queue[0]), g_write_data_index);
    TEST_ASSERT_LESS_OR_EQUAL_UINT(sizeof(g_write_data_queue[0]), len);
    memcpy(g_write_data_queue[g_write_data_count], data, len);
    g_write_data_len_queue[g_write_data_count] = len;
    g_write_data_count++;
    return g_write_data_ret_queue[g_write_data_index++];
}

xy_ret_t xy_i2c_read_data(xy_i2c_dev_t *dev, uint8_t *data, uint16_t len)
{
    TEST_ASSERT_NOT_NULL(dev);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_LESS_THAN_UINT(sizeof(g_read_queue) / sizeof(g_read_queue[0]), g_read_index);
    TEST_ASSERT_EQUAL_UINT16(g_read_len_queue[g_read_index], len);

    xy_ret_t ret = g_read_ret_queue[g_read_index];
    if (ret == 0) {
        memcpy(data, g_read_queue[g_read_index], len);
    }
    g_read_index++;
    return ret;
}

void xy_delay_ms(uint32_t ms)
{
    g_delay_total += ms;
    g_delay_count++;
}

static void queue_command_ret(xy_ret_t ret)
{
    TEST_ASSERT_LESS_THAN_UINT(sizeof(g_command_ret_queue) / sizeof(g_command_ret_queue[0]), g_command_count + g_command_index);
    g_command_ret_queue[g_command_count + g_command_index] = ret;
}

static void queue_read_bytes(const uint8_t *bytes, uint16_t len, xy_ret_t ret)
{
    TEST_ASSERT_LESS_THAN_UINT(sizeof(g_read_queue) / sizeof(g_read_queue[0]), g_read_count);
    TEST_ASSERT_LESS_OR_EQUAL_UINT(sizeof(g_read_queue[0]), len);
    if (bytes != NULL && len > 0U) {
        memcpy(g_read_queue[g_read_count], bytes, len);
    }
    g_read_len_queue[g_read_count] = len;
    g_read_ret_queue[g_read_count] = ret;
    g_read_count++;
}

static void queue_read_u16(uint16_t value, xy_ret_t ret)
{
    uint8_t bytes[3] = {(uint8_t)(value >> 8), (uint8_t)value, 0U};
    bytes[2] = xy_sgp40_crc8(bytes, 2U);
    queue_read_bytes(bytes, 3U, ret);
}

static void queue_read_serial(uint16_t a, uint16_t b, uint16_t c, xy_ret_t ret)
{
    uint8_t bytes[9] = {
        (uint8_t)(a >> 8), (uint8_t)a, 0U,
        (uint8_t)(b >> 8), (uint8_t)b, 0U,
        (uint8_t)(c >> 8), (uint8_t)c, 0U,
    };
    bytes[2] = xy_sgp40_crc8(&bytes[0], 2U);
    bytes[5] = xy_sgp40_crc8(&bytes[3], 2U);
    bytes[8] = xy_sgp40_crc8(&bytes[6], 2U);
    queue_read_bytes(bytes, 9U, ret);
}

void setUp(void)
{
    memset(g_command_queue, 0, sizeof(g_command_queue));
    memset(g_command_ret_queue, 0, sizeof(g_command_ret_queue));
    memset(g_read_queue, 0, sizeof(g_read_queue));
    memset(g_read_len_queue, 0, sizeof(g_read_len_queue));
    memset(g_read_ret_queue, 0, sizeof(g_read_ret_queue));
    memset(g_write_data_queue, 0, sizeof(g_write_data_queue));
    memset(g_write_data_len_queue, 0, sizeof(g_write_data_len_queue));
    memset(g_write_data_ret_queue, 0, sizeof(g_write_data_ret_queue));
    g_command_count = 0;
    g_command_index = 0;
    g_read_count = 0;
    g_read_index = 0;
    g_write_data_count = 0;
    g_write_data_index = 0;
    g_delay_total = 0;
    g_delay_count = 0;
}

void tearDown(void)
{
}

static void queue_init_success(void)
{
    queue_read_u16(0x1234U, 0);
    queue_read_serial(0x1111U, 0x2222U, 0x3333U, 0);
    queue_read_u16(0xD400U, 0);
}

static xy_i2c_dev_t fake_i2c(void)
{
    xy_i2c_dev_t i2c = {.handle = (void *)0x1234, .address = SGP40_I2C_ADDR};
    return i2c;
}

static void init_ok(xy_sgp40_dev_t *dev, xy_i2c_dev_t *i2c)
{
    queue_init_success();
    TEST_ASSERT_EQUAL_INT(0, xy_sgp40_init(dev, i2c, NULL));
}

static void test_crc8_matches_sensirion_examples(void)
{
    uint8_t pass_bytes[2] = {0xD4U, 0x00U};
    uint8_t serial_bytes[2] = {0xBEU, 0xEFU};

    TEST_ASSERT_EQUAL_UINT8(0xC6U, xy_sgp40_crc8(pass_bytes, 2U));
    TEST_ASSERT_EQUAL_UINT8(xy_sgp40_crc8(serial_bytes, 2U), xy_sgp40_crc8(serial_bytes, 2U));
}

static void test_init_default_config_reads_identity_and_self_test(void)
{
    xy_sgp40_dev_t dev;
    xy_i2c_dev_t i2c = fake_i2c();

    TEST_ASSERT_EQUAL_INT(-1, xy_sgp40_init(NULL, &i2c, NULL));
    TEST_ASSERT_EQUAL_INT(-1, xy_sgp40_init(&dev, NULL, NULL));

    init_ok(&dev, &i2c);
    TEST_ASSERT_TRUE(dev.is_initialized);
    TEST_ASSERT_FALSE(dev.is_warmed_up);
    TEST_ASSERT_EQUAL_UINT16(0x1234U, dev.feature_set);
    TEST_ASSERT_EQUAL_UINT32(0x1111U, dev.serial_id[0]);
    TEST_ASSERT_EQUAL_UINT32(0x2222U, dev.serial_id[1]);
    TEST_ASSERT_EQUAL_UINT32(0x3333U, dev.serial_id[2]);
    TEST_ASSERT_TRUE(dev.config.enable_compensation);
    TEST_ASSERT_EQUAL_UINT8(SGP40_I2C_ADDR, dev.config.i2c_address);
    TEST_ASSERT_EQUAL_UINT(3U, g_command_count);
    TEST_ASSERT_EQUAL_UINT16(SGP40_CMD_FEATURE_SET, g_command_queue[0]);
    TEST_ASSERT_EQUAL_UINT16(SGP40_CMD_SERIAL_ID, g_command_queue[1]);
    TEST_ASSERT_EQUAL_UINT16(SGP40_CMD_SELF_TEST, g_command_queue[2]);
    TEST_ASSERT_EQUAL_UINT32(280U, g_delay_total);
}

static void test_init_uses_custom_config_and_propagates_identity_failures(void)
{
    xy_sgp40_dev_t dev;
    xy_i2c_dev_t i2c = fake_i2c();
    xy_sgp40_config_t cfg = {
        .enable_compensation = false,
        .default_temperature = 12.5f,
        .default_humidity = 34.0f,
        .i2c_address = 0x5AU,
    };

    queue_init_success();
    TEST_ASSERT_EQUAL_INT(0, xy_sgp40_init(&dev, &i2c, &cfg));
    TEST_ASSERT_FALSE(dev.config.enable_compensation);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 12.5f, dev.config.default_temperature);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 34.0f, dev.config.default_humidity);
    TEST_ASSERT_EQUAL_UINT8(0x5AU, dev.config.i2c_address);

    setUp();
    queue_command_ret(-1);
    TEST_ASSERT_EQUAL_INT(-1, xy_sgp40_init(&dev, &i2c, NULL));

    setUp();
    queue_read_u16(0x1234U, 0);
    queue_read_serial(0x1111U, 0x2222U, 0x3333U, 0);
    queue_read_u16(0x0000U, 0);
    TEST_ASSERT_EQUAL_INT(-1, xy_sgp40_init(&dev, &i2c, NULL));
}

static void test_feature_serial_and_self_test_crc_paths(void)
{
    xy_sgp40_dev_t dev = {.i2c = &(xy_i2c_dev_t){.handle = (void *)0x1234, .address = SGP40_I2C_ADDR}};
    uint16_t feature = 0U;
    uint32_t serial[3] = {0};
    bool passed = true;
    uint8_t bad_crc[3] = {0x12U, 0x34U, 0x00U};

    TEST_ASSERT_EQUAL_INT(-1, xy_sgp40_read_feature_set(NULL, &feature));
    TEST_ASSERT_EQUAL_INT(-1, xy_sgp40_read_feature_set(&dev, NULL));
    queue_read_u16(0xABCDU, 0);
    TEST_ASSERT_EQUAL_INT(0, xy_sgp40_read_feature_set(&dev, &feature));
    TEST_ASSERT_EQUAL_UINT16(0xABCDU, feature);

    queue_read_u16(0x0000U, -1);
    feature = 0x7777U;
    TEST_ASSERT_EQUAL_INT(-1, xy_sgp40_read_feature_set(&dev, &feature));
    TEST_ASSERT_EQUAL_UINT16(0x7777U, feature);

    TEST_ASSERT_EQUAL_INT(-1, xy_sgp40_read_serial_id(NULL, serial));
    TEST_ASSERT_EQUAL_INT(-1, xy_sgp40_read_serial_id(&dev, NULL));
    queue_read_serial(0x0001U, 0x0002U, 0x0003U, 0);
    TEST_ASSERT_EQUAL_INT(0, xy_sgp40_read_serial_id(&dev, serial));
    TEST_ASSERT_EQUAL_UINT32(1U, serial[0]);
    TEST_ASSERT_EQUAL_UINT32(2U, serial[1]);
    TEST_ASSERT_EQUAL_UINT32(3U, serial[2]);

    queue_read_serial(0x4444U, 0x5555U, 0x6666U, -1);
    serial[0] = 7U;
    serial[1] = 8U;
    serial[2] = 9U;
    TEST_ASSERT_EQUAL_INT(-1, xy_sgp40_read_serial_id(&dev, serial));
    TEST_ASSERT_EQUAL_UINT32(7U, serial[0]);
    TEST_ASSERT_EQUAL_UINT32(8U, serial[1]);
    TEST_ASSERT_EQUAL_UINT32(9U, serial[2]);

    TEST_ASSERT_EQUAL_INT(-1, xy_sgp40_self_test(NULL, &passed));
    TEST_ASSERT_EQUAL_INT(-1, xy_sgp40_self_test(&dev, NULL));
    queue_read_u16(0x0000U, 0);
    TEST_ASSERT_EQUAL_INT(0, xy_sgp40_self_test(&dev, &passed));
    TEST_ASSERT_FALSE(passed);

    queue_read_bytes(bad_crc, 3U, 0);
    passed = true;
    TEST_ASSERT_EQUAL_INT(-1, xy_sgp40_self_test(&dev, &passed));
    TEST_ASSERT_FALSE(passed);
}

static void test_measurement_flow_waits_reads_and_updates_last_data(void)
{
    xy_sgp40_dev_t dev;
    xy_sgp40_data_t data;
    xy_i2c_dev_t i2c = fake_i2c();

    init_ok(&dev, &i2c);
    queue_read_u16(321U, 0);
    TEST_ASSERT_EQUAL_INT(0, xy_sgp40_measure_voc(&dev, &data, 30U));
    TEST_ASSERT_EQUAL_UINT16(SGP40_CMD_MEASURE_VOC, g_command_queue[3]);
    TEST_ASSERT_EQUAL_UINT16(321U, data.voc_index);
    TEST_ASSERT_EQUAL_UINT32(1U, dev.measurement_count);
    TEST_ASSERT_EQUAL_UINT16(321U, dev.last_data.voc_index);
    TEST_ASSERT_EQUAL_UINT32(310U, g_delay_total);

    TEST_ASSERT_EQUAL_INT(-1, xy_sgp40_measure_voc(NULL, &data, 30U));
    TEST_ASSERT_EQUAL_INT(-1, xy_sgp40_measure_voc(&dev, NULL, 30U));
    TEST_ASSERT_EQUAL_INT(-2, xy_sgp40_measure_voc(&dev, &data, 20U));
}

static void test_measurement_error_paths_preserve_last_data_and_state(void)
{
    xy_sgp40_dev_t dev;
    xy_sgp40_data_t data = {.voc_index = 0xEEEEU};
    xy_i2c_dev_t i2c = fake_i2c();

    TEST_ASSERT_EQUAL_INT(-1, xy_sgp40_start_measurement(NULL));
    memset(&dev, 0, sizeof(dev));
    TEST_ASSERT_EQUAL_INT(-1, xy_sgp40_start_measurement(&dev));
    TEST_ASSERT_EQUAL_INT(-1, xy_sgp40_start_continuous(NULL));
    TEST_ASSERT_EQUAL_INT(-1, xy_sgp40_start_continuous(&dev));
    TEST_ASSERT_EQUAL_INT(-1, xy_sgp40_read_voc(NULL, &data));
    TEST_ASSERT_EQUAL_INT(-1, xy_sgp40_read_voc(&dev, NULL));

    init_ok(&dev, &i2c);
    TEST_ASSERT_EQUAL_INT(0, xy_sgp40_start_continuous(&dev));
    dev.last_data.voc_index = 77U;
    dev.measurement_count = 5U;
    queue_read_u16(0U, -1);
    TEST_ASSERT_EQUAL_INT(-1, xy_sgp40_read_voc(&dev, &data));
    TEST_ASSERT_EQUAL_UINT16(0xEEEEU, data.voc_index);
    TEST_ASSERT_EQUAL_UINT16(77U, dev.last_data.voc_index);
    TEST_ASSERT_EQUAL_UINT32(5U, dev.measurement_count);

    g_command_ret_queue[g_command_index] = -1;
    data.voc_index = 0xDDDDU;
    TEST_ASSERT_EQUAL_INT(-1, xy_sgp40_measure_voc(&dev, &data, 30U));
    TEST_ASSERT_EQUAL_UINT16(0xDDDDU, data.voc_index);
    TEST_ASSERT_EQUAL_UINT16(77U, dev.last_data.voc_index);
    TEST_ASSERT_EQUAL_UINT32(5U, dev.measurement_count);

    data.voc_index = 0xCCCCU;
    TEST_ASSERT_EQUAL_INT(-2, xy_sgp40_measure_voc(&dev, &data, 20U));
    TEST_ASSERT_EQUAL_UINT16(0xCCCCU, data.voc_index);
    TEST_ASSERT_EQUAL_UINT16(77U, dev.last_data.voc_index);
    TEST_ASSERT_EQUAL_UINT32(5U, dev.measurement_count);
}

static void test_state_helpers_compensation_burn_in_and_deinit(void)
{
    xy_sgp40_dev_t dev;
    xy_i2c_dev_t i2c = fake_i2c();

    TEST_ASSERT_FALSE(xy_sgp40_is_ready(NULL));
    TEST_ASSERT_FALSE(xy_sgp40_is_warmed_up(NULL));
    TEST_ASSERT_NULL(xy_sgp40_get_last_data(NULL));
    TEST_ASSERT_EQUAL_UINT32(0U, xy_sgp40_get_uptime(NULL));
    TEST_ASSERT_EQUAL_INT(-1, xy_sgp40_set_compensation(NULL, 1.0f, 2.0f));
    TEST_ASSERT_EQUAL_INT(-1, xy_sgp40_enable_burn_in(NULL));
    TEST_ASSERT_EQUAL_INT(-1, xy_sgp40_deinit(NULL));

    init_ok(&dev, &i2c);
    TEST_ASSERT_TRUE(xy_sgp40_is_ready(&dev));
    TEST_ASSERT_FALSE(xy_sgp40_is_warmed_up(&dev));
    dev.uptime_ms = 10000U;
    TEST_ASSERT_TRUE(xy_sgp40_is_warmed_up(&dev));
    TEST_ASSERT_EQUAL_UINT32(10000U, xy_sgp40_get_uptime(&dev));

    TEST_ASSERT_EQUAL_INT(0, xy_sgp40_set_compensation(&dev, 26.5f, 55.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 26.5f, dev.config.default_temperature);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 55.0f, dev.config.default_humidity);

    g_command_ret_queue[g_command_index] = -1;
    TEST_ASSERT_EQUAL_INT(-1, xy_sgp40_enable_burn_in(&dev));
    TEST_ASSERT_FALSE(dev.is_burned_in);

    TEST_ASSERT_EQUAL_INT(0, xy_sgp40_enable_burn_in(&dev));
    TEST_ASSERT_TRUE(dev.is_burned_in);
    TEST_ASSERT_EQUAL_UINT16(SGP40_CMD_BURN_IN, g_command_queue[4]);

    xy_sgp40_set_offset(&dev, -12);
    TEST_ASSERT_EQUAL_INT16(-12, dev.offset);
    TEST_ASSERT_EQUAL_PTR(&dev.last_data, xy_sgp40_get_last_data(&dev));

    TEST_ASSERT_EQUAL_INT(0, xy_sgp40_stop(&dev));
    TEST_ASSERT_EQUAL_INT(0, xy_sgp40_deinit(&dev));
    TEST_ASSERT_FALSE(dev.is_initialized);
}

static void test_compensation_and_stop_error_paths_preserve_state(void)
{
    xy_sgp40_dev_t dev;
    xy_i2c_dev_t i2c = fake_i2c();

    memset(&dev, 0, sizeof(dev));
    dev.config.default_temperature = 11.0f;
    dev.config.default_humidity = 22.0f;
    TEST_ASSERT_EQUAL_INT(-1, xy_sgp40_set_compensation(&dev, 33.0f, 44.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 11.0f, dev.config.default_temperature);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 22.0f, dev.config.default_humidity);

    init_ok(&dev, &i2c);
    TEST_ASSERT_EQUAL_INT(0, xy_sgp40_start_continuous(&dev));
    TEST_ASSERT_EQUAL_INT(0, xy_sgp40_stop(&dev));
    TEST_ASSERT_TRUE(dev.is_initialized);

    TEST_ASSERT_EQUAL_INT(-1, xy_sgp40_deinit(&(xy_sgp40_dev_t){0}));
}

static void test_init_serial_crc_failure_leaves_device_uninitialized(void)
{
    xy_sgp40_dev_t dev;
    xy_i2c_dev_t i2c = fake_i2c();
    uint8_t bad_serial[9] = {
        0x11U, 0x11U, 0U,
        0x22U, 0x22U, 0U,
        0x33U, 0x33U, 0U,
    };

    bad_serial[2] = xy_sgp40_crc8(&bad_serial[0], 2U);
    bad_serial[5] = (uint8_t)(xy_sgp40_crc8(&bad_serial[3], 2U) ^ 0xFFU);
    bad_serial[8] = xy_sgp40_crc8(&bad_serial[6], 2U);

    queue_read_u16(0x1234U, 0);
    queue_read_bytes(bad_serial, sizeof(bad_serial), 0);
    TEST_ASSERT_EQUAL_INT(-1, xy_sgp40_init(&dev, &i2c, NULL));
    TEST_ASSERT_FALSE(dev.is_initialized);
}

static void test_command_failures_return_before_delay_or_read(void)
{
    xy_sgp40_dev_t dev = {.i2c = &(xy_i2c_dev_t){.handle = (void *)0x1234, .address = SGP40_I2C_ADDR}};
    uint16_t feature = 0xAAAAU;
    uint32_t serial[3] = {1U, 2U, 3U};
    bool passed = true;

    g_command_ret_queue[g_command_index] = -1;
    TEST_ASSERT_EQUAL_INT(-1, xy_sgp40_read_feature_set(&dev, &feature));
    TEST_ASSERT_EQUAL_UINT16(0xAAAAU, feature);
    TEST_ASSERT_EQUAL_UINT(0U, g_read_index);
    TEST_ASSERT_EQUAL_UINT32(0U, g_delay_total);

    setUp();
    g_command_ret_queue[g_command_index] = -1;
    TEST_ASSERT_EQUAL_INT(-1, xy_sgp40_read_serial_id(&dev, serial));
    TEST_ASSERT_EQUAL_UINT32(1U, serial[0]);
    TEST_ASSERT_EQUAL_UINT32(2U, serial[1]);
    TEST_ASSERT_EQUAL_UINT32(3U, serial[2]);
    TEST_ASSERT_EQUAL_UINT(0U, g_read_index);

    setUp();
    g_command_ret_queue[g_command_index] = -1;
    TEST_ASSERT_EQUAL_INT(-1, xy_sgp40_self_test(&dev, &passed));
    TEST_ASSERT_TRUE(passed);
    TEST_ASSERT_EQUAL_UINT(0U, g_read_index);
}

static void test_read_voc_crc_failure_preserves_output_and_last_data(void)
{
    xy_sgp40_dev_t dev;
    xy_sgp40_data_t data = {.voc_index = 0xAAAAU, .raw_signal = 0xBBBBU, .temperature = 1.0f};
    xy_i2c_dev_t i2c = fake_i2c();
    uint8_t bad_crc[3] = {0x01U, 0x23U, 0x00U};

    init_ok(&dev, &i2c);
    dev.last_data.voc_index = 55U;
    dev.measurement_count = 4U;
    bad_crc[2] = (uint8_t)(xy_sgp40_crc8(bad_crc, 2U) ^ 0xFFU);
    queue_read_bytes(bad_crc, sizeof(bad_crc), 0);

    TEST_ASSERT_EQUAL_INT(-1, xy_sgp40_read_voc(&dev, &data));
    TEST_ASSERT_EQUAL_UINT16(0xAAAAU, data.voc_index);
    TEST_ASSERT_EQUAL_UINT16(0xBBBBU, data.raw_signal);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, data.temperature);
    TEST_ASSERT_EQUAL_UINT16(55U, dev.last_data.voc_index);
    TEST_ASSERT_EQUAL_UINT32(4U, dev.measurement_count);
}

static void test_voc_level_boundaries(void)
{
    TEST_ASSERT_EQUAL_INT(XY_SGP40_VOC_EXCELLENT, xy_sgp40_get_voc_level(0U));
    TEST_ASSERT_EQUAL_INT(XY_SGP40_VOC_EXCELLENT, xy_sgp40_get_voc_level(99U));
    TEST_ASSERT_EQUAL_INT(XY_SGP40_VOC_GOOD, xy_sgp40_get_voc_level(100U));
    TEST_ASSERT_EQUAL_INT(XY_SGP40_VOC_GOOD, xy_sgp40_get_voc_level(199U));
    TEST_ASSERT_EQUAL_INT(XY_SGP40_VOC_MODERATE, xy_sgp40_get_voc_level(200U));
    TEST_ASSERT_EQUAL_INT(XY_SGP40_VOC_MODERATE, xy_sgp40_get_voc_level(299U));
    TEST_ASSERT_EQUAL_INT(XY_SGP40_VOC_POOR, xy_sgp40_get_voc_level(300U));
    TEST_ASSERT_EQUAL_INT(XY_SGP40_VOC_POOR, xy_sgp40_get_voc_level(399U));
    TEST_ASSERT_EQUAL_INT(XY_SGP40_VOC_BAD, xy_sgp40_get_voc_level(400U));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_crc8_matches_sensirion_examples);
    RUN_TEST(test_init_default_config_reads_identity_and_self_test);
    RUN_TEST(test_init_uses_custom_config_and_propagates_identity_failures);
    RUN_TEST(test_feature_serial_and_self_test_crc_paths);
    RUN_TEST(test_measurement_flow_waits_reads_and_updates_last_data);
    RUN_TEST(test_measurement_error_paths_preserve_last_data_and_state);
    RUN_TEST(test_state_helpers_compensation_burn_in_and_deinit);
    RUN_TEST(test_compensation_and_stop_error_paths_preserve_state);
    RUN_TEST(test_init_serial_crc_failure_leaves_device_uninitialized);
    RUN_TEST(test_command_failures_return_before_delay_or_read);
    RUN_TEST(test_read_voc_crc_failure_preserves_output_and_last_data);
    RUN_TEST(test_voc_level_boundaries);
    return UNITY_END();
}
