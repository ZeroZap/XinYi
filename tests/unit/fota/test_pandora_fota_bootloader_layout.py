#!/usr/bin/env python3
"""Fail-closed source contract for the resident Pandora FOTA bootloader."""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))
from source_scan import read_code, read_hash_code  # noqa: E402

ROOT = Path(__file__).resolve().parents[3]
BOARD = ROOT / "boards" / "pandora_stm32l475"


def require(text: str, token: str) -> None:
    assert token in text, f"missing required bootloader contract token: {token}"


def main() -> int:
    cmake = read_hash_code(BOARD / "CMakeLists.txt")
    linker = read_code(BOARD / "STM32L475VETX_BOOTLOADER.ld")
    boot = read_code(BOARD / "fota_bootloader_main.c")
    boot_flow = read_code(BOARD / "pandora_fota_boot_flow.c")
    boot_flow_header = read_code(BOARD / "pandora_fota_boot_flow.h")
    flash = read_code(BOARD / "pandora_fota_install_flash.c")
    application = read_code(BOARD / "rtos_main.c")
    programmer = read_code(BOARD / "fota_candidate_programmer_main.c")

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
        # The confirm slot is cleared when an install replaces the image; a request that
        # could not be cleared halts with its own marker instead of silently continuing.
        "PANDORA_BOOT_CONFIRM_REQUEST_ERROR",
        # Reading the slot and publishing the acknowledgement are separate steps: each
        # halt must name itself, because a boot frame that ends without a decision marker
        # is what the capture validator reports as a truncated capture.
        "PANDORA_BOOT_CONFIRM_STATE_ERROR",
        "PANDORA_BOOT_CONFIRM_ACK_ERROR",
        "PANDORA_BOOT_RESTAGE_STATE_ERROR",
        "PANDORA_BOOT_RESTAGE_DONE_ERROR",
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
    # The confirmation fixture ends at the acknowledgement. If it falls through to the
    # stress chain it writes the candidate into W25Q128 again and stages another reset,
    # so the next boot spends a boot attempt on a second install cycle and the confirm
    # boot ends up behind a rollback - which is exactly how the branch stayed unobserved.
    ack_branch = application.split("PANDORA_FOTA_CONFIRM_ACKNOWLEDGED", 1)[1].split(
        "PANDORA_FOTA_CONFIRM_REQUESTED", 1
    )[0]
    assert "PANDORA_FOTA_CONFIRM_FIXTURE_DONE" in ack_branch, (
        "the confirmation fixture must not continue into the stress chain after the "
        "acknowledgement: re-staging there spends the bootloader's attempt budget"
    )
    assert "for (;;)" in ack_branch, "the confirmation fixture must halt after the acknowledgement"
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
