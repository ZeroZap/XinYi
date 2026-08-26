# Deprecated historical STM32U5 FOTA cache preset.
#
# The root build regenerates Kconfig values and uses HAL_PLATFORM=STM32U5;
# variables formerly listed here (XY_PLATFORM_STM32U5, FOTA_ENABLED,
# NOR_FLASH_ENABLED, and similar unprefixed names) are not canonical inputs.
# Keep this file only as a migration marker. Use the root Makefile/CMake path:
#
#   cmake -S . -B build/stm32u5_fota \
#     -DHAL_PLATFORM=STM32U5 -DCMAKE_BUILD_TYPE=Release
#
# Enabling FOTA or a board backend requires an intentional Kconfig selection
# and a board-owned implementation. The project main.c remains fail-closed
# until durable flash/metadata/bootloader callbacks are supplied.
