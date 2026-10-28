/**
 * @file stm32l4xx_hal.h
 * @brief Host-only stand-in for the STM32L4 vendor HAL surface used by the
 *        Pandora board-owned startup and I2C owners.
 *
 * Only the symbols referenced by `boards/pandora_stm32l475/pandora_soft_i2c.c`,
 * `pandora_hw_i2c.c` and `pandora_platform_startup.c` exist here. Host unit
 * tests place this directory *before* the board directory on the include path,
 * so the vendored SDK is never compiled into a host target; the board files are
 * observed through the canonical `xy_hal_gpio_*` / `xy_hal_i2c_*` seams and the
 * counted vendor clock hooks below.
 *
 * Every constant below mirrors the numeric value the real STM32L4 vendor headers
 * define (each carries a "real" note), so host assertions pin the vendor
 * semantics the board code relies on rather than a local invention.
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
    I2C_TypeDef* Instance;
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

/* --- vendor clock / power surface for pandora_platform_startup.c --------- */

typedef enum {
    HAL_OK = 0x00,
    HAL_ERROR = 0x01,
    HAL_BUSY = 0x02,
    HAL_TIMEOUT = 0x03,
} HAL_StatusTypeDef;

/* real: stm32l4xx_hal_rcc.h RCC_OscInitTypeDef field order (L4 non-LSIPREDIV). */
typedef struct {
    uint32_t PLLState;  /* real: RCC_PLL_ON = 0x00000002U */
    uint32_t PLLSource; /* real: RCC_PLLSOURCE_HSE = 0x00000003U */
    uint32_t PLLM;      /* real: Min_Data = 1, Max_Data = 8 */
    uint32_t PLLN;      /* real: Min_Data = 8, Max_Data = 86 */
    uint32_t PLLP;      /* real: RCC_PLLP_DIV7 = 0x00000007U */
    uint32_t PLLQ;      /* real: RCC_PLLQ_DIV2 = 0x00000002U */
    uint32_t PLLR;      /* real: RCC_PLLR_DIV2 = 0x00000002U */
} RCC_PLLInitTypeDef;

typedef struct {
    uint32_t OscillatorType; /* real: RCC_OSCILLATORTYPE_HSE = 0x00000001U */
    uint32_t HSEState;       /* real: RCC_HSE_ON = 0x00010000U */
    uint32_t LSEState;       /* real: RCC_LSE_OFF = 0x00000000U */
    uint32_t HSIState;       /* real: RCC_HSI_OFF = 0x00000000U */
    uint32_t HSICalibrationValue;
    uint32_t LSIState; /* real: RCC_LSI_OFF = 0x00000000U */
    uint32_t MSIState; /* real: RCC_MSI_OFF = 0x00000000U */
    uint32_t MSICalibrationValue;
    uint32_t MSIClockRange;
    uint32_t HSI48State; /* real: RCC_HSI48_OFF = 0x00000000U */
    RCC_PLLInitTypeDef PLL;
} RCC_OscInitTypeDef;

typedef struct {
    uint32_t ClockType;
    uint32_t SYSCLKSource;   /* real: RCC_SYSCLKSOURCE_PLLCLK = 0x00000003U */
    uint32_t AHBCLKDivider;  /* real: RCC_SYSCLK_DIV1 = 0x00000000U */
    uint32_t APB1CLKDivider; /* real: RCC_HCLK_DIV1 = 0x00000000U */
    uint32_t APB2CLKDivider; /* real: RCC_HCLK_DIV1 = 0x00000000U */
} RCC_ClkInitTypeDef;

/* real: stm32l4xx_hal_rcc.h / stm32l4xx_hal_pwr_ex.h / stm32l4xx_hal_flash.h */
#define RCC_OSCILLATORTYPE_HSE 0x00000001U
#define RCC_HSE_ON 0x00010000U /* RCC_CR_HSEON (bit 16) */
#define RCC_PLL_ON 0x00000002U
#define RCC_PLLSOURCE_HSE 0x00000003U /* RCC_PLLCFGR_PLLSRC_HSE */
#define RCC_PLLP_DIV7 0x00000007U
#define RCC_PLLQ_DIV2 0x00000002U
#define RCC_PLLR_DIV2 0x00000002U
#define RCC_CLOCKTYPE_SYSCLK 0x00000001U
#define RCC_CLOCKTYPE_HCLK 0x00000002U
#define RCC_CLOCKTYPE_PCLK1 0x00000004U
#define RCC_CLOCKTYPE_PCLK2 0x00000008U
#define RCC_SYSCLKSOURCE_PLLCLK 0x00000003U
#define RCC_SYSCLK_DIV1 0x00000000U
#define RCC_HCLK_DIV1 0x00000000U
#define FLASH_LATENCY_4 0x00000004U
#define PWR_REGULATOR_VOLTAGE_SCALE1 0x00000200U /* PWR_CR1_VOS_0 (bit 9) */

/* Counted vendor clock hooks; the board startup owner is the only caller. */
void xy_test_fake_rcc_pwr_enable(void);
#define __HAL_RCC_PWR_CLK_ENABLE() xy_test_fake_rcc_pwr_enable()

HAL_StatusTypeDef HAL_PWREx_ControlVoltageScaling(uint32_t VoltageScaling);
HAL_StatusTypeDef HAL_RCC_OscConfig(RCC_OscInitTypeDef* RCC_OscInitStruct);
HAL_StatusTypeDef HAL_RCC_ClockConfig(RCC_ClkInitTypeDef* RCC_ClkInitStruct, uint32_t FLatency);

#endif /* PANDORA_FAKE_STM32L4XX_HAL_H */
