#include "pandora_soft_i2c.h"

#include "stm32l4xx_hal.h"
#include "xy_hal_i2c.h"

typedef struct {
    GPIO_TypeDef *sda_port;
    uint16_t sda_pin;
    GPIO_TypeDef *scl_port;
    uint16_t scl_pin;
} pandora_soft_i2c_bus_t;

/* Official Pandora BSP: software-I2C3 PC0=SCL, PC1=SDA; I2C4 PD6=SCL, PC1=SDA. */
static pandora_soft_i2c_bus_t soft_i2c3 = {GPIOC, GPIO_PIN_1, GPIOC, GPIO_PIN_0};
static pandora_soft_i2c_bus_t soft_i2c4 = {GPIOC, GPIO_PIN_1, GPIOD, GPIO_PIN_6};
static pandora_soft_i2c_bus_t *active_bus;

static void soft_i2c_delay(void)
{
    for (volatile uint32_t i = 0; i < 80U; ++i) {
        __NOP();
    }
}

static void soft_i2c_sda(GPIO_PinState state)
{
    HAL_GPIO_WritePin(active_bus->sda_port, active_bus->sda_pin, state);
    soft_i2c_delay();
}

static void soft_i2c_scl(GPIO_PinState state)
{
    HAL_GPIO_WritePin(active_bus->scl_port, active_bus->scl_pin, state);
    soft_i2c_delay();
}

static void soft_i2c_start(void)
{
    soft_i2c_sda(GPIO_PIN_SET);
    soft_i2c_scl(GPIO_PIN_SET);
    soft_i2c_sda(GPIO_PIN_RESET);
    soft_i2c_scl(GPIO_PIN_RESET);
}

static void soft_i2c_stop(void)
{
    soft_i2c_sda(GPIO_PIN_RESET);
    soft_i2c_scl(GPIO_PIN_SET);
    soft_i2c_sda(GPIO_PIN_SET);
}

static int soft_i2c_write_byte(uint8_t value)
{
    GPIO_InitTypeDef gpio = {0};

    for (uint32_t bit = 0; bit < 8U; ++bit) {
        soft_i2c_sda((value & 0x80U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        soft_i2c_scl(GPIO_PIN_SET);
        soft_i2c_scl(GPIO_PIN_RESET);
        value <<= 1;
    }
    gpio.Pin = active_bus->sda_pin;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(active_bus->sda_port, &gpio);
    soft_i2c_scl(GPIO_PIN_SET);
    int ack = HAL_GPIO_ReadPin(active_bus->sda_port, active_bus->sda_pin) == GPIO_PIN_RESET;
    soft_i2c_scl(GPIO_PIN_RESET);
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    HAL_GPIO_Init(active_bus->sda_port, &gpio);
    return ack;
}

static uint8_t soft_i2c_read_byte(int send_ack)
{
    GPIO_InitTypeDef gpio = {0};
    uint8_t value = 0;

    gpio.Pin = active_bus->sda_pin;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(active_bus->sda_port, &gpio);
    for (uint32_t bit = 0; bit < 8U; ++bit) {
        value <<= 1;
        soft_i2c_scl(GPIO_PIN_SET);
        if (HAL_GPIO_ReadPin(active_bus->sda_port, active_bus->sda_pin) == GPIO_PIN_SET) {
            value |= 1U;
        }
        soft_i2c_scl(GPIO_PIN_RESET);
    }
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    HAL_GPIO_Init(active_bus->sda_port, &gpio);
    soft_i2c_sda(send_ack ? GPIO_PIN_RESET : GPIO_PIN_SET);
    soft_i2c_scl(GPIO_PIN_SET);
    soft_i2c_scl(GPIO_PIN_RESET);
    soft_i2c_sda(GPIO_PIN_SET);
    return value;
}

static void *soft_i2c_init(pandora_soft_i2c_bus_t *bus)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    active_bus = bus;
    gpio.Pin = bus->sda_pin;
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(bus->sda_port, &gpio);
    gpio.Pin = bus->scl_pin;
    HAL_GPIO_Init(bus->scl_port, &gpio);
    soft_i2c_sda(GPIO_PIN_SET);
    soft_i2c_scl(GPIO_PIN_SET);
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
