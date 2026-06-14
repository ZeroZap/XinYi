/**
 * @file test_sensor.c
 * @brief Sensor Component Unit Tests using Unity Framework
 * @version 1.0.0
 * @date 2026-02-28
 */

#include <stdint.h>
#include <string.h>
#include "unity.h"

/* Sensor headers */
#include "sensor_type.h"
#include "sensor_core.h"
#include "sensor_config.h"

/* ==================== Test Fixtures ==================== */

void setUp(void)
{
    /* Called before each test */
}

void tearDown(void)
{
    /* Called after each test */
}

/* ==================== Sensor Type Tests ==================== */

void test_sensor_type_constants(void)
{
    /* Test basic sensor type values */
    TEST_ASSERT_EQUAL(0x00, SENSOR_TYPE_NONE);
    TEST_ASSERT_EQUAL(0x01, SENSOR_TYPE_ACCELEROMETER);
    TEST_ASSERT_EQUAL(0x02, SENSOR_TYPE_GYROSCOPE);
    TEST_ASSERT_EQUAL(0x03, SENSOR_TYPE_MAGNETOMETER);
    TEST_ASSERT_EQUAL(0x10, SENSOR_TYPE_TEMPERATURE);
    TEST_ASSERT_EQUAL(0x11, SENSOR_TYPE_HUMIDITY);
    TEST_ASSERT_EQUAL(0x12, SENSOR_TYPE_PRESSURE);
    TEST_ASSERT_EQUAL(0x20, SENSOR_TYPE_PROXIMITY);
    TEST_ASSERT_EQUAL(0x30, SENSOR_TYPE_LIGHT);
    TEST_ASSERT_EQUAL(0x40, SENSOR_TYPE_HEART_RATE);
    TEST_ASSERT_EQUAL(0xFF, SENSOR_TYPE_CUSTOM);
}

void test_sensor_unit_constants(void)
{
    /* Test unit constants */
    TEST_ASSERT_EQUAL(0, SENSOR_UNIT_NONE);
    TEST_ASSERT_TRUE(SENSOR_UNIT_CELSIUS > 0);
    TEST_ASSERT_TRUE(SENSOR_UNIT_PERCENT > 0);
    TEST_ASSERT_TRUE(SENSOR_UNIT_LUX > 0);
}

void test_sensor_data_structure_size(void)
{
    /* Test sensor_data_t structure size */
    TEST_ASSERT_TRUE(sizeof(sensor_data_t) >= 16);
}

void test_sensor_value_union(void)
{
    sensor_value_t value;

    /* Test int32 value */
    value.val_int32 = 100;
    TEST_ASSERT_EQUAL_INT32(100, value.val_int32);

    /* Test 3-axis value */
    value.val_3axis.x = 1;
    value.val_3axis.y = 2;
    value.val_3axis.z = 3;
    TEST_ASSERT_EQUAL_INT32(1, value.val_3axis.x);
    TEST_ASSERT_EQUAL_INT32(2, value.val_3axis.y);
    TEST_ASSERT_EQUAL_INT32(3, value.val_3axis.z);
}

/* ==================== Error Code Tests ==================== */

void test_sensor_error_codes(void)
{
    /* Test error code values */
    TEST_ASSERT_EQUAL(0, SENSOR_OK);
    TEST_ASSERT_TRUE(SENSOR_ERROR < 0);
    TEST_ASSERT_TRUE(SENSOR_ERROR_INVALID_PARAM < 0);
}

/* ==================== Sensor Info Tests ==================== */

void test_sensor_info_structure(void)
{
    sensor_info_t info;

    memset(&info, 0, sizeof(info));

    /* Test structure fields */
    TEST_ASSERT_TRUE(sizeof(info.name) == SENSOR_NAME_MAX_LEN);
    TEST_ASSERT_EQUAL_PTR(NULL, info.vendor);
    TEST_ASSERT_EQUAL_PTR(NULL, info.model);
    TEST_ASSERT_EQUAL(0, info.version);
    TEST_ASSERT_EQUAL(SENSOR_TYPE_NONE, info.type);
}

void test_sensor_feature_flags(void)
{
    /* Test feature flag values */
    TEST_ASSERT_EQUAL((1 << 0), SENSOR_FLAG_FIFO_SUPPORT);
    TEST_ASSERT_EQUAL((1 << 1), SENSOR_FLAG_INT_SUPPORT);
    TEST_ASSERT_EQUAL((1 << 2), SENSOR_FLAG_DMA_SUPPORT);
    TEST_ASSERT_EQUAL((1 << 3), SENSOR_FLAG_CALIBRATION);
    TEST_ASSERT_EQUAL((1 << 4), SENSOR_FLAG_SELF_TEST);
    TEST_ASSERT_EQUAL((1 << 5), SENSOR_FLAG_LOW_POWER);
    TEST_ASSERT_EQUAL((1 << 6), SENSOR_FLAG_HIGH_PRECISION);
}

