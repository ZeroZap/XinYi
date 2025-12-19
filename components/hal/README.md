# XY HAL (Hardware Abstraction Layer)

## Overview

The XY HAL provides a unified, platform-independent interface for common embedded peripherals. This abstraction layer allows application code to be portable across different microcontroller platforms.

## Supported Platforms

- **STM32**: Full implementation for STM32 series microcontrollers
- **HC32**: Placeholder (can be implemented)
- **Linux**: Placeholder (can be implemented for testing)
- **Win32**: Simulation layer for development

## Supported Peripherals

### GPIO/Pin (`xy_hal_pin.h`)
- Digital input/output
- Configurable pull-up/pull-down
- Output type (push-pull/open-drain)
- Speed configuration
- External interrupt support with callbacks

### UART (`xy_hal_uart.h`)
- Configurable baudrate, word length, stop bits, parity
- Hardware flow control (RTS/CTS)
- Blocking and DMA-based transfers
- Callback support for events

### SPI (`xy_hal_spi.h`)
- Master/Slave mode
- 4 SPI modes (CPOL/CPHA)
- 8-bit and 16-bit data size
- Software and hardware NSS
- Blocking and DMA-based transfers

### I2C (`xy_hal_i2c.h`)
- Master mode operation
- 7-bit and 10-bit addressing
- Standard (100kHz) and Fast (400kHz) modes
- Memory read/write operations
- Device ready check

### Timer (`xy_hal_timer.h`)
- Multiple count modes (up, down, center-aligned)
- Configurable prescaler and period
- Interrupt support
- Capture/Compare support

### PWM (`xy_hal_pwm.h`)
- Up to 4 channels per timer
- Configurable frequency and duty cycle
- Polarity control
- Duty cycle range: 0.00% - 100.00% (0-10000)

### RTC (Real-Time Clock) (`xy_hal_rtc.h`)
- Time and date management
- Alarm support (Alarm A and B)
- Unix timestamp conversion
- BCD and binary formats

### DMA (`xy_hal_dma.h`)
- Memory-to-memory, memory-to-peripheral, peripheral-to-memory
- Normal and circular modes
- Configurable priority levels
- Multiple data widths (8/16/32-bit)

### Low Power Timer (`xy_hal_lp_timer.h`)
- Low power timer for battery-powered applications
- Configurable clock source and prescaler
- Suitable for sleep mode timing

## Usage

### Building for STM32

1. Define the platform macro in your build system:
   ```c
   #define STM32_HAL_ENABLED
   #define STM32F4  // or STM32F1, STM32L4, etc.
   ```

2. Include the HAL header in your application:
   ```c
   #include "xy_hal.h"
   ```

3. Link the appropriate STM32 implementation files from `bsp/xy_hal/stm32/`

### Example: GPIO Usage

```c
#include "xy_hal_pin.h"

void setup_led(void)
{
    xy_hal_pin_config_t config = {
        .mode = XY_HAL_PIN_MODE_OUTPUT,
        .pull = XY_HAL_PIN_PULL_NONE,
        .otype = XY_HAL_PIN_OTYPE_PP,
        .speed = XY_HAL_PIN_SPEED_LOW,
    };

    xy_hal_pin_init(GPIOA, 5, &config);  // PA5 as output
}

void toggle_led(void)
{
    xy_hal_pin_toggle(GPIOA, 5);
}
```

### Example: UART Usage

```c
#include "xy_hal_uart.h"

UART_HandleTypeDef huart1;

void setup_uart(void)
{
    xy_hal_uart_config_t config = {
        .baudrate = 115200,
        .wordlen = XY_HAL_UART_WORDLEN_8B,
        .stopbits = XY_HAL_UART_STOPBITS_1,
        .parity = XY_HAL_UART_PARITY_NONE,
        .flowctrl = XY_HAL_UART_FLOWCTRL_NONE,
        .mode = XY_HAL_UART_MODE_TX_RX,
    };

    xy_hal_uart_init(&huart1, &config);
}

void send_message(void)
{
    const char *msg = "Hello World!\r\n";
    xy_hal_uart_send(&huart1, (uint8_t *)msg, strlen(msg), 1000);
}
```

### Example: I2C Sensor Reading

```c
#include "xy_hal_i2c.h"

I2C_HandleTypeDef hi2c1;

void read_sensor(void)
{
    uint8_t data[2];
    uint16_t sensor_addr = 0x68;  // Example sensor address
    uint16_t reg_addr = 0x00;      // Example register

    xy_hal_i2c_mem_read(&hi2c1, sensor_addr, reg_addr, data, 2, 1000);
}
```

### Example: PWM LED Dimming

```c
#include "xy_hal_pwm.h"

TIM_HandleTypeDef htim2;

void setup_pwm(void)
{
    xy_hal_pwm_config_t config = {
        .frequency = 1000,        // 1kHz
        .duty_cycle = 5000,       // 50% duty cycle
        .polarity = XY_HAL_PWM_POLARITY_HIGH,
    };

    xy_hal_pwm_init(&htim2, XY_HAL_PWM_CHANNEL_1, &config);
    xy_hal_pwm_start(&htim2, XY_HAL_PWM_CHANNEL_1);
}

void set_brightness(uint8_t percent)
{
    uint32_t duty = (percent * 100);  // Convert 0-100 to 0-10000
    xy_hal_pwm_set_duty_cycle(&htim2, XY_HAL_PWM_CHANNEL_1, duty);
}
```

## Porting to New Platforms

To add support for a new platform:

1. Create a new directory under `bsp/xy_hal/` (e.g., `bsp/xy_hal/esp32/`)
2. Implement all HAL functions for each peripheral with **exact same function names** as in `inc/`
3. Use conditional compilation to enable platform-specific code
4. Follow the naming convention: `xy_hal_<peripheral>.c` (no platform suffix)

For STM32 sub-series:

1. Create a sub-directory under `bsp/xy_hal/stm32/` (e.g., `stm32u0/`, `stm32h7/`)
2. Implement all peripherals: `xy_hal_pin.c`, `xy_hal_uart.c`, etc.
3. Function names must match exactly with `inc/xy_hal_*.h` declarations

Example structure:
```
bsp/xy_hal/
├── inc/              # Platform-independent headers
│   ├── xy_hal.h
│   ├── xy_hal_pin.h
│   └── ...
├── stm32/            # STM32 implementation
│   ├── stm32f4/      # STM32F4 series
│   │   ├── xy_hal_pin.c
│   │   └── ...
│   ├── stm32u0/      # STM32U0 series
│   │   ├── xy_hal_pin.c
│   │   └── ...
│   ├── stm32l4/      # STM32L4 series
│   └── stm32h7/      # STM32H7 series
└── esp32/            # ESP32 implementation (example)
    ├── xy_hal_pin.c
    └── ...
```

## Notes

- All functions return 0 on success, negative values on error
- Callbacks are registered separately and executed in interrupt context
- DMA operations require proper buffer alignment and cache management
- Some features may not be available on all platforms (e.g., LP Timer)

## Platform-Specific Considerations

### STM32
- Requires STM32 HAL library to be included in the project
- GPIO ports are passed as `GPIO_TypeDef *` (e.g., GPIOA, GPIOB)
- UART/SPI/I2C instances are passed as handle pointers
- Timer instances are passed as `TIM_HandleTypeDef *`
- Enable clock for peripherals before initialization

### Future Platforms
- Document platform-specific requirements here

## License

This HAL is part of the XinYi embedded framework project.
