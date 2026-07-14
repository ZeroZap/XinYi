/**
 * @file test_sht30_integration.c
 * @brief Real driver (SHT30) end-to-end test through the device framework.
 *
 * Verifies a concrete driver — not just synthetic xy_i2c_device_t fixtures —
 * can be initialised, registered, found by name, and unregistered. PC's
 * I2C HAL is a stub, so the read path is exercised but the temperature
 * value is not asserted; what we validate is framework integration.
 */

#include <string.h>

#include "unity.h"
#include "xy_device.h"
#include "xy_device_core.h"
#include "xy_sht30.h"

static int g_fake_i2c_bus = 1;

void setUp(void)
{
}

void tearDown(void)
{
}

/* Drain the registry between tests by repeatedly unregistering the head
 * entry. Keeps each test independent of prior state. */
static void drain_registry(void)
{
    while (xy_device_registry_count() > 0) {
        size_t n = 0;
        const xy_device_registry_entry_t *entries = xy_device_get_registry(&n);
        if (n == 0) break;
        xy_device_registry_unregister(entries[0].device);
    }
}

static void test_sht30_init_and_read_reject_null_inputs(void)
{
    xy_device_registry_init();
    drain_registry();

    xy_sht30_t sht;
    memset(&sht, 0, sizeof(sht));

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sht30_init(NULL, &g_fake_i2c_bus));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sht30_init(&sht, NULL));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, xy_sht30_read(NULL));
    TEST_ASSERT_FALSE(sht.i2c_dev.base.initialized);
    TEST_ASSERT_EQUAL_UINT(0U, xy_device_registry_count());
}

static void test_init_registers_nothing_by_default(void)
{
    xy_device_registry_init();
    drain_registry();

    xy_sht30_t sht;
    memset(&sht, 0, sizeof(sht));
    xy_sht30_init(&sht, &g_fake_i2c_bus);

    TEST_ASSERT_TRUE(sht.i2c_dev.base.initialized);
    TEST_ASSERT_NULL(sht.i2c_dev.base.name);
    TEST_ASSERT_EQUAL_UINT(0U, xy_device_registry_count());
}

static void test_register_exposes_through_framework(void)
{
    drain_registry();

    static xy_sht30_t sht;
    memset(&sht, 0, sizeof(sht));
    xy_sht30_init(&sht, &g_fake_i2c_bus);

    int ret = xy_i2c_device_register(&sht.i2c_dev, "sht30_top", XY_DEV_TYPE_SENSOR);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, ret);

    xy_device_t *found = xy_device_find("sht30_top");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_PTR(&sht.i2c_dev.base, found);
    TEST_ASSERT_EQUAL(XY_DEV_TYPE_SENSOR, found->type);
}

static void test_register_rejects_bad_inputs_and_preserves_registry(void)
{
    drain_registry();

    static xy_sht30_t sht;
    xy_i2c_device_t uninitialized;
    memset(&sht, 0, sizeof(sht));
    memset(&uninitialized, 0, sizeof(uninitialized));
    xy_sht30_init(&sht, &g_fake_i2c_bus);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_i2c_device_register(NULL, "null", XY_DEV_TYPE_SENSOR));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_i2c_device_register(&sht.i2c_dev, NULL, XY_DEV_TYPE_SENSOR));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM,
                          xy_i2c_device_register(&uninitialized, "ghost", XY_DEV_TYPE_SENSOR));
    TEST_ASSERT_EQUAL_UINT(0U, xy_device_registry_count());
    TEST_ASSERT_NULL(xy_device_find("ghost"));
}

static void test_unregister_allows_name_reuse(void)
{
    drain_registry();

    static xy_sht30_t first;
    static xy_sht30_t second;
    memset(&first, 0, sizeof(first));
    memset(&second, 0, sizeof(second));
    xy_sht30_init(&first, &g_fake_i2c_bus);
    xy_sht30_init(&second, &g_fake_i2c_bus);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_i2c_device_register(&first.i2c_dev, "sht30_reuse", XY_DEV_TYPE_SENSOR));
    TEST_ASSERT_EQUAL_PTR(&first.i2c_dev.base, xy_device_find("sht30_reuse"));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_registry_unregister(&first.i2c_dev.base));
    TEST_ASSERT_NULL(xy_device_find("sht30_reuse"));

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_i2c_device_register(&second.i2c_dev, "sht30_reuse", XY_DEV_TYPE_SENSOR));
    TEST_ASSERT_EQUAL_PTR(&second.i2c_dev.base, xy_device_find("sht30_reuse"));
}

static void test_multiple_sht30_instances(void)
{
    drain_registry();

    static xy_sht30_t a, b;
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));

    xy_sht30_init(&a, &g_fake_i2c_bus);
    xy_sht30_init(&b, &g_fake_i2c_bus);
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_i2c_device_register(&a.i2c_dev, "sht30_a", XY_DEV_TYPE_SENSOR));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK,
                          xy_i2c_device_register(&b.i2c_dev, "sht30_b", XY_DEV_TYPE_SENSOR));

    TEST_ASSERT_EQUAL_UINT(2U, xy_device_registry_count());
    TEST_ASSERT_EQUAL_PTR(&a.i2c_dev.base, xy_device_find("sht30_a"));
    TEST_ASSERT_EQUAL_PTR(&b.i2c_dev.base, xy_device_find("sht30_b"));

    xy_device_stats_t stats;
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, xy_device_get_stats(&stats));
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(2U, stats.sensor_count);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_sht30_init_and_read_reject_null_inputs);
    RUN_TEST(test_init_registers_nothing_by_default);
    RUN_TEST(test_register_exposes_through_framework);
    RUN_TEST(test_register_rejects_bad_inputs_and_preserves_registry);
    RUN_TEST(test_unregister_allows_name_reuse);
    RUN_TEST(test_multiple_sht30_instances);

    return UNITY_END();
}
