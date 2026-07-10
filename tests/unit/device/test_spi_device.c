/**
 * @file test_spi_device.c
 * @brief SPI device helper + heterogeneous bus coexistence tests.
 *
 * Exercises xy_spi_device_init / xy_spi_device_register — the SPI parallel
 * to the I2C helpers covered by test_device.c. Also verifies that I2C and
 * SPI devices share a single registry without collision.
 */

#include <string.h>

#include "unity.h"
#include "xy_device.h"
#include "xy_device_core.h"

static int g_fake_spi_bus = 1;
static int g_fake_i2c_bus = 2;
static int g_fake_cs_pin  = 3;

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

/* ---------- xy_spi_device_init basic behaviour ---------- */

static void test_spi_init_sets_defaults(void)
{
    xy_device_registry_init();
    drain_registry();

    xy_spi_device_t dev;
    memset(&dev, 0, sizeof(dev));

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_spi_device_init(&dev, &g_fake_spi_bus, &g_fake_cs_pin, 1000000, 0));

    TEST_ASSERT_EQUAL(XY_DEVICE_TYPE_SPI, dev.base.type);
    TEST_ASSERT_TRUE(dev.base.initialized);
    TEST_ASSERT_NULL(dev.base.name);  /* init does not assign a default name */
    TEST_ASSERT_EQUAL_PTR(&g_fake_spi_bus, dev.spi_handle);
    TEST_ASSERT_EQUAL_PTR(&g_fake_cs_pin, dev.cs_pin);
    TEST_ASSERT_EQUAL_UINT32(1000000U, dev.speed);
}

static void test_spi_init_speed_default(void)
{
    xy_spi_device_t dev;
    memset(&dev, 0, sizeof(dev));

    /* speed=0 should fall back to 1 MHz default. */
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_spi_device_init(&dev, &g_fake_spi_bus, NULL, 0, 3));
    TEST_ASSERT_EQUAL_UINT32(1000000U, dev.speed);
    TEST_ASSERT_EQUAL_UINT8(3U, dev.mode);
    TEST_ASSERT_NULL(dev.cs_pin);
}

static void test_spi_init_rejects_null(void)
{
    xy_spi_device_t dev;
    memset(&dev, 0, sizeof(dev));

    TEST_ASSERT_NOT_EQUAL(XY_DEVICE_OK, xy_spi_device_init(NULL, &g_fake_spi_bus, NULL, 0, 0));
    TEST_ASSERT_NOT_EQUAL(XY_DEVICE_OK, xy_spi_device_init(&dev, NULL, NULL, 0, 0));
}

/* ---------- xy_spi_device_register opt-in registration ---------- */

static void test_spi_register_exposes_through_framework(void)
{
    drain_registry();

    static xy_spi_device_t dev;
    memset(&dev, 0, sizeof(dev));
    xy_spi_device_init(&dev, &g_fake_spi_bus, &g_fake_cs_pin, 8000000, 0);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_spi_device_register(&dev, "flash0", XY_DEV_TYPE_STORAGE));

    xy_device_t *found = xy_device_find("flash0");
    TEST_ASSERT_EQUAL_PTR(&dev.base, found);
    TEST_ASSERT_EQUAL(XY_DEV_TYPE_STORAGE, found->type);
}

static void test_spi_register_rejects_uninitialised(void)
{
    drain_registry();

    xy_spi_device_t dev;
    memset(&dev, 0, sizeof(dev));  /* init NOT called -> base.initialized = 0 */

    TEST_ASSERT_NOT_EQUAL(XY_DEVICE_OK, xy_spi_device_register(&dev, "ghost", XY_DEV_TYPE_SPI));
    TEST_ASSERT_NULL(xy_device_find("ghost"));
}

static void test_spi_register_rejects_null_name(void)
{
    drain_registry();

    static xy_spi_device_t dev;
    memset(&dev, 0, sizeof(dev));
    xy_spi_device_init(&dev, &g_fake_spi_bus, NULL, 0, 0);

    TEST_ASSERT_NOT_EQUAL(XY_DEVICE_OK, xy_spi_device_register(&dev, NULL, XY_DEV_TYPE_SPI));
}

/* ---------- Mixed I2C + SPI in one registry ---------- */

