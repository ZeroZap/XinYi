/**
 * @file test_hal_gpio.c
 * @brief HAL GPIO Unit Tests using Unity Framework
 * @version 1.0.0
 * @date 2026-03-01
 */

#include <stdint.h>
#include <string.h>
#include "unity.h"

/* HAL GPIO header */
#include "xy_hal_gpio.h"

/* ==================== Test Fixtures ==================== */

static xy_hal_gpio_config_t test_config;

void setUp(void)
{
    /* Initialize test configuration */
    test_config.mode = XY_HAL_GPIO_MODE_INPUT;
    test_config.pull = XY_HAL_GPIO_PULL_NONE;
    test_config.otype = XY_HAL_GPIO_OTYPE_PP;
    test_config.speed = XY_HAL_GPIO_SPEED_LOW;
    test_config.alternate = 0;
}

void tearDown(void)
{
    /* Cleanup after each test */
}

/* ==================== GPIO Mode Tests ==================== */

void test_gpio_mode_values(void)
{
    /* Test GPIO mode enum values */
    TEST_ASSERT_EQUAL(0, XY_HAL_GPIO_MODE_INPUT);
    TEST_ASSERT_EQUAL(1, XY_HAL_GPIO_MODE_OUTPUT);
    TEST_ASSERT_EQUAL(2, XY_HAL_GPIO_MODE_AF);
    TEST_ASSERT_EQUAL(3, XY_HAL_GPIO_MODE_ANALOG);
}

void test_gpio_pull_values(void)
{
    /* Test GPIO pull enum values */
    TEST_ASSERT_EQUAL(0, XY_HAL_GPIO_PULL_NONE);
    TEST_ASSERT_EQUAL(1, XY_HAL_GPIO_PULL_UP);
    TEST_ASSERT_EQUAL(2, XY_HAL_GPIO_PULL_DOWN);
}

void test_gpio_otype_values(void)
{
    /* Test GPIO output type enum values */
    TEST_ASSERT_EQUAL(0, XY_HAL_GPIO_OTYPE_PP);
    TEST_ASSERT_EQUAL(1, XY_HAL_GPIO_OTYPE_OD);
}

void test_gpio_speed_values(void)
{
    /* Test GPIO speed enum values */
    TEST_ASSERT_EQUAL(0, XY_HAL_GPIO_SPEED_LOW);
    TEST_ASSERT_EQUAL(1, XY_HAL_GPIO_SPEED_MEDIUM);
    TEST_ASSERT_EQUAL(2, XY_HAL_GPIO_SPEED_HIGH);
    TEST_ASSERT_EQUAL(3, XY_HAL_GPIO_SPEED_VERY_HIGH);
}

void test_gpio_state_values(void)
{
    /* Test GPIO state enum values */
    TEST_ASSERT_EQUAL(0, XY_HAL_GPIO_LOW);
    TEST_ASSERT_EQUAL(1, XY_HAL_GPIO_HIGH);
}

/* ==================== GPIO Config Tests ==================== */

void test_gpio_config_structure_size(void)
{
    /* Test configuration structure size */
    TEST_ASSERT_TRUE(sizeof(xy_hal_gpio_config_t) >= 6);
}

void test_gpio_config_initialization(void)
{
    xy_hal_gpio_config_t config;
    memset(&config, 0, sizeof(config));

    TEST_ASSERT_EQUAL(XY_HAL_GPIO_MODE_INPUT, config.mode);
    TEST_ASSERT_EQUAL(XY_HAL_GPIO_PULL_NONE, config.pull);
    TEST_ASSERT_EQUAL(XY_HAL_GPIO_OTYPE_PP, config.otype);
    TEST_ASSERT_EQUAL(XY_HAL_GPIO_SPEED_LOW, config.speed);
    TEST_ASSERT_EQUAL(0, config.alternate);
}

void test_gpio_config_output_mode(void)
{
    xy_hal_gpio_config_t config;
    config.mode = XY_HAL_GPIO_MODE_OUTPUT;
    config.pull = XY_HAL_GPIO_PULL_UP;
    config.otype = XY_HAL_GPIO_OTYPE_OD;
    config.speed = XY_HAL_GPIO_SPEED_HIGH;
    config.alternate = 0;

    TEST_ASSERT_EQUAL(XY_HAL_GPIO_MODE_OUTPUT, config.mode);
    TEST_ASSERT_EQUAL(XY_HAL_GPIO_PULL_UP, config.pull);
    TEST_ASSERT_EQUAL(XY_HAL_GPIO_OTYPE_OD, config.otype);
    TEST_ASSERT_EQUAL(XY_HAL_GPIO_SPEED_HIGH, config.speed);
}

