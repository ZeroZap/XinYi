#include "milliohm_meter.h"
#include "config.h"

/* 示例主程序 */
int main(void)
{
    milliohm_init();

    while (1) {
        measurement_t result = milliohm_measure();

        if (result.valid) {
            // 显示结果: 电阻值(mΩ), 电压(mV), 电流档位
            // printf("R: %.2f mΩ, V: %.2f mV\n", result.resistance_mohm,
            // result.voltage_mv);
        }

        // 延时
        // delay_ms(500);
    }

    return 0;
}
