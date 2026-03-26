#include "xy_hal_gpio.h"
#include "xy_hal_error.h"
#include "xy_hal_pc.h"  // Include PC-specific header
#include "../../../third_party/unity/unity.h"

// Global simulated GPIO port for testing
static struct xy_hal_gpio_port g_test_gpio_port = {0};

void setUp(void) {
    // Initialize the test GPIO port
    g_test_gpio_port.port_id = 0;
    g_test_gpio_port.pin_mask = (1 << 5);
    g_test_gpio_port.direction = 1; // Output
    g_test_gpio_port.pull = 0;      // No pull
}

void tearDown(void) {
    // Cleanup if needed
}

void test_gpio_init(void) {
    xy_hal_gpio_config_t config = {0};
    xy_hal_error_t result = xy_hal_gpio_init(&g_test_gpio_port, 5, &config);
    TEST_ASSERT_EQUAL(XY_HAL_OK, result);
}

void test_gpio_write(void) {
    xy_hal_error_t result = xy_hal_gpio_write(&g_test_gpio_port, 5, 1);
    TEST_ASSERT_EQUAL(XY_HAL_OK, result);
}

void test_gpio_read(void) {
    int32_t value = xy_hal_gpio_read(&g_test_gpio_port, 5);
    TEST_ASSERT_TRUE(value >= 0); // Should return non-negative value
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_gpio_init);
    RUN_TEST(test_gpio_write);
    RUN_TEST(test_gpio_read);
    return UNITY_END();
}