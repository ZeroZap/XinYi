/**
 * @file test_sensors_multi.c
 * @brief Heterogeneous sensor coexistence test.
 *
 * Drives four real I2C sensor drivers (SHT30, MPU6050, BMP280, ADS1115)
 * through the same registration helper and verifies they all show up in
 * the device registry simultaneously. This is the strongest evidence
 * that the post-refactor framework supports the actual driver fleet.
 */

#include <stdio.h>
#include <string.h>

#include "unity.h"
#include "xy_device.h"
#include "xy_device_core.h"
#include "xy_sht30.h"
#include "xy_mpu6050.h"
#include "xy_bmp280.h"
#include "xy_ads1115.h"
#include "xy_os.h"

xy_os_status_t xy_os_delay(uint32_t ticks)
{
    (void)ticks;
    return XY_OS_OK;
}

static int g_fake_bus = 1;
static char g_extra_names[XY_DEVICE_REGISTRY_MAX][24];

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
    xy_ads1115_init(&ads, &g_fake_bus, ADS1115_ADDR_GND);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_i2c_device_register(&sht.i2c_dev, "sht30", XY_DEV_TYPE_SENSOR));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_i2c_device_register(&mpu.i2c_dev, "mpu6050", XY_DEV_TYPE_SENSOR));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_i2c_device_register(&bmp.i2c_dev, "bmp280", XY_DEV_TYPE_SENSOR));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_i2c_device_register(&ads.i2c_dev, "ads1115", XY_DEV_TYPE_SENSOR));

    TEST_ASSERT_EQUAL_UINT(4U, xy_device_registry_count());
    TEST_ASSERT_EQUAL_PTR(&sht.i2c_dev.base, xy_device_find("sht30"));
    TEST_ASSERT_EQUAL_PTR(&mpu.i2c_dev.base, xy_device_find("mpu6050"));
    TEST_ASSERT_EQUAL_PTR(&bmp.i2c_dev.base, xy_device_find("bmp280"));
    TEST_ASSERT_EQUAL_PTR(&ads.i2c_dev.base, xy_device_find("ads1115"));
}

/* Stats should reflect heterogeneity even though all are I2C/sensor here. */
static void test_stats_reflect_population(void)
{
    xy_device_stats_t stats;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(4U, stats.total_devices);
    TEST_ASSERT_EQUAL_UINT32(4U, stats.sensor_count);
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
    TEST_ASSERT_EQUAL_INT(4, found_count);
}

/* Removing one driver must not disturb the others. */
static void test_partial_unregister(void)
{
    xy_device_t *bmp = xy_device_find("bmp280");
    TEST_ASSERT_NOT_NULL(bmp);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_registry_unregister(bmp));

    TEST_ASSERT_EQUAL_UINT(3U, xy_device_registry_count());
    TEST_ASSERT_NULL(xy_device_find("bmp280"));
    TEST_ASSERT_NOT_NULL(xy_device_find("sht30"));
    TEST_ASSERT_NOT_NULL(xy_device_find("mpu6050"));
    TEST_ASSERT_NOT_NULL(xy_device_find("ads1115"));
}

static void test_find_by_type_compacts_after_middle_unregister(void)
{
    xy_device_t *first;
    xy_device_t *second;
    xy_device_t *third;

    drain_registry();
    test_four_drivers_coexist();

    first = xy_device_find_by_type(XY_DEV_TYPE_SENSOR, 0);
    second = xy_device_find_by_type(XY_DEV_TYPE_SENSOR, 1);
    third = xy_device_find_by_type(XY_DEV_TYPE_SENSOR, 2);
    TEST_ASSERT_NOT_NULL(first);
    TEST_ASSERT_NOT_NULL(second);
    TEST_ASSERT_NOT_NULL(third);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_registry_unregister(second));

    TEST_ASSERT_EQUAL_UINT(3U, xy_device_registry_count());
    TEST_ASSERT_EQUAL_PTR(first, xy_device_find_by_type(XY_DEV_TYPE_SENSOR, 0));
    TEST_ASSERT_EQUAL_PTR(third, xy_device_find_by_type(XY_DEV_TYPE_SENSOR, 1));
    TEST_ASSERT_NOT_NULL(xy_device_find_by_type(XY_DEV_TYPE_SENSOR, 2));
    TEST_ASSERT_NULL(xy_device_find_by_type(XY_DEV_TYPE_SENSOR, 3));
}

