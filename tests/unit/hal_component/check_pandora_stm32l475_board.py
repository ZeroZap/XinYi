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
        "pandora_sys.c",
        "pandora_fota_flash.c",
        "xy_fota_metadata.c",
        "startup_stm32l475xx.s",
        "STM32L475VETX_FLASH.ld",
        "-O binary",
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
        "HAL_FLASHEx_Erase",
        "HAL_FLASH_Program",
        "pandora_fota_metadata_backend",
    )
    require(
        BOARD / "pandora_soft_i2c.c",
        "GPIO_PIN_1",
        "GPIO_PIN_6",
        "soft_i2c_start",
        "soft_i2c_write_byte",
        "soft_i2c_read_byte",
        "xy_hal_i2c_master_transmit",
        "xy_hal_i2c_master_receive",
        "dev_addr << 1",
    )
    require(
        BOARD / "main.c",
        "GPIO_PIN_7",
        "GPIO_PIN_9",
        "GPIO_PIN_10",
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
        "xy_fota_metadata_flash_validate",
        "xy_fota_metadata_flash_commit",
        "xy_fota_metadata_flash_load",
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
    print("Pandora STM32L475VE board contract: PASS")


if __name__ == "__main__":
    main()
