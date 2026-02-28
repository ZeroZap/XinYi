/**
 * @file test_hal_uart.c
 * @brief HAL UART Unit Tests using Unity Framework
 * @version 1.0.0
 * @date 2026-03-01
 */

#include <stdint.h>
#include <string.h>
#include "unity.h"

/* HAL UART header */
#include "xy_hal_uart.h"

/* ==================== Test Fixtures ==================== */

static xy_hal_uart_config_t test_config;

void setUp(void)
{
    /* Initialize test configuration */
    test_config.baudrate = 115200;
    test_config.data_bits = 8;
    test_config.stop_bits = 1;
    test_config.parity = XY_HAL_UART_PARITY_NONE;
    test_config.flow_control = XY_HAL_UART_FLOW_CONTROL_NONE;
}

void tearDown(void)
{
    /* Cleanup after each test */
}

/* ==================== UART Config Tests ==================== */

void test_uart_config_structure_size(void)
{
    TEST_ASSERT_TRUE(sizeof(xy_hal_uart_config_t) >= 8);
}

void test_uart_config_initialization(void)
{
    xy_hal_uart_config_t config;
    memset(&config, 0, sizeof(config));

    TEST_ASSERT_EQUAL(0, config.baudrate);
    TEST_ASSERT_EQUAL(0, config.data_bits);
    TEST_ASSERT_EQUAL(0, config.stop_bits);
}

void test_uart_config_standard_baudrate(void)
{
    test_config.baudrate = 9600;
    TEST_ASSERT_EQUAL(9600, test_config.baudrate);

    test_config.baudrate = 115200;
    TEST_ASSERT_EQUAL(115200, test_config.baudrate);

    test_config.baudrate = 921600;
    TEST_ASSERT_EQUAL(921600, test_config.baudrate);
}

void test_uart_config_data_bits(void)
{
    test_config.data_bits = 8;
    TEST_ASSERT_EQUAL(8, test_config.data_bits);

    test_config.data_bits = 9;
    TEST_ASSERT_EQUAL(9, test_config.data_bits);
}

/* ==================== UART Handle Tests ==================== */

void test_uart_handle_structure_size(void)
{
    TEST_ASSERT_TRUE(sizeof(xy_hal_uart_handle_t) >= (sizeof(void*) + 1));
}

void test_uart_handle_initialization(void)
{
    xy_hal_uart_handle_t handle;
    memset(&handle, 0, sizeof(handle));

    TEST_ASSERT_EQUAL_PTR(NULL, handle.instance);
    TEST_ASSERT_EQUAL(0, handle.initialized);
}

/* ==================== UART Function Tests ==================== */

void test_uart_init_null_param(void)
{
    xy_hal_error_t ret = xy_hal_uart_init(NULL, &test_config);
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, ret);
}

void test_uart_init_null_config(void)
{
    xy_hal_uart_handle_t handle;
    xy_hal_error_t ret = xy_hal_uart_init(&handle, NULL);
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, ret);
}

void test_uart_deinit_null_param(void)
{
    xy_hal_error_t ret = xy_hal_uart_deinit(NULL);
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, ret);
}

void test_uart_send_null_param(void)
{
    xy_hal_error_t ret = xy_hal_uart_send(NULL, NULL, 0, 0);
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, ret);
}

void test_uart_receive_null_param(void)
{
    xy_hal_error_t ret = xy_hal_uart_receive(NULL, NULL, 0, 0);
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, ret);
}

/* ==================== Main ==================== */

int main(void)
{
    UNITY_BEGIN();

    /* UART Config Tests */
    RUN_TEST(test_uart_config_structure_size);
    RUN_TEST(test_uart_config_initialization);
    RUN_TEST(test_uart_config_standard_baudrate);
    RUN_TEST(test_uart_config_data_bits);

    /* UART Handle Tests */
    RUN_TEST(test_uart_handle_structure_size);
    RUN_TEST(test_uart_handle_initialization);

    /* UART Function Tests */
    RUN_TEST(test_uart_init_null_param);
    RUN_TEST(test_uart_init_null_config);
    RUN_TEST(test_uart_deinit_null_param);
    RUN_TEST(test_uart_send_null_param);
    RUN_TEST(test_uart_receive_null_param);

    return UNITY_END();
}
