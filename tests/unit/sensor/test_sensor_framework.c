#include "unity.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "fff.h"
#include "sensor_core.h"
#include "sensor_config.h"
#include "sensor_type.h"

DEFINE_FFF_GLOBALS;

static int g_self_test_mode;
static int g_read_fail_after;
static sensor_err_t g_read_failure;
static sensor_data_t g_last_write;
static sensor_config_type_t g_last_cfg;
static int g_last_cmd;
static bool g_last_enable;

static sensor_err_t mock_init_impl(sensor_device_t *sensor);
static sensor_err_t mock_deinit_impl(sensor_device_t *sensor);
static sensor_err_t mock_read_impl(sensor_device_t *sensor, sensor_data_t *data);
static sensor_err_t mock_write_impl(sensor_device_t *sensor, const sensor_data_t *data);
static sensor_err_t mock_config_impl(sensor_device_t *sensor, sensor_config_type_t cfg, void *value);
static sensor_err_t mock_control_impl(sensor_device_t *sensor, int cmd, void *args);
static sensor_err_t mock_enable_impl(sensor_device_t *sensor, bool enable);
static sensor_err_t mock_self_test_impl(sensor_device_t *sensor, sensor_self_test_result_t *result);
static void mock_callback_impl(sensor_device_t *sensor, sensor_data_t *data, void *user_data);

FAKE_VALUE_FUNC(sensor_err_t, mock_init, sensor_device_t *);
FAKE_VALUE_FUNC(sensor_err_t, mock_deinit, sensor_device_t *);
FAKE_VALUE_FUNC(sensor_err_t, mock_read, sensor_device_t *, sensor_data_t *);
FAKE_VALUE_FUNC(sensor_err_t, mock_write, sensor_device_t *, const sensor_data_t *);
FAKE_VALUE_FUNC(sensor_err_t, mock_config, sensor_device_t *, sensor_config_type_t, void *);
FAKE_VALUE_FUNC(sensor_err_t, mock_control, sensor_device_t *, int, void *);
FAKE_VALUE_FUNC(sensor_err_t, mock_enable, sensor_device_t *, bool);
FAKE_VALUE_FUNC(sensor_err_t, mock_self_test, sensor_device_t *, sensor_self_test_result_t *);
FAKE_VOID_FUNC(mock_callback, sensor_device_t *, sensor_data_t *, void *);

void delay_ms(uint32_t ms)
{
    (void)ms;
}

uint32_t get_tick_ms(void)
{
    static uint32_t tick;
    return ++tick;
}

uint32_t xy_os_tick_get(void)
{
    return get_tick_ms();
}

static sensor_err_t mock_init_impl(sensor_device_t *sensor)
{
    TEST_ASSERT_NOT_NULL(sensor);
    return SENSOR_EOK;
}

static sensor_err_t mock_deinit_impl(sensor_device_t *sensor)
{
    TEST_ASSERT_NOT_NULL(sensor);
    return SENSOR_EOK;
}

static sensor_err_t mock_read_impl(sensor_device_t *sensor, sensor_data_t *data)
{
    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_NOT_NULL(data);
    if (g_read_fail_after == 0) {
        return g_read_failure;
    }
    if (g_read_fail_after > 0) {
        g_read_fail_after--;
    }
    data->type = sensor->info.type;
    data->unit = sensor->info.unit;
    data->value.val_3axis.x = 10;
    data->value.val_3axis.y = 20;
    data->value.val_3axis.z = 30;
    data->timestamp = 1234;
    return SENSOR_EOK;
}

static sensor_err_t mock_write_impl(sensor_device_t *sensor, const sensor_data_t *data)
{
    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_NOT_NULL(data);
    g_last_write = *data;
    return SENSOR_EOK;
}

static sensor_err_t mock_config_impl(sensor_device_t *sensor, sensor_config_type_t cfg, void *value)
{
    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_NOT_NULL(value);
    g_last_cfg = cfg;
    return SENSOR_EOK;
}

