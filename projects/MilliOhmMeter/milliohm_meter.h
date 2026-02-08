#ifndef MILLIOHM_METER_H
#define MILLIOHM_METER_H

#include <stdint.h>

typedef enum { CURRENT_10MA = 0, CURRENT_50MA, CURRENT_100MA } test_current_t;

typedef struct {
    float resistance_mohm; // 电阻值(mΩ)
    float voltage_mv;      // 测量电压(mV)
    uint8_t current_range; // 电流档位
    uint8_t valid;         // 测量有效标志
} measurement_t;

/* 初始化 */
void milliohm_init(void);

/* 设置测试电流 */
void milliohm_set_current(test_current_t current);

/* 执行测量 */
measurement_t milliohm_measure(void);

/* 获取ADC值 */
uint16_t milliohm_read_adc(void);

#endif
