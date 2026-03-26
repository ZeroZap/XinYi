# HAL Tests for PC Platform

add_executable(test_gpio_pc test_gpio_pc.c)
target_link_libraries(test_gpio_pc xy_hal)
target_include_directories(test_gpio_pc PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../components/hal/inc
)