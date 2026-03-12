/**
 * @file test_device_core.c
 * @brief Device Core Framework Unit Tests (No HAL dependency)
 * @version 1.0.0
 * @date 2026-03-13
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Minimal device types for testing */
typedef enum {
    XY_DEVICE_TYPE_NONE = 0,
    XY_DEVICE_TYPE_I2C,
    XY_DEVICE_TYPE_SENSOR,
} xy_device_type_t;

typedef struct {
    const char *name;
    xy_device_type_t type;
    void *bus;
    uint16_t address;
    int initialized;
    void *user_data;
} xy_device_t;

/* Forward declarations from xy_device_core.c */
extern int xy_device_registry_init(void);
extern int xy_device_registry_register(xy_device_t *dev);
extern xy_device_t *xy_device_find_by_name(const char *name);
extern xy_device_t *xy_device_find_by_type(xy_device_type_t type, size_t index);
extern size_t xy_device_get_count(void);

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) static void name(void)
#define RUN_TEST(name) do { \
    printf("Running %s... ", #name); \
    tests_run++; \
    name(); \
    tests_passed++; \
    printf("PASSED\n"); \
} while(0)

#define ASSERT(cond) do { if (!(cond)) { \
    printf("FAILED: %s:%d - %s\n", __FILE__, __LINE__, #cond); \
    exit(1); \
} } while(0)

TEST(test_registry_init)
{
    int ret = xy_device_registry_init();
    ASSERT(ret == 0);
    ASSERT(xy_device_get_count() == 0);
}

TEST(test_device_register)
{
    static xy_device_t test_dev;
    
    memset(&test_dev, 0, sizeof(test_dev));
    test_dev.name = "test_sensor";
    test_dev.type = XY_DEVICE_TYPE_SENSOR;
    test_dev.initialized = 1;
    
    int ret = xy_device_registry_register(&test_dev);
    ASSERT(ret == 0);
    ASSERT(xy_device_get_count() == 1);
}

TEST(test_find_by_name)
{
    xy_device_t *dev = xy_device_find_by_name("test_sensor");
    ASSERT(dev != NULL);
    ASSERT(strcmp(dev->name, "test_sensor") == 0);
}

TEST(test_find_by_type)
{
    xy_device_t *dev = xy_device_find_by_type(XY_DEVICE_TYPE_SENSOR, 0);
    ASSERT(dev != NULL);
    ASSERT(dev->type == XY_DEVICE_TYPE_SENSOR);
}

TEST(test_duplicate_name)
{
    static xy_device_t test_dev2;
    
    memset(&test_dev2, 0, sizeof(test_dev2));
    test_dev2.name = "test_sensor";  /* Same name */
    test_dev2.type = XY_DEVICE_TYPE_SENSOR;
    
    int ret = xy_device_registry_register(&test_dev2);
    ASSERT(ret != 0);  /* Should fail */
}

int main(void)
{
    printf("=== Device Core Unit Tests ===\n\n");
    
    RUN_TEST(test_registry_init);
    RUN_TEST(test_device_register);
    RUN_TEST(test_find_by_name);
    RUN_TEST(test_find_by_type);
    RUN_TEST(test_duplicate_name);
    
    printf("\n=== Test Summary ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    
    if (tests_passed == tests_run) {
        printf("\n✓ All tests passed!\n");
        return 0;
    } else {
        printf("\n✗ Some tests failed!\n");
        return 1;
    }
}