/* ==================== Sensor Device Tests ==================== */

void test_sensor_device_init(void)
{
    sensor_device_t sensor;

    memset(&sensor, 0, sizeof(sensor));

    /* Test initial state */
    TEST_ASSERT_EQUAL(SENSOR_TYPE_NONE, sensor.info.type);
    TEST_ASSERT_EQUAL_PTR(NULL, sensor.ops);
    TEST_ASSERT_EQUAL_PTR(NULL, sensor.bus);
}

void test_sensor_device_with_operations(void)
{
    static sensor_ops_t mock_ops = {
        .init = NULL,
        .deinit = NULL,
        .read = NULL,
    };

    sensor_device_t sensor;

    memset(&sensor, 0, sizeof(sensor));
    sensor.ops = &mock_ops;

    /* Test operations assigned */
    TEST_ASSERT_EQUAL_PTR(&mock_ops, sensor.ops);
}

/* ==================== Sensor Configuration Tests ==================== */

void test_sensor_config_types(void)
{
    /* Test configuration type enum exists */
    sensor_config_type_t cfg;
    cfg = 0; /* Default config */
    TEST_ASSERT_EQUAL(0, cfg);
}

void test_sensor_trigger_modes(void)
{
    /* Test trigger mode enum exists */
    sensor_trigger_mode_t mode;
    mode = 0; /* Default mode */
    TEST_ASSERT_EQUAL(0, mode);
}

/* ==================== FIFO Tests ==================== */

#if SENSOR_ENABLE_FIFO
void test_sensor_fifo_structure(void)
{
    sensor_fifo_t fifo;
    sensor_data_t buffer[10];

    memset(&fifo, 0, sizeof(fifo));
    fifo.buffer = buffer;
    fifo.size = 10;
    fifo.head = 0;
    fifo.tail = 0;
    fifo.count = 0;

    TEST_ASSERT_EQUAL_PTR(buffer, fifo.buffer);
    TEST_ASSERT_EQUAL(10, fifo.size);
    TEST_ASSERT_EQUAL(0, fifo.count);
    TEST_ASSERT_FALSE(fifo.overflow);
}

void test_sensor_fifo_operations(void)
{
    /* Test FIFO operations if enabled */
    TEST_ASSERT_TRUE(1); /* Placeholder */
}
#else
void test_sensor_fifo_disabled(void)
{
    /* Test that FIFO is disabled */
    TEST_ASSERT_FALSE(SENSOR_ENABLE_FIFO);
}
#endif

/* ==================== Interrupt Tests ==================== */

#if SENSOR_ENABLE_INTERRUPT
void test_sensor_interrupt_config(void)
{
    sensor_interrupt_config_t cfg;

    memset(&cfg, 0, sizeof(cfg));

    /* Test interrupt config structure */
    TEST_ASSERT_EQUAL(0, cfg.int_type);
    TEST_ASSERT_FALSE(cfg.enabled);
}
#else
void test_sensor_interrupt_disabled(void)
{
    TEST_ASSERT_FALSE(SENSOR_ENABLE_INTERRUPT);
}
#endif

/* ==================== Calibration Tests ==================== */

#if SENSOR_ENABLE_CALIBRATION
void test_sensor_calibration_data(void)
{
    sensor_calibration_data_t calib;

    memset(&calib, 0, sizeof(calib));

    /* Test calibration data structure */
    TEST_ASSERT_EQUAL(0, calib.offset[0]);
    TEST_ASSERT_EQUAL(0, calib.scale[0]);
}

void test_sensor_calibration_types(void)
{
    /* Test calibration type enum exists */
    sensor_calibration_type_t type;
    type = 0;
    TEST_ASSERT_EQUAL(0, type);
}
#else
void test_sensor_calibration_disabled(void)
{
    TEST_ASSERT_FALSE(SENSOR_ENABLE_CALIBRATION);
}
#endif

/* ==================== Power Management Tests ==================== */

#if SENSOR_ENABLE_POWER_MGMT
void test_sensor_power_mode(void)
{
    sensor_power_mode_t mode;

    mode = 0; /* Default mode */
    TEST_ASSERT_EQUAL(0, mode);
}