static sensor_err_t mock_control_impl(sensor_device_t *sensor, int cmd, void *args)
{
    TEST_ASSERT_NOT_NULL(sensor);
    (void)args;
    g_last_cmd = cmd;
    return SENSOR_EOK;
}

static sensor_err_t mock_enable_impl(sensor_device_t *sensor, bool enable)
{
    TEST_ASSERT_NOT_NULL(sensor);
    g_last_enable = enable;
    return SENSOR_EOK;
}

static sensor_err_t mock_self_test_impl(sensor_device_t *sensor, sensor_self_test_result_t *result)
{
    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_NOT_NULL(result);
    result->passed = (g_self_test_mode == 0);
    result->error_code = (g_self_test_mode == 0) ? 0 : 77;
    strncpy(result->message, (g_self_test_mode == 0) ? "driver self test passed" : "driver self test failed",
            sizeof(result->message) - 1U);
    return (g_self_test_mode == 0) ? SENSOR_EOK : SENSOR_ERROR;
}

static void mock_callback_impl(sensor_device_t *sensor, sensor_data_t *data, void *user_data)
{
    TEST_ASSERT_NOT_NULL(sensor);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL_PTR((void *)0x12345678, user_data);
}

static const sensor_ops_t full_ops = {
    .init = mock_init,
    .deinit = mock_deinit,
    .read = mock_read,
    .write = mock_write,
    .control = mock_control,
    .config = mock_config,
    .enable = mock_enable,
};

static const sensor_ops_t self_test_ops = {
    .init = mock_init,
    .deinit = mock_deinit,
    .read = mock_read,
    .self_test = mock_self_test,
};

static const sensor_ops_t required_ops = {
    .init = mock_init,
    .deinit = mock_deinit,
    .read = mock_read,
};

static void init_sensor(sensor_device_t *sensor, const char *name, const sensor_ops_t *ops)
{
    memset(sensor, 0, sizeof(*sensor));
    strncpy(sensor->info.name, name, sizeof(sensor->info.name) - 1U);
    sensor->info.type = SENSOR_TYPE_ACCELEROMETER;
    sensor->info.unit = SENSOR_UNIT_MILLI_G;
    sensor->ops = ops;
}

void setUp(void)
{
    g_self_test_mode = 0;
    g_read_fail_after = -1;
    g_read_failure = SENSOR_ERROR;

    RESET_FAKE(mock_init);
    RESET_FAKE(mock_deinit);
    RESET_FAKE(mock_read);
    RESET_FAKE(mock_write);
    RESET_FAKE(mock_config);
    RESET_FAKE(mock_control);
    RESET_FAKE(mock_enable);
    RESET_FAKE(mock_self_test);
    RESET_FAKE(mock_callback);
    FFF_RESET_HISTORY();

    mock_init_fake.custom_fake = mock_init_impl;
    mock_deinit_fake.custom_fake = mock_deinit_impl;
    mock_read_fake.custom_fake = mock_read_impl;
    mock_write_fake.custom_fake = mock_write_impl;
    mock_config_fake.custom_fake = mock_config_impl;
    mock_control_fake.custom_fake = mock_control_impl;
    mock_enable_fake.custom_fake = mock_enable_impl;
    mock_self_test_fake.custom_fake = mock_self_test_impl;
    mock_callback_fake.custom_fake = mock_callback_impl;
    memset(&g_last_write, 0, sizeof(g_last_write));
    g_last_cfg = SENSOR_CFG_ODR;
    g_last_cmd = 0;
    g_last_enable = false;
}

void tearDown(void)
{
}

