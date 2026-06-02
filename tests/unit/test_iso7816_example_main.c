/**
 * @file test_iso7816_example_main.c
 * @brief Link/run guard for the ISO7816 usage example.
 */

#include "xy_hal_uart.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>

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

int main(void)
{
    /* Keep this as a link/run smoke guard. Avoid invoking examples because
     * they intentionally demonstrate hardware-backed SIM-card workflows. */
    (void)example_init_and_atr;
    (void)example_read_sim_info;
    (void)example_verify_pin;
    (void)example_2g_authentication;
    (void)example_3g_authentication;
    (void)example_manual_file_access;
    (void)example_custom_apdu;
    (void)example_complete_workflow;
    return 0;
}
