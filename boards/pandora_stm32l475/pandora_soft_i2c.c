#include "pandora_soft_i2c.h"

#include "stm32l4xx_hal.h"
#include "xy_hal_gpio.h"
#include "xy_hal_i2c.h"

typedef struct {
    GPIO_TypeDef *sda_port;
    uint8_t sda_pin;
    GPIO_TypeDef *scl_port;
    uint8_t scl_pin;
} pandora_soft_i2c_bus_t;

/* Official Pandora BSP: software-I2C3 PC0=SCL, PC1=SDA; I2C4 PD6=SCL, PC1=SDA. */
static pandora_soft_i2c_bus_t soft_i2c3 = {GPIOC, 1U, GPIOC, 0U};
static pandora_soft_i2c_bus_t soft_i2c4 = {GPIOC, 1U, GPIOD, 6U};
static pandora_soft_i2c_bus_t *active_bus;

static void soft_i2c_delay(void)
{
    for (volatile uint32_t i = 0; i < 80U; ++i) {
        __NOP();
    }
}

static void soft_i2c_sda(uint8_t state)
{
    (void)xy_hal_gpio_write(active_bus->sda_port, active_bus->sda_pin, state);
    soft_i2c_delay();
}

static void soft_i2c_scl(uint8_t state)
{
    (void)xy_hal_gpio_write(active_bus->scl_port, active_bus->scl_pin, state);
    soft_i2c_delay();
}

static void soft_i2c_start(void)
{
    soft_i2c_sda(1U);
    soft_i2c_scl(1U);
    soft_i2c_sda(0U);
    soft_i2c_scl(0U);
}

static void soft_i2c_stop(void)
{
    soft_i2c_sda(0U);
    soft_i2c_scl(1U);
    soft_i2c_sda(1U);
}

static void soft_i2c_sda_config(xy_hal_gpio_mode_t mode)
{
    xy_hal_gpio_config_t gpio = {
        .mode = mode,
        .pull = XY_HAL_GPIO_PULL_UP,
        .otype = XY_HAL_GPIO_OTYPE_OD,
        .speed = XY_HAL_GPIO_SPEED_HIGH,
    };
    (void)xy_hal_gpio_init(active_bus->sda_port, active_bus->sda_pin, &gpio);
}

static int soft_i2c_write_byte(uint8_t value)
{
    for (uint32_t bit = 0; bit < 8U; ++bit) {
        soft_i2c_sda((value & 0x80U) ? 1U : 0U);
        soft_i2c_scl(1U);
        soft_i2c_scl(0U);
        value <<= 1;
    }
    soft_i2c_sda_config(XY_HAL_GPIO_MODE_INPUT);
    soft_i2c_scl(1U);
    int ack = xy_hal_gpio_read(active_bus->sda_port, active_bus->sda_pin) == 0;
    soft_i2c_scl(0U);
    soft_i2c_sda_config(XY_HAL_GPIO_MODE_OUTPUT);
    return ack;
}

static uint8_t soft_i2c_read_byte(int send_ack)
{
    uint8_t value = 0;

    soft_i2c_sda_config(XY_HAL_GPIO_MODE_INPUT);
    for (uint32_t bit = 0; bit < 8U; ++bit) {
        value <<= 1;
        soft_i2c_scl(1U);
        if (xy_hal_gpio_read(active_bus->sda_port, active_bus->sda_pin) == 1) {
            value |= 1U;
        }
        soft_i2c_scl(0U);
    }
    soft_i2c_sda_config(XY_HAL_GPIO_MODE_OUTPUT);
    soft_i2c_sda(send_ack ? 0U : 1U);
    soft_i2c_scl(1U);
    soft_i2c_scl(0U);
    soft_i2c_sda(1U);
    return value;
}

static void *soft_i2c_init(pandora_soft_i2c_bus_t *bus)
{
    xy_hal_gpio_config_t gpio = {
        .mode = XY_HAL_GPIO_MODE_OUTPUT,
        .pull = XY_HAL_GPIO_PULL_UP,
        .otype = XY_HAL_GPIO_OTYPE_OD,
        .speed = XY_HAL_GPIO_SPEED_HIGH,
    };

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    active_bus = bus;
    if (xy_hal_gpio_init(bus->sda_port, bus->sda_pin, &gpio) != XY_HAL_OK ||
        xy_hal_gpio_init(bus->scl_port, bus->scl_pin, &gpio) != XY_HAL_OK) {
        return NULL;
    }
    soft_i2c_sda(1U);
    soft_i2c_scl(1U);
    return bus;
}

void *pandora_soft_i2c3_init(void)
{
    return soft_i2c_init(&soft_i2c3);
}

void *pandora_soft_i2c4_init(void)
{
    return soft_i2c_init(&soft_i2c4);
}

void *pandora_soft_i2c_init(void)
{
    return pandora_soft_i2c4_init();
}

int pandora_soft_i2c_probe(void *i2c, uint8_t dev_addr)
{
    if ((i2c != &soft_i2c3 && i2c != &soft_i2c4) || dev_addr > 0x7FU) {
        return 0;
    }

    active_bus = i2c;
    soft_i2c_start();
    int ack = soft_i2c_write_byte((uint8_t)(dev_addr << 1));
    soft_i2c_stop();
    return ack;
}

xy_hal_error_t xy_hal_i2c_master_transmit(void *i2c, uint16_t dev_addr,
                                          const uint8_t *data, size_t len,
                                          uint32_t timeout)
{
    (void)timeout;
    if ((i2c != &soft_i2c3 && i2c != &soft_i2c4) || data == NULL || len == 0U ||
        dev_addr > 0x7FU) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    active_bus = i2c;
    soft_i2c_start();
    if (!soft_i2c_write_byte((uint8_t)(dev_addr << 1))) {
        soft_i2c_stop();
        return XY_HAL_ERROR_IO;
    }
    for (size_t i = 0; i < len; ++i) {
        if (!soft_i2c_write_byte(data[i])) {
            soft_i2c_stop();
            return XY_HAL_ERROR_IO;
        }
    }
    soft_i2c_stop();
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_master_receive(void *i2c, uint16_t dev_addr, uint8_t *data,
                                         size_t len, uint32_t timeout)
{
    (void)timeout;
    if ((i2c != &soft_i2c3 && i2c != &soft_i2c4) || data == NULL || len == 0U ||
        dev_addr > 0x7FU) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    active_bus = i2c;
    soft_i2c_start();
    if (!soft_i2c_write_byte((uint8_t)((dev_addr << 1) | 1U))) {
        soft_i2c_stop();
        return XY_HAL_ERROR_IO;
    }
    for (size_t i = 0; i < len; ++i) {
        data[i] = soft_i2c_read_byte(i + 1U < len);
    }
    soft_i2c_stop();
    return XY_HAL_OK;
}