static void test_sensor_type_and_feature_contract(void)
{
    TEST_ASSERT_EQUAL(0x00, SENSOR_TYPE_NONE);
    TEST_ASSERT_EQUAL(0x01, SENSOR_TYPE_ACCELEROMETER);
    TEST_ASSERT_EQUAL(0x02, SENSOR_TYPE_GYROSCOPE);
    TEST_ASSERT_EQUAL(0x03, SENSOR_TYPE_MAGNETOMETER);
    TEST_ASSERT_EQUAL(0x10, SENSOR_TYPE_TEMPERATURE);
    TEST_ASSERT_EQUAL(0x11, SENSOR_TYPE_HUMIDITY);
    TEST_ASSERT_EQUAL(0x12, SENSOR_TYPE_PRESSURE);
    TEST_ASSERT_EQUAL(0x22, SENSOR_TYPE_GPS);
    TEST_ASSERT_EQUAL(0x30, SENSOR_TYPE_LIGHT);
    TEST_ASSERT_EQUAL(0xFF, SENSOR_TYPE_CUSTOM);

    TEST_ASSERT_EQUAL(0, SENSOR_EOK);
    TEST_ASSERT_LESS_THAN_INT(0, SENSOR_ERROR);
    TEST_ASSERT_LESS_THAN_INT(0, SENSOR_EINVAL);

    TEST_ASSERT_EQUAL((1 << 0), SENSOR_FLAG_FIFO_SUPPORT);
    TEST_ASSERT_EQUAL((1 << 1), SENSOR_FLAG_INT_SUPPORT);
    TEST_ASSERT_EQUAL((1 << 2), SENSOR_FLAG_DMA_SUPPORT);
    TEST_ASSERT_EQUAL((1 << 3), SENSOR_FLAG_CALIBRATION);
    TEST_ASSERT_EQUAL((1 << 4), SENSOR_FLAG_SELF_TEST);
    TEST_ASSERT_EQUAL((1 << 5), SENSOR_FLAG_LOW_POWER);
    TEST_ASSERT_EQUAL((1 << 6), SENSOR_FLAG_HIGH_PRECISION);
}

static void test_sensor_register_find_duplicate_and_unregister(void)
{
    sensor_device_t first;
    sensor_device_t duplicate;
    sensor_device_t second;
    init_sensor(&first, "unit_reg_a", &required_ops);
    init_sensor(&duplicate, "unit_reg_a", &required_ops);
    init_sensor(&second, "unit_reg_b", &required_ops);
    second.info.type = SENSOR_TYPE_GYROSCOPE;

    TEST_ASSERT_EQUAL(SENSOR_EINVAL, sensor_register(NULL));
    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_register(&first));
    TEST_ASSERT_EQUAL(SENSOR_STATUS_IDLE, first.status);
    TEST_ASSERT_EQUAL_PTR(&first, sensor_find_by_name("unit_reg_a"));
    TEST_ASSERT_EQUAL_PTR(&first, sensor_find_by_type(SENSOR_TYPE_ACCELEROMETER));
    TEST_ASSERT_EQUAL(SENSOR_ERROR, sensor_register(&duplicate));

    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_register(&second));
    TEST_ASSERT_EQUAL_PTR(&second, sensor_find_by_name("unit_reg_b"));
    TEST_ASSERT_EQUAL_PTR(&second, sensor_find_by_type(SENSOR_TYPE_GYROSCOPE));

    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_unregister(&first));
    TEST_ASSERT_NULL(sensor_find_by_name("unit_reg_a"));
    TEST_ASSERT_EQUAL(SENSOR_ENODEV, sensor_unregister(&first));
    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_unregister(&second));
}

