#!/usr/bin/env python3
"""Fail-closed contract for the Pandora STM32L475VE smoke target."""
import hashlib
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))
from source_scan import code_only  # noqa: E402
BOARD = ROOT / "boards" / "pandora_stm32l475"
RECORD = ROOT / "docs" / "validation" / "xinyi-pandora-stm32l475-board-smoke-record.md"
EVIDENCE = ROOT / "docs" / "validation" / "evidence" / "pandora-stm32l475" / "2026-09-03"
FIRMWARE_COMMIT = "9b50ec38c4e32f9e37c93a0f3f70379453c9622f"
UART_LOG_SHA256 = "93d4dc22669b26b8b666f4bc4d25968f9b2aa02959968f476fbfe4191730a658"
METADATA_SHA256 = "54f357183833ace492f2382e591fda3e4f75dc90002d625c7ae4d0f676652532"
KEY0_LOG_SHA256 = "57e69784ce8a436f969fd9562826cf667aca34c3561523a99544e8e33b720f1b"
SYS_EVIDENCE = ROOT / "docs" / "validation" / "evidence" / "pandora-stm32l475" / "2026-09-04"
SYS_LOG_SHA256 = "15bf255077d82b0aeec1496a9d0e38e11ff240ceb7131f8173857cc3b9b6ae9a"
SYS_METADATA_SHA256 = "a0a1edf0e0f529c6121748f1b752be4029c4addf695e88e4fa86200b791f2099"
SYS_FIRMWARE_COMMIT = "28f4b21dc06e2189145dadc67e78340f9a22be90"
DEVICE_FIRMWARE_COMMIT = "b94fc3c2161042603b7c02dd055f86caf21ed36b"
DEVICE_LOG_SHA256 = "3b00c2be1fe95dab4c4de21902ffc38c06d70fdc8f76599232d6bce9ccb0972c"
DEVICE_METADATA_SHA256 = "118446a3d8894781b609915ce231c73e9362142fb0f284b07010ba45a0971ebc"
WATCHDOG_EVIDENCE = ROOT / "docs" / "validation" / "evidence" / "pandora-stm32l475" / "2026-09-06"
WATCHDOG_LOG_SHA256 = "01841e3fb69ca9c7e1ca588da2caed431a5ef660ff73f6c643ca2efeab142b85"
WATCHDOG_FIRMWARE_COMMIT = "12bf990f2488d3810575781e2ce0d92890047d02"
EXTERNAL_RESET_LOG_SHA256 = "bf244d5557a6834108e97c712c8015cf8f6da0d3d6972f8db01adb2bc8a140e8"
EXTERNAL_RESET_FIRMWARE_COMMIT = "960f68b0fc9e97d26c4bb13720af439513639a92"


def require(path: Path, *needles: str) -> None:
    assert path.is_file(), f"missing {path.relative_to(ROOT)}"
    text = path.read_text(encoding="utf-8")
    for needle in needles:
        assert needle in text, f"{path.relative_to(ROOT)} missing {needle!r}"


def function_body(text: str, signature: str) -> str:
    """Return the brace-balanced body of the function starting at `signature`."""
    start = text.index(signature)
    open_brace = text.index("{", start)
    depth = 0
    for offset in range(open_brace, len(text)):
        if text[offset] == "{":
            depth += 1
        elif text[offset] == "}":
            depth -= 1
            if depth == 0:
                return text[open_brace : offset + 1]
    raise AssertionError(f"unbalanced braces after {signature!r}")


def board_target_sources(cmake_text: str) -> dict[str, set[str]]:
    """Map each board `add_executable()` target to the source paths it lists."""
    targets: dict[str, set[str]] = {}
    current: str | None = None
    for line in cmake_text.splitlines():
        stripped = line.strip()
        if stripped.startswith("add_executable("):
            tokens = stripped[len("add_executable("):].replace(")", " ").split()
            current = tokens[0] if tokens else None
            if current is not None:
                targets[current] = set(tokens[1:])
            continue
        if current is None:
            continue
        if stripped.startswith(")"):
            current = None
            continue
        for token in stripped.replace(")", " ").split():
            if token.endswith(".c") or token.endswith(".s"):
                targets[current].add(token)
    return targets


def require_sha256(path: Path, expected: str, normalized_expected: str | None = None) -> bytes:
    assert path.is_file(), f"missing {path.relative_to(ROOT)}"
    content = path.read_bytes()
    actual = hashlib.sha256(content).hexdigest()
    if actual != expected:
        normalized = content.replace(b"\r\n", b"\n")
        normalized_actual = hashlib.sha256(normalized).hexdigest()
        assert normalized_expected is not None and normalized_actual == normalized_expected, (
            f"{path.relative_to(ROOT)} SHA-256 mismatch: raw={actual} normalized={normalized_actual}"
        )
    return content