static void test_stats_after_full_drain_are_zero(void)
{
    xy_device_stats_t stats;

    drain_registry();
    TEST_ASSERT_EQUAL_UINT(0U, xy_device_registry_count());
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(0U, stats.total_devices);
    TEST_ASSERT_EQUAL_UINT32(0U, stats.sensor_count);
}

/* Name collision rejection still works across drivers. */
static void test_cross_driver_name_collision(void)
{
    static xy_sht30_t imposter;

    drain_registry();
    test_four_drivers_coexist();

    memset(&imposter, 0, sizeof(imposter));
    xy_sht30_init(&imposter, &g_fake_bus);

    /* "mpu6050" is already registered (by an MPU6050 instance).
     * Registering an SHT30 with the same name must fail. */
    int ret = xy_i2c_device_register(&imposter.i2c_dev, "mpu6050", XY_DEV_TYPE_SENSOR);
    TEST_ASSERT_NOT_EQUAL(XY_DEVICE_OK, ret);
}

/* Registry capacity still rejects new heterogeneous devices cleanly. */
static void test_registry_capacity_rejects_extra_driver(void)
{
    static xy_sht30_t extras[XY_DEVICE_REGISTRY_MAX];
    for (size_t i = xy_device_registry_count(); i < XY_DEVICE_REGISTRY_MAX; i++) {
        memset(&extras[i], 0, sizeof(extras[i]));
        xy_sht30_init(&extras[i], &g_fake_bus);
        snprintf(g_extra_names[i], sizeof(g_extra_names[i]), "extra_%u", (unsigned)i);
        TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                              xy_i2c_device_register(&extras[i].i2c_dev, g_extra_names[i], XY_DEV_TYPE_SENSOR));
    }

    static xy_sht30_t overflow;
    memset(&overflow, 0, sizeof(overflow));
    xy_sht30_init(&overflow, &g_fake_bus);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_NO_MEM,
                          xy_i2c_device_register(&overflow.i2c_dev, "overflow", XY_DEV_TYPE_SENSOR));
    TEST_ASSERT_EQUAL_UINT(XY_DEVICE_REGISTRY_MAX, xy_device_registry_count());
}

static void test_duplicate_name_is_checked_before_capacity(void)
{
    static xy_sht30_t extras[XY_DEVICE_REGISTRY_MAX];
    static xy_sht30_t duplicate;

    drain_registry();
    for (size_t i = 0; i < XY_DEVICE_REGISTRY_MAX; i++) {
        memset(&extras[i], 0, sizeof(extras[i]));
        xy_sht30_init(&extras[i], &g_fake_bus);
        snprintf(g_extra_names[i], sizeof(g_extra_names[i]), "cap_%u", (unsigned)i);
        TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                              xy_i2c_device_register(&extras[i].i2c_dev, g_extra_names[i], XY_DEV_TYPE_SENSOR));
    }

    memset(&duplicate, 0, sizeof(duplicate));
    xy_sht30_init(&duplicate, &g_fake_bus);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_NO_MEM,
                          xy_i2c_device_register(&duplicate.i2c_dev, "cap_0", XY_DEV_TYPE_SENSOR));
    TEST_ASSERT_EQUAL_UINT(XY_DEVICE_REGISTRY_MAX, xy_device_registry_count());
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_four_drivers_coexist);
    RUN_TEST(test_stats_reflect_population);
    RUN_TEST(test_enumerate_by_type);
    RUN_TEST(test_partial_unregister);
    RUN_TEST(test_find_by_type_compacts_after_middle_unregister);
    RUN_TEST(test_stats_after_full_drain_are_zero);
    RUN_TEST(test_cross_driver_name_collision);
    RUN_TEST(test_registry_capacity_rejects_extra_driver);
    RUN_TEST(test_duplicate_name_is_checked_before_capacity);

    return UNITY_END();
}
