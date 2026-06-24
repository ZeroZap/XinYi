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

/* ---------- xy_spi_device_init basic behaviour ---------- */

static void test_spi_init_sets_defaults(void)
{
    xy_device_registry_init();
    drain_registry();

    xy_spi_device_t dev;
    memset(&dev, 0, sizeof(dev));

    ASSERT(xy_spi_device_init(&dev, &g_fake_spi_bus, &g_fake_cs_pin,
                              1000000, 0) == XY_DEVICE_OK);

    ASSERT(dev.base.type == XY_DEVICE_TYPE_SPI);
    ASSERT(dev.base.initialized);
    ASSERT(dev.base.name == NULL);  /* init does not assign a default name */
    ASSERT(dev.spi_handle == &g_fake_spi_bus);
    ASSERT(dev.cs_pin == &g_fake_cs_pin);
    ASSERT(dev.speed == 1000000);
}

static void test_spi_init_speed_default(void)
{
    xy_spi_device_t dev;
    memset(&dev, 0, sizeof(dev));

    /* speed=0 should fall back to 1 MHz default. */
    ASSERT(xy_spi_device_init(&dev, &g_fake_spi_bus, NULL, 0, 3) == XY_DEVICE_OK);
    ASSERT(dev.speed == 1000000);
    ASSERT(dev.mode == 3);
    ASSERT(dev.cs_pin == NULL);
}

static void test_spi_init_rejects_null(void)
{
    xy_spi_device_t dev;
    memset(&dev, 0, sizeof(dev));

    ASSERT(xy_spi_device_init(NULL, &g_fake_spi_bus, NULL, 0, 0) != XY_DEVICE_OK);
    ASSERT(xy_spi_device_init(&dev, NULL, NULL, 0, 0) != XY_DEVICE_OK);
}

/* ---------- xy_spi_device_register opt-in registration ---------- */

static void test_spi_register_exposes_through_framework(void)
{
    drain_registry();

    static xy_spi_device_t dev;
    memset(&dev, 0, sizeof(dev));
    xy_spi_device_init(&dev, &g_fake_spi_bus, &g_fake_cs_pin, 8000000, 0);

    ASSERT(xy_spi_device_register(&dev, "flash0", XY_DEV_TYPE_STORAGE) == XY_DEVICE_OK);

    xy_device_t *found = xy_device_find("flash0");
    ASSERT(found == &dev.base);
    ASSERT(found->type == XY_DEV_TYPE_STORAGE);
}

static void test_spi_register_rejects_uninitialised(void)
{
    drain_registry();

    xy_spi_device_t dev;
    memset(&dev, 0, sizeof(dev));  /* init NOT called -> base.initialized = 0 */

    ASSERT(xy_spi_device_register(&dev, "ghost", XY_DEV_TYPE_SPI) != XY_DEVICE_OK);
    ASSERT(xy_device_find("ghost") == NULL);
}

static void test_spi_register_rejects_null_name(void)
{
    drain_registry();

    static xy_spi_device_t dev;
    memset(&dev, 0, sizeof(dev));
    xy_spi_device_init(&dev, &g_fake_spi_bus, NULL, 0, 0);

    ASSERT(xy_spi_device_register(&dev, NULL, XY_DEV_TYPE_SPI) != XY_DEVICE_OK);
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

    ASSERT(xy_i2c_device_register(&i2c_a, "sensor_a", XY_DEV_TYPE_SENSOR) == XY_DEVICE_OK);
    ASSERT(xy_spi_device_register(&spi_a, "flash_a", XY_DEV_TYPE_STORAGE) == XY_DEVICE_OK);
    ASSERT(xy_i2c_device_register(&i2c_b, "sensor_b", XY_DEV_TYPE_SENSOR) == XY_DEVICE_OK);
    ASSERT(xy_spi_device_register(&spi_b, "flash_b", XY_DEV_TYPE_STORAGE) == XY_DEVICE_OK);

    ASSERT(xy_device_registry_count() == 4);

    /* Each device routes back to the right base pointer regardless of bus type. */
    ASSERT(xy_device_find("sensor_a") == &i2c_a.base);
    ASSERT(xy_device_find("sensor_b") == &i2c_b.base);
    ASSERT(xy_device_find("flash_a")  == &spi_a.base);
    ASSERT(xy_device_find("flash_b")  == &spi_b.base);

    /* find_by_type enumerates within a type independent of bus underneath. */
    ASSERT(xy_device_find_by_type(XY_DEV_TYPE_SENSOR,  0) != NULL);
    ASSERT(xy_device_find_by_type(XY_DEV_TYPE_SENSOR,  1) != NULL);
    ASSERT(xy_device_find_by_type(XY_DEV_TYPE_SENSOR,  2) == NULL);
    ASSERT(xy_device_find_by_type(XY_DEV_TYPE_STORAGE, 0) != NULL);
    ASSERT(xy_device_find_by_type(XY_DEV_TYPE_STORAGE, 1) != NULL);
    ASSERT(xy_device_find_by_type(XY_DEV_TYPE_STORAGE, 2) == NULL);
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

    ASSERT(xy_i2c_device_register(&i2c, "bus_dev", XY_DEV_TYPE_SENSOR) == XY_DEVICE_OK);
    /* Same name from SPI side must be rejected. */
    ASSERT(xy_spi_device_register(&spi, "bus_dev", XY_DEV_TYPE_STORAGE) != XY_DEVICE_OK);
    /* The original I2C device must still be findable and intact. */
    ASSERT(xy_device_find("bus_dev") == &i2c.base);
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