static void test_sensor_lifecycle_read_callback_and_optional_ops(void)
{
    sensor_device_t sensor;
    sensor_data_t data;
    sensor_data_t write_data;
    uint32_t odr = 100;
    int control_arg = 7;
    init_sensor(&sensor, "unit_lifecycle", &full_ops);

    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_register(&sensor));
    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_init(&sensor));
    TEST_ASSERT_EQUAL_UINT(1U, mock_init_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&sensor, mock_init_fake.arg0_val);
    TEST_ASSERT_EQUAL(SENSOR_STATUS_READY, sensor_get_status(&sensor));
    TEST_ASSERT_EQUAL_PTR(&sensor.info, sensor_get_info(&sensor));

    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_set_callback(&sensor, mock_callback, (void *)0x12345678));
    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_read(&sensor, &data));
    TEST_ASSERT_EQUAL_UINT(1U, mock_read_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&sensor, mock_read_fake.arg0_val);
    TEST_ASSERT_EQUAL_PTR(&data, mock_read_fake.arg1_val);
    TEST_ASSERT_EQUAL_UINT(1U, mock_callback_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&sensor, mock_callback_fake.arg0_val);
    TEST_ASSERT_EQUAL_PTR(&data, mock_callback_fake.arg1_val);
    TEST_ASSERT_EQUAL_PTR((void *)0x12345678, mock_callback_fake.arg2_val);
    TEST_ASSERT_EQUAL(SENSOR_TYPE_ACCELEROMETER, data.type);
    TEST_ASSERT_EQUAL(10, data.value.val_3axis.x);
    TEST_ASSERT_EQUAL(SENSOR_STATUS_READY, sensor.status);

    memset(&write_data, 0, sizeof(write_data));
    write_data.type = SENSOR_TYPE_ACCELEROMETER;
    write_data.value.val_int32 = 42;
    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_write(&sensor, &write_data));
    TEST_ASSERT_EQUAL_UINT(1U, mock_write_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&sensor, mock_write_fake.arg0_val);
    TEST_ASSERT_EQUAL_PTR(&write_data, mock_write_fake.arg1_val);
    TEST_ASSERT_EQUAL(42, g_last_write.value.val_int32);

    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_config(&sensor, SENSOR_CFG_ODR, &odr));
    TEST_ASSERT_EQUAL_UINT(1U, mock_config_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&sensor, mock_config_fake.arg0_val);
    TEST_ASSERT_EQUAL(SENSOR_CFG_ODR, mock_config_fake.arg1_val);
    TEST_ASSERT_EQUAL_PTR(&odr, mock_config_fake.arg2_val);
    TEST_ASSERT_EQUAL(SENSOR_CFG_ODR, g_last_cfg);

    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_control(&sensor, 99, &control_arg));
    TEST_ASSERT_EQUAL_UINT(1U, mock_control_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&sensor, mock_control_fake.arg0_val);
    TEST_ASSERT_EQUAL(99, mock_control_fake.arg1_val);
    TEST_ASSERT_EQUAL_PTR(&control_arg, mock_control_fake.arg2_val);
    TEST_ASSERT_EQUAL(99, g_last_cmd);

    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_enable(&sensor, true));
    TEST_ASSERT_EQUAL_UINT(1U, mock_enable_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&sensor, mock_enable_fake.arg0_val);
    TEST_ASSERT_TRUE(mock_enable_fake.arg1_val);
    TEST_ASSERT_TRUE(g_last_enable);

    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_deinit(&sensor));
    TEST_ASSERT_EQUAL_UINT(1U, mock_deinit_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&sensor, mock_deinit_fake.arg0_val);
    TEST_ASSERT_EQUAL(SENSOR_STATUS_IDLE, sensor.status);
    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_unregister(&sensor));
}

static void test_sensor_read_failure_restores_status_and_skips_callback(void)
{
    sensor_device_t sensor;
    sensor_data_t data;
    init_sensor(&sensor, "unit_read_fail", &required_ops);
    memset(&data, 0xA5, sizeof(data));
    g_read_fail_after = 0;
    g_read_failure = SENSOR_EIO;

    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_register(&sensor));
    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_init(&sensor));
    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_set_callback(&sensor, mock_callback, (void *)0x12345678));

    TEST_ASSERT_EQUAL(SENSOR_EIO, sensor_read(&sensor, &data));
    TEST_ASSERT_EQUAL_UINT(1U, mock_read_fake.call_count);
    TEST_ASSERT_EQUAL_UINT(0U, mock_callback_fake.call_count);
    TEST_ASSERT_EQUAL(SENSOR_STATUS_READY, sensor.status);
    TEST_ASSERT_EQUAL_HEX8(0xA5U, ((uint8_t *)&data)[0]);
    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_unregister(&sensor));
}

