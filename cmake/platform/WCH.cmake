# WCH Platform Configuration — RISC-V (default: CH32V307, qingke V4F core)
# Variables only; compiler/linker paths are set in root CMakeLists before project()

# CH32V307 uses Qingke V4F: RV32IMAC + single-precision FPU.
# Override with -DWCH_CHIP=CH32V30x (default), CH32X03x, CH32H41x, etc.
if(NOT DEFINED WCH_CHIP)
    set(WCH_CHIP CH32V30x)
endif()

set(CPU_FLAGS
    -march=rv32imafc_zicsr
    -mabi=ilp32f
    -msmall-data-limit=8
    -msave-restore
)

set(PLATFORM_DEFINES
    HAL_PLATFORM_WCH
    PLATFORM_WCH
    WCH
    MCU_CH32
    ${WCH_CHIP}
)

# Include dirs: XY HAL wrapper dir + WCH SDK peripheral / core headers.
set(PLATFORM_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/components/hal/wch/ch32x/src
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${CMAKE_SOURCE_DIR}/MCU/wch/${WCH_CHIP}/SRC/Peripheral/inc
    ${CMAKE_SOURCE_DIR}/MCU/wch/${WCH_CHIP}/SRC/Core
)

set(PLATFORM_LINKER_FLAGS "-Wl,--gc-sections -Wl,--print-memory-usage -nostartfiles")

message(STATUS "Platform: WCH (RISC-V Qingke V4F, chip=${WCH_CHIP})")
