#include "unity.h"

#include "xy_dmp.h"
#include "xy_mpu6050.h"

#include <stdarg.h>
#include <string.h>

static float g_accel[3];
static float g_gyro[3];
static int g_accel_ret;
static int g_gyro_ret;
static uint32_t g_tick;
static uint32_t g_delay_calls;
static uint32_t g_last_delay_ms;

void setUp(void)
{
    g_accel[0] = 0.0f;
    g_accel[1] = 0.0f;
    g_accel[2] = 1.0f;
    g_gyro[0] = 0.0f;
    g_gyro[1] = 0.0f;
    g_gyro[2] = 0.0f;
    g_accel_ret = XY_MPU6050_OK;
    g_gyro_ret = XY_MPU6050_OK;
    g_tick = 1234;
    g_delay_calls = 0;
    g_last_delay_ms = 0;
}

void tearDown(void)
{
}

int xy_printf(const char *fmt, ...)
{
    (void)fmt;
    return 0;
}

uint32_t xy_os_tick_get(void)
{
    return g_tick;
}

void xy_hal_delay_ms(uint32_t ms)
{
    g_delay_calls++;
    g_last_delay_ms = ms;
}

int xy_mpu6050_read_accel(xy_mpu6050_t *dev, float *ax, float *ay, float *az)
{
    TEST_ASSERT_NOT_NULL(dev);
    if (g_accel_ret != XY_MPU6050_OK) {
        return g_accel_ret;
    }
    *ax = g_accel[0];
    *ay = g_accel[1];
    *az = g_accel[2];
    return XY_MPU6050_OK;
}

int xy_mpu6050_read_gyro(xy_mpu6050_t *dev, float *gx, float *gy, float *gz)
{
    TEST_ASSERT_NOT_NULL(dev);
    if (g_gyro_ret != XY_MPU6050_OK) {
        return g_gyro_ret;
    }
    *gx = g_gyro[0];
    *gy = g_gyro[1];
    *gz = g_gyro[2];
    return XY_MPU6050_OK;
}

static xy_dmp_t init_dmp(xy_mpu6050_t *mpu)
{
    xy_dmp_t dmp;
    TEST_ASSERT_EQUAL_INT(XY_DMP_OK, xy_dmp_init(&dmp, mpu));
    return dmp;
}

void test_init_sets_identity_quaternion_and_guards_params(void)
{
    xy_dmp_t dmp;
    xy_mpu6050_t mpu;
    memset(&mpu, 0, sizeof(mpu));

    TEST_ASSERT_EQUAL_INT(XY_DMP_INVALID_PARAM, xy_dmp_init(NULL, &mpu));
    TEST_ASSERT_EQUAL_INT(XY_DMP_INVALID_PARAM, xy_dmp_init(&dmp, NULL));

    memset(&dmp, 0xA5, sizeof(dmp));
    TEST_ASSERT_EQUAL_INT(XY_DMP_OK, xy_dmp_init(&dmp, &mpu));
    TEST_ASSERT_EQUAL_PTR(&mpu, dmp.mpu);
    TEST_ASSERT_TRUE(dmp.initialized);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, dmp.q.w);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, dmp.q.x);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, dmp.q.y);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, dmp.q.z);
}

void test_update_uses_accel_gyro_and_records_tick(void)
{
    xy_mpu6050_t mpu;
    xy_dmp_t dmp = init_dmp(&mpu);
    xy_euler_t euler;
    xy_quaternion_t q;
    float gx, gy, gz;

    g_accel[0] = 0.0f;
    g_accel[1] = 0.0f;
    g_accel[2] = 1.0f;
    g_gyro[2] = 90.0f;
    g_tick = 5678;

    TEST_ASSERT_EQUAL_INT(XY_DMP_OK, xy_dmp_update(&dmp));
    TEST_ASSERT_EQUAL_UINT32(5678, dmp.last_update);

    TEST_ASSERT_EQUAL_INT(XY_DMP_OK, xy_dmp_get_euler(&dmp, &euler));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0361f, euler.yaw);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, euler.roll);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, euler.pitch);

    TEST_ASSERT_EQUAL_INT(XY_DMP_OK, xy_dmp_get_quaternion(&dmp, &q));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, q.w);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0180f, q.z);

    TEST_ASSERT_EQUAL_INT(XY_DMP_OK, xy_dmp_get_gravity(&dmp, &gx, &gy, &gz));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, gx);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, gy);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, gz);
}