static void test_sensor_invalid_and_busy_paths_do_not_call_driver(void)
{
    sensor_device_t sensor;
    sensor_data_t data;
    init_sensor(&sensor, "unit_invalid_busy", &required_ops);
    memset(&data, 0, sizeof(data));

    TEST_ASSERT_EQUAL(SENSOR_EINVAL, sensor_read(NULL, &data));
    TEST_ASSERT_EQUAL(SENSOR_EINVAL, sensor_read(&sensor, NULL));
    TEST_ASSERT_EQUAL(SENSOR_ERROR, sensor_read(&sensor, &data));
    TEST_ASSERT_EQUAL_UINT(0U, mock_read_fake.call_count);

    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_register(&sensor));
    sensor.status = SENSOR_STATUS_BUSY;
    TEST_ASSERT_EQUAL(SENSOR_ERROR, sensor_read(&sensor, &data));
    TEST_ASSERT_EQUAL_UINT(0U, mock_read_fake.call_count);
    sensor.status = SENSOR_STATUS_IDLE;
    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_unregister(&sensor));
}

static void test_sensor_init_and_deinit_failure_preserve_status(void)
{
    sensor_device_t sensor;
    init_sensor(&sensor, "unit_lifecycle_fail", &required_ops);
    mock_init_fake.custom_fake = NULL;
    mock_init_fake.return_val = SENSOR_EIO;

    TEST_ASSERT_EQUAL(SENSOR_EIO, sensor_init(&sensor));
    TEST_ASSERT_EQUAL(SENSOR_STATUS_IDLE, sensor.status);

    sensor.status = SENSOR_STATUS_READY;
    mock_deinit_fake.custom_fake = NULL;
    mock_deinit_fake.return_val = SENSOR_EIO;
    TEST_ASSERT_EQUAL(SENSOR_EIO, sensor_deinit(&sensor));
    TEST_ASSERT_EQUAL(SENSOR_STATUS_READY, sensor.status);
}

static void test_sensor_missing_optional_ops_report_enosys(void)
{
    sensor_device_t sensor;
    sensor_data_t data;
    uint32_t odr = 10;
    init_sensor(&sensor, "unit_enosys", &required_ops);

    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_register(&sensor));
    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_init(&sensor));
    TEST_ASSERT_EQUAL(SENSOR_ENOSYS, sensor_enable(&sensor, true));
    TEST_ASSERT_EQUAL(SENSOR_ENOSYS, sensor_write(&sensor, &data));
    TEST_ASSERT_EQUAL(SENSOR_ENOSYS, sensor_config(&sensor, SENSOR_CFG_ODR, &odr));
    TEST_ASSERT_EQUAL(SENSOR_ENOSYS, sensor_control(&sensor, 1, NULL));
    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_unregister(&sensor));
}

#if SENSOR_ENABLE_FIFO
static void test_sensor_fifo_push_read_flush_and_watermark(void)
{
    sensor_device_t sensor;
    sensor_data_t in[3];
    sensor_data_t out[3];
    uint32_t read_count = 0;
    uint32_t fifo_count = 0;
    init_sensor(&sensor, "unit_fifo", &required_ops);
    memset(in, 0, sizeof(in));
    memset(out, 0, sizeof(out));
    in[0].value.val_int32 = 11;
    in[1].value.val_int32 = 22;
    in[2].value.val_int32 = 33;

    TEST_ASSERT_EQUAL(SENSOR_EINVAL, sensor_fifo_init(NULL, 2));
    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_fifo_init(&sensor, 2));
    TEST_ASSERT_FALSE(sensor_fifo_is_full(&sensor));
    TEST_ASSERT_TRUE(sensor_fifo_is_empty(&sensor));
    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_fifo_set_watermark(&sensor, 1));

    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_fifo_push(&sensor, &in[0]));
    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_fifo_push(&sensor, &in[1]));
    TEST_ASSERT_TRUE(sensor_fifo_is_full(&sensor));
    TEST_ASSERT_EQUAL(SENSOR_EOVERFLOW, sensor_fifo_push(&sensor, &in[2]));

    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_fifo_get_count(&sensor, &fifo_count));
    TEST_ASSERT_EQUAL(2U, fifo_count);
    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_fifo_read(&sensor, out, 3, &read_count));
    TEST_ASSERT_EQUAL(2U, read_count);
    TEST_ASSERT_EQUAL(11, out[0].value.val_int32);
    TEST_ASSERT_EQUAL(22, out[1].value.val_int32);
    TEST_ASSERT_TRUE(sensor_fifo_is_empty(&sensor));

    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_fifo_push(&sensor, &in[2]));
    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_fifo_flush(&sensor));
    TEST_ASSERT_TRUE(sensor_fifo_is_empty(&sensor));
    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_fifo_deinit(&sensor));
    TEST_ASSERT_NULL(sensor.fifo);
}
#endif

