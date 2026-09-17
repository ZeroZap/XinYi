#include "xy_bme680.h"
#include "xy_hal_delay.h"
#include <string.h>
static BME68X_INTF_RET_TYPE bus_read(uint8_t r,uint8_t*p,uint32_t n,void*x){return xy_i2c_device_read_reg(x,r,p,n)==XY_DEVICE_OK?BME68X_INTF_RET_SUCCESS:-1;}
static BME68X_INTF_RET_TYPE bus_write(uint8_t r,const uint8_t*p,uint32_t n,void*x){return xy_i2c_device_write_reg(x,r,p,n)==XY_DEVICE_OK?BME68X_INTF_RET_SUCCESS:-1;}
static void delay_us(uint32_t us,void*x){(void)x;xy_hal_delay_ms((us+999U)/1000U);}
static xy_error_t map(int8_t r){return r==BME68X_OK?XY_DEVICE_OK:r==BME68X_E_DEV_NOT_FOUND?XY_DEVICE_NOT_FOUND:r==BME68X_E_NULL_PTR||r==BME68X_E_INVALID_LENGTH?XY_DEVICE_INVALID_PARAM:XY_DEVICE_IO_ERROR;}
xy_error_t xy_bme680_init(xy_bme680_t*d,void*h,uint8_t a){if(!d||!h||(a!=0x76U&&a!=0x77U))return XY_DEVICE_INVALID_PARAM;memset(d,0,sizeof(*d));xy_error_t r=xy_i2c_device_init(&d->i2c_dev,h,a,100U);if(r!=XY_DEVICE_OK)return r;d->bosch.intf=BME68X_I2C_INTF;d->bosch.intf_ptr=&d->i2c_dev;d->bosch.read=bus_read;d->bosch.write=bus_write;d->bosch.delay_us=delay_us;d->bosch.amb_temp=25;r=map(bme68x_init(&d->bosch));if(r!=XY_DEVICE_OK){memset(d,0,sizeof(*d));return r;}d->config.os_hum=BME68X_OS_2X;d->config.os_pres=BME68X_OS_4X;d->config.os_temp=BME68X_OS_8X;d->config.filter=BME68X_FILTER_SIZE_3;d->config.odr=BME68X_ODR_NONE;r=map(bme68x_set_conf(&d->config,&d->bosch));if(r!=XY_DEVICE_OK){memset(d,0,sizeof(*d));return r;}d->heater.enable=BME68X_ENABLE;d->heater.heatr_temp=320U;d->heater.heatr_dur=150U;r=map(bme68x_set_heatr_conf(BME68X_FORCED_MODE,&d->heater,&d->bosch));if(r!=XY_DEVICE_OK){memset(d,0,sizeof(*d));return r;}d->initialized=1U;return XY_DEVICE_OK;}
xy_error_t xy_bme680_deinit(xy_bme680_t*d){if(!d||!d->initialized)return XY_DEVICE_INVALID_PARAM;xy_error_t r=map(bme68x_set_op_mode(BME68X_SLEEP_MODE,&d->bosch));if(r==XY_DEVICE_OK){d->initialized=0;d->i2c_dev.base.initialized=0;}return r;}
xy_error_t xy_bme680_read(xy_bme680_t*d,xy_bme680_data_t*out){struct bme68x_data b;xy_bme680_data_t v;uint8_t n=0;if(!d||!out||!d->initialized||!d->i2c_dev.base.initialized)return XY_DEVICE_INVALID_PARAM;xy_error_t r=map(bme68x_set_op_mode(BME68X_FORCED_MODE,&d->bosch));if(r!=XY_DEVICE_OK)return r;uint32_t us=bme68x_get_meas_dur(BME68X_FORCED_MODE,&d->config,&d->bosch)+(uint32_t)d->heater.heatr_dur*1000U;delay_us(us+5000U,&d->i2c_dev);r=map(bme68x_get_data(BME68X_FORCED_MODE,&b,&n,&d->bosch));if(r!=XY_DEVICE_OK)return r;if(n==0U)return XY_DEVICE_BUSY;if((b.status&(BME68X_GASM_VALID_MSK|BME68X_HEAT_STAB_MSK))!=(BME68X_GASM_VALID_MSK|BME68X_HEAT_STAB_MSK))return XY_DEVICE_BUSY;
#ifdef BME68X_USE_FPU
v.temperature_centi_c=(int32_t)(b.temperature*100.0f);v.pressure_pa=(uint32_t)b.pressure;v.humidity_milli_pct=(uint32_t)(b.humidity*1000.0f);v.gas_ohms=(uint32_t)b.gas_resistance;
#else
v.temperature_centi_c=b.temperature;v.pressure_pa=b.pressure;v.humidity_milli_pct=b.humidity;v.gas_ohms=b.gas_resistance;
#endif
v.status=b.status;d->data=v;*out=v;return XY_DEVICE_OK;}
