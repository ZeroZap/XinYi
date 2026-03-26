#include "../../../third_party/unity/src/unity.h"
#include "xy_hal_gpio.h"
#include "xy_hal_error.h"

void test_gpio_init(void) {
    // Test GPIO initialization with correct parameters
    xy_hal_gpio_config_t config = {0};
    xy_hal_error_t result = xy_hal_gpio_init((void*)0, 0, &config);
    TEST_ASSERT_EQUAL(XY_HAL_OK, result);
}

void test_gpio_set_pin(void) {
    // Test GPIO pin setting with correct function and parameters
    // Value should be 0 (low) or 1 (high), not XY_HAL_GPIO_H
    xy_hal_error_t result = xy_hal_gpio_write((void*)0, 0, 1); // High level
    TEST_ASSERT_EQUAL(XY_HAL_OK, result);
}

void test_gpio_get_pin(void) {
    // Test GPIO pin reading with correct function 
    // Returns int32_t (0/1), not error code, no pointer parameter
    int32_t value = xy_hal_gpio_read((void*)0, 0);
    TEST_ASSERT_TRUE(value >= 0); // Should return non-negative value on success
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_gpio_init);
    RUN_TEST(test_gpio_set_pin);
    RUN_TEST(test_gpio_get_pin);
    return UNITY_END();
}