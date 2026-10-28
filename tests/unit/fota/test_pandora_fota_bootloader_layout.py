#!/usr/bin/env python3
"""Fail-closed source contract for the resident Pandora FOTA bootloader."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
BOARD = ROOT / "boards" / "pandora_stm32l475"


def require(text: str, token: str) -> None:
    assert token in text, f"missing required bootloader contract token: {token}"


def main() -> int:
    cmake = (BOARD / "CMakeLists.txt").read_text(encoding="utf-8")
    linker = (BOARD / "STM32L475VETX_BOOTLOADER.ld").read_text(encoding="utf-8")
    boot = (BOARD / "fota_bootloader_main.c").read_text(encoding="utf-8")
    boot_flow = (BOARD / "pandora_fota_boot_flow.c").read_text(encoding="utf-8")
    boot_flow_header = (BOARD / "pandora_fota_boot_flow.h").read_text(encoding="utf-8")
    flash = (BOARD / "pandora_fota_install_flash.c").read_text(encoding="utf-8")
    application = (BOARD / "rtos_main.c").read_text(encoding="utf-8")
    programmer = (BOARD / "fota_candidate_programmer_main.c").read_text(encoding="utf-8")

    for token in (
        "add_executable(pandora_stm32l475_fota_bootloader",
        "STM32L475VETX_BOOTLOADER.ld",
        "pandora_fota_install_flash.c",
    ):
        require(cmake, token)
    require(linker, "ORIGIN = 0x08000000, LENGTH = 32K")
    require(linker, "ASSERT(__flash_image_end <= 0x08008000")
    for token in (
        "xy_fota_boot_candidate_validate",
        "xy_fota_boot_candidate_install",
        "void SysTick_Handler(void)",
        "SCB->VTOR = PANDORA_FOTA_APP_BASE",
        "__set_MSP(app_vectors[0])",
        "xy_fota_boot_candidate_authorize_reviewed_restage",
        "PANDORA_BOOT_RESTAGE_AUTHORIZATION_ERROR",
        "xy_fota_boot_candidate_record_attempt",
        "xy_fota_boot_candidate_confirm",
        "PANDORA_BOOT_JUMP_APP",
        "pandora_fota_boot_flow_run",
    ):
        require(boot, token)
    # The sequence and its ordered markers moved into the host-testable board
    # owner; the entry point now supplies the board operations and halts on HALT.
    require(cmake, "pandora_fota_boot_flow.c")
    for token in (
        "pandora_fota_boot_flow_run",
        "PANDORA_FOTA_BOOT_OUTCOME_HALT",
        "PANDORA_BOOT_CANDIDATE_INSTALLED",
        "PANDORA_BOOT_CANDIDATE_ALREADY_INSTALLED",
        "PANDORA_BOOT_ATTEMPT_COMMITTED",
        "PANDORA_BOOT_ATTEMPT_ERROR",
        "PANDORA_BOOT_CANDIDATE_CONFIRMED",
        "PANDORA_BOOT_CONFIRM_ERROR",
        "PANDORA_BOOT_ROLLBACK_REQUIRED",
        "PANDORA_BOOT_NO_VALID_IMAGE",
        "PANDORA_BOOT_INSTALL_ERROR",
        "PANDORA_BOOT_RESTAGE_AUTHORIZED",
    ):
        require(boot_flow, token)
    for token in (
        "PANDORA_FOTA_CONFIRM_REQUEST_MAGIC",
        "PANDORA_FOTA_CONFIRM_ACK_MAGIC",
        "PANDORA_FOTA_RESTAGE_DONE_MAGIC",
        "PANDORA_FOTA_RTC_SLOT_CONFIRM",
        "PANDORA_FOTA_RTC_SLOT_RESTAGE",
        "PANDORA_FOTA_FLOW_MAX_ATTEMPTS",
    ):
        require(boot_flow_header, token)
    for token in (
        "xy_hal_flash_erase",
        "xy_hal_flash_write",
        "xy_hal_flash_read",
        "PANDORA_FOTA_APP_BASE",
        "PANDORA_FOTA_EXECUTION_LIMIT",
    ):
        require(flash, token)
    for forbidden in ("HAL_FLASH_", "HAL_FLASHEx_", "FLASH_TYPEPROGRAM_", "FLASH_TYPEERASE_"):
        assert forbidden not in flash, f"install owner bypasses canonical Flash HAL: {forbidden}"
    require(cmake, "PANDORA_FOTA_APPLICATION")
    require(cmake, "option(PANDORA_FOTA_AUTHORIZE_RESTAGE")
    require(cmake, "PANDORA_FOTA_RESTAGE_SOURCE_COMMIT")
    require(cmake, "PANDORA_FOTA_CANDIDATE")
    require(cmake, "pandora_stm32l475_fota_candidate_programmer")
    for token in (
        "PANDORA_FOTA_CANDIDATE_PROGRAMMED_VERIFIED",
        "PANDORA_FOTA_CANDIDATE_VERIFY_ERROR",
        "PANDORA_FOTA_CANDIDATE_SOURCE_COMMIT",
        "ops->erase",
        "ops->write",
        "ops->read",
    ):
        require(programmer, token)
    require(application, "PANDORA_FOTA_CONFIRM_REQUESTED")
    require(application, "PANDORA_FOTA_CONFIRM_ACKNOWLEDGED")
    require(application, "else if (w25q128_mcu_reset_recovery_pending != 0U)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
