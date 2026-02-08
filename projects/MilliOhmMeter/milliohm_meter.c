#include "milliohm_meter.h"
#include "config.h"

static test_current_t current_range = CURRENT_50MA;

void milliohm_init(void)
{
    // TODO: 初始化ADC、GPIO、恒流源
    current_range = CURRENT_50MA;
}

void milliohm_set_current(test_current_t current)
{
    current_range = current;
    // TODO: 切换恒流源档位
}

uint16_t milliohm_read_adc(void)
{
    // TODO: 读取ADC值
    return 0;
}

measurement_t milliohm_measure(void)
{
    measurement_t result = { 0 };
    uint32_t adc_sum     = 0;

    // 多次采样平均
    for (int i = 0; i < SAMPLE_COUNT; i++) {
        adc_sum += milliohm_read_adc();
    }
    uint16_t adc_avg = adc_sum / SAMPLE_COUNT;

    // 计算实际电压 (mV)
    float voltage_mv = (float)adc_avg * ADC_VREF_MV / ADC_RESOLUTION / AMP_GAIN;

    // 获取测试电流 (mA)
    float current_ma;
    switch (current_range) {
    case CURRENT_10MA:
        current_ma = TEST_CURRENT_10MA;
        break;
    case CURRENT_50MA:
        current_ma = TEST_CURRENT_50MA;
        break;
    case CURRENT_100MA:
        current_ma = TEST_CURRENT_100MA;
        break;
    default:
        current_ma = TEST_CURRENT_50MA;
    }

    // 计算电阻 R(mΩ) = V(mV) / I(mA) * 1000
    float resistance_mohm = (voltage_mv / current_ma) * 1000.0f;

    result.resistance_mohm = resistance_mohm;
    result.voltage_mv      = voltage_mv;
    result.current_range   = current_range;
    result.valid =
        (resistance_mohm >= R_MIN_MOHM && resistance_mohm <= R_MAX_MOHM);

    return result;
}