static void test_i2c_and_spi_coexist(void)
{
    drain_registry();

    static xy_i2c_device_t i2c_a, i2c_b;
    static xy_spi_device_t spi_a, spi_b;
    memset(&i2c_a, 0, sizeof(i2c_a));
    memset(&i2c_b, 0, sizeof(i2c_b));
    memset(&spi_a, 0, sizeof(spi_a));
    memset(&spi_b, 0, sizeof(spi_b));

    xy_i2c_device_init(&i2c_a, &g_fake_i2c_bus, 0x44, 0);
    xy_i2c_device_init(&i2c_b, &g_fake_i2c_bus, 0x48, 0);
    xy_spi_device_init(&spi_a, &g_fake_spi_bus, &g_fake_cs_pin, 0, 0);
    xy_spi_device_init(&spi_b, &g_fake_spi_bus, &g_fake_cs_pin, 0, 0);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_i2c_device_register(&i2c_a, "sensor_a", XY_DEV_TYPE_SENSOR));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_spi_device_register(&spi_a, "flash_a", XY_DEV_TYPE_STORAGE));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_i2c_device_register(&i2c_b, "sensor_b", XY_DEV_TYPE_SENSOR));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_spi_device_register(&spi_b, "flash_b", XY_DEV_TYPE_STORAGE));

    TEST_ASSERT_EQUAL_UINT(4U, xy_device_registry_count());

    /* Each device routes back to the right base pointer regardless of bus type. */
    TEST_ASSERT_EQUAL_PTR(&i2c_a.base, xy_device_find("sensor_a"));
    TEST_ASSERT_EQUAL_PTR(&i2c_b.base, xy_device_find("sensor_b"));
    TEST_ASSERT_EQUAL_PTR(&spi_a.base, xy_device_find("flash_a"));
    TEST_ASSERT_EQUAL_PTR(&spi_b.base, xy_device_find("flash_b"));

    /* find_by_type enumerates within a type independent of bus underneath. */
    TEST_ASSERT_NOT_NULL(xy_device_find_by_type(XY_DEV_TYPE_SENSOR, 0));
    TEST_ASSERT_NOT_NULL(xy_device_find_by_type(XY_DEV_TYPE_SENSOR, 1));
    TEST_ASSERT_NULL(xy_device_find_by_type(XY_DEV_TYPE_SENSOR, 2));
    TEST_ASSERT_NOT_NULL(xy_device_find_by_type(XY_DEV_TYPE_STORAGE, 0));
    TEST_ASSERT_NOT_NULL(xy_device_find_by_type(XY_DEV_TYPE_STORAGE, 1));
    TEST_ASSERT_NULL(xy_device_find_by_type(XY_DEV_TYPE_STORAGE, 2));
}

/* SPI registering a name an I2C device already owns must fail. */
static void test_cross_bus_name_collision(void)
{
    static xy_i2c_device_t i2c;
    static xy_spi_device_t spi;
    drain_registry();
    memset(&i2c, 0, sizeof(i2c));
    memset(&spi, 0, sizeof(spi));

    xy_i2c_device_init(&i2c, &g_fake_i2c_bus, 0x50, 0);
    xy_spi_device_init(&spi, &g_fake_spi_bus, NULL, 0, 0);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_i2c_device_register(&i2c, "bus_dev", XY_DEV_TYPE_SENSOR));
    /* Same name from SPI side must be rejected. */
    TEST_ASSERT_NOT_EQUAL(XY_DEVICE_OK, xy_spi_device_register(&spi, "bus_dev", XY_DEV_TYPE_STORAGE));
    /* The original I2C device must still be findable and intact. */
    TEST_ASSERT_EQUAL_PTR(&i2c.base, xy_device_find("bus_dev"));
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_spi_init_sets_defaults);
    RUN_TEST(test_spi_init_speed_default);
    RUN_TEST(test_spi_init_rejects_null);
    RUN_TEST(test_spi_register_exposes_through_framework);
    RUN_TEST(test_spi_register_rejects_uninitialised);
    RUN_TEST(test_spi_register_rejects_null_name);
    RUN_TEST(test_i2c_and_spi_coexist);
    RUN_TEST(test_cross_bus_name_collision);

    return UNITY_END();
}
