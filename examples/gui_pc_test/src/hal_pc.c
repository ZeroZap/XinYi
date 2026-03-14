/**
 * @file hal_pc.c
 * @brief HAL Implementation for PC Simulation
 * @version 1.0.0
 * @date 2026-03-14
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include "xy_hal.h"

/* ==================== HAL Core ==================== */

/**
 * @brief Initialize HAL
 */
xy_hal_status_t xy_hal_init(void)
{
    printf("[HAL] Initialized for PC simulation\n");
    return XY_HAL_OK;
}

/**
 * @brief Deinitialize HAL
 */
xy_hal_status_t xy_hal_deinit(void)
{
    printf("[HAL] Deinitialized\n");
    return XY_HAL_OK;
}

/**
 * @brief Get system tick in milliseconds
 */
uint32_t xy_hal_get_tick_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

/**
 * @brief Delay in milliseconds
 */
void xy_hal_delay_ms(uint32_t ms)
{
    usleep(ms * 1000);
}

/**
 * @brief Delay in microseconds
 */
void xy_hal_delay_us(uint32_t us)
{
    usleep(us);
}

/* ==================== GPIO ==================== */

xy_hal_status_t xy_hal_gpio_init(xy_hal_gpio_t *gpio, const xy_hal_gpio_config_t *config)
{
    if (!gpio || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    gpio->pin = config->pin;
    gpio->port = config->port;
    gpio->mode = config->mode;
    gpio->pull = config->pull;
    gpio->speed = config->speed;
    
    printf("[GPIO] Init: P%d.%d, mode=%d, pull=%d\n", 
           gpio->port, gpio->pin, gpio->mode, gpio->pull);
    
    return XY_HAL_OK;
}

xy_hal_status_t xy_hal_gpio_write(xy_hal_gpio_t *gpio, bool value)
{
    if (!gpio) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    gpio->value = value;
    printf("[GPIO] Write: P%d.%d = %d\n", gpio->port, gpio->pin, value ? 1 : 0);
    
    return XY_HAL_OK;
}

bool xy_hal_gpio_read(xy_hal_gpio_t *gpio)
{
    if (!gpio) {
        return false;
    }
    
    return gpio->value;
}

xy_hal_status_t xy_hal_gpio_toggle(xy_hal_gpio_t *gpio)
{
    if (!gpio) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    gpio->value = !gpio->value;
    printf("[GPIO] Toggle: P%d.%d = %d\n", gpio->port, gpio->pin, gpio->value ? 1 : 0);
    
    return XY_HAL_OK;
}

/* ==================== UART ==================== */

xy_hal_status_t xy_hal_uart_init(xy_hal_uart_t *uart, const xy_hal_uart_config_t *config)
{
    if (!uart || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    uart->instance = config->instance;
    uart->baudrate = config->baudrate;
    uart->word_length = config->word_length;
    uart->stop_bits = config->stop_bits;
    uart->parity = config->parity;
    
    printf("[UART] Init: Instance=%d, Baud=%d\n", uart->instance, uart->baudrate);
    
    return XY_HAL_OK;
}

xy_hal_status_t xy_hal_uart_transmit(xy_hal_uart_t *uart, const uint8_t *data, uint16_t size, uint32_t timeout)
{
    if (!uart || !data) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    (void)timeout;
    
    /* Print to stdout for PC simulation */
    for (uint16_t i = 0; i < size; i++) {
        putchar(data[i]);
    }
    fflush(stdout);
    
    return XY_HAL_OK;
}

xy_hal_status_t xy_hal_uart_receive(xy_hal_uart_t *uart, uint8_t *data, uint16_t size, uint32_t timeout)
{
    if (!uart || !data) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    (void)timeout;
    
    /* PC simulation - return dummy data */
    for (uint16_t i = 0; i < size; i++) {
        data[i] = 0;
    }
    
    return XY_HAL_OK;
}

/* ==================== Timer ==================== */

xy_hal_status_t xy_hal_timer_init(xy_hal_timer_t *timer, const xy_hal_timer_config_t *config)
{
    if (!timer || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    timer->instance = config->instance;
    timer->period = config->period;
    timer->prescaler = config->prescaler;
    timer->auto_reload = config->auto_reload;
    
    printf("[TIMER] Init: Instance=%d, Period=%d\n", timer->instance, timer->period);
    
    return XY_HAL_OK;
}

xy_hal_status_t xy_hal_timer_start(xy_hal_timer_t *timer)
{
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    timer->running = true;
    printf("[TIMER] Start: Instance=%d\n", timer->instance);
    
    return XY_HAL_OK;
}

xy_hal_status_t xy_hal_timer_stop(xy_hal_timer_t *timer)
{
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    timer->running = false;
    printf("[TIMER] Stop: Instance=%d\n", timer->instance);
    
    return XY_HAL_OK;
}

/* ==================== I2C ==================== */

xy_hal_status_t xy_hal_i2c_init(xy_hal_i2c_t *i2c, const xy_hal_i2c_config_t *config)
{
    if (!i2c || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    i2c->instance = config->instance;
    i2c->speed = config->speed;
    i2c->address = config->address;
    
    printf("[I2C] Init: Instance=%d, Speed=%d kHz\n", i2c->instance, i2c->speed);
    
    return XY_HAL_OK;
}

xy_hal_status_t xy_hal_i2c_write(xy_hal_i2c_t *i2c, uint16_t dev_addr, const uint8_t *data, uint16_t size)
{
    if (!i2c || !data) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    printf("[I2C] Write: Dev=0x%02X, Size=%d\n", dev_addr, size);
    (void)i2c;
    
    return XY_HAL_OK;
}

xy_hal_status_t xy_hal_i2c_read(xy_hal_i2c_t *i2c, uint16_t dev_addr, uint8_t *data, uint16_t size)
{
    if (!i2c || !data) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    printf("[I2C] Read: Dev=0x%02X, Size=%d\n", dev_addr, size);
    (void)i2c;
    
    return XY_HAL_OK;
}

/* ==================== SPI ==================== */

xy_hal_status_t xy_hal_spi_init(xy_hal_spi_t *spi, const xy_hal_spi_config_t *config)
{
    if (!spi || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    spi->instance = config->instance;
    spi->mode = config->mode;
    spi->speed = config->speed;
    spi->data_size = config->data_size;
    
    printf("[SPI] Init: Instance=%d, Mode=%d, Speed=%d kHz\n", 
           spi->instance, spi->mode, spi->speed);
    
    return XY_HAL_OK;
}

xy_hal_status_t xy_hal_spi_transmit(xy_hal_spi_t *spi, const uint8_t *tx_data, uint8_t *rx_data, uint16_t size)
{
    if (!spi || !tx_data) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    printf("[SPI] Transmit: Size=%d\n", size);
    
    if (rx_data) {
        for (uint16_t i = 0; i < size; i++) {
            rx_data[i] = tx_data[i];
        }
    }
    
    return XY_HAL_OK;
}

/* ==================== ADC ==================== */

xy_hal_status_t xy_hal_adc_init(xy_hal_adc_t *adc, const xy_hal_adc_config_t *config)
{
    if (!adc || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    adc->instance = config->instance;
    adc->resolution = config->resolution;
    adc->alignment = config->alignment;
    
    printf("[ADC] Init: Instance=%d, Resolution=%d bits\n", 
           adc->instance, adc->resolution);
    
    return XY_HAL_OK;
}

xy_hal_status_t xy_hal_adc_read(xy_hal_adc_t *adc, uint32_t *value)
{
    if (!adc || !value) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* PC simulation - return random value */
    *value = rand() % (1 << adc->resolution);
    
    return XY_HAL_OK;
}

/* ==================== Flash ==================== */

xy_hal_status_t xy_hal_flash_init(xy_hal_flash_t *flash)
{
    if (!flash) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    flash->base_address = 0x08000000;
    flash->size = 2 * 1024 * 1024; /* 2MB */
    
    printf("[FLASH] Init: Base=0x%08X, Size=%d KB\n", 
           flash->base_address, flash->size / 1024);
    
    return XY_HAL_OK;
}

xy_hal_status_t xy_hal_flash_write(xy_hal_flash_t *flash, uint32_t address, const uint8_t *data, uint16_t size)
{
    if (!flash || !data) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    printf("[FLASH] Write: Addr=0x%08X, Size=%d\n", address, size);
    (void)flash;
    
    return XY_HAL_OK;
}

xy_hal_status_t xy_hal_flash_read(xy_hal_flash_t *flash, uint32_t address, uint8_t *data, uint16_t size)
{
    if (!flash || !data) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    printf("[FLASH] Read: Addr=0x%08X, Size=%d\n", address, size);
    (void)flash;
    
    return XY_HAL_OK;
}

xy_hal_status_t xy_hal_flash_erase(xy_hal_flash_t *flash, uint32_t address, uint32_t size)
{
    if (!flash) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    printf("[FLASH] Erase: Addr=0x%08X, Size=%d\n", address, size);
    
    return XY_HAL_OK;
}

/* ==================== RTC ==================== */

xy_hal_status_t xy_hal_rtc_init(xy_hal_rtc_t *rtc, const xy_hal_rtc_config_t *config)
{
    if (!rtc || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    rtc->format = config->format;
    
    printf("[RTC] Init: Format=%d\n", rtc->format);
    
    return XY_HAL_OK;
}

xy_hal_status_t xy_hal_rtc_get_time(xy_hal_rtc_t *rtc, xy_hal_rtc_time_t *time)
{
    if (!rtc || !time) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    time->year = tm_info->tm_year + 1900;
    time->month = tm_info->tm_mon + 1;
    time->day = tm_info->tm_mday;
    time->hours = tm_info->tm_hour;
    time->minutes = tm_info->tm_min;
    time->seconds = tm_info->tm_sec;
    
    return XY_HAL_OK;
}
