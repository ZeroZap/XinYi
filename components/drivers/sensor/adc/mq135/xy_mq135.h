#ifndef XY_MQ135_H
#define XY_MQ135_H

#include "xy_adc.h"
#include "xy_device.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* MQ-135 air-quality gas sensor. The legacy owner divided the raw reading by 8.0 and called it gas concentration; that constant has no datasheet backing (the MQ-series response is a nonlinear Rs/Ro ratio needing load-resistor calibration). This owner returns the raw ADC reading and leaves calibration to the caller. */
#define XY_MQ135_DEFAULT_TIMEOUT_MS 1000U

typedef struct {
    uint32_t raw;       /**< raw ADC reading */
    uint32_t timestamp; /**< sample tick */
} xy_mq135_sample_t;

typedef struct {
    void *adc;        /**< ADC device handle, owned and initialised by the caller */
    uint8_t channel;  /**< ADC channel the sensor is wired to */
    uint8_t initialized;
} xy_mq135_t;

xy_error_t xy_mq135_init(xy_mq135_t *dev, void *adc, uint8_t channel);
xy_error_t xy_mq135_deinit(xy_mq135_t *dev);
xy_error_t xy_mq135_read(xy_mq135_t *dev, xy_mq135_sample_t *sample);

#ifdef __cplusplus
}
#endif

#endif /* XY_MQ135_H */