void test_update_guards_not_initialized_and_sensor_errors(void)
{
    xy_mpu6050_t mpu;
    xy_dmp_t dmp;
    xy_quaternion_t before_q;
    xy_euler_t before_euler;
    float before_gravity[3];
    uint32_t before_tick;
    memset(&dmp, 0, sizeof(dmp));
    dmp.mpu = &mpu;

    TEST_ASSERT_EQUAL_INT(XY_DMP_NOT_INIT, xy_dmp_update(NULL));
    TEST_ASSERT_EQUAL_INT(XY_DMP_NOT_INIT, xy_dmp_update(&dmp));

    dmp = init_dmp(&mpu);
    dmp.euler.roll = 1.0f;
    dmp.euler.pitch = -0.5f;
    dmp.euler.yaw = 0.25f;
    dmp.q.w = 0.5f;
    dmp.q.x = 0.5f;
    dmp.q.y = -0.5f;
    dmp.q.z = 0.5f;
    dmp.gravity[0] = 0.1f;
    dmp.gravity[1] = 0.2f;
    dmp.gravity[2] = 0.3f;
    dmp.last_update = 0xCAFEU;
    before_q = dmp.q;
    before_euler = dmp.euler;
    memcpy(before_gravity, dmp.gravity, sizeof(before_gravity));
    before_tick = dmp.last_update;

    g_accel_ret = XY_MPU6050_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_DMP_ERROR, xy_dmp_update(&dmp));
    TEST_ASSERT_EQUAL_MEMORY(&before_q, &dmp.q, sizeof(before_q));
    TEST_ASSERT_EQUAL_MEMORY(&before_euler, &dmp.euler, sizeof(before_euler));
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(before_gravity, dmp.gravity, 3);
    TEST_ASSERT_EQUAL_UINT32(before_tick, dmp.last_update);

    g_accel_ret = XY_MPU6050_OK;
    g_gyro_ret = XY_MPU6050_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_DMP_ERROR, xy_dmp_update(&dmp));
    TEST_ASSERT_EQUAL_MEMORY(&before_q, &dmp.q, sizeof(before_q));
    TEST_ASSERT_EQUAL_MEMORY(&before_euler, &dmp.euler, sizeof(before_euler));
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(before_gravity, dmp.gravity, 3);
    TEST_ASSERT_EQUAL_UINT32(before_tick, dmp.last_update);
}

void test_getters_convert_angles_to_degrees_and_allow_partial_outputs(void)
{
    xy_mpu6050_t mpu;
    xy_dmp_t dmp = init_dmp(&mpu);
    float roll = -1.0f;
    float pitch = -1.0f;
    float yaw = -1.0f;
    float gx = -2.0f;
    float gz = -2.0f;

    dmp.euler.roll = 3.14159265358979323846f / 2.0f;
    dmp.euler.pitch = -3.14159265358979323846f / 4.0f;
    dmp.euler.yaw = 3.14159265358979323846f;
    dmp.gravity[0] = 0.25f;
    dmp.gravity[1] = -0.5f;
    dmp.gravity[2] = 0.75f;

    TEST_ASSERT_EQUAL_INT(XY_DMP_INVALID_PARAM, xy_dmp_get_quaternion(NULL, &(xy_quaternion_t){0}));
    TEST_ASSERT_EQUAL_INT(XY_DMP_INVALID_PARAM, xy_dmp_get_euler(&dmp, NULL));
    TEST_ASSERT_EQUAL_INT(XY_DMP_INVALID_PARAM, xy_dmp_get_angles(NULL, &roll, &pitch, &yaw));
    TEST_ASSERT_EQUAL_INT(XY_DMP_INVALID_PARAM, xy_dmp_get_gravity(NULL, &gx, NULL, &gz));

    TEST_ASSERT_EQUAL_INT(XY_DMP_OK, xy_dmp_get_angles(&dmp, &roll, &pitch, &yaw));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 90.0f, roll);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -45.0f, pitch);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 180.0f, yaw);

    TEST_ASSERT_EQUAL_INT(XY_DMP_OK, xy_dmp_get_gravity(&dmp, &gx, NULL, &gz));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.25f, gx);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.75f, gz);

    roll = -9.0f;
    pitch = -8.0f;
    yaw = -7.0f;
    TEST_ASSERT_EQUAL_INT(XY_DMP_OK, xy_dmp_get_angles(&dmp, &roll, NULL, &yaw));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 90.0f, roll);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -8.0f, pitch);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 180.0f, yaw);

    gx = -6.0f;
    gz = -4.0f;
    TEST_ASSERT_EQUAL_INT(XY_DMP_OK, xy_dmp_get_gravity(&dmp, NULL, NULL, NULL));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -6.0f, gx);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -4.0f, gz);
}

void test_calibrate_runs_requested_samples_and_delay(void)
{
    xy_mpu6050_t mpu;
    xy_dmp_t dmp = init_dmp(&mpu);

    TEST_ASSERT_EQUAL_INT(XY_DMP_INVALID_PARAM, xy_dmp_calibrate(NULL, 1));
    TEST_ASSERT_EQUAL_INT(XY_DMP_INVALID_PARAM, xy_dmp_calibrate(&dmp, 0));

    TEST_ASSERT_EQUAL_INT(XY_DMP_OK, xy_dmp_calibrate(&dmp, 3));
    TEST_ASSERT_EQUAL_UINT32(3, g_delay_calls);
    TEST_ASSERT_EQUAL_UINT32(23, g_last_delay_ms);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_sets_identity_quaternion_and_guards_params);
    RUN_TEST(test_update_uses_accel_gyro_and_records_tick);
    RUN_TEST(test_update_guards_not_initialized_and_sensor_errors);
    RUN_TEST(test_getters_convert_angles_to_degrees_and_allow_partial_outputs);
    RUN_TEST(test_calibrate_runs_requested_samples_and_delay);
    return UNITY_END();
}