#if SENSOR_ENABLE_CALIBRATION
static void test_sensor_calibration_set_get_and_apply(void)
{
    sensor_device_t sensor;
    sensor_calibration_data_t calib;
    sensor_calibration_data_t got;
    sensor_data_t data;
    init_sensor(&sensor, "unit_calib", &required_ops);
    memset(&calib, 0, sizeof(calib));
    memset(&got, 0, sizeof(got));
    memset(&data, 0, sizeof(data));

    calib.type = SENSOR_CALIB_OFFSET;
    calib.offset.val_3axis.x = 1;
    calib.offset.val_3axis.y = -2;
    calib.offset.val_3axis.z = 3;
    calib.valid = true;

    TEST_ASSERT_EQUAL(SENSOR_ECALIB, sensor_get_calibration(&sensor, &got));
    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_set_calibration(&sensor, &calib));
    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_get_calibration(&sensor, &got));
    TEST_ASSERT_TRUE(got.valid);
    TEST_ASSERT_EQUAL(1, got.offset.val_3axis.x);

    data.value.val_3axis.x = 10;
    data.value.val_3axis.y = 20;
    data.value.val_3axis.z = 30;
    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_apply_calibration(&sensor, &data));
    TEST_ASSERT_EQUAL(9, data.value.val_3axis.x);
    TEST_ASSERT_EQUAL(22, data.value.val_3axis.y);
    TEST_ASSERT_EQUAL(27, data.value.val_3axis.z);
}
#endif

#if SENSOR_ENABLE_FILTER
static void test_sensor_moving_average_filter(void)
{
    sensor_device_t sensor;
    sensor_filter_config_t config;
    sensor_data_t data;
    init_sensor(&sensor, "unit_filter", &required_ops);
    memset(&config, 0, sizeof(config));
    memset(&data, 0, sizeof(data));
    config.type = SENSOR_FILTER_MOVING_AVERAGE;
    config.window_size = 2;
    config.enable = true;

    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_filter_init(&sensor, &config));
    data.value.val_3axis.x = 10;
    data.value.val_3axis.y = 20;
    data.value.val_3axis.z = 30;
    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_filter_process(&sensor, &data));
    TEST_ASSERT_EQUAL(10, data.value.val_3axis.x);

    data.value.val_3axis.x = 30;
    data.value.val_3axis.y = 40;
    data.value.val_3axis.z = 50;
    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_filter_process(&sensor, &data));
    TEST_ASSERT_EQUAL(20, data.value.val_3axis.x);
    TEST_ASSERT_EQUAL(30, data.value.val_3axis.y);
    TEST_ASSERT_EQUAL(40, data.value.val_3axis.z);
    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_filter_reset(&sensor));
    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_filter_deinit(&sensor));
}
#endif

