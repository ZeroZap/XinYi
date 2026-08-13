# XinYi Analog Devices component
#
# Keep this target intentionally narrow: only the host-guarded ADC/DAC helper,
# MCP3008 SPI ADC, and HX711 load-cell ADC sources are compiled here. Historical
# declarations in inc/xy_adc_ext.h for ADS*/MAX*/MCP3208 remain API-shape only
# until they gain implementation and focused tests.

set(ANALOG_DEVICES_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/xy_adc.c
    ${CMAKE_CURRENT_LIST_DIR}/src/xy_mcp3008.c
    ${CMAKE_CURRENT_LIST_DIR}/src/xy_hx711.c
)

add_library(xy_adc STATIC ${ANALOG_DEVICES_SOURCES})
add_library(analog_devices_component ALIAS xy_adc)

target_include_directories(xy_adc PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/inc
    ${CMAKE_SOURCE_DIR}/components/hal/inc
    ${CMAKE_SOURCE_DIR}/components/trace/xy_log/inc
    ${CMAKE_SOURCE_DIR}/components/clib/xy_clib
    ${CMAKE_SOURCE_DIR}/components/device/inc
    ${CMAKE_SOURCE_DIR}/components/sensor/inc
)

if(TARGET xy_clib)
    target_link_libraries(xy_adc PRIVATE xy_clib)
endif()

if(TARGET xy_trace)
    target_link_libraries(xy_adc PRIVATE xy_trace)
endif()

set_target_properties(xy_adc PROPERTIES
    OUTPUT_NAME xy_adc
)
