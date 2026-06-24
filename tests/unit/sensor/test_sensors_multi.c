/**
 * @file test_sensors_multi.c
 * @brief Heterogeneous sensor coexistence test.
 *
 * Drives four real I2C sensor drivers (SHT30, MPU6050, BMP280, ADS1115)
 * through the same registration helper and verifies they all show up in
 * the device registry simultaneously. This is the strongest evidence
 * that the post-refactor framework supports the actual driver fleet.
 */

#include <string.h>

#include "unity.h"
#include "xy_device.h"
#include "xy_device_core.h"
#include "xy_sht30.h"
#include "xy_mpu6050.h"
#include "xy_bmp280.h"
#include "xy_ads1115.h"

static int g_fake_bus = 1;

#define ASSERT(cond) TEST_ASSERT_TRUE(cond)

void setUp(void)
{
}

void tearDown(void)
{
}

static void drain_registry(void)
{
    while (xy_device_registry_count() > 0) {
        size_t n = 0;
        const xy_device_registry_entry_t *entries = xy_device_get_registry(&n);
        if (n == 0) break;
        xy_device_registry_unregister(entries[0].device);
    }
}

/* All four drivers init + register, then we look each up by name. */
static void test_four_drivers_coexist(void)
{
    xy_device_registry_init();
    drain_registry();

    static xy_sht30_t   sht;
    static xy_mpu6050_t mpu;
    static xy_bmp280_t  bmp;
    static xy_ads1115_t ads;
    memset(&sht, 0, sizeof(sht));
    memset(&mpu, 0, sizeof(mpu));
    memset(&bmp, 0, sizeof(bmp));
    memset(&ads, 0, sizeof(ads));

    xy_sht30_init(&sht, &g_fake_bus);
    xy_mpu6050_init(&mpu, &g_fake_bus);
    xy_bmp280_init(&bmp, &g_fake_bus);
    xy_ads1115_init(&ads, &g_fake_bus, 3.3f);

    ASSERT(xy_i2c_device_register(&sht.i2c_dev, "sht30",   XY_DEV_TYPE_SENSOR) == XY_DEVICE_OK);
    ASSERT(xy_i2c_device_register(&mpu.i2c_dev, "mpu6050", XY_DEV_TYPE_SENSOR) == XY_DEVICE_OK);
    ASSERT(xy_i2c_device_register(&bmp.i2c_dev, "bmp280",  XY_DEV_TYPE_SENSOR) == XY_DEVICE_OK);
    ASSERT(xy_i2c_device_register(&ads.i2c_dev, "ads1115", XY_DEV_TYPE_SENSOR) == XY_DEVICE_OK);

    ASSERT(xy_device_registry_count() == 4);
    ASSERT(xy_device_find("sht30")   == &sht.i2c_dev.base);
    ASSERT(xy_device_find("mpu6050") == &mpu.i2c_dev.base);
    ASSERT(xy_device_find("bmp280")  == &bmp.i2c_dev.base);
    ASSERT(xy_device_find("ads1115") == &ads.i2c_dev.base);
}

/* Stats should reflect heterogeneity even though all are I2C/sensor here. */
static void test_stats_reflect_population(void)
{
    xy_device_stats_t stats;
    ASSERT(xy_device_get_stats(&stats) == XY_DEVICE_OK);
    ASSERT(stats.total_devices == 4);
    ASSERT(stats.sensor_count == 4);
}

/* find_by_type with index lets you enumerate same-type devices. */
static void test_enumerate_by_type(void)
{
    int found_count = 0;
    for (size_t i = 0; i < 8; i++) {
        xy_device_t *dev = xy_device_find_by_type(XY_DEV_TYPE_SENSOR, i);
        if (!dev) break;
        found_count++;
    }
    ASSERT(found_count == 4);
}

/* Removing one driver must not disturb the others. */
static void test_partial_unregister(void)
{
    xy_device_t *bmp = xy_device_find("bmp280");
    ASSERT(bmp != NULL);
    ASSERT(xy_device_registry_unregister(bmp) == XY_DEVICE_OK);

    ASSERT(xy_device_registry_count() == 3);
    ASSERT(xy_device_find("bmp280") == NULL);
    ASSERT(xy_device_find("sht30")   != NULL);
    ASSERT(xy_device_find("mpu6050") != NULL);
    ASSERT(xy_device_find("ads1115") != NULL);
}

/* Name collision rejection still works across drivers. */
static void test_cross_driver_name_collision(void)
{
    static xy_sht30_t imposter;
    memset(&imposter, 0, sizeof(imposter));
    xy_sht30_init(&imposter, &g_fake_bus);

    /* "mpu6050" is already registered (by an MPU6050 instance).
     * Registering an SHT30 with the same name must fail. */
    int ret = xy_i2c_device_register(&imposter.i2c_dev, "mpu6050", XY_DEV_TYPE_SENSOR);
    ASSERT(ret != XY_DEVICE_OK);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_four_drivers_coexist);
    RUN_TEST(test_stats_reflect_population);
    RUN_TEST(test_enumerate_by_type);
    RUN_TEST(test_partial_unregister);
    RUN_TEST(test_cross_driver_name_collision);

    return UNITY_END();
}
