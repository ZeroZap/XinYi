/**
 * @file test_iso7816_example_main.c
 * @brief Link/run guard for the ISO7816 usage example.
 */

#include "xy_hal_uart.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "unity.h"

xy_hal_error_t xy_hal_uart_init(void *uart, const xy_hal_uart_config_t *config);

/* Prototypes from xy_iso7816_example.c */
void example_init_and_atr(void);
void example_read_sim_info(void);
void example_verify_pin(const char *pin);
void example_2g_authentication(void);
void example_3g_authentication(void);
void example_manual_file_access(void);
void example_custom_apdu(void);
void example_complete_workflow(const char *pin);

xy_hal_error_t xy_hal_uart_init(void *uart, const xy_hal_uart_config_t *config)
{
    (void)uart;
    (void)config;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_send(void *uart, const uint8_t *data, size_t len,
                                uint32_t timeout)
{
    (void)uart;
    (void)data;
    (void)len;
    (void)timeout;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_recv(void *uart, uint8_t *data, size_t len,
                                uint32_t timeout)
{
    (void)uart;
    (void)timeout;
    if (data && len) {
        memset(data, 0, len);
    }
    return XY_HAL_ERROR_TIMEOUT;
}

xy_hal_error_t xy_hal_uart_flush(void *uart)
{
    (void)uart;
    return XY_HAL_OK;
}

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_iso7816_example_symbols_link(void)
{
    /* Keep this as a link/run smoke guard. Avoid invoking examples because
     * they intentionally demonstrate hardware-backed SIM-card workflows. */
    TEST_ASSERT_NOT_NULL((void *)example_init_and_atr);
    TEST_ASSERT_NOT_NULL((void *)example_read_sim_info);
    TEST_ASSERT_NOT_NULL((void *)example_verify_pin);
    TEST_ASSERT_NOT_NULL((void *)example_2g_authentication);
    TEST_ASSERT_NOT_NULL((void *)example_3g_authentication);
    TEST_ASSERT_NOT_NULL((void *)example_manual_file_access);
    TEST_ASSERT_NOT_NULL((void *)example_custom_apdu);
    TEST_ASSERT_NOT_NULL((void *)example_complete_workflow);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_iso7816_example_symbols_link);
    return UNITY_END();
}