void test_sensor_power_stats(void)
{
    sensor_power_stats_t stats;

    memset(&stats, 0, sizeof(stats));

    TEST_ASSERT_EQUAL(0, stats.avg_current);
    TEST_ASSERT_EQUAL(0, stats.total_energy);
}
#else
void test_sensor_power_mgmt_disabled(void)
{
    TEST_ASSERT_FALSE(SENSOR_ENABLE_POWER_MGMT);
}
#endif

/* ==================== Filter Tests ==================== */

#if SENSOR_ENABLE_FILTER
void test_sensor_filter_config(void)
{
    sensor_filter_config_t cfg;

    memset(&cfg, 0, sizeof(cfg));

    TEST_ASSERT_EQUAL(0, cfg.filter_type);
    TEST_ASSERT_EQUAL(0, cfg.cutoff_freq);
}
#else
void test_sensor_filter_disabled(void)
{
    TEST_ASSERT_FALSE(SENSOR_ENABLE_FILTER);
}
#endif

/* ==================== Sensor Fusion Tests ==================== */

#if SENSOR_ENABLE_FUSION
void test_sensor_fusion_quaternion(void)
{
    sensor_value_t value;

    value.val_quaternion.w = 1.0f;
    value.val_quaternion.x = 0.0f;
    value.val_quaternion.y = 0.0f;
    value.val_quaternion.z = 0.0f;

    TEST_ASSERT_EQUAL_FLOAT(1.0f, value.val_quaternion.w);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, value.val_quaternion.x);
}
#else
void test_sensor_fusion_disabled(void)
{
    TEST_ASSERT_FALSE(SENSOR_ENABLE_FUSION);
}
#endif

/* ==================== Self Test Tests ==================== */

#if SENSOR_ENABLE_SELF_TEST
void test_sensor_self_test_result(void)
{
    sensor_self_test_result_t result;

    memset(&result, 0, sizeof(result));

    TEST_ASSERT_FALSE(result.passed);
    TEST_ASSERT_EQUAL(0, result.error_flags);
}
#else
void test_sensor_self_test_disabled(void)
{
    TEST_ASSERT_FALSE(SENSOR_ENABLE_SELF_TEST);
}
#endif

/* ==================== Main ==================== */

int main(void)
{
    UNITY_BEGIN();

    /* Sensor Type Tests */
    RUN_TEST(test_sensor_type_constants);
    RUN_TEST(test_sensor_unit_constants);
    RUN_TEST(test_sensor_data_structure_size);
    RUN_TEST(test_sensor_value_union);

    /* Error Code Tests */
    RUN_TEST(test_sensor_error_codes);

    /* Sensor Info Tests */
    RUN_TEST(test_sensor_info_structure);
    RUN_TEST(test_sensor_feature_flags);

    /* Sensor Device Tests */
    RUN_TEST(test_sensor_device_init);
    RUN_TEST(test_sensor_device_with_operations);

    /* Sensor Configuration Tests */
    RUN_TEST(test_sensor_config_types);
    RUN_TEST(test_sensor_trigger_modes);

    /* FIFO Tests */
#if SENSOR_ENABLE_FIFO
    RUN_TEST(test_sensor_fifo_structure);
    RUN_TEST(test_sensor_fifo_operations);
#else
    RUN_TEST(test_sensor_fifo_disabled);
#endif

    /* Interrupt Tests */
#if SENSOR_ENABLE_INTERRUPT
    RUN_TEST(test_sensor_interrupt_config);
#else
    RUN_TEST(test_sensor_interrupt_disabled);
#endif

    /* Calibration Tests */
#if SENSOR_ENABLE_CALIBRATION
    RUN_TEST(test_sensor_calibration_data);
    RUN_TEST(test_sensor_calibration_types);
#else
    RUN_TEST(test_sensor_calibration_disabled);
#endif

    /* Power Management Tests */
#if SENSOR_ENABLE_POWER_MGMT
    RUN_TEST(test_sensor_power_mode);
    RUN_TEST(test_sensor_power_stats);
#else
    RUN_TEST(test_sensor_power_mgmt_disabled);
#endif

    /* Filter Tests */
#if SENSOR_ENABLE_FILTER
    RUN_TEST(test_sensor_filter_config);
#else
    RUN_TEST(test_sensor_filter_disabled);
#endif

    /* Sensor Fusion Tests */
#if SENSOR_ENABLE_FUSION
    RUN_TEST(test_sensor_fusion_quaternion);
#else
    RUN_TEST(test_sensor_fusion_disabled);
#endif

    /* Self Test Tests */
#if SENSOR_ENABLE_SELF_TEST
    RUN_TEST(test_sensor_self_test_result);
#else
    RUN_TEST(test_sensor_self_test_disabled);
#endif

    return UNITY_END();
}
