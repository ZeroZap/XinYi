#!/usr/bin/env python3
"""Which public HAL entries the PC backend has, pinned.

The PC backend is what host tests and any simulator run against, and it defines a fraction of the
entries the public headers declare - the count is printed on every run rather than quoted here, so it
cannot go stale. That is not a defect by itself: a host backend has no reason to implement a UART's
flow control or a cache maintenance call. It does mean a host test says something about the part it
touches and nothing about the rest, and until this guard existed nothing recorded which part that
was.

So this guard pins the exact set of entries the PC backend does not define. It fails when a family
gains a newly missing entry, and it fails when an entry has been implemented until the pin is
updated: shrinking this list is progress and should be a deliberate edit that shows up in review, not
drift in either direction.

How the counts are taken: declarations are read from the headers through read_code(), so doxygen text
cannot be mistaken for a declaration; definitions are matched as "name(...) {" so that any return
type counts - an earlier version matched a fixed list of return types and reported
xy_hal_gpio_read as absent merely because it returns int32_t. The *_pc helpers are excluded: they are
an older internal interface, not public entries.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))
from source_scan import read_code  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
HAL_INC = ROOT / "components" / "hal" / "inc"
PC = ROOT / "components" / "hal" / "PC"

FAMILIES = {
    "gpio": "xy_hal_gpio", "uart": "xy_hal_uart", "i2c": "xy_hal_i2c",
    "spi": "xy_hal_spi", "adc": "xy_hal_adc", "rtc": "xy_hal_rtc",
    "dma": "xy_hal_dma", "timer": "xy_hal_timer", "pwm": "xy_hal_pwm",
    "flash": "xy_hal_flash", "exti": "xy_hal_exti", "sys": "xy_hal_sys",
}

PREFIXES = "|".join(sorted({prefix.split("_")[2] for prefix in FAMILIES.values()}))
DECLARATION = re.compile(
    rf"^[A-Za-z_][\w \t\*]*?\b(xy_hal_(?:{PREFIXES})_\w+)\s*\(", re.M)
DEFINITION = re.compile(
    rf"\b(xy_hal_(?:{PREFIXES})_\w+)\s*\([^;{{]*?\)\s*\{{", re.S)

PC_MISSING = {
    "adc": {
        "xy_hal_adc_calibrate", "xy_hal_adc_config_channels", "xy_hal_adc_control",
        "xy_hal_adc_deinit", "xy_hal_adc_disable_temp_sensor", "xy_hal_adc_disable_vbat",
        "xy_hal_adc_disable_vrefint", "xy_hal_adc_disable_window_watchdog", "xy_hal_adc_enable_temp_sensor",
        "xy_hal_adc_enable_vbat", "xy_hal_adc_enable_vrefint", "xy_hal_adc_enable_window_watchdog",
        "xy_hal_adc_event_handler", "xy_hal_adc_get_continuous", "xy_hal_adc_get_max_value",
        "xy_hal_adc_get_oversampling", "xy_hal_adc_get_prescaler", "xy_hal_adc_get_resolution",
        "xy_hal_adc_get_sampling_time", "xy_hal_adc_get_state", "xy_hal_adc_get_temp_value",
        "xy_hal_adc_get_trigger", "xy_hal_adc_get_vbat_value", "xy_hal_adc_get_vrefint_value",
        "xy_hal_adc_get_window_threshold", "xy_hal_adc_init", "xy_hal_adc_read",
        "xy_hal_adc_read_dma", "xy_hal_adc_read_multi", "xy_hal_adc_read_nb",
        "xy_hal_adc_register_callback", "xy_hal_adc_set_continuous", "xy_hal_adc_set_oversampling",
        "xy_hal_adc_set_prescaler", "xy_hal_adc_set_sampling_time", "xy_hal_adc_set_trigger",
        "xy_hal_adc_set_window_threshold", "xy_hal_adc_start", "xy_hal_adc_stop",
        "xy_hal_adc_value_to_mv",
    },
    "exti": {
        "xy_hal_exti_clear_pending", "xy_hal_exti_configure", "xy_hal_exti_deinit",
        "xy_hal_exti_disable", "xy_hal_exti_enable", "xy_hal_exti_generate_software_interrupt",
        "xy_hal_exti_get_pending", "xy_hal_exti_init", "xy_hal_exti_irq_handler",
        "xy_hal_exti_map_gpio", "xy_hal_exti_register_callback", "xy_hal_exti_set_pending",
        "xy_hal_exti_unregister_callback",
    },
    "flash": {
        "xy_hal_flash_deinit", "xy_hal_flash_erase", "xy_hal_flash_get_info",
        "xy_hal_flash_get_read_protect", "xy_hal_flash_get_sector_info", "xy_hal_flash_init",
        "xy_hal_flash_is_valid_address", "xy_hal_flash_lock", "xy_hal_flash_mass_erase",
        "xy_hal_flash_read", "xy_hal_flash_set_read_protect", "xy_hal_flash_unlock",
        "xy_hal_flash_write",
    },
    "gpio": {
        "xy_hal_gpio_irq_handler",
    },
    "i2c": {
        "xy_hal_i2c_error", "xy_hal_i2c_is_device_ready", "xy_hal_i2c_master_receive_dma",
        "xy_hal_i2c_master_transmit_dma", "xy_hal_i2c_register_callback",
    },
    "pwm": {
        "xy_hal_pwm_check_fault", "xy_hal_pwm_clear_fault", "xy_hal_pwm_compare_to_duty",
        "xy_hal_pwm_config_encoder_mode", "xy_hal_pwm_config_fault_protection", "xy_hal_pwm_config_pulse_mode",
        "xy_hal_pwm_control", "xy_hal_pwm_deinit", "xy_hal_pwm_disable_complementary",
        "xy_hal_pwm_disable_pulse_mode", "xy_hal_pwm_duty_to_compare", "xy_hal_pwm_enable_complementary",
        "xy_hal_pwm_enable_pulse_mode", "xy_hal_pwm_get_autoreload", "xy_hal_pwm_get_capture_polarity",
        "xy_hal_pwm_get_compare", "xy_hal_pwm_get_deadtime", "xy_hal_pwm_get_duty_cycle",
        "xy_hal_pwm_get_encoder_count", "xy_hal_pwm_get_fault_state", "xy_hal_pwm_get_frequency",
        "xy_hal_pwm_get_mode", "xy_hal_pwm_get_output_state", "xy_hal_pwm_get_period_ns",
        "xy_hal_pwm_get_period_us", "xy_hal_pwm_get_polarity", "xy_hal_pwm_get_prescaler",
        "xy_hal_pwm_get_state", "xy_hal_pwm_get_wave_shape", "xy_hal_pwm_init",
        "xy_hal_pwm_input_capture", "xy_hal_pwm_register_callback", "xy_hal_pwm_reset_encoder_count",
        "xy_hal_pwm_set_autoreload", "xy_hal_pwm_set_capture_polarity", "xy_hal_pwm_set_compare",
        "xy_hal_pwm_set_deadtime", "xy_hal_pwm_set_duty_cycle", "xy_hal_pwm_set_encoder_count",
        "xy_hal_pwm_set_fault_state", "xy_hal_pwm_set_frequency", "xy_hal_pwm_set_mode",
        "xy_hal_pwm_set_output_state", "xy_hal_pwm_set_polarity", "xy_hal_pwm_set_prescaler",
        "xy_hal_pwm_set_wave_shape", "xy_hal_pwm_start", "xy_hal_pwm_stop",
        "xy_hal_pwm_sync_output",
    },
    "rtc": {
        "xy_hal_rtc_deinit", "xy_hal_rtc_disable_alarm", "xy_hal_rtc_enable_alarm",
        "xy_hal_rtc_get_alarm", "xy_hal_rtc_get_date", "xy_hal_rtc_get_time",
        "xy_hal_rtc_get_timestamp", "xy_hal_rtc_init", "xy_hal_rtc_register_callback",
        "xy_hal_rtc_set_alarm", "xy_hal_rtc_set_date", "xy_hal_rtc_set_time",
        "xy_hal_rtc_set_timestamp",
    },
    "spi": {
        "xy_hal_spi_error", "xy_hal_spi_receive_dma", "xy_hal_spi_register_callback",
        "xy_hal_spi_set_cs", "xy_hal_spi_transmit_dma", "xy_hal_spi_transmit_receive_dma",
    },
    "sys": {
        "xy_hal_sys_clear_irq_flag", "xy_hal_sys_clear_reset_reason", "xy_hal_sys_config_clk_domain",
        "xy_hal_sys_dcache_clean", "xy_hal_sys_dcache_clean_invalidate", "xy_hal_sys_dcache_invalidate",
        "xy_hal_sys_deinit", "xy_hal_sys_delay_ms", "xy_hal_sys_delay_us",
        "xy_hal_sys_diff_ms", "xy_hal_sys_diff_us", "xy_hal_sys_disable_fpu",
        "xy_hal_sys_disable_irq", "xy_hal_sys_disable_periph_clock", "xy_hal_sys_elapsed_ms",
        "xy_hal_sys_elapsed_us", "xy_hal_sys_enable_clk_domain", "xy_hal_sys_enable_debug_feature",
        "xy_hal_sys_enable_fpu", "xy_hal_sys_enable_irq", "xy_hal_sys_enable_periph_clock",
        "xy_hal_sys_enable_pwr_domain", "xy_hal_sys_enable_security", "xy_hal_sys_enable_trustzone",
        "xy_hal_sys_enter_critical", "xy_hal_sys_exit_critical", "xy_hal_sys_get_available_ram",
        "xy_hal_sys_get_chip_id", "xy_hal_sys_get_chip_name", "xy_hal_sys_get_chip_serial",
        "xy_hal_sys_get_chip_version", "xy_hal_sys_get_clk_domain_state", "xy_hal_sys_get_clk_freq",
        "xy_hal_sys_get_clk_source", "xy_hal_sys_get_cpu_freq", "xy_hal_sys_get_current_irq",
        "xy_hal_sys_get_debug_feature_state", "xy_hal_sys_get_debug_level", "xy_hal_sys_get_flash_size",
        "xy_hal_sys_get_heap_usage", "xy_hal_sys_get_info", "xy_hal_sys_get_irq_nest_level",
        "xy_hal_sys_get_mem_info", "xy_hal_sys_get_periph_base_addr", "xy_hal_sys_get_periph_clock_freq",
        "xy_hal_sys_get_periph_irq_num", "xy_hal_sys_get_pwr_current", "xy_hal_sys_get_pwr_domain_config",
        "xy_hal_sys_get_pwr_domain_state", "xy_hal_sys_get_pwr_power", "xy_hal_sys_get_pwr_voltage",
        "xy_hal_sys_get_ram_size", "xy_hal_sys_get_reset_raw_flags", "xy_hal_sys_get_reset_reason",
        "xy_hal_sys_get_security_state", "xy_hal_sys_get_stack_usage", "xy_hal_sys_get_timestamp_ms",
        "xy_hal_sys_get_timestamp_us", "xy_hal_sys_get_unique_id", "xy_hal_sys_get_uptime_ms",
        "xy_hal_sys_get_uptime_sec", "xy_hal_sys_icache_invalidate", "xy_hal_sys_in_isr",
        "xy_hal_sys_init", "xy_hal_sys_instruction_barrier", "xy_hal_sys_is_fpu_enabled",
        "xy_hal_sys_is_trustzone_enabled", "xy_hal_sys_memory_barrier", "xy_hal_sys_reset",
        "xy_hal_sys_reset_periph", "xy_hal_sys_restore_irq_state", "xy_hal_sys_save_irq_state",
        "xy_hal_sys_set_clk_freq", "xy_hal_sys_set_clk_source", "xy_hal_sys_set_debug_level",
        "xy_hal_sys_set_pwr_domain_config", "xy_hal_sys_set_pwr_voltage", "xy_hal_sys_software_reset",
        "xy_hal_sys_trigger_software_irq",
    },
    "uart": {
        "xy_hal_uart_available", "xy_hal_uart_enable_rx_irq", "xy_hal_uart_error",
        "xy_hal_uart_flush", "xy_hal_uart_irq_handler", "xy_hal_uart_recv_dma",
        "xy_hal_uart_register_callback", "xy_hal_uart_rx_byte", "xy_hal_uart_send_dma",
        "xy_hal_uart_set_error_cb",
    },
}


def measured(inc: Path | None = None, pc: Path | None = None) -> dict[str, set[str]]:
    inc = HAL_INC if inc is None else inc
    pc = PC if pc is None else pc
    declared: dict[str, set[str]] = {}
    for family, prefix in FAMILIES.items():
        header = inc / f"{prefix}.h"
        if not header.exists():
            continue
        names = set(DECLARATION.findall(read_code(header)))
        if names:
            declared[family] = names
    source = "\n".join(read_code(path) for path in sorted(pc.rglob("*.c")))
    defined = {name for name in DEFINITION.findall(source) if not name.endswith("_pc")}
    return {family: {name for name in names if name not in defined}
            for family, names in declared.items()}


def findings(inc: Path | None = None, pc: Path | None = None,
             pinned: dict | None = None) -> list[str]:
    pinned = PC_MISSING if pinned is None else pinned
    actual = measured(inc, pc)
    problems: list[str] = []
    for family in sorted(actual):
        known = set(pinned.get(family, set()))
        added = sorted(actual[family] - known)
        if added:
            problems.append(
                f"{family}: {len(added)} newly missing on the PC backend ({', '.join(added[:5])}"
                f"{' …' if len(added) > 5 else ''}) - either an entry was dropped, or the pin needs "
                f"an explicit update")
        closed = sorted(known - actual[family])
        if closed:
            problems.append(
                f"{family}: {len(closed)} entry has been implemented ({', '.join(closed[:5])}"
                f"{' …' if len(closed) > 5 else ''}) - tighten the pinned list so the gain is on "
                f"the record")
    for family in sorted(set(pinned) - set(actual)):
        problems.append(f"{family}: pinned, but no header declares that family")
    return problems


def main() -> int:
    problems = findings()
    if problems:
        print("hal_pc_coverage failed:")
        for problem in problems:
            print(f"- {problem}")
        return 1
    actual = measured()
    total = 0
    for family, prefix in FAMILIES.items():
        header = HAL_INC / f"{prefix}.h"
        if header.exists():
            total += len(set(DECLARATION.findall(read_code(header))))
    missing = sum(len(entries) for entries in actual.values())
    print(f"hal_pc_coverage ok (pc backend defines {total - missing} of {total} public entries; "
          f"{missing} pinned as absent)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
