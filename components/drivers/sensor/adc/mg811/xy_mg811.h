#ifndef XY_MG811_H
#define XY_MG811_H

#include "xy_adc.h"
#include "xy_device.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* MG811 CO2 sensor: an electrochemical cell whose output is read through the ADC. The legacy owner
 * scaled the raw reading by 10000/4096 and called it gas concentration; that is a normalisation with
 * no datasheet calibration (the cell's response is a Nernst curve that needs a reference), so this
 * owner returns the raw ADC reading and leaves calibration to the caller. */
#define XY_MG811_DEFAULT_TIMEOUT_MS 1000U

typedef struct {
    uint32_t raw;       /**< raw ADC reading */
    uint32_t timestamp; /**< sample tick */
} xy_mg811_sample_t;

typedef struct {
    void *adc;        /**< ADC device handle, owned and initialised by the caller */
    uint8_t channel;  /**< ADC channel the sensor is wired to */
    uint8_t initialized;
} xy_mg811_t;

xy_error_t xy_mg811_init(xy_mg811_t *dev, void *adc, uint8_t channel);
xy_error_t xy_mg811_deinit(xy_mg811_t *dev);
xy_error_t xy_mg811_read(xy_mg811_t *dev, xy_mg811_sample_t *sample);

#ifdef __cplusplus
}
#endif

#endif /* XY_MG811_H */
