#include "xy_hal_error.h"
#include "xy_hal_gpio.h"
#include "xy_hal_pc.h"
#include "unity.h"

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_pc_gpio_simulation_smoke(void)
{
    struct xy_hal_gpio_port gpio_port = {0};
    gpio_port.port_id = 0;
    gpio_port.pin_mask = (1 << 5);
    gpio_port.direction = 1;
    gpio_port.pull = 0;

    xy_hal_gpio_port_t port = &gpio_port;
    xy_hal_gpio_config_t config = {0};

    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_gpio_init(port, 5, &config));
    TEST_ASSERT_EQUAL(XY_HAL_OK, xy_hal_gpio_write(port, 5, 1));
    TEST_ASSERT_EQUAL_INT32(1, xy_hal_gpio_read(port, 5));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_pc_gpio_simulation_smoke);
    return UNITY_END();
}