void test_gpio_config_alternate_function(void)
{
    xy_hal_gpio_config_t config;
    config.mode = XY_HAL_GPIO_MODE_AF;
    config.alternate = 5;  /* AF5 */

    TEST_ASSERT_EQUAL(XY_HAL_GPIO_MODE_AF, config.mode);
    TEST_ASSERT_EQUAL(5, config.alternate);
}

/* ==================== GPIO Port Tests ==================== */

void test_gpio_port_structure_size(void)
{
    /* Test port structure size */
    TEST_ASSERT_TRUE(sizeof(xy_hal_gpio_port_t) >= (sizeof(void*) + 2));
}

void test_gpio_port_initialization(void)
{
    xy_hal_gpio_port_t port;
    memset(&port, 0, sizeof(port));

    TEST_ASSERT_EQUAL_PTR(NULL, port.port_base);
    TEST_ASSERT_EQUAL(0, port.port_id);
    TEST_ASSERT_EQUAL(0, port.initialized);
}

void test_gpio_port_with_data(void)
{
    xy_hal_gpio_port_t port;
    int dummy_base = 0x40020000;

    port.port_base = &dummy_base;
    port.port_id = 1;
    port.initialized = 1;

    TEST_ASSERT_EQUAL_PTR(&dummy_base, port.port_base);
    TEST_ASSERT_EQUAL(1, port.port_id);
    TEST_ASSERT_EQUAL(1, port.initialized);
}

/* ==================== GPIO Function Tests ==================== */

void test_gpio_init_null_param(void)
{
    /* Test xy_hal_gpio_init with NULL port */
    xy_hal_error_t ret = xy_hal_gpio_init(NULL, 5, &test_config);
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, ret);
}

void test_gpio_init_null_config(void)
{
    /* Test xy_hal_gpio_init with NULL config */
    xy_hal_gpio_port_t port;
    xy_hal_error_t ret = xy_hal_gpio_init(&port, 5, NULL);
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, ret);
}

void test_gpio_deinit_null_param(void)
{
    /* Test xy_hal_gpio_deinit with NULL port */
    xy_hal_error_t ret = xy_hal_gpio_deinit(NULL, 5);
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, ret);
}

void test_gpio_read_null_param(void)
{
    /* Test xy_hal_gpio_read with NULL port */
    xy_hal_gpio_state_t state;
    xy_hal_error_t ret = xy_hal_gpio_read(NULL, 5, &state);
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, ret);
}

void test_gpio_read_null_state(void)
{
    /* Test xy_hal_gpio_read with NULL state */
    xy_hal_gpio_port_t port;
    xy_hal_error_t ret = xy_hal_gpio_read(&port, 5, NULL);
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, ret);
}

void test_gpio_write_null_param(void)
{
    /* Test xy_hal_gpio_write with NULL port */
    xy_hal_error_t ret = xy_hal_gpio_write(NULL, 5, XY_HAL_GPIO_HIGH);
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, ret);
}

void test_gpio_toggle_null_param(void)
{
    /* Test xy_hal_gpio_toggle with NULL port */
    xy_hal_error_t ret = xy_hal_gpio_toggle(NULL, 5);
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, ret);
}

/* ==================== Main ==================== */

int main(void)
{
    UNITY_BEGIN();

    /* GPIO Mode Tests */
    RUN_TEST(test_gpio_mode_values);
    RUN_TEST(test_gpio_pull_values);
    RUN_TEST(test_gpio_otype_values);
    RUN_TEST(test_gpio_speed_values);
    RUN_TEST(test_gpio_state_values);

    /* GPIO Config Tests */
    RUN_TEST(test_gpio_config_structure_size);
    RUN_TEST(test_gpio_config_initialization);
    RUN_TEST(test_gpio_config_output_mode);
    RUN_TEST(test_gpio_config_alternate_function);

    /* GPIO Port Tests */
    RUN_TEST(test_gpio_port_structure_size);
    RUN_TEST(test_gpio_port_initialization);
    RUN_TEST(test_gpio_port_with_data);

    /* GPIO Function Tests */
    RUN_TEST(test_gpio_init_null_param);
    RUN_TEST(test_gpio_init_null_config);
    RUN_TEST(test_gpio_deinit_null_param);
    RUN_TEST(test_gpio_read_null_param);
    RUN_TEST(test_gpio_read_null_state);
    RUN_TEST(test_gpio_write_null_param);
    RUN_TEST(test_gpio_toggle_null_param);

    return UNITY_END();
}
