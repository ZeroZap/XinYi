cmake_minimum_required(VERSION 3.10)

# XinYi GUI Component Library
project(xy_gui VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# GUI Core Sources
set(GUI_CORE_SRCS
    src/xy_font.c
    src/xy_gui_widget.c
    src/xy_gui_event.c
    src/xy_gui_draw.c
    src/xy_gui_layout.c
    src/xy_gui_theme.c
    src/xy_gui_button.c
    src/xy_gui_label.c
    src/xy_gui_slider.c
    src/xy_gui_checkbox.c
    src/xy_gui_progress.c
    src/xy_gui_list.c
    src/xy_gui_dropdown.c
    src/xy_gui_textbox.c
    src/xy_gui_tab.c
    src/xy_gui_container.c
)

# Check if building for MCU or PC
if(DEFINED MCU_TARGET)
    # MCU Build (STM32)
    message(STATUS "Building GUI for MCU: ${MCU_TARGET}")
    add_definitions(-DMCU_TARGET=${MCU_TARGET})
    add_definitions(-DXY_GUI_MCU)
else()
    # PC Simulation
    message(STATUS "Building GUI for PC simulation")
    add_definitions(-DXY_HAL_PC_SIM)
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../hal/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../hal/stm32/stm32u5
    ${CMAKE_CURRENT_SOURCE_DIR}/../../kernel/osal/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../kernel/misc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../trace/xy_log/inc
)

# Create library
if(DEFINED BUILD_SHARED_LIBS AND BUILD_SHARED_LIBS)
    add_library(xy_gui SHARED ${GUI_CORE_SRCS})
else()
    add_library(xy_gui STATIC ${GUI_CORE_SRCS})
endif()

# Set include directories for consumers
target_include_directories(xy_gui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

# Install headers
install(DIRECTORY inc/
    DESTINATION include/xy_gui
    FILES_MATCHING PATTERN "*.h"
)

# Install library
install(TARGETS xy_gui
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

message(STATUS "GUI Component: xy_gui v${PROJECT_VERSION}")
message(STATUS "  Sources: ${GUI_CORE_SRCS}")
