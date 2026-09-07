#!/usr/bin/env python3
"""Fail-closed contract for the Pandora STM32L475VE smoke target."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
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
        "sensor_icm20608.c",
        "pandora_sys.c",
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
        "xy_sys_get_chip_id",
        "NVIC_SystemReset",
        "HAL_GetUIDw0",
        "__HAL_RCC_CLEAR_RESET_FLAGS",
        "RCC->CSR",
    )
    require(
        BOARD / "pandora_fota_flash.c",
        "PANDORA_FOTA_METADATA_BASE",
        "FLASH_TYPEPROGRAM_DOUBLEWORD",
        "FLASH_TYPEERASE_PAGES",
        "FLASH_BANK_SIZE",
        "FLASH_BANK_2",
        "HAL_FLASHEx_Erase",
        "HAL_FLASH_Program",
        "pandora_fota_metadata_backend",
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
        "pandora_soft_i2c_probe",
        "xy_hal_i2c_master_transmit",
        "xy_hal_i2c_master_receive",
        "dev_addr << 1",
    )
    soft_i2c = (BOARD / "pandora_soft_i2c.c").read_text(encoding="utf-8")
    for forbidden in ("HAL_GPIO_Init(", "HAL_GPIO_WritePin(", "HAL_GPIO_ReadPin("):
        assert forbidden not in soft_i2c, f"Pandora software I2C bypasses xy_hal GPIO: {forbidden}"
    for board_entry in (
        "buzzer_main.c",
        "motor_main.c",
        "rgb_led_main.c",
        "st7789_main.c",
        "sensor_ap3216c_main.c",
        "sensor_icm20608_main.c",
    ):
        entry = (BOARD / board_entry).read_text(encoding="utf-8")
        for required in ("xy_hal_uart_init", "xy_hal_uart_send", "xy_hal_gpio_init"):
            assert required in entry, f"{board_entry} missing canonical HAL call {required}"
        for forbidden in ("HAL_UART_Init(", "HAL_UART_Transmit(", "HAL_GPIO_Init("):
            assert forbidden not in entry, f"{board_entry} bypasses canonical XinYi HAL: {forbidden}"
    for fota_entry in ("fota_bootloader_main.c", "fota_candidate_programmer_main.c"):
        entry = (BOARD / fota_entry).read_text(encoding="utf-8")
        for required in ("xy_hal_uart_init", "xy_hal_uart_send", "xy_hal_gpio_init"):
            assert required in entry, f"{fota_entry} missing canonical HAL call {required}"
        for forbidden in ("HAL_UART_Init(", "HAL_UART_Transmit(", "HAL_GPIO_Init("):
            assert forbidden not in entry, f"{fota_entry} bypasses canonical XinYi HAL: {forbidden}"
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
            assert forbidden not in entry, f"{primary_entry} bypasses canonical XinYi HAL: {forbidden}"
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
        "xy_i2c_device_init",
        "AP3216C_MODE_ALS_PS",
    )
    require(
        BOARD / "sensor_icm20608_main.c",
        "PANDORA ICM20608 SENSOR READY",
        "I2C3_PINS SCL=PC0 SDA=PC1",
        "PANDORA_I2C3_INIT_DONE",
        "I2C3_ACK=0x",
        "PANDORA_I2C3_SCAN_DONE count=",
        "xy_i2c_device_init",
        "xy_i2c_device_read_reg",
        "icm20608_create_accel",
        "icm20608_create_gyro",
        "ICM20608_ADDR=0x68 WHO_AM_I=0xAE",
        "ICM20608 ACCEL_mg=",
        "GYRO_dps=",
        "PANDORA_ICM20608_INIT_ERROR",
        "PANDORA_ICM20608_READ_ERROR",
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
        "AHT10 0x38 ACK",
        "FIRMWARE_COMMIT",
        "XINYI_FIRMWARE_COMMIT",
        "KEY0",
        "PANDORA STM32L475VE",
        "xy_sys_init",
        "xy_sys_reboot_reason",
        "xy_sys_get_chip_id",
        "RCC_CSR_SFTRSTF",
        "SYS_RESET_CSR",
        "SYS_CHIP_ID",
        "SYS_RESET_KIND SOFTWARE",
        "SYS_SOFTWARE_RESET_REQUEST",
        "SYS_SOFTWARE_RESET_OK",
        "RCC_CSR_PINRSTF",
        "RCC_CSR_BORRSTF",
        "SYS_RESET_KIND EXTERNAL_PIN",
        "SYS_EXTERNAL_PIN_RESET_OK",
        "SYS_RESET_KIND POWER_ON",
        "RCC_CSR_IWDGRSTF",
        "SYS_WATCHDOG_RESET_REQUEST",
        "SYS_RESET_KIND WATCHDOG",
        "SYS_WATCHDOG_RESET_OK",
        "HAL_IWDG_Init",
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
