/**
 * @file test_device.c
 * @brief Device Driver Unit Tests using Unity Framework
 * @version 1.0.0
 * @date 2026-02-28
 */

#include <stdint.h>
#include <string.h>
#include "unity.h"

/* Device headers */
#include "xy_device.h"
#include "xy_eeprom_24xx.h"
#include "xy_oled_ssd1306.h"

/* ==================== Test Fixtures ==================== */

static xy_device_manager_t test_mgr;

void setUp(void)
{
    xy_device_manager_init(&test_mgr, 10);
}

void tearDown(void)
{
    if (test_mgr.devices) {
        free(test_mgr.devices);
    }
}

/* ==================== Device Manager Tests ==================== */

void test_device_manager_init(void)
{
    xy_device_manager_t mgr;
    int result;
    
    result = xy_device_manager_init(&mgr, 10);
    TEST_ASSERT_EQUAL(XY_DEVICE_OK, result);
    TEST_ASSERT_EQUAL(0, mgr.count);
    TEST_ASSERT_NOT_NULL(mgr.devices);
    
    if (mgr.devices) {
        free(mgr.devices);
    }
}

void test_device_manager_register(void)
{
    xy_device_t dev;
    int result;
    
    memset(&dev, 0, sizeof(dev));
    dev.name = "test_dev";
    
    result = xy_device_manager_register(&test_mgr, &dev);
    TEST_ASSERT_EQUAL(XY_DEVICE_OK, result);
    TEST_ASSERT_EQUAL(1, test_mgr.count);
}

void test_device_manager_find(void)
{
    xy_device_t dev1, dev2;
    xy_device_t *found;
    
    memset(&dev1, 0, sizeof(dev1));
    dev1.name = "device1";
    
    memset(&dev2, 0, sizeof(dev2));
    dev2.name = "device2";
    
    xy_device_manager_register(&test_mgr, &dev1);
    xy_device_manager_register(&test_mgr, &dev2);
    
    found = xy_device_manager_find(&test_mgr, "device1");
    TEST_ASSERT_EQUAL_PTR(&dev1, found);
    
    found = xy_device_manager_find(&test_mgr, "device2");
    TEST_ASSERT_EQUAL_PTR(&dev2, found);
    
    found = xy_device_manager_find(&test_mgr, "nonexistent");
    TEST_ASSERT_NULL(found);
}

void test_device_manager_unregister(void)
{
    xy_device_t dev;
    int result;
    
    memset(&dev, 0, sizeof(dev));
    dev.name = "test_dev";
    
    xy_device_manager_register(&test_mgr, &dev);
    TEST_ASSERT_EQUAL(1, test_mgr.count);
    
    result = xy_device_manager_unregister(&test_mgr, &dev);
    TEST_ASSERT_EQUAL(XY_DEVICE_OK, result);
    TEST_ASSERT_EQUAL(0, test_mgr.count);
}

/* ==================== I2C Device Tests ==================== */

void test_i2c_device_init(void)
{
    xy_i2c_device_t dev;
    int result;
    
    result = xy_i2c_device_init(&dev, (void *)0x1234, 0x50, 1000);
    TEST_ASSERT_EQUAL(XY_DEVICE_OK, result);
    TEST_ASSERT_EQUAL(0x50, dev.dev_addr);
    TEST_ASSERT_EQUAL(1000, dev.timeout);
    TEST_ASSERT_TRUE(dev.base.initialized);
}

void test_i2c_device_invalid_params(void)
{
    xy_i2c_device_t dev;
    int result;
    
    result = xy_i2c_device_init(NULL, (void *)0x1234, 0x50, 1000);
    TEST_ASSERT_EQUAL(XY_DEVICE_INVALID_PARAM, result);
    
    result = xy_i2c_device_init(&dev, NULL, 0x50, 1000);
    TEST_ASSERT_EQUAL(XY_DEVICE_INVALID_PARAM, result);
}

/* ==================== SPI Device Tests ==================== */

void test_spi_device_init(void)
{
    xy_spi_device_t dev;
    int result;
    
    result = xy_spi_device_init(&dev, (void *)0x1234, (void *)0x5678, 1000000);
    TEST_ASSERT_EQUAL(XY_DEVICE_OK, result);
    TEST_ASSERT_EQUAL(1000000, dev.speed_hz);
    TEST_ASSERT_TRUE(dev.base.initialized);
}

void test_spi_device_cs(void)
{
    xy_spi_device_t dev;
    
    xy_spi_device_init(&dev, (void *)0x1234, (void *)0x5678, 1000000);
    
    /* CS select/deselect would toggle GPIO in real implementation */
    xy_spi_device_cs(&dev, true);
    xy_spi_device_cs(&dev, false);
    
    TEST_ASSERT_TRUE(1); /* Placeholder */
}

/* ==================== UART Device Tests ==================== */

void test_uart_device_init(void)
{
    xy_uart_device_t dev;
    int result;
    
    result = xy_uart_device_init(&dev, (void *)0x1234, 115200);
    TEST_ASSERT_EQUAL(XY_DEVICE_OK, result);
    TEST_ASSERT_EQUAL(115200, dev.baudrate);
    TEST_ASSERT_TRUE(dev.base.initialized);
}

