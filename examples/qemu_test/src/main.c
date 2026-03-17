/**
 * XinYi QEMU 测试程序
 * 用于验证 QEMU STM32 仿真环境
 */

#include <stdint.h>

/* 简单的 LED 闪烁测试 */
int main(void)
{
    volatile uint32_t counter = 0;
    
    while (1) {
        counter++;
    }
    
    return 0;
}