def main() -> None:
    require(
        ROOT / "cmake" / "platform" / "STM32L4.cmake",
        "STM32L475xx",
        "STM32L4_BOARD",
    )
    require(
        BOARD / "CMakeLists.txt",
        "pandora_stm32l475_smoke",
        "pandora_stm32l475_icm20608",
        "pandora_stm32l475_aht10",
        "pandora_aht10_owner.c",
        "pandora_icm20608_owner.c",
        "pandora_ap3216c_owner.c",
        "pandora_sys.c",
        "pandora_platform_startup.c",
        "stm32l4xx_hal_iwdg.c",
        "pandora_fota_flash.c",
        "xy_fota_metadata.c",
        'string(LENGTH "${PANDORA_FOTA_RESTAGE_SOURCE_COMMIT}" _restage_commit_length)',
        "startup_stm32l475xx.s",
        "STM32L475VETX_FLASH.ld",
        "-O binary",
    )
    require(
        BOARD / "stm32l4xx_hal_conf.h",
        "HAL_IWDG_MODULE_ENABLED",
        "stm32l4xx_hal_iwdg.h",
    )
    require(
        BOARD / "capture_uart.py",
        "NO_DATA_TIMEOUT",
        "DEVICE_OPEN_FAILED",
        "CAPTURE_CONTENT_MISMATCH",
        "PANDORA STM32L475VE XINYI SMOKE OK",
        'default="/dev/ttyACM0"',
        "--firmware-commit",
        "firmware_commit",
        "firmware_commit_marker_matched",
        "bytes_captured",
    )
    require(
        BOARD / "pandora_sys.c",
        "xy_sys_init",
        "xy_sys_reset",
        "xy_sys_reboot_reason",
        "xy_sys_reset_request",
        "xy_sys_get_chip_id",
        "xy_hal_sys_get_reset_reason",
        "xy_hal_sys_clear_reset_reason",
        "xy_hal_sys_get_unique_id",
        "xy_hal_sys_software_reset",
        "xy_hal_rtc_backup_read",
        "xy_hal_rtc_backup_write",
    )
    pandora_sys = (BOARD / "pandora_sys.c").read_text(encoding="utf-8")
    # Scan the code, not the prose: a comment naming a forbidden call (usually to explain
    # why it is not used) is evidence the boundary holds, not a violation.
    pandora_sys_code = code_only(pandora_sys)
    for forbidden in ("NVIC_SystemReset", "HAL_GetUID", "__HAL_RCC_", "RCC->"):
        assert forbidden not in code_only(pandora_sys_code), (
            f"pandora_sys.c bypasses canonical SYS HAL: {forbidden}"
        )
    require(
        ROOT / "components" / "hal" / "stm32" / "stm32l4" / "xy_hal_sys.c",
        "xy_hal_sys_deinit",
        "HAL_DeInit",
        "xy_hal_sys_get_reset_reason",
        "xy_hal_sys_clear_reset_reason",
        "xy_hal_sys_get_unique_id",
        "xy_hal_sys_software_reset",
        "NVIC_SystemReset",
        "HAL_GetUIDw0",
        "RCC->CSR",
    )
    fota_bootloader = (BOARD / "fota_bootloader_main.c").read_text(encoding="utf-8")
    assert "xy_hal_sys_deinit()" in fota_bootloader
    assert "HAL_DeInit(" not in fota_bootloader
    require(
        ROOT / "components" / "hal" / "stm32" / "stm32l4" / "xy_hal_delay.c",
        "xy_hal_delay_ms",
        "xy_hal_delay_us",
        "HAL_Delay",
    )
    require(
        ROOT / "components" / "hal" / "stm32" / "stm32l4" / "xy_hal_wdg.c",
        "xy_hal_iwdg_init",
        "xy_hal_iwdg_feed",
        "HAL_IWDG_Init",
        "HAL_IWDG_Refresh",
    )
    require(
        BOARD / "pandora_fota_flash.c",
        "PANDORA_FOTA_METADATA_BASE",
        "xy_hal_flash_write",
        "xy_hal_flash_erase",
        "pandora_fota_metadata_backend",
    )
    for flash_owner in ("pandora_fota_flash.c", "pandora_fota_install_flash.c"):
        source = (BOARD / flash_owner).read_text(encoding="utf-8")
        for forbidden in ("HAL_FLASH_", "HAL_FLASHEx_", "FLASH_TYPEPROGRAM_", "FLASH_TYPEERASE_"):
            assert forbidden not in code_only(source), f"{flash_owner} bypasses canonical Flash HAL: {forbidden}"
    require(
        BOARD / "pandora_fota_install_flash.c",
        "PANDORA_FOTA_INSTALL_PROGRAM_SIZE",
        "pandora_fota_install_ops",
        "pandora_fota_application_vectors_valid",
        "pandora_fota_application_vectors_check",
    )
    require(
        BOARD / "pandora_fota_boot_flow.c",
        "pandora_fota_boot_flow_run",
        "PANDORA_FOTA_RTC_SLOT_CONFIRM",
        "PANDORA_FOTA_RTC_SLOT_RESTAGE",
        "PANDORA_FOTA_CONFIRM_REQUEST_MAGIC",
        "PANDORA_BOOT_ROLLBACK_REQUIRED",
    )
    require(
        ROOT / "components" / "hal" / "stm32" / "stm32l4" / "xy_hal_flash.c",
        "xy_hal_flash_init",
        "xy_hal_flash_write",
        "xy_hal_flash_erase",
        "HAL_FLASH_Program",
        "HAL_FLASHEx_Erase",
    )
    require(
        BOARD / "pandora_soft_i2c.c",
        "xy_hal_gpio_init",
        "xy_hal_gpio_write",
        "xy_hal_gpio_read",
        "pandora_soft_i2c3_init",
        "soft_i2c_start",
        "soft_i2c_write_byte",
        "soft_i2c_read_byte",
        "soft_i2c_bus_is_owned",
        "soft_i2c_address_probe",
        "soft_i2c_enable_port_clock",
        "pandora_soft_i2c_probe",
        "xy_hal_i2c_master_transmit",
        "xy_hal_i2c_master_receive",
        "xy_hal_i2c_mem_write",
        "xy_hal_i2c_mem_read",
        "xy_hal_i2c_is_device_ready",
        "dev_addr << 1",
    )
    require(
        ROOT / "tests" / "unit" / "hal_component" / "fake_vendor" / "stm32l4xx_hal.h",
        "struct xy_hal_gpio_port",
        "GPIO_AF4_I2C3",
        "__HAL_RCC_I2C3_CLK_ENABLE",
    )
    require(
        ROOT / "tests" / "unit" / "hal_component" / "test_pandora_i2c_owners.c",
        "pandora_soft_i2c3_init",
        "pandora_soft_i2c4_init",
        "xy_hal_i2c_mem_write",
        "xy_hal_i2c_mem_read",
        "xy_hal_i2c_is_device_ready",
        "pandora_hw_i2c3_init",
    )
    soft_i2c = (BOARD / "pandora_soft_i2c.c").read_text(encoding="utf-8")
    for forbidden in ("HAL_GPIO_Init(", "HAL_GPIO_WritePin(", "HAL_GPIO_ReadPin("):
        assert forbidden not in code_only(soft_i2c), f"Pandora software I2C bypasses xy_hal GPIO: {forbidden}"
    delay_entries = (
        "main.c",
        "buzzer_main.c",
        "motor_main.c",
        "rgb_led_main.c",
        "st7789_main.c",
        "sensor_ap3216c_main.c",
        "sensor_icm20608_main.c",
        "sensor_aht10_main.c",
    )
    init_entries = delay_entries + (
        "fota_bootloader_main.c",
        "fota_candidate_programmer_main.c",
        "rtos_main.c",
    )
    for board_entry in init_entries:
        entry = (BOARD / board_entry).read_text(encoding="utf-8")
        assert "xy_hal_sys_init()" in entry, f"{board_entry} missing canonical SYS init"
        assert "HAL_Init(" not in entry, f"{board_entry} bypasses canonical SYS init HAL"
        assert "pandora_platform_startup()" in entry, (
            f"{board_entry} missing board-owned startup policy"
        )
        for forbidden in (
            "HAL_PWREx_ControlVoltageScaling(",
            "HAL_RCC_OscConfig(",
            "HAL_RCC_ClockConfig(",
        ):
            assert forbidden not in code_only(entry), (
                f"{board_entry} bypasses board-owned startup policy: {forbidden}"
            )
    require(
        BOARD / "pandora_platform_startup.c",
        "pandora_platform_startup",
        "HAL_PWREx_ControlVoltageScaling",
        "HAL_RCC_OscConfig",
        "HAL_RCC_ClockConfig",
        "PWR_REGULATOR_VOLTAGE_SCALE1",
        "RCC_PLLSOURCE_HSE",
        "FLASH_LATENCY_4",
    )
    tick_entries = delay_entries + (
        "fota_bootloader_main.c",
        "fota_candidate_programmer_main.c",
        "rtos_handlers.c",
    )
    for board_entry in tick_entries:
        entry = (BOARD / board_entry).read_text(encoding="utf-8")
        assert "xy_hal_sys_tick_irq_handler" in entry, (
            f"{board_entry} missing canonical SysTick dispatch"
        )
        assert "HAL_IncTick(" not in entry, f"{board_entry} owns vendor SysTick dispatch"
    for board_entry in delay_entries[1:]:
        entry = (BOARD / board_entry).read_text(encoding="utf-8")
        for required in (
            "xy_hal_uart_init",
            "xy_hal_uart_send",
            "xy_hal_gpio_init",
            "xy_hal_delay_ms",
        ):
            assert required in entry, f"{board_entry} missing canonical HAL call {required}"
        for forbidden in (
            "HAL_UART_Init(",
            "HAL_UART_Transmit(",
            "HAL_GPIO_Init(",
            "HAL_Delay(",
            "HAL_GetTick(",
        ):
            assert forbidden not in code_only(entry), f"{board_entry} bypasses canonical XinYi HAL: {forbidden}"
    smoke_entry = (BOARD / "main.c").read_text(encoding="utf-8")
    assert "xy_hal_delay_ms" in smoke_entry
    assert "HAL_Delay(" not in smoke_entry
    for sensor_entry in ("sensor_ap3216c_main.c", "sensor_icm20608_main.c"):
        entry = (BOARD / sensor_entry).read_text(encoding="utf-8")
        assert "xy_hal_sys_get_tick_count" in entry
        assert "HAL_GetTick(" not in entry
    for fota_entry in ("fota_bootloader_main.c", "fota_candidate_programmer_main.c"):
        entry = (BOARD / fota_entry).read_text(encoding="utf-8")
        for required in ("xy_hal_uart_init", "xy_hal_uart_send", "xy_hal_gpio_init"):
            assert required in entry, f"{fota_entry} missing canonical HAL call {required}"
        for forbidden in ("HAL_UART_Init(", "HAL_UART_Transmit(", "HAL_GPIO_Init("):
            assert forbidden not in code_only(entry), f"{fota_entry} bypasses canonical XinYi HAL: {forbidden}"
    for primary_entry in ("main.c", "rtos_main.c"):
        entry = (BOARD / primary_entry).read_text(encoding="utf-8")
        for required in (
            "xy_hal_uart_init",
            "xy_hal_uart_send",
            "xy_hal_gpio_init",
            "xy_hal_gpio_toggle",
        ):
            assert required in entry, f"{primary_entry} missing canonical HAL call {required}"
        for forbidden in (
            "HAL_UART_Init(",
            "HAL_UART_Transmit(",
            "HAL_GPIO_Init(",
            "HAL_GPIO_TogglePin(",
        ):
            assert forbidden not in code_only(entry), f"{primary_entry} bypasses canonical XinYi HAL: {forbidden}"
    rtos_entry = (BOARD / "rtos_main.c").read_text(encoding="utf-8")
    assert "uart1.Instance = USART1;" in rtos_entry
    assert "xy_hal_sys_software_reset" in rtos_entry
    assert "NVIC_SystemReset" not in rtos_entry
    # A halted image must be diagnosable. Both halt paths (the FreeRTOS stack-overflow hook
    # and fail()) stop the CPU with interrupts disabled and print nothing, so a wedged board
    # is indistinguishable from a quiet UART; that is how the osal-slow stack overflow went
    # unnoticed across HIL runs. The state words are published before the spin and have to
    # stay non-static so a debugger can read them.
    for declaration in (
        "volatile uint32_t xy_image_failed;",
        "volatile const char *xy_image_failed_task;",
        "volatile const char *xy_image_last_marker;",
    ):
        assert declaration in rtos_entry, f"rtos_main.c missing halt state {declaration!r}"
    overflow_hook = function_body(rtos_entry, "void vApplicationStackOverflowHook(")
    assert "xy_image_failed_task = task_name;" in overflow_hook
    assert "xy_image_failed = 1U;" in overflow_hook
    assert overflow_hook.index("xy_image_failed = 1U;") < overflow_hook.index("for (;;)")
    # fail() is a macro over fail_at(__LINE__) now, so the halt state lives in
    # fail_at(); the line number is what makes a halted image identifiable.
    fail_body = function_body(rtos_entry, "static void fail_at(")
    assert "xy_image_failed = 2U;" in fail_body
    assert fail_body.index("xy_image_failed = 2U;") < fail_body.index("for (;;)")
    uart_body = function_body(rtos_entry, "static void uart_text(")
    assert "xy_image_last_marker = text;" in uart_body
    for backup_entry in ("rtos_main.c", "fota_bootloader_main.c"):
        entry = (BOARD / backup_entry).read_text(encoding="utf-8")
        for required in ("xy_hal_rtc_backup_read", "xy_hal_rtc_backup_write"):
            assert required in entry, f"{backup_entry} missing canonical RTC backup call {required}"
        for forbidden in ("HAL_PWR_EnableBkUpAccess", "RTC->BKP"):
            assert forbidden not in code_only(entry), f"{backup_entry} bypasses canonical RTC HAL: {forbidden}"
    require(
        ROOT / "components" / "hal" / "stm32" / "stm32l4" / "xy_hal_rtc.c",
        "xy_hal_rtc_backup_read",
        "xy_hal_rtc_backup_write",
        "HAL_PWR_EnableBkUpAccess",
        "HAL_RTCEx_BKUPRead",
        "HAL_RTCEx_BKUPWrite",
    )
    require(
        ROOT / "components" / "hal" / "stm32" / "stm32l4" / "xy_hal_uart.c",
        "xy_hal_uart_init",
        "xy_hal_uart_send",
        "HAL_UART_Init",
        "HAL_UART_Transmit",
    )
    require(
        BOARD / "pandora_hw_i2c.c",
        "I2C3",
        "xy_hal_gpio_init(GPIOC, 0U",
        "xy_hal_gpio_init(GPIOC, 1U",
        "GPIO_AF4_I2C3",
        "xy_hal_i2c_init",
    )
    require(
        ROOT / "components" / "hal" / "stm32" / "stm32l4" / "xy_hal_i2c.c",
        "HAL_I2C_Init",
        "HAL_I2C_Master_Transmit",
        "HAL_I2C_Master_Receive",
        "dev_addr << 1",
    )
    hal_cmake = (ROOT / "components" / "hal" / "CMakeLists.txt").read_text(encoding="utf-8")
    assert 'stm32/stm32l4/xy_hal_i2c.c' in hal_cmake
    assert 'stm32/stm32l4/xy_hal_wdg.c' in hal_cmake
    board_cmake = (BOARD / "CMakeLists.txt").read_text(encoding="utf-8")
    assert "components/hal/stm32/stm32f4/xy_hal_wdg.c" not in board_cmake
    assert "components/hal/stm32/stm32l4/xy_hal_wdg.c" in board_cmake
    # The software-I2C owner now defines the same xy_hal_i2c_* symbols as the
    # handle-based STM32L4 backend, so a board target must never compile both.
    for target, sources in board_target_sources(board_cmake).items():
        soft_owner = any(source.endswith("pandora_soft_i2c.c") for source in sources)
        handle_backend = any(source.endswith("stm32l4/xy_hal_i2c.c") for source in sources)
        assert not (soft_owner and handle_backend), (
            f"{target} compiles both pandora_soft_i2c.c and the STM32L4 handle-based "
            "I2C backend; the xy_hal_i2c_* definitions would collide"
        )
    board_i2c = (BOARD / "pandora_hw_i2c.c").read_text(encoding="utf-8")
    assert "HAL_I2C_Master_Transmit" not in board_i2c
    assert "HAL_I2C_Master_Receive" not in board_i2c
    assert "xy_hal_gpio_init" in board_i2c
    assert "HAL_GPIO_Init(" not in board_i2c
    require(
        BOARD / "sensor_ap3216c_main.c",
        "AP3216C_BUS=HW_I2C3",
        "pandora_hw_i2c3_init",
        "PANDORA_AP3216C_HW_I2C_INIT_ERROR",
        "AP3216C_HW_I2C_READY",
        "AP3216C_NACK_OBSERVED ADDR=0x7F",
        "PANDORA_AP3216C_NACK_EXPECTED_ERROR",
        "AP3216C_NACK_RECOVERED ADDR=0x1E",
        "pandora_ap3216c_owner_init",
        "pandora_ap3216c_owner_sample",
        "pandora_ap3216c_owner_read_raw",
        "pandora_ap3216c_owner_read_reg",
        "XY_AP3216C_MODE_ALS_PS",
    )
    require(
        BOARD / "pandora_ap3216c_owner.c",
        "pandora_ap3216c_owner_init",
        "xy_ap3216c_init",
        "XY_AP3216C_MODE_ALS_PS",
        "xy_ap3216c_read_light",
        "xy_ap3216c_read_proximity",
        "xy_ap3216c_read_ir",
    )
    require(
        BOARD / "sensor_icm20608_main.c",
        "PANDORA ICM20608 SENSOR READY",
        "I2C3_PINS SCL=PC0 SDA=PC1",
        "PANDORA_I2C3_INIT_DONE",
        "I2C3_ACK=0x",
        "PANDORA_I2C3_SCAN_DONE count=",
        "pandora_icm20608_owner_init",
        "pandora_icm20608_owner_sample",
        "pandora_icm20608_owner_read_raw",
        "pandora_icm20608_owner_read_reg",
        "ICM20608_ADDR=0x68 WHO_AM_I=0xAE",
        "ICM20608 ACCEL_mg=",
        "GYRO_dps=",
        "PANDORA_ICM20608_INIT_ERROR",
        "PANDORA_ICM20608_READ_ERROR",
    )
    require(
        BOARD / "pandora_icm20608_owner.c",
        "pandora_icm20608_owner_init",
        "xy_icm20608_init_i2c",
        "xy_icm20608_read_sample",
        "XY_ICM20608_REG_ACCEL_XOUT_H",
    )
    require(
        BOARD / "sensor_aht10_main.c",
        "PANDORA AHT10 SENSOR READY",
        "AHT10_BUS=SOFT_I2C4 SCL=PD6 SDA=PC1 ADDR=0x38",
        "PANDORA_I2C4_INIT_DONE",
        "I2C4_ACK=0x",
        "PANDORA_I2C4_SCAN_DONE count=",
        "AHT10_DRIVER_READY ADDR=0x38",
        "pandora_aht10_owner_init",
        "pandora_aht10_owner_sample",
        "AHT10 SAMPLE hum_centi=",
        "temp_centi=",
        "PANDORA_AHT10_READ_ERROR",
        "PANDORA_AHT10_RANGE_ERROR",
    )
    # The board image must own U7 through the canonical driver, never through the
    # inline `xy_i2c_device_*` protocol copy that `main.c` still carries.
    require(
        BOARD / "pandora_aht10_owner.c",
        "pandora_aht10_owner_init",
        "pandora_aht10_owner_sample",
        "pandora_aht10_owner_deinit",
        "xy_aht10_init",
        "xy_aht10_read",
        "XY_AHT10_DEFAULT_ADDRESS",
    )
    aht10_entry = (BOARD / "sensor_aht10_main.c").read_text(encoding="utf-8")
    for forbidden in ("0xE1U", "0xACU", "xy_i2c_device_init", "xy_i2c_device_read",
                      "xy_i2c_device_write", "50000U", "200000U"):
        assert forbidden not in code_only(aht10_entry), (
            f"sensor_aht10_main.c re-implements the AHT10 protocol instead of using "
            f"the canonical driver: {forbidden}"
        )
    # The validation images own their devices through the canonical drivers in
    # `components/drivers/sensor/**`, never through the legacy `sensor_*`
    # compatibility wrappers in `components/sensor/sensors`.
    for sensor_owner in (
        "sensor_ap3216c_main.c",
        "sensor_icm20608_main.c",
        "sensor_aht10_main.c",
        "pandora_ap3216c_owner.c",
        "pandora_icm20608_owner.c",
        "pandora_aht10_owner.c",
    ):
        source = (BOARD / sensor_owner).read_text(encoding="utf-8")
        for forbidden in (
            "sensor_core.h",
            "sensor_device_t",
            "sensor_data_t",
            "sensor_err_t",
            "ap3216c_create_",
            "icm20608_create_",
            "icm20608_priv_t",
            "SENSOR_EOK",
        ):
            assert forbidden not in code_only(source), (
                f"{sensor_owner} still depends on the legacy sensor wrapper: {forbidden}"
            )
    board_cmake_text = (BOARD / "CMakeLists.txt").read_text(encoding="utf-8")
    for legacy_wrapper in ("sensor_ap3216c.c", "sensor_icm20608.c"):
        assert legacy_wrapper not in board_cmake_text, (
            f"board CMakeLists.txt still links the legacy wrapper {legacy_wrapper}"
        )
    require(
        BOARD / "main.c",
        "xy_hal_gpio_init",
        "xy_hal_gpio_read",
        "xy_hal_gpio_toggle",
        "xy_hal_uart_init",
        "xy_hal_uart_send",
        "xy_i2c_device_init",
        "xy_i2c_device_write",
        "xy_i2c_device_read",
        "aht10_init",
        "aht10_measure",
        "0xACU",
        "0x33U",
        "humidity_milli_percent",
        "temperature_milli_c",
        "USART1",
        "uart1.Instance = USART1;",
        "AHT10 0x38 ACK",
        "FIRMWARE_COMMIT",
        "XINYI_FIRMWARE_COMMIT",
        "KEY0",
        "PANDORA STM32L475VE",
        "xy_sys_init",
        "xy_sys_reboot_reason",
        "xy_sys_get_chip_id",
        "SYS_RESET_CSR",
        "SYS_RESET_FLAGS",
        "SYS_CHIP_ID",
        "pandora_sys_reset_kind",
        "pandora_reset_kind_name",
        "SYS_RESET_KIND ",
        "SYS_SOFTWARE_RESET_REQUEST",
        "SYS_SOFTWARE_RESET_OK",
        "SYS_EXTERNAL_PIN_RESET_OK",
        "SYS_WATCHDOG_RESET_REQUEST",
        "SYS_WATCHDOG_RESET_OK",
        "SYS_RESET_KIND_UNRECOGNIZED",
        "xy_hal_iwdg_init",
        "xy_fota_metadata_flash_validate",
        "xy_fota_metadata_flash_commit",
        "xy_fota_metadata_flash_load",
        "xy_fota_metadata_boot_handoff",
        "xy_fota_metadata_boot_attempt",
        "xy_fota_metadata_boot_confirm",
        "FOTA_FIRMWARE_COMMIT",
        "FOTA_METADATA_INITIALIZED",
        "FOTA_BOOT_HANDOFF_COMMITTED",
        "FOTA_BOOT_ATTEMPT_COMMITTED",
        "FOTA_BOOT_CONFIRM_COMMITTED",
        "FOTA_BOOT_CONTRACT_OK",
        "FOTA_ANTI_ROLLBACK_REJECTED",
        "FOTA_ROLLBACK_HANDOFF_COMMITTED",
        "FOTA_ROLLBACK_ATTEMPT_COMMITTED",
        "FOTA_AUTOMATIC_ROLLBACK_COMMITTED",
        "FOTA_METADATA_FLASH_OK",
    )
    smoke_entry = (BOARD / "main.c").read_text(encoding="utf-8")
    assert "HAL_IWDG_Init(" not in smoke_entry
    # Reset-cause attribution is a contract with host coverage in pandora_sys.c.
    # An inline bit chain in the entry point is how a brownout boot got reported
    # as POWER_ON, so keep the decision out of main.c entirely. Scan the code:
    # a comment that names a reason bit is not a decision.
    smoke_code = code_only(smoke_entry)
    assert "XY_HAL_SYS_RESET_REASON_" not in smoke_code, (
        "main.c attributes the reset cause inline; dispatch on pandora_sys_reset_kind()"
    )
    require(
        BOARD / "pandora_sys.c",
        "pandora_reset_kind_from_reason",
        "PANDORA_RESET_KIND_UNKNOWN",
        "XY_HAL_SYS_RESET_REASON_EXTERNAL_PIN",
        "XY_HAL_SYS_RESET_REASON_BROWNOUT",
        "XY_HAL_SYS_RESET_REASON_SOFTWARE",
        "XY_HAL_SYS_RESET_REASON_WATCHDOG",
        "XY_HAL_SYS_RESET_REASON_LOW_POWER",
        '"POWER_ON"',
        '"EXTERNAL_PIN"',
        '"BROWNOUT"',
        '"SOFTWARE"',
        '"WATCHDOG"',
        '"LOW_POWER"',
        '"UNKNOWN"',
    )
    require(
        BOARD / "STM32L475VETX_FLASH.ld",
        "ORIGIN = 0x08000000",
        "LENGTH = 504K",
        "FOTA_METADATA (rx) : ORIGIN = 0x0807E000, LENGTH = 4K",
        "BOARD_RESERVED (rx) : ORIGIN = 0x0807F000, LENGTH = 4K",
        "ORIGIN = 0x20000000",
        "LENGTH = 96K",
    )
    require(ROOT / "CMakeLists.txt", "STM32L4_BOARD", "add_subdirectory(boards/pandora_stm32l475)")
    require(
        RECORD,
        "B1_BOARD_SMOKE_VERIFIED",
        "B2_PENDING",
        FIRMWARE_COMMIT,
        "0483:374b",
        "V2J24S11",
        "Flash written and verified",
        "CAPTURE_BYTES=0",
        "BOARD_RUNTIME_PENDING",
        UART_LOG_SHA256,
        METADATA_SHA256,
        KEY0_LOG_SHA256,
    )
    uart_log = require_sha256(
        EVIDENCE / "uart-wchlink-b1.txt",
        UART_LOG_SHA256,
        "bd9247df2798ad2550270b58e33a97bfd936abb6d8632a15cc850fea5a4a04bc",
    ).decode("utf-8")
    metadata_bytes = require_sha256(EVIDENCE / "uart-wchlink-b1.json", METADATA_SHA256)
    metadata = json.loads(metadata_bytes)
    assert metadata["device"].startswith("/dev/serial/by-id/usb-wch.cn_WCH-Link_")
    assert metadata["firmware_commit"] == FIRMWARE_COMMIT
    assert metadata["firmware_commit_marker_matched"] is True
    assert metadata["status"] == "CAPTURED"
    assert metadata["runtime_evidence"] == "B1_REVIEW_CANDIDATE"
    assert metadata["bytes_captured"] == 1560
    assert uart_log.count("PANDORA STM32L475VE XINYI SMOKE OK") == 10
    assert uart_log.count(f"FIRMWARE_COMMIT {FIRMWARE_COMMIT}") == 10
    assert uart_log.count("AHT10 0x38 ACK") == 10
    assert uart_log.count("AHT10 RH_milli_percent=") == 10
    assert "KEY0" not in uart_log
    assert "AHT10 0x38 NACK" not in uart_log
    key0_log = require_sha256(
        EVIDENCE / "uart-wchlink-key0.txt",
        KEY0_LOG_SHA256,
        "d98119206ff7bffb1296932fc90f30ec8323a186265fc79f047005a5434976d3",
    ).decode("utf-8")
    assert key0_log.count("KEY0") == 4
    assert key0_log.count(f"FIRMWARE_COMMIT {FIRMWARE_COMMIT}") == 13
    assert "AHT10 0x38 NACK" not in key0_log
    sys_log = require_sha256(
        SYS_EVIDENCE / "uart-wchlink-sys-reset-28f4b21d.txt", SYS_LOG_SHA256
    ).decode("utf-8")
    sys_metadata = json.loads(
        require_sha256(
            SYS_EVIDENCE / "uart-wchlink-sys-reset-28f4b21d.json", SYS_METADATA_SHA256
        )
    )
    assert sys_metadata["firmware_commit"] == SYS_FIRMWARE_COMMIT
    assert sys_metadata["firmware_commit_marker_matched"] is True
    assert sys_metadata["status"] == "CAPTURED"
    assert sys_log.count("SYS_SOFTWARE_RESET_REQUEST") == 1
    assert sys_log.count("SYS_SOFTWARE_RESET_OK") == 1
    assert sys_log.count("SYS_RESET_KIND SOFTWARE") == 1
    assert sys_log.count("SYS_CHIP_ID 001B002E3647501320313556") == 2
    assert sys_log.count(f"FIRMWARE_COMMIT {SYS_FIRMWARE_COMMIT}") == 2

    device_log = require_sha256(
        SYS_EVIDENCE / "uart-wchlink-device-aht10-b94fc3c2.txt", DEVICE_LOG_SHA256
    ).decode("utf-8")
    device_metadata = json.loads(
        require_sha256(
            SYS_EVIDENCE / "uart-wchlink-device-aht10-b94fc3c2.json", DEVICE_METADATA_SHA256
        )
    )
    assert device_metadata["firmware_commit"] == DEVICE_FIRMWARE_COMMIT
    assert device_metadata["firmware_commit_marker_matched"] is True
    assert device_metadata["runtime_evidence"] == "B1_REVIEW_CANDIDATE"
    assert device_metadata["bytes_captured"] == 2355
    assert device_log.count(f"FIRMWARE_COMMIT {DEVICE_FIRMWARE_COMMIT}") == 14
    assert device_log.count("AHT10 0x38 ACK") == 13
    assert device_log.count("AHT10 RH_milli_percent=") == 13
    assert "AHT10 0x38 NACK" not in device_log

    watchdog_log = require_sha256(
        WATCHDOG_EVIDENCE / "uart-wchlink-sys-watchdog-12bf990f.txt",
        WATCHDOG_LOG_SHA256,
    ).decode("utf-8")
    watchdog_metadata = json.loads(
        (WATCHDOG_EVIDENCE / "sys-watchdog-12bf990f.json").read_text(encoding="utf-8")
    )
    assert watchdog_metadata["status"] == "WATCHDOG_RESET_B2_REVIEW_CANDIDATE"
    assert watchdog_metadata["firmware_commit"] == WATCHDOG_FIRMWARE_COMMIT
    assert watchdog_metadata["capture_sha256"] == WATCHDOG_LOG_SHA256
    assert watchdog_metadata["flash_readback_sha256"] == watchdog_metadata["firmware_bin_sha256"]
    assert watchdog_log.count("SYS_RESET_KIND WATCHDOG") == 1
    assert watchdog_log.count("SYS_WATCHDOG_RESET_OK") == 1
    assert watchdog_log.count("SYS_CHIP_ID 001B002E3647501320313556") == 1
    assert watchdog_log.count(f"FIRMWARE_COMMIT {WATCHDOG_FIRMWARE_COMMIT}") >= 1
    assert watchdog_log.count("AHT10 0x38 ACK") == 8

    external_reset_log = require_sha256(
        WATCHDOG_EVIDENCE / "uart-wchlink-sys-external-reset-960f68b0.txt",
        EXTERNAL_RESET_LOG_SHA256,
    ).decode("utf-8")
    external_reset_metadata = json.loads(
        (WATCHDOG_EVIDENCE / "sys-external-reset-960f68b0.json").read_text(encoding="utf-8")
    )
    assert external_reset_metadata["status"] == "EXTERNAL_RESET_B2_REVIEW_CANDIDATE"
    assert external_reset_metadata["firmware_commit"] == EXTERNAL_RESET_FIRMWARE_COMMIT
    assert external_reset_metadata["capture_sha256"] == EXTERNAL_RESET_LOG_SHA256
    assert (
        external_reset_metadata["flash_readback_sha256"]
        == external_reset_metadata["firmware_bin_sha256"]
    )
    assert external_reset_log.count("SYS_RESET_KIND EXTERNAL_PIN") == 1
    assert external_reset_log.count("SYS_EXTERNAL_PIN_RESET_OK") == 1
    assert external_reset_log.count("SYS_CHIP_ID 001B002E3647501320313556") == 1
    assert external_reset_log.count(f"FIRMWARE_COMMIT {EXTERNAL_RESET_FIRMWARE_COMMIT}") >= 1
    assert external_reset_log.count("AHT10 0x38 ACK") == 15
    print("Pandora STM32L475VE board contract: PASS")


if __name__ == "__main__":
    main()
