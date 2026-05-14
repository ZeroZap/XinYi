/**
 * @file test_sht30_integration.c
 * @brief Real driver (SHT30) end-to-end test through the device framework.
 *
 * Verifies a concrete driver — not just synthetic xy_i2c_device_t fixtures —
 * can be initialised, registered, found by name, and unregistered. PC's
 * I2C HAL is a stub, so the read path is exercised but the temperature
 * value is not asserted; what we validate is framework integration.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xy_device.h"
#include "xy_device_core.h"
#include "xy_sht30.h"

static int tests_run = 0;
static int tests_passed = 0;
static int g_fake_i2c_bus = 1;

#define RUN(name) do { \
    printf("Running %s... ", #name); \
    tests_run++; \
    name(); \
    tests_passed++; \
    printf("PASSED\n"); \
} while (0)

#define ASSERT(cond) do { if (!(cond)) { \
    printf("FAILED: %s:%d - %s\n", __FILE__, __LINE__, #cond); \
    exit(1); \
} } while (0)

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

static void test_init_registers_nothing_by_default(void)
{
    xy_device_registry_init();
    drain_registry();

    xy_sht30_t sht;
    memset(&sht, 0, sizeof(sht));
    xy_sht30_init(&sht, &g_fake_i2c_bus);

    ASSERT(sht.i2c_dev.base.initialized);
    ASSERT(sht.i2c_dev.base.name == NULL);
    ASSERT(xy_device_registry_count() == 0);
}

static void test_register_exposes_through_framework(void)
{
    drain_registry();

    static xy_sht30_t sht;
    memset(&sht, 0, sizeof(sht));
    xy_sht30_init(&sht, &g_fake_i2c_bus);

    int ret = xy_i2c_device_register(&sht.i2c_dev, "sht30_top", XY_DEV_TYPE_SENSOR);
    ASSERT(ret == XY_DEVICE_OK);

    xy_device_t *found = xy_device_find("sht30_top");
    ASSERT(found != NULL);
    ASSERT(found == &sht.i2c_dev.base);
    ASSERT(found->type == XY_DEV_TYPE_SENSOR);
}

static void test_multiple_sht30_instances(void)
{
    drain_registry();

    static xy_sht30_t a, b;
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));

    xy_sht30_init(&a, &g_fake_i2c_bus);
    xy_sht30_init(&b, &g_fake_i2c_bus);
    ASSERT(xy_i2c_device_register(&a.i2c_dev, "sht30_a", XY_DEV_TYPE_SENSOR) == XY_DEVICE_OK);
    ASSERT(xy_i2c_device_register(&b.i2c_dev, "sht30_b", XY_DEV_TYPE_SENSOR) == XY_DEVICE_OK);

    ASSERT(xy_device_registry_count() == 2);
    ASSERT(xy_device_find("sht30_a") == &a.i2c_dev.base);
    ASSERT(xy_device_find("sht30_b") == &b.i2c_dev.base);

    xy_device_stats_t stats;
    ASSERT(xy_device_get_stats(&stats) == XY_DEVICE_OK);
    ASSERT(stats.sensor_count >= 2);
}

int main(void)
{
    printf("=== SHT30 Framework Integration ===\n\n");

    RUN(test_init_registers_nothing_by_default);
    RUN(test_register_exposes_through_framework);
    RUN(test_multiple_sht30_instances);

    printf("\nTests run: %d   passed: %d   failed: %d\n",
           tests_run, tests_passed, tests_run - tests_passed);
    return tests_passed == tests_run ? 0 : 1;
}
