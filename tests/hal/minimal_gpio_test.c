#include <stdio.h>
#include "xy_hal_gpio.h"
#include "xy_hal_error.h"

int main() {
    printf("Testing HC32L021 GPIO HAL integration...\n");
    
    // Test GPIO initialization
    xy_hal_gpio_config_t config = {0};
    xy_hal_error_t result = xy_hal_gpio_init((void*)0, 0, &config);
    printf("GPIO init result: %d\n", result);
    
    // Test GPIO write
    result = xy_hal_gpio_write((void*)0, 0, 1);
    printf("GPIO write result: %d\n", result);
    
    // Test GPIO read
    int32_t value = xy_hal_gpio_read((void*)0, 0);
    printf("GPIO read value: %d\n", value);
    
    printf("Minimal GPIO test completed.\n");
    return 0;
}