# GUI Component CMakeLists.txt

cmake_minimum_required(VERSION 3.10)

if(NOT XY_GUI_ENABLED)
    return()
endif()

# Include effects and fonts subdirectories
add_subdirectory(effects)
add_subdirectory(fonts)

# Collect GUI source files
set(GUI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/xy_gui_widget.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/xy_gui_draw.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/xy_gui_event.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/xy_gui_label.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/xy_gui_button.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/xy_gui_container.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/xy_gui_layout.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/xy_gui_textbox.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/xy_gui_slider.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/xy_gui_progress.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/xy_gui_checkbox.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/xy_gui_dropdown.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/xy_gui_list.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/xy_gui_tab.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/xy_gui_theme.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/xy_gui_ssd1306_adapter.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/xy_font.c
)

# Collect font source files
set(FONT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/fonts/xy_font_8x16.c
    ${CMAKE_CURRENT_SOURCE_DIR}/fonts/xy_font_16x24.c
    ${CMAKE_CURRENT_SOURCE_DIR}/fonts/xy_font_chinese_16x16.c
)

# PC SDL backend is explicit opt-in: never silently downgrade an enabled backend.
if(XY_GUI_SDL)
    if(NOT HAL_PLATFORM STREQUAL "PC")
        message(FATAL_ERROR "GUI: GUI_SDL is supported only with HAL_PLATFORM=PC")
    endif()

    find_package(SDL2 REQUIRED)

    set(_gui_sdl_source ${CMAKE_CURRENT_SOURCE_DIR}/src/xy_gui_sdl.c)
    if(NOT EXISTS ${_gui_sdl_source})
        message(FATAL_ERROR
            "GUI: GUI_SDL=ON requires the missing backend source ${_gui_sdl_source}; "
            "disable GUI_SDL until the SDL backend implementation is restored")
    endif()
    list(APPEND GUI_SOURCES ${_gui_sdl_source})
endif()

# Create GUI library
add_library(xy_gui STATIC ${GUI_SOURCES} ${FONT_SOURCES})

target_include_directories(xy_gui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/fonts
    ${CMAKE_SOURCE_DIR}/components/device
    ${CMAKE_SOURCE_DIR}/components/device/inc
    ${CMAKE_SOURCE_DIR}/components/drivers/display/oled/ssd1306
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${CMAKE_SOURCE_DIR}/components/trace/xy_log/inc
    ${CMAKE_SOURCE_DIR}/components/kernel/osal/inc
    ${CMAKE_SOURCE_DIR}/components/kernel/osal
)

target_compile_definitions(xy_gui PRIVATE GUI_ENABLED=1)

# Link with trace for logging
target_link_libraries(xy_gui PUBLIC xy_trace)

# Link with effects library
target_link_libraries(xy_gui PUBLIC xy_gui_effects)

if(XY_GUI_SDL)
    target_link_libraries(xy_gui PRIVATE SDL2::SDL2)
    target_compile_definitions(xy_gui PRIVATE GUI_BACKEND_SDL2 HAVE_SDL2)
    message(STATUS "GUI: SDL2 backend enabled")
endif()

# Compiler options
target_compile_options(xy_gui PRIVATE -Wall -Wextra -O2)

message(STATUS "GUI: Built successfully")
