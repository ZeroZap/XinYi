# HC32 Platform Configuration — Cortex-M0+ (soft-float)
# Variables only; compiler/linker paths are set in root CMakeLists before project()

set(CPU_FLAGS
    -mcpu=cortex-m0plus
    -mthumb
    -mfloat-abi=soft
)

# Default chip; override with -DHC32_CHIP=HC32L19x etc.
if(NOT DEFINED HC32_CHIP)
    set(HC32_CHIP HC32L021)
endif()
string(TOLOWER "${HC32_CHIP}" _HC32_CHIP_LOWER)

set(PLATFORM_DEFINES
    HAL_PLATFORM_HC32
    PLATFORM_HC32
    ${HC32_CHIP}
    XY_HAL_${HC32_CHIP}
    ARM_CORTEX_M0PLUS
)

# Include dirs: XY HAL impl dir + vendor SDK + CMSIS-Core.
# The HC32 HAL impl bundles vendor driver headers locally under
# components/hal/hc32/<chip>/ so the MCU/HC32/ submodule paths are
# included as a fallback for things like core_cm0plus.h pulled by
# the chip header.
set(PLATFORM_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/components/hal/hc32/${_HC32_CHIP_LOWER}
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${CMAKE_SOURCE_DIR}/MCU/HC32/${HC32_CHIP}/driver/inc
    ${CMAKE_SOURCE_DIR}/MCU/HC32/${HC32_CHIP}/mcu/common
    ${CMAKE_SOURCE_DIR}/MCU/CMSIS/Include
)

set(PLATFORM_LINKER_FLAGS "-Wl,--gc-sections -Wl,--print-memory-usage -nostartfiles")

message(STATUS "Platform: HC32 (Cortex-M0+, chip=${HC32_CHIP})")
