#ifndef CONFIG_H
#define CONFIG_H

/* 测量配置 */
#define TEST_CURRENT_10MA  10  // 10mA测试电流
#define TEST_CURRENT_50MA  50  // 50mA测试电流
#define TEST_CURRENT_100MA 100 // 100mA测试电流

/* ADC配置 */
#define ADC_RESOLUTION 4096 // 12位ADC
#define ADC_VREF_MV    3300 // 参考电压3.3V

/* 放大器增益 */
#define AMP_GAIN 100 // 差分放大器增益

/* 测量范围 */
#define R_MIN_MOHM 1    // 最小1mΩ
#define R_MAX_MOHM 1000 // 最大1000mΩ

/* 采样配置 */
#define SAMPLE_COUNT 16 // 平均采样次数

#endif
