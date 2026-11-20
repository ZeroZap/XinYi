/* The helper counts resolution bits; the Device API uses HAL option enums.
 * Both declarations must survive either include order without hiding each other. */
#ifdef XY_TEST_ADC_DEVICE_FIRST
#include "xy_adc.h"

#include "xy_analog_adc.h"
#else
#include "xy_analog_adc.h"

#include "xy_adc.h"
#endif

typedef char xy_test_adc_helper_bits[(ADC_RESOLUTION_12BIT == 12) ? 1 : -1];
typedef char xy_test_adc_distinct_encodings
    [((int)ADC_RESOLUTION_12BIT != (int)XY_HAL_ADC_RESOLUTION_12B) ? 1 : -1];

void xy_test_adc_header_types(void) {
    xy_adc_t helper = {.res = ADC_RESOLUTION_12BIT};
    xy_adc_config_t device = {.resolution = XY_HAL_ADC_RESOLUTION_12B};
    xy_analog_adc_resolution_t helper_resolution = helper.res;
    xy_adc_resolution_t device_resolution = device.resolution;

    /* Assignment checks make both public type names part of the compile contract. */
    helper.res = helper_resolution;
    device.resolution = device_resolution;
    (void)helper;
    (void)device;
}
