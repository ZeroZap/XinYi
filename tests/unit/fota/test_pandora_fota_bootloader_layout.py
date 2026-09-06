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
        "PANDORA_BOOT_CANDIDATE_INSTALLED",
        "xy_fota_boot_candidate_authorize_reviewed_restage",
        "PANDORA_BOOT_RESTAGE_AUTHORIZED",
        "PANDORA_BOOT_RESTAGE_AUTHORIZATION_ERROR",
        "xy_fota_boot_candidate_record_attempt",
        "xy_fota_boot_candidate_confirm",
        "PANDORA_BOOT_ATTEMPT_COMMITTED",
        "PANDORA_BOOT_CANDIDATE_CONFIRMED",
        "PANDORA_BOOT_ROLLBACK_REQUIRED",
        "PANDORA_BOOT_JUMP_APP",
    ):
        require(boot, token)
    for token in (
        "HAL_FLASHEx_Erase",
        "HAL_FLASH_Program",
        "PANDORA_FOTA_APP_BASE",
        "PANDORA_FOTA_EXECUTION_LIMIT",
    ):
        require(flash, token)
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
