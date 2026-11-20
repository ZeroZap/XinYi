/**
 * @file xy_hal.h
 * @brief HAL main header: the whole peripheral surface behind one include.
 *
 * Each `xy_hal_<peripheral>.h` includes this file for the shared return type, so including a single
 * peripheral header on its own has always worked. Including *this* header is the other direction -
 * it is the HAL's front door - and it exported 8 of the 22 peripherals: a consumer that only did
 * `#include "xy_hal.h"` got GPIO/UART/SPI/QSPI/I2C/ADC/PWM/TIMER and an implicit declaration for
 * everything else (`xy_hal_delay_ms`, `xy_hal_sys_get_tick_count`, the DAC and Flash families).
 *
 * Both directions are now pinned by `tests/unit/hal_component/check_hal_umbrella_coverage.py`: a
 * new peripheral header cannot land under `components/hal/inc/` without being exported here, and an
 * include that names a header which no longer exists is a finding rather than a silent no-op.
 */

#ifndef XY_HAL_H
#define XY_HAL_H

/* The return type every other header below is written against. */
#include "xy_hal_error.h"

#include "xy_hal_adc.h"
#include "xy_hal_can.h"
#include "xy_hal_dac.h"
#include "xy_hal_delay.h"
#include "xy_hal_dma.h"
#include "xy_hal_exti.h"
#include "xy_hal_flash.h"
#include "xy_hal_gpio.h"
#include "xy_hal_i2c.h"
#include "xy_hal_i2s.h"
#include "xy_hal_lp_timer.h"
#include "xy_hal_pwm.h"
#include "xy_hal_qspi.h"
#include "xy_hal_rng.h"
#include "xy_hal_rtc.h"
#include "xy_hal_spi.h"
#include "xy_hal_sys.h"
#include "xy_hal_time.h"
#include "xy_hal_timer.h"
#include "xy_hal_uart.h"
#include "xy_hal_wdg.h"

#endif /* XY_HAL_H */
