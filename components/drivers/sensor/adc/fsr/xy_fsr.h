#ifndef XY_FSR_H
#define XY_FSR_H

#include "xy_adc.h"
#include "xy_device.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* FSR (force-sensitive resistor) read through a voltage divider. The legacy owner divided the raw
 * reading by 4.0 and called it pressure; that constant has no datasheet backing - the resistance and
 * therefore the force depend on the divider circuit - so this owner returns the raw ADC reading and
 * leaves calibration to the caller, matching the other canonical owners that return raw samples. */
#define XY_FSR_DEFAULT_TIMEOUT_MS 1000U

typedef struct {
    uint32_t raw;       /**< raw ADC reading */
    uint32_t timestamp; /**< sample tick */
} xy_fsr_sample_t;

typedef struct {
    void *adc;        /**< ADC device handle, owned and initialised by the caller */
    uint8_t channel;  /**< ADC channel the sensor is wired to */
    uint8_t initialized;
} xy_fsr_t;

xy_error_t xy_fsr_init(xy_fsr_t *dev, void *adc, uint8_t channel);
xy_error_t xy_fsr_deinit(xy_fsr_t *dev);
xy_error_t xy_fsr_read(xy_fsr_t *dev, xy_fsr_sample_t *sample);

#ifdef __cplusplus
}
#endif

#endif /* XY_FSR_H */
