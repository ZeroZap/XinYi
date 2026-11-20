#ifndef XY_ACS712_H
#define XY_ACS712_H

#include "xy_adc.h"
#include "xy_device.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The ACS712 is a Hall-effect current sensor: its output is a voltage centred on Vcc/2 and offset by
 * sensitivity * current. This owner targets the 30 A variant (66 mV/A) on a 3.3 V supply read by a
 * 12-bit ADC, matching the legacy driver it replaces - the constants are the datasheet numbers for
 * that variant, and they are fixed rather than configurable so that the owner stays a concrete
 * device, not a generic analog scalar.
 */
#define XY_ACS712_SENSITIVITY_MV_PER_A 66U
#define XY_ACS712_SUPPLY_MV            3300U
#define XY_ACS712_ADC_MAX              4096U
#define XY_ACS712_DEFAULT_TIMEOUT_MS   1000U

typedef struct {
    int32_t current_ma;  /**< signed current, milli-amperes; positive = forward */
    uint32_t timestamp;  /**< sample tick */
} xy_acs712_sample_t;

typedef struct {
    void *adc;        /**< ADC device handle, owned and initialised by the caller */
    uint8_t channel;  /**< ADC channel the sensor is wired to */
    uint8_t initialized;
} xy_acs712_t;

xy_error_t xy_acs712_init(xy_acs712_t *dev, void *adc, uint8_t channel);
xy_error_t xy_acs712_deinit(xy_acs712_t *dev);
xy_error_t xy_acs712_read(xy_acs712_t *dev, xy_acs712_sample_t *sample);

#ifdef __cplusplus
}
#endif

#endif /* XY_ACS712_H */
