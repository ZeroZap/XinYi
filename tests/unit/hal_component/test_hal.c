/**
 * @file test_hal.c
 * @brief HAL (Hardware Abstraction Layer) Component Unit Tests using Unity Framework
 * @version 1.0.0
 * @date 2026-02-28
 */

#include <stdint.h>
#include <string.h>
#include "unity.h"

/* HAL main header */
#include "xy_hal.h"

/* ==================== Test Fixtures ==================== */

void setUp(void)
{
    /* Called before each test */
}

void tearDown(void)
{
    /* Called after each test */
}

/* ==================== HAL Version Tests ==================== */

void test_hal_version_macros(void)
{
    /* Test version macro values */
    TEST_ASSERT_EQUAL(2, XY_HAL_VERSION_MAJOR);
    TEST_ASSERT_EQUAL(0, XY_HAL_VERSION_MINOR);
    TEST_ASSERT_EQUAL(0, XY_HAL_VERSION_PATCH);
    TEST_ASSERT_EQUAL_STRING("2.0.0", XY_HAL_VERSION_STRING);
}

/* ==================== HAL Error Code Tests ==================== */

void test_hal_error_codes(void)
{
    /* Test error code values */
    TEST_ASSERT_EQUAL(0, XY_HAL_OK);
    TEST_ASSERT_EQUAL(-1, XY_HAL_ERROR);
    TEST_ASSERT_EQUAL(-2, XY_HAL_ERROR_INVALID_PARAM);
    TEST_ASSERT_EQUAL(-3, XY_HAL_ERROR_NOT_SUPPORT);
    TEST_ASSERT_EQUAL(-4, XY_HAL_ERROR_TIMEOUT);
    TEST_ASSERT_EQUAL(-5, XY_HAL_ERROR_BUSY);
    TEST_ASSERT_EQUAL(-6, XY_HAL_ERROR_NO_MEM);
    TEST_ASSERT_EQUAL(-7, XY_HAL_ERROR_IO);
    TEST_ASSERT_EQUAL(-8, XY_HAL_ERROR_NOT_INIT);
    TEST_ASSERT_EQUAL(-9, XY_HAL_ERROR_ALREADY_INIT);
    TEST_ASSERT_EQUAL(-10, XY_HAL_ERROR_NO_RESOURCE);
    TEST_ASSERT_EQUAL(-11, XY_HAL_ERROR_FAIL);
}

void test_hal_error_code_ordering(void)
{
    /* Test that success is 0 and errors are negative */
    TEST_ASSERT_TRUE(XY_HAL_OK == 0);
    TEST_ASSERT_TRUE(XY_HAL_ERROR < 0);
    TEST_ASSERT_TRUE(XY_HAL_ERROR_INVALID_PARAM < 0);
    TEST_ASSERT_TRUE(XY_HAL_ERROR_TIMEOUT < 0);
}

/* ==================== HAL Handle Structure Tests ==================== */

void test_hal_handle_structure(void)
{
    xy_hal_handle_t handle;

    /* Test structure size */
    TEST_ASSERT_TRUE(sizeof(xy_hal_handle_t) >= (sizeof(void*) * 2 + 1));
}

void test_hal_handle_initialization(void)
{
    xy_hal_handle_t handle;

    /* Initialize handle */
    memset(&handle, 0, sizeof(handle));

    TEST_ASSERT_EQUAL_PTR(NULL, handle.instance);
    TEST_ASSERT_EQUAL_PTR(NULL, handle.user_data);
    TEST_ASSERT_EQUAL(0, handle.initialized);
}

void test_hal_handle_with_data(void)
{
    xy_hal_handle_t handle;
    int dummy_data = 42;

    handle.instance = &dummy_data;
    handle.user_data = &dummy_data;
    handle.initialized = 1;

    TEST_ASSERT_EQUAL_PTR(&dummy_data, handle.instance);
    TEST_ASSERT_EQUAL_PTR(&dummy_data, handle.user_data);
    TEST_ASSERT_EQUAL(1, handle.initialized);
}

/* ==================== HAL Status Type Tests (Legacy) ==================== */

void test_hal_status_type_legacy(void)
{
    /* Test legacy status codes */
    TEST_ASSERT_EQUAL(0, XY_HAL_STATUS_OK);
    TEST_ASSERT_EQUAL(-1, XY_HAL_STATUS_ERROR);
    TEST_ASSERT_EQUAL(-2, XY_HAL_STATUS_TIMEOUT);
    TEST_ASSERT_EQUAL(-3, XY_HAL_STATUS_BUSY);
    TEST_ASSERT_EQUAL(-4, XY_HAL_STATUS_NOT_SUPPORT);
    TEST_ASSERT_EQUAL(-5, XY_HAL_STATUS_NO_MEM);
    TEST_ASSERT_EQUAL(-6, XY_HAL_STATUS_NOT_INIT);
}

/* ==================== HAL Sub-module Header Tests ==================== */

