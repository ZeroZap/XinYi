#ifndef XY_GUVAS12SD_H
#define XY_GUVAS12SD_H

#include "xy_adc.h"
#include "xy_device.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* GUVA-S12SD UV sensor. The legacy owner multiplied the raw reading by 0.1 and called it a UV index, but that is not a datasheet calibration - a 12-bit ADC scaled by 0.1 spans 0..409, far past the 0..11+ UV-index scale, and the sensor's true response needs a per-unit gain. This owner returns the raw ADC reading and leaves calibration to the caller. */
#define XY_GUVAS12SD_DEFAULT_TIMEOUT_MS 1000U

typedef struct {
    uint32_t raw;       /**< raw ADC reading */
    uint32_t timestamp; /**< sample tick */
} xy_guvas12sd_sample_t;

typedef struct {
    void *adc;        /**< ADC device handle, owned and initialised by the caller */
    uint8_t channel;  /**< ADC channel the sensor is wired to */
    uint8_t initialized;
} xy_guvas12sd_t;

xy_error_t xy_guvas12sd_init(xy_guvas12sd_t *dev, void *adc, uint8_t channel);
xy_error_t xy_guvas12sd_deinit(xy_guvas12sd_t *dev);
xy_error_t xy_guvas12sd_read(xy_guvas12sd_t *dev, xy_guvas12sd_sample_t *sample);

#ifdef __cplusplus
}
#endif

#endif /* XY_GUVAS12SD_H */