#if SENSOR_ENABLE_SELF_TEST
static void test_sensor_self_test_uses_driver_override_and_restores_status(void)
{
    sensor_device_t sensor;
    sensor_self_test_result_t result;
    init_sensor(&sensor, "unit_self_driver", &self_test_ops);
    memset(&result, 0xA5, sizeof(result));

    TEST_ASSERT_EQUAL(SENSOR_EINVAL, sensor_self_test(NULL, &result));
    TEST_ASSERT_EQUAL(SENSOR_EINVAL, sensor_self_test(&sensor, NULL));

    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_self_test(&sensor, &result));
    TEST_ASSERT_EQUAL_UINT(1U, mock_self_test_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(&sensor, mock_self_test_fake.arg0_val);
    TEST_ASSERT_EQUAL_PTR(&result, mock_self_test_fake.arg1_val);
    TEST_ASSERT_TRUE(result.passed);
    TEST_ASSERT_EQUAL(0, result.error_code);
    TEST_ASSERT_EQUAL_STRING("driver self test passed", result.message);
    TEST_ASSERT_EQUAL(SENSOR_STATUS_READY, sensor.status);

    g_self_test_mode = 1;
    TEST_ASSERT_EQUAL(SENSOR_ERROR, sensor_self_test(&sensor, &result));
    TEST_ASSERT_EQUAL_UINT(2U, mock_self_test_fake.call_count);
    TEST_ASSERT_FALSE(result.passed);
    TEST_ASSERT_EQUAL(77, result.error_code);
    TEST_ASSERT_EQUAL(SENSOR_STATUS_READY, sensor.status);
}

static void test_sensor_self_test_generic_read_range_and_noise_paths(void)
{
    sensor_device_t sensor;
    sensor_self_test_result_t result;
    init_sensor(&sensor, "unit_self_generic", &required_ops);
    sensor.info.type = SENSOR_TYPE_LIGHT;

    TEST_ASSERT_EQUAL(SENSOR_EOK, sensor_self_test(&sensor, &result));
    TEST_ASSERT_TRUE(result.passed);
    TEST_ASSERT_EQUAL(0, result.error_code);
    TEST_ASSERT_EQUAL_STRING("Self test passed", result.message);
    TEST_ASSERT_EQUAL_UINT(11U, mock_read_fake.call_count);
    TEST_ASSERT_EQUAL(SENSOR_STATUS_READY, sensor.status);

    sensor.info.type = SENSOR_TYPE_ACCELEROMETER;
    TEST_ASSERT_EQUAL(SENSOR_ERROR, sensor_self_test(&sensor, &result));
    TEST_ASSERT_FALSE(result.passed);
    TEST_ASSERT_EQUAL(2, result.error_code);
    TEST_ASSERT_NOT_NULL(strstr(result.message, "Data out of range"));
    TEST_ASSERT_EQUAL(SENSOR_STATUS_READY, sensor.status);
}
#endif

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_sensor_type_and_feature_contract);
    RUN_TEST(test_sensor_register_find_duplicate_and_unregister);
    RUN_TEST(test_sensor_lifecycle_read_callback_and_optional_ops);
    RUN_TEST(test_sensor_read_failure_restores_status_and_skips_callback);
    RUN_TEST(test_sensor_invalid_and_busy_paths_do_not_call_driver);
    RUN_TEST(test_sensor_init_and_deinit_failure_preserve_status);
    RUN_TEST(test_sensor_missing_optional_ops_report_enosys);
#if SENSOR_ENABLE_FIFO
    RUN_TEST(test_sensor_fifo_push_read_flush_and_watermark);
#endif
#if SENSOR_ENABLE_CALIBRATION
    RUN_TEST(test_sensor_calibration_set_get_and_apply);
#endif
#if SENSOR_ENABLE_FILTER
    RUN_TEST(test_sensor_moving_average_filter);
#endif
#if SENSOR_ENABLE_SELF_TEST
    RUN_TEST(test_sensor_self_test_uses_driver_override_and_restores_status);
    RUN_TEST(test_sensor_self_test_generic_read_range_and_noise_paths);
#endif
    return UNITY_END();
}