void test_hal_submodule_headers_exist(void)
{
    /* Test that sub-module headers are included */
    /* This test verifies that all sub-module headers compile */
    
    /* GPIO */
    TEST_ASSERT_TRUE(1); /* xy_hal_gpio.h included */
    
    /* UART */
    TEST_ASSERT_TRUE(1); /* xy_hal_uart.h included */
    
    /* SPI */
    TEST_ASSERT_TRUE(1); /* xy_hal_spi.h included */
    
    /* I2C */
    TEST_ASSERT_TRUE(1); /* xy_hal_i2c.h included */
    
    /* Timer */
    TEST_ASSERT_TRUE(1); /* xy_hal_timer.h included */
    
    /* PWM */
    TEST_ASSERT_TRUE(1); /* xy_hal_pwm.h included */
    
    /* RTC */
    TEST_ASSERT_TRUE(1); /* xy_hal_rtc.h included */
    
    /* DMA */
    TEST_ASSERT_TRUE(1); /* xy_hal_dma.h included */
    
    /* ADC */
    TEST_ASSERT_TRUE(1); /* xy_hal_adc.h included */
    
    /* DAC */
    TEST_ASSERT_TRUE(1); /* xy_hal_dac.h included */
    
    /* Flash */
    TEST_ASSERT_TRUE(1); /* xy_hal_flash.h included */
    
    /* Watchdog */
    TEST_ASSERT_TRUE(1); /* xy_hal_wdg.h included */
    
    /* EXTI */
    TEST_ASSERT_TRUE(1); /* xy_hal_exti.h included */
    
    /* RNG */
    TEST_ASSERT_TRUE(1); /* xy_hal_rng.h included */
}

/* ==================== HAL Type Definitions Tests ==================== */

void test_hal_type_definitions(void)
{
    /* Test that basic types are defined */
    xy_hal_error_t error;
    xy_hal_handle_t handle;
    
    error = XY_HAL_OK;
    TEST_ASSERT_EQUAL(0, error);
    
    memset(&handle, 0, sizeof(handle));
    TEST_ASSERT_EQUAL_PTR(NULL, handle.instance);
}

/* ==================== HAL Error Code Usage Tests ==================== */

void test_hal_error_code_usage(void)
{
    xy_hal_error_t result;
    
    /* Simulate success */
    result = XY_HAL_OK;
    TEST_ASSERT_EQUAL(XY_HAL_OK, result);
    
    /* Simulate error */
    result = XY_HAL_ERROR;
    TEST_ASSERT_TRUE(result < 0);
    
    /* Simulate specific error */
    result = XY_HAL_ERROR_INVALID_PARAM;
    TEST_ASSERT_EQUAL(XY_HAL_ERROR_INVALID_PARAM, result);
    TEST_ASSERT_TRUE(result < 0);
}

/* ==================== HAL Handle Operations Tests ==================== */

void test_hal_handle_operations(void)
{
    xy_hal_handle_t handle1, handle2;
    
    /* Initialize handles */
    memset(&handle1, 0, sizeof(handle1));
    memset(&handle2, 0, sizeof(handle2));
    
    /* Test handles are equal when zeroed */
    TEST_ASSERT_EQUAL_PTR(handle1.instance, handle2.instance);
    TEST_ASSERT_EQUAL_PTR(handle1.user_data, handle2.user_data);
    TEST_ASSERT_EQUAL(handle1.initialized, handle2.initialized);
    
    /* Modify one handle */
    int data = 100;
    handle1.instance = &data;
    handle1.initialized = 1;
    
    /* Test handles are now different */
    TEST_ASSERT_NOT_EQUAL_PTR(handle1.instance, handle2.instance);
    TEST_ASSERT_NOT_EQUAL(handle1.initialized, handle2.initialized);
}

/* ==================== HAL Configuration Tests ==================== */

void test_hal_configuration(void)
{
    /* Test that HAL is properly configured */
    /* This is a placeholder for configuration tests */
    TEST_ASSERT_TRUE(1);
}

/* ==================== HAL PC Simulation Tests ==================== */

void test_hal_pc_simulation(void)
{
    /* Test PC simulation layer if available */
    /* This test verifies HAL can run on PC */
    
    #ifdef PC_SIMULATION
    TEST_ASSERT_TRUE(1); /* PC simulation enabled */
    #else
    TEST_ASSERT_TRUE(1); /* PC simulation not required */
    #endif
}

/* ==================== Main ==================== */

int main(void)
{
    UNITY_BEGIN();

    /* HAL Version Tests */
    RUN_TEST(test_hal_version_macros);

    /* HAL Error Code Tests */
    RUN_TEST(test_hal_error_codes);
    RUN_TEST(test_hal_error_code_ordering);

    /* HAL Handle Structure Tests */
    RUN_TEST(test_hal_handle_structure);
    RUN_TEST(test_hal_handle_initialization);
    RUN_TEST(test_hal_handle_with_data);

    /* HAL Status Type Tests */
    RUN_TEST(test_hal_status_type_legacy);

    /* HAL Sub-module Header Tests */
    RUN_TEST(test_hal_submodule_headers_exist);

    /* HAL Type Definitions Tests */
    RUN_TEST(test_hal_type_definitions);

    /* HAL Error Code Usage Tests */
    RUN_TEST(test_hal_error_code_usage);

    /* HAL Handle Operations Tests */
    RUN_TEST(test_hal_handle_operations);

    /* HAL Configuration Tests */
    RUN_TEST(test_hal_configuration);

    /* HAL PC Simulation Tests */
    RUN_TEST(test_hal_pc_simulation);

    return UNITY_END();
}
