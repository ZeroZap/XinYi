/**
 * @file stm32l4xx_hal.h
 * @brief Host-only stand-in for the STM32L4 vendor HAL surface used by the
 *        Pandora board-owned I2C transports.
 *
 * Only the symbols referenced by `boards/pandora_stm32l475/pandora_soft_i2c.c`
 * and `pandora_hw_i2c.c` exist here. Host unit tests place this directory
 * *before* the board directory on the include path, so the vendored SDK is
 * never compiled into a host target; the transports are observed through the
 * canonical `xy_hal_gpio_*` / `xy_hal_i2c_*` seams those board files call.
 */

#ifndef PANDORA_FAKE_STM32L4XX_HAL_H
#define PANDORA_FAKE_STM32L4XX_HAL_H

#include <stdint.h>

/* GPIO_TypeDef stays compatible with xy_hal_gpio_port_t on the PC platform
 * (struct xy_hal_gpio_port *), so board code that passes GPIOC/GPIOD into
 * xy_hal_gpio_* keeps one port type in host tests. */
struct xy_hal_gpio_port;
typedef struct xy_hal_gpio_port GPIO_TypeDef;

extern GPIO_TypeDef xy_test_fake_gpioc;
extern GPIO_TypeDef xy_test_fake_gpiod;
#define GPIOC (&xy_test_fake_gpioc)
#define GPIOD (&xy_test_fake_gpiod)

typedef struct {
    uint32_t reserved;
} I2C_TypeDef;

extern I2C_TypeDef xy_test_fake_i2c3;
#define I2C3 (&xy_test_fake_i2c3)

typedef struct {
    I2C_TypeDef *Instance;
    struct {
        uint32_t Timing;
    } Init;
} I2C_HandleTypeDef;

#define GPIO_AF4_I2C3 4U

#define __NOP() ((void)0)

/* Clock-enable hooks are counted by the host fixture. */
void xy_test_fake_rcc_gpioc_enable(void);
void xy_test_fake_rcc_gpiod_enable(void);
void xy_test_fake_rcc_i2c3_enable(void);
#define __HAL_RCC_GPIOC_CLK_ENABLE() xy_test_fake_rcc_gpioc_enable()
#define __HAL_RCC_GPIOD_CLK_ENABLE() xy_test_fake_rcc_gpiod_enable()
#define __HAL_RCC_I2C3_CLK_ENABLE() xy_test_fake_rcc_i2c3_enable()

#endif /* PANDORA_FAKE_STM32L4XX_HAL_H */