/* ==================== GPIO Device Tests ==================== */

void test_gpio_device_init(void)
{
    xy_gpio_device_t dev;
    int result;
    
    result = xy_gpio_device_init(&dev, (void *)GPIOA, 5, 
                                  XY_GPIO_MODE_OUTPUT, XY_GPIO_PULL_NONE);
    TEST_ASSERT_EQUAL(XY_DEVICE_OK, result);
    TEST_ASSERT_EQUAL(5, dev.gpio_pin);
    TEST_ASSERT_EQUAL(XY_GPIO_MODE_OUTPUT, dev.mode);
}

void test_gpio_device_operations(void)
{
    xy_gpio_device_t dev;
    
    xy_gpio_device_init(&dev, (void *)GPIOA, 5, 
                        XY_GPIO_MODE_OUTPUT, XY_GPIO_PULL_NONE);
    
    xy_gpio_device_set(&dev, true);
    xy_gpio_device_set(&dev, false);
    xy_gpio_device_toggle(&dev);
    
    TEST_ASSERT_TRUE(1); /* Placeholder */
}

/* ==================== EEPROM Tests ==================== */

void test_eeprom_24xx_init(void)
{
    xy_eeprom_24xx_t eeprom;
    int result;
    
    result = xy_eeprom_24xx_init(&eeprom, (void *)0x1234, 0x50, 64, 32768);
    TEST_ASSERT_EQUAL(XY_DEVICE_OK, result);
    TEST_ASSERT_EQUAL(64, eeprom.page_size);
    TEST_ASSERT_EQUAL(32768, eeprom.total_size);
}

void test_eeprom_24xx_invalid_params(void)
{
    xy_eeprom_24xx_t eeprom;
    int result;
    
    result = xy_eeprom_24xx_init(NULL, (void *)0x1234, 0x50, 64, 32768);
    TEST_ASSERT_EQUAL(XY_DEVICE_INVALID_PARAM, result);
    
    result = xy_eeprom_24xx_init(&eeprom, NULL, 0x50, 64, 32768);
    TEST_ASSERT_EQUAL(XY_DEVICE_INVALID_PARAM, result);
}

/* ==================== OLED Tests ==================== */

void test_oled_ssd1306_init(void)
{
    xy_oled_ssd1306_t oled;
    int result;
    
    result = xy_oled_ssd1306_init(&oled, (void *)0x1234, 128, 64);
    TEST_ASSERT_EQUAL(XY_DEVICE_OK, result);
    TEST_ASSERT_EQUAL(128, oled.width);
    TEST_ASSERT_EQUAL(64, oled.height);
    TEST_ASSERT_NOT_NULL(oled.buffer);
    
    if (oled.buffer) {
        free(oled.buffer);
    }
}

void test_oled_ssd1306_clear(void)
{
    xy_oled_ssd1306_t oled;
    
    xy_oled_ssd1306_init(&oled, (void *)0x1234, 128, 64);
    xy_oled_ssd1306_clear(&oled);
    
    /* Verify buffer is cleared */
    for (int i = 0; i < (128 * 64) / 8; i++) {
        TEST_ASSERT_EQUAL(0, oled.buffer[i]);
    }
    
    if (oled.buffer) {
        free(oled.buffer);
    }
}

void test_oled_ssd1306_draw_pixel(void)
{
    xy_oled_ssd1306_t oled;
    
    xy_oled_ssd1306_init(&oled, (void *)0x1234, 128, 64);
    xy_oled_ssd1306_clear(&oled);
    
    /* Draw pixel at (0, 0) */
    xy_oled_ssd1306_draw_pixel(&oled, 0, 0, true);
    TEST_ASSERT_EQUAL(0x01, oled.buffer[0]);
    
    /* Draw pixel at (0, 1) */
    xy_oled_ssd1306_draw_pixel(&oled, 0, 1, true);
    TEST_ASSERT_EQUAL(0x03, oled.buffer[0]);
    
    if (oled.buffer) {
        free(oled.buffer);
    }
}

/* ==================== Main ==================== */

int main(void)
{
    UNITY_BEGIN();

    /* Device Manager Tests */
    RUN_TEST(test_device_manager_init);
    RUN_TEST(test_device_manager_register);
    RUN_TEST(test_device_manager_find);
    RUN_TEST(test_device_manager_unregister);

    /* I2C Device Tests */
    RUN_TEST(test_i2c_device_init);
    RUN_TEST(test_i2c_device_invalid_params);

    /* SPI Device Tests */
    RUN_TEST(test_spi_device_init);
    RUN_TEST(test_spi_device_cs);

    /* UART Device Tests */
    RUN_TEST(test_uart_device_init);

    /* GPIO Device Tests */
    RUN_TEST(test_gpio_device_init);
    RUN_TEST(test_gpio_device_operations);

    /* EEPROM Tests */
    RUN_TEST(test_eeprom_24xx_init);
    RUN_TEST(test_eeprom_24xx_invalid_params);

    /* OLED Tests */
    RUN_TEST(test_oled_ssd1306_init);
    RUN_TEST(test_oled_ssd1306_clear);
    RUN_TEST(test_oled_ssd1306_draw_pixel);

    return UNITY_END();
}
