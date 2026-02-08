/**
 * @file xy_hal_gpio.h
 * @author Eugene Chan (zerozap2020@gmail.com)
 * @brief
 * @version 0.1
 * @date 2026-02-08
 *
 * @copyright Copyright (c) ZeroZap 2026
 *
 * Zephyr RTOS 中的 Time-aware GPIO
是一项高级功能，允许开发者以微秒级甚至纳秒级的精度来控制GPIO引脚状态变化的时间。它不仅仅是在某个时刻设置引脚高低电平，而是能够精确规划未来多个时间点的GPIO动作序列。

核心概念
与传统GPIO的区别
传统GPIO：gpio_pin_set() 立即设置状态，实际执行时间受系统调度影响

Time-aware GPIO：规划"在特定时间将引脚设置为特定状态"，由硬件定时器确保执行

关键特性
1. 基于时间戳的触发
所有GPIO动作都与绝对时间戳关联

支持未来的某个精确时刻执行

2. 硬件加速支持
依赖SoC的特定硬件外设（如定时器、PWM、GPIO事件生成器）

常见实现：使用Timer Counter (TC) 或 Programmable Real-Time Unit (PRU)

3. 低延迟与高确定性
避开操作系统调度延迟

直接由硬件定时器触发，响应时间可预测

典型使用场景
1. 精确时序协议
c
// 示例：生成精确的脉冲序列
struct gpio_timeout_t events[] = {
    {.timestamp = start_time, .pin_state = 1},
    {.timestamp = start_time + 1000, .pin_state = 0}, // 1微秒后拉低
    {.timestamp = start_time + 2000, .pin_state = 1}, // 2微秒后拉高
};
2. PWM波形生成
创建占空比可调的高精度PWM

多个引脚同步输出

3. 通信协议实现
单总线协议（如DHT11、DS18B20）

WS2812 LED时序控制

红外遥控编码

在Zephyr中的实现
依赖的硬件支持
并非所有MCU都支持，通常需要：

支持比较匹配的定时器

GPIO与定时器直接连接

如：Nordic nRF系列、某些STM32系列、Microchip SAM系列

API概览
c
// 配置时间感知GPIO
int gpio_timeout_configure(const struct device *dev,
                          const struct gpio_timeout_config *config);

// 添加定时事件
int gpio_timeout_add(const struct device *dev,
                     const struct gpio_timeout_event *event);

// 启动/停止时序生成
int gpio_timeout_start(const struct device *dev);
int gpio_timeout_stop(const struct device *dev);
实际应用示例
WS2812 LED控制
c
// WS2812需要非常精确的时序：
// 0码：高电平0.35µs，低电平0.80µs
// 1码：高电平0.70µs，低电平0.60µs

// Time-aware GPIO可以确保：
// 1. 精确的0.35µs高电平脉冲
// 2. 位与位之间严格的时间间隔
// 3. RESET信号的50µs低电平
步进电机控制
c
// 精确控制步进时序：
// 1. 规划加速度曲线
// 2. 精确的步进脉冲间隔
// 3. 多轴同步移动
配置与注意事项
内核配置
kconfig
CONFIG_GPIO_TIMEOUT=y
CONFIG_GPIO_TIMEOUT_NUM_EVENTS=32  # 最大事件数
硬件限制
事件队列深度有限

时间分辨率依赖硬件定时器

引脚可能分组绑定到特定定时器

性能考量
优势：极高的时间精度，CPU负载低

代价：占用硬件定时器资源，配置相对复杂

总结
Zephyr的Time-aware GPIO是为硬实时应用设计的专业级功能，特别适用于：

工业控制（PLC、运动控制）

通信协议（精确时序要求）

测试测量设备（信号生成）

音频/视频同步

对于大多数通用应用，标准GPIO或PWM已经足够。但当需要纳秒级精度或复杂的时间序列控制时，Time-aware
GPIO是Zephyr提供的强大解决方案。
 */