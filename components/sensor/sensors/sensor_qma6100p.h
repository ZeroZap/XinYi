#ifndef SENSOR_QMA6100P_H
#define SENSOR_QMA6100P_H

#include "sensor_core.h"
#include "xy_qma6100p.h"

typedef struct {
    xy_qma6100p_t device;
    uint8_t address;
} qma6100p_priv_t;

sensor_device_t *qma6100p_create_accel(const char *name, void *bus, uint8_t address);
sensor_err_t qma6100p_set_range(sensor_device_t *sensor, uint8_t range);

#endif
