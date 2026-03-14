cmake_minimum_required(VERSION 3.10)

# XinYi GUI PC Test (Standalone - no OSAL dependency)
project(gui_pc_test VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# PC Simulation Mode
add_definitions(-DXY_HAL_PC_SIM -DHAL_PLATFORM_PC)

message(STATUS "Building GUI PC Test (Standalone)")

# XinYi Framework Root
set(XINYI_ROOT ${CMAKE_SOURCE_DIR}/../..)

# Include Directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${XINYI_ROOT}/components/gui
    ${XINYI_ROOT}/components/gui/inc
    ${XINYI_ROOT}/components/hal/inc
    ${XINYI_ROOT}/components/hal/PC
)

# Source Files
set(SOURCES
    src/main.c
    src/hal_pc.c
)

# GUI Sources (inline, no subdirectory)
set(GUI_SOURCES
    ${XINYI_ROOT}/components/gui/src/xy_font.c
    ${XINYI_ROOT}/components/gui/src/xy_gui_widget.c
    ${XINYI_ROOT}/components/gui/src/xy_gui_button.c
    ${XINYI_ROOT}/components/gui/src/xy_gui_label.c
    ${XINYI_ROOT}/components/gui/src/xy_gui_slider.c
    ${XINYI_ROOT}/components/gui/src/xy_gui_checkbox.c
    ${XINYI_ROOT}/components/gui/src/xy_gui_progress.c
    ${XINYI_ROOT}/components/gui/src/xy_gui_list.c
    ${XINYI_ROOT}/components/gui/src/xy_gui_dropdown.c
    ${XINYI_ROOT}/components/gui/src/xy_gui_textbox.c
    ${XINYI_ROOT}/components/gui/src/xy_gui_tab.c
    ${XINYI_ROOT}/components/gui/src/xy_gui_container.c
)

# Create executable with all sources
add_executable(${PROJECT_NAME} ${SOURCES} ${GUI_SOURCES})

# Link math library
target_link_libraries(${PROJECT_NAME} m)

message(STATUS "GUI PC Test configured")
message(STATUS "  Output: ${CMAKE_BINARY_DIR}/${PROJECT_NAME}")
