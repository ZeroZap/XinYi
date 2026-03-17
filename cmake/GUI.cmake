# GUI Subsystem Configuration
# Cross-platform GUI support (SDL2 for PC, TFT for embedded)

if(CONFIG_GUI_ENABLED)
    message(STATUS "GUI Subsystem: ENABLED")
    
    # PC Platform with SDL2
    if(CONFIG_PLATFORM_PC AND CONFIG_GUI_SDL)
        message(STATUS "  Backend: SDL2 (PC)")
        
        # Find SDL2
        find_package(SDL2 REQUIRED)
        
        # GUI sources for PC
        set(GUI_SOURCES
            ${CMAKE_SOURCE_DIR}/components/gui/PC/xy_gui_sdl.c
            ${CMAKE_SOURCE_DIR}/components/gui/PC/xy_gui_window_sdl.c
            ${CMAKE_SOURCE_DIR}/components/gui/PC/xy_gui_input_sdl.c
        )
        
        # Include directories
        set(GUI_INCLUDE_DIRS
            ${SDL2_INCLUDE_DIRS}
            ${CMAKE_SOURCE_DIR}/components/gui/inc
        )
        
        # Link libraries
        set(GUI_LIBS ${SDL2_LIBRARIES})
        
        # Compile definitions
        set(GUI_DEFINES
            GUI_BACKEND_SDL2
            HAVE_SDL2
        )
    
    # Embedded Platform with TFT
    elseif(CONFIG_PLATFORM_STM32 AND CONFIG_GUI_TFT)
        message(STATUS "  Backend: TFT LCD (Embedded)")
        
        # GUI sources for embedded
        set(GUI_SOURCES
            ${CMAKE_SOURCE_DIR}/components/gui/embedded/xy_gui_tft.c
            ${CMAKE_SOURCE_DIR}/components/gui/embedded/xy_gui_spi.c
            ${CMAKE_SOURCE_DIR}/components/gui/embedded/xy_gui_i8080.c
        )
        
        # Include directories
        set(GUI_INCLUDE_DIRS
            ${CMAKE_SOURCE_DIR}/components/gui/inc
            ${CMAKE_SOURCE_DIR}/components/hal/inc
        )
        
        # No external libraries for embedded
        set(GUI_LIBS)
        
        # Compile definitions
        set(GUI_DEFINES
            GUI_BACKEND_TFT
            GUI_EMBEDDED
        )
    
    # LVGL Integration
    if(CONFIG_GUI_LVGL)
        message(STATUS "  LVGL: ENABLED")
        
        # Add LVGL sources (should be in third_party/lvgl)
        if(EXISTS ${CMAKE_SOURCE_DIR}/third_party/lvgl/CMakeLists.txt)
            add_subdirectory(third_party/lvgl)
            list(APPEND GUI_LIBS lvgl)
        else()
            message(WARNING "LVGL not found in third_party/lvgl")
        endif()
        
        list(APPEND GUI_DEFINES GUI_LVGL_ENABLED)
    endif()
    
    # Built-in Widgets
    if(CONFIG_GUI_WIDGETS)
        message(STATUS "  Widgets: ENABLED")
        
        list(APPEND GUI_SOURCES
            ${CMAKE_SOURCE_DIR}/components/gui/widgets/xy_gui_button.c
            ${CMAKE_SOURCE_DIR}/components/gui/widgets/xy_gui_label.c
            ${CMAKE_SOURCE_DIR}/components/gui/widgets/xy_gui_slider.c
            ${CMAKE_SOURCE_DIR}/components/gui/widgets/xy_gui_checkbox.c
            ${CMAKE_SOURCE_DIR}/components/gui/widgets/xy_gui_progress.c
            ${CMAKE_SOURCE_DIR}/components/gui/widgets/xy_gui_list.c
            ${CMAKE_SOURCE_DIR}/components/gui/widgets/xy_gui_dropdown.c
            ${CMAKE_SOURCE_DIR}/components/gui/widgets/xy_gui_textbox.c
            ${CMAKE_SOURCE_DIR}/components/gui/widgets/xy_gui_tab.c
            ${CMAKE_SOURCE_DIR}/components/gui/widgets/xy_gui_container.c
        )
        
        list(APPEND GUI_DEFINES GUI_WIDGETS_ENABLED)
    endif()
    
else()
    message(STATUS "GUI Subsystem: DISABLED")
    set(GUI_SOURCES)
    set(GUI_INCLUDE_DIRS)
    set(GUI_LIBS)
    set(GUI_DEFINES)
endif()

# Export GUI variables
set(GUI_SOURCES ${GUI_SOURCES} PARENT_SCOPE)
set(GUI_INCLUDE_DIRS ${GUI_INCLUDE_DIRS} PARENT_SCOPE)
set(GUI_LIBS ${GUI_LIBS} PARENT_SCOPE)
set(GUI_DEFINES ${GUI_DEFINES} PARENT_SCOPE)
