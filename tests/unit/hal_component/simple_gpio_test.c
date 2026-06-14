#include <stdio.h>
#include "xy_hal_gpio.h"
#include "xy_hal_error.h"
#include "xy_hal_pc.h"  // Include PC-specific header (now in include path)

int main() {
    printf("Testing PC GPIO HAL simulation...\n");
    
    // Create a simulated GPIO port
    struct xy_hal_gpio_port gpio_port = {0};
    gpio_port.port_id = 0;
    gpio_port.pin_mask = (1 << 5);
    gpio_port.direction = 1; // Output
    gpio_port.pull = 0;      // No pull
    
    xy_hal_gpio_port_t port = &gpio_port;
    
    // Test GPIO initialization
    xy_hal_gpio_config_t config = {0};
    xy_hal_error_t result = xy_hal_gpio_init(port, 5, &config);
    printf("GPIO init result: %d\n", result);
    
    // Test GPIO write
    result = xy_hal_gpio_write(port, 5, 1);
    printf("GPIO write result: %d\n", result);
    
    // Test GPIO read
    int32_t value = xy_hal_gpio_read(port, 5);
    printf("GPIO read value: %d\n", value);
    
    printf("PC GPIO HAL simulation test completed successfully!\n");
    return 0;
}