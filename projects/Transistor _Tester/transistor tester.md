# M328晶体管测试仪原理解析及单片机移植设计

## 一、M328晶体管测试仪简介

M328晶体管测试仪是一款非常实用的开源电子测量工具，最早基于Atmel ATMega8开发，后升级为**ATMega328P**单片机。它具有以下主要功能：

### 1. 测量功能

- **自动识别元件类型**：PNP/NPN三极管、N/P沟道MOSFET、JFET场效应管、二极管、双二极管、可控硅等
- **电阻测量**：分辨率0.1Ω，最高可达50MΩ
- **电容测量**：范围25pF~100mF(10万μF)，分辨率1pF
- **电感测量**：范围0.01mH~20H
- **ESR测量**：可检测2μF以上电容的等效串联电阻，分辨率0.01Ω
- **自动识别引脚定义**：无需区分管脚顺序

### 2. 其他特性

- 一键测量操作，延时自动关机
- 关断电流仅20nA，支持电池供电
- 测量三极管的电流放大系数(β)和发射结导通电压(Uf)
- 测量MOSFET的阈值电压和栅极电容值

------

## 二、工作原理解析

### 1. 核心测量原理

M328测试仪的核心原理是通过**三个测试端口(TP1、TP2、TP3)**对被测元件施加不同的测试信号，然后通过ADC采集响应信号进行分析。

```text
测试端口结构：
        ┌─────────────────────────────────────┐
        │                                     │
TP1 ────┼──[680Ω]──┬──[470kΩ]──┬──→ ADC      │
        │          │           │              │
TP2 ────┼──[680Ω]──┼──[470kΩ]──┼──→ ADC      │
        │          │           │              │
TP3 ────┼──[680Ω]──┼──[470kΩ]──┼──→ ADC      │
        │          │           │              │
        └─────────────────────────────────────┘
```

**关键电阻配置**：

- **680Ω电阻**：用于大电流测试（约7mA@5V）
- **470kΩ电阻**：用于小电流测试（约10μA@5V）

### 2. 元件识别原理

| 测试步骤 |           操作            |        目的        |
| :------: | :-----------------------: | :----------------: |
|  步骤1   | 三个端口依次输出高/低电平 |  检测元件导通特性  |
|  步骤2   |     通过680Ω施加电流      | 测量大电流下的压降 |
|  步骤3   |     通过470kΩ施加电流     | 测量小电流下的压降 |
|  步骤4   |     分析电压响应曲线      | 判断元件类型和参数 |

### 3. 电容测量原理

采用**RC充放电时间常数法**：

- 通过已知电阻对电容充电
- 测量电压达到特定阈值(如63.2%Vcc)的时间
- 根据τ = RC计算电容值

### 4. 电感测量原理

采用**LC谐振法**或**RL时间常数法**：

- 与已知电容组成LC振荡电路
- 测量谐振频率
- 根据f = 1/(2π√LC)计算电感值

### 5. 一键开关机电路原理

这是M328测试仪的一个精妙设计：

```text
电路原理图简化：
                    Vbat(9V)
                      │
                      ├──[R]──┬──→ Q1(PNP)基极
                      │       │
              SW1 ────┴───────┤
                              │
                    Q2(NPN)───┴──→ 稳压器使能
                      │
              MCU GPIO-Out ───┘
```

**开机过程**：

1. 按下SW1 → Q1导通 → 稳压器输出5V
2. MCU上电 → GPIO-Out输出高电平 → Q2导通
3. 松开SW1后，Q2维持Q1导通状态

**关机过程**：

1. MCU检测到长按 → GPIO-Out输出低电平
2. Q2截止 → Q1截止 → 系统断电
3. 关机功耗几乎为0

------

## 三、移植到新单片机的设计方案

### 1. 可选的替代单片机

|     单片机型号     |             优势             |     注意事项     |
| :----------------: | :--------------------------: | :--------------: |
| **STM32F103C8T6**  | 资源丰富、价格便宜、生态完善 | 需要3.3V电平转换 |
| **STM32G030/G031** |        低成本、低功耗        |  ADC精度需验证   |
|     **ESP32**      |  带WiFi/蓝牙、可做数据上传   |     功耗较高     |
|   **ATMega644**    |       与原版兼容性最好       | 已有成熟移植方案 |
|    **CH32V003**    |     国产RISC-V、超低成本     |     资源有限     |

### 2. 以STM32F103为例的移植设计

#### 2.1 硬件设计要点

```text
STM32F103C8T6 移植原理图框架：

                    ┌─────────────────────────────────────┐
                    │           STM32F103C8T6             │
                    │                                     │
    测试端口 TP1 ───┼── PA0(ADC) ── PA1(GPIO-680Ω控制)   │
    测试端口 TP2 ───┼── PA2(ADC) ── PA3(GPIO-680Ω控制)   │
    测试端口 TP3 ───┼── PA4(ADC) ── PA5(GPIO-680Ω控制)   │
                    │                                     │
    LCD显示屏 ──────┼── SPI1 (PA5-PA7) 或 I2C            │
    编码器/按键 ────┼── PB0-PB2 (外部中断)               │
    电池电压检测 ───┼── PA6(ADC)                         │
                    │                                     │
                    └─────────────────────────────────────┘
```

#### 2.2 关键移植步骤

**步骤1：GPIO配置移植**

```c
// 原AVR代码
DDRB |= (1<<PB0);  // 设置为输出
PORTB |= (1<<PB0); // 输出高电平

// STM32移植代码
GPIO_InitTypeDef GPIO_InitStruct;
GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0;
GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
GPIO_Init(GPIOB, &GPIO_InitStruct);
GPIO_SetBits(GPIOB, GPIO_Pin_0);
```

**步骤2：ADC配置移植**

```c
// STM32 ADC初始化
void ADC_Configuration(void)
{
    ADC_InitTypeDef ADC_InitStruct;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    
    ADC_InitStruct.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStruct.ADC_ScanConvMode = DISABLE;
    ADC_InitStruct.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStruct.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStruct.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStruct);
    
    ADC_Cmd(ADC1, ENABLE);
    // ADC校准
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
}
```

**步骤3：定时器配置（用于精确计时）**

```c
// 1μs精度定时器
void TIM_Configuration(void)
{
    TIM_TimeBaseInitTypeDef TIM_InitStruct;
    
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    
    TIM_InitStruct.TIM_Period = 0xFFFF;
    TIM_InitStruct.TIM_Prescaler = 72 - 1;  // 72MHz/72 = 1MHz = 1μs
    TIM_InitStruct.TIM_ClockDivision = 0;
    TIM_InitStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_InitStruct);
    
    TIM_Cmd(TIM2, ENABLE);
}
```

#### 2.3 软件架构移植

```text
软件层次结构：
┌─────────────────────────────────────────────┐
│              应用层 (Application)            │
│   - 元件识别算法                             │
│   - 参数计算                                 │
│   - 用户界面                                 │
├─────────────────────────────────────────────┤
│              硬件抽象层 (HAL)                 │  ← 主要移植工作
│   - GPIO操作封装                             │
│   - ADC操作封装                              │
│   - 定时器操作封装                           │
│   - 显示驱动封装                             │
├─────────────────────────────────────────────┤
│              底层驱动 (Driver)               │
│   - STM32 HAL库 / 标准库                     │
└─────────────────────────────────────────────┘
```

#### 2.4 移植注意事项

|   项目    | AVR ATMega328P | STM32F103 |          处理方法          |
| :-------: | :------------: | :-------: | :------------------------: |
| 工作电压  |       5V       |   3.3V    | 需要电平转换或调整测试电阻 |
|  ADC精度  |      10位      |   12位    |     可获得更高测量精度     |
| 时钟频率  |    8/16MHz     |   72MHz   |        调整延时函数        |
| Flash大小 |      32KB      |   64KB    |         空间更充裕         |
|  EEPROM   |    内置1KB     |  无内置   | 使用Flash模拟或外挂EEPROM  |

### 3. 固件资源

原版固件开源地址：

- GitHub: https://github.com/mikrocontroller-net/transistortester
- 常用固件版本：m328p-trunk-113k-st7565

**熔丝位设置（AVR）**：

- 低位：0xF7
- 高位：0xD9
- 扩展：0xFC

------

## 四、进阶改进建议

1. **增加稳压管测试功能**：添加升压电路（48V/5mA恒流输出）
2. **改用彩色TFT屏**：如ST7735、ILI9341等
3. **添加Type-C充电接口**：方便供电
4. **增加蓝牙/WiFi功能**：数据可上传手机APP
5. **使用旋转编码器**：替代按键，操作更便捷

如果您需要更详细的某个部分（如具体的电路图、完整的移植代码等），请告诉我，我可以进一步展开说明！

参考资料





```
完整的移植代码
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

已完成生成

我来为您提供一个完整的M328晶体管测试仪移植到STM32F103C8T6的代码框架。

# M328晶体管测试仪 STM32F103 移植完整代码

## 一、项目文件结构

```text
TransistorTester_STM32/
├── Core/
│   ├── Inc/
│   │   ├── main.h
│   │   ├── config.h
│   │   ├── gpio.h
│   │   ├── adc.h
│   │   ├── timer.h
│   │   ├── lcd.h
│   │   ├── measure.h
│   │   ├── analyze.h
│   │   └── ui.h
│   └── Src/
│       ├── main.c
│       ├── gpio.c
│       ├── adc.c
│       ├── timer.c
│       ├── lcd.c
│       ├── measure.c
│       ├── analyze.c
│       └── ui.c
├── Drivers/
│   ├── CMSIS/
│   └── STM32F1xx_HAL_Driver/
└── startup_stm32f103xb.s
```

## 二、核心头文件

### 1. config.h - 配置文件

```c
/**
 * @file config.h
 * @brief M328晶体管测试仪 STM32移植版 - 配置文件
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include "stm32f1xx_hal.h"

/*============================================================================
 *                          硬件配置
 *============================================================================*/

/* 系统时钟配置 */
#define SYSTEM_CLOCK_MHZ        72

/* ADC参考电压 (mV) */
#define ADC_VREF_MV             3300
#define ADC_RESOLUTION          4096    // 12位ADC

/* 测试端口引脚定义 */
// TP1 - 测试端口1
#define TP1_PORT                GPIOA
#define TP1_PIN                 GPIO_PIN_0      // ADC通道0
#define TP1_ADC_CHANNEL         ADC_CHANNEL_0
#define TP1_RL_PORT             GPIOA           // 680Ω控制
#define TP1_RL_PIN              GPIO_PIN_1
#define TP1_RH_PORT             GPIOA           // 470kΩ控制
#define TP1_RH_PIN              GPIO_PIN_2

// TP2 - 测试端口2
#define TP2_PORT                GPIOA
#define TP2_PIN                 GPIO_PIN_3      // ADC通道3
#define TP2_ADC_CHANNEL         ADC_CHANNEL_3
#define TP2_RL_PORT             GPIOA           // 680Ω控制
#define TP2_RL_PIN              GPIO_PIN_4
#define TP2_RH_PORT             GPIOA           // 470kΩ控制
#define TP2_RH_PIN              GPIO_PIN_5

// TP3 - 测试端口3
#define TP3_PORT                GPIOA
#define TP3_PIN                 GPIO_PIN_6      // ADC通道6
#define TP3_ADC_CHANNEL         ADC_CHANNEL_6
#define TP3_RL_PORT             GPIOA           // 680Ω控制
#define TP3_RL_PIN              GPIO_PIN_7
#define TP3_RH_PORT             GPIOB           // 470kΩ控制
#define TP3_RH_PIN              GPIO_PIN_0

/* 测试电阻值 (Ω) */
#define R_LOW                   680             // 低阻值电阻
#define R_HIGH                  470000          // 高阻值电阻

/* 按键引脚 */
#define KEY_PORT                GPIOB
#define KEY_PIN                 GPIO_PIN_1

/* 电池电压检测 */
#define VBAT_PORT               GPIOB
#define VBAT_PIN                GPIO_PIN_1
#define VBAT_ADC_CHANNEL        ADC_CHANNEL_9
#define VBAT_DIVIDER_RATIO      3               // 分压比

/* LCD配置 (ST7735 SPI) */
#define LCD_SPI                 SPI1
#define LCD_CS_PORT             GPIOB
#define LCD_CS_PIN              GPIO_PIN_12
#define LCD_DC_PORT             GPIOB
#define LCD_DC_PIN              GPIO_PIN_13
#define LCD_RST_PORT            GPIOB
#define LCD_RST_PIN             GPIO_PIN_14
#define LCD_BL_PORT             GPIOB
#define LCD_BL_PIN              GPIO_PIN_15

/* LCD尺寸 */
#define LCD_WIDTH               128
#define LCD_HEIGHT              160

/*============================================================================
 *                          测量参数配置
 *============================================================================*/

/* 电容测量范围 */
#define CAP_MIN_PF              25              // 最小可测电容 (pF)
#define CAP_MAX_UF              100000          // 最大可测电容 (μF)

/* 电阻测量范围 */
#define RES_MIN_OHM             0.1             // 最小可测电阻 (Ω)
#define RES_MAX_OHM             50000000        // 最大可测电阻 (Ω)

/* 电感测量范围 */
#define IND_MIN_UH              10              // 最小可测电感 (μH)
#define IND_MAX_MH              20000           // 最大可测电感 (mH)

/* 阈值定义 */
#define THRESHOLD_DIODE_MV      300             // 二极管导通阈值 (mV)
#define THRESHOLD_BJT_MV        500             // BJT导通阈值 (mV)
#define THRESHOLD_MOSFET_MV     1500            // MOSFET阈值电压上限 (mV)

/* 自动关机时间 (秒) */
#define AUTO_POWEROFF_SEC       30

/*============================================================================
 *                          元件类型定义
 *============================================================================*/

typedef enum {
    COMP_NONE = 0,          // 未检测到元件
    COMP_RESISTOR,          // 电阻
    COMP_CAPACITOR,         // 电容
    COMP_INDUCTOR,          // 电感
    COMP_DIODE,             // 二极管
    COMP_DUAL_DIODE,        // 双二极管
    COMP_BJT_NPN,           // NPN三极管
    COMP_BJT_PNP,           // PNP三极管
    COMP_MOSFET_N,          // N沟道MOSFET
    COMP_MOSFET_P,          // P沟道MOSFET
    COMP_JFET_N,            // N沟道JFET
    COMP_JFET_P,            // P沟道JFET
    COMP_SCR,               // 可控硅
    COMP_TRIAC,             // 双向可控硅
    COMP_UNKNOWN            // 未知元件
} ComponentType_t;

/* 引脚功能定义 */
typedef enum {
    PIN_UNKNOWN = 0,
    PIN_BASE,               // 基极 (BJT)
    PIN_COLLECTOR,          // 集电极 (BJT)
    PIN_EMITTER,            // 发射极 (BJT)
    PIN_GATE,               // 栅极 (FET)
    PIN_DRAIN,              // 漏极 (FET)
    PIN_SOURCE,             // 源极 (FET)
    PIN_ANODE,              // 阳极 (二极管)
    PIN_CATHODE,            // 阴极 (二极管)
} PinFunction_t;

/* 测量结果结构体 */
typedef struct {
    ComponentType_t type;           // 元件类型
    
    uint8_t pin1;                   // 引脚1对应的测试端口 (1-3)
    uint8_t pin2;                   // 引脚2对应的测试端口
    uint8_t pin3;                   // 引脚3对应的测试端口
    
    PinFunction_t pin1_func;        // 引脚1功能
    PinFunction_t pin2_func;        // 引脚2功能
    PinFunction_t pin3_func;        // 引脚3功能
    
    /* 电阻参数 */
    float resistance;               // 电阻值 (Ω)
    
    /* 电容参数 */
    float capacitance;              // 电容值 (F)
    float esr;                      // 等效串联电阻 (Ω)
    float vloss;                    // 损耗因子 (%)
    
    /* 电感参数 */
    float inductance;               // 电感值 (H)
    float dcr;                      // 直流电阻 (Ω)
    
    /* 二极管参数 */
    float vf;                       // 正向压降 (V)
    float ir;                       // 反向漏电流 (A)
    float cj;                       // 结电容 (F)
    
    /* 三极管参数 */
    float hfe;                      // 电流放大系数
    float vbe;                      // BE结压降 (V)
    float ice0;                     // 集电极截止电流 (A)
    
    /* MOSFET参数 */
    float vth;                      // 阈值电压 (V)
    float cgs;                      // 栅源电容 (F)
    float rds_on;                   // 导通电阻 (Ω)
    
} MeasureResult_t;

/* 全局测量结果 */
extern MeasureResult_t g_result;

#endif /* __CONFIG_H */
```

### 2. gpio.h - GPIO驱动头文件

```c
/**
 * @file gpio.h
 * @brief GPIO驱动头文件
 */

#ifndef __GPIO_H
#define __GPIO_H

#include "config.h"

/* 端口状态定义 */
typedef enum {
    PORT_HIZ = 0,           // 高阻态 (输入模式)
    PORT_LOW,               // 输出低电平
    PORT_HIGH,              // 输出高电平
    PORT_PULLUP,            // 上拉输入
    PORT_PULLDOWN           // 下拉输入
} PortState_t;

/* 测试端口结构体 */
typedef struct {
    GPIO_TypeDef* adc_port;
    uint16_t adc_pin;
    uint32_t adc_channel;
    GPIO_TypeDef* rl_port;      // 680Ω控制端口
    uint16_t rl_pin;
    GPIO_TypeDef* rh_port;      // 470kΩ控制端口
    uint16_t rh_pin;
} TestPort_t;

/* 函数声明 */
void GPIO_Init_All(void);
void GPIO_SetPortState(uint8_t port_num, PortState_t state);
void GPIO_SetRL(uint8_t port_num, uint8_t state);      // 控制680Ω
void GPIO_SetRH(uint8_t port_num, uint8_t state);      // 控制470kΩ
void GPIO_DischargeAll(void);                           // 放电所有端口
uint8_t GPIO_ReadKey(void);

/* 测试端口数组 */
extern TestPort_t TestPorts[3];

#endif /* __GPIO_H */
```

### 3. adc.h - ADC驱动头文件

```c
/**
 * @file adc.h
 * @brief ADC驱动头文件
 */

#ifndef __ADC_H
#define __ADC_H

#include "config.h"

/* 函数声明 */
void ADC_Init_All(void);
uint16_t ADC_ReadChannel(uint32_t channel);
uint16_t ADC_ReadAverage(uint32_t channel, uint8_t samples);
uint32_t ADC_ReadVoltage_mV(uint32_t channel);
uint32_t ADC_ReadBatteryVoltage(void);

/* 读取测试端口电压 */
uint32_t ADC_ReadTP_mV(uint8_t port_num);

#endif /* __ADC_H */
```

### 4. measure.h - 测量功能头文件

```c
/**
 * @file measure.h
 * @brief 测量功能头文件
 */

#ifndef __MEASURE_H
#define __MEASURE_H

#include "config.h"

/* 函数声明 */
void Measure_Init(void);
void Measure_Start(void);
void Measure_Resistance(void);
void Measure_Capacitance(void);
void Measure_Inductance(void);
void Measure_Diode(void);
void Measure_BJT(void);
void Measure_MOSFET(void);
void Measure_ESR(void);

/* 辅助函数 */
float Calculate_Resistance(uint8_t port_h, uint8_t port_l, uint8_t use_rl);
float Calculate_Capacitance(uint8_t port_h, uint8_t port_l);
uint32_t Measure_ChargeTime_us(uint8_t port_h, uint8_t port_l, uint32_t threshold_mv);

#endif /* __MEASURE_H */
```

## 三、核心源文件

### 1. main.c - 主程序

```c
/**
 * @file main.c
 * @brief M328晶体管测试仪 STM32移植版 - 主程序
 */

#include "main.h"
#include "config.h"
#include "gpio.h"
#include "adc.h"
#include "timer.h"
#include "lcd.h"
#include "measure.h"
#include "analyze.h"
#include "ui.h"

/* 全局变量 */
MeasureResult_t g_result;
volatile uint32_t g_systick_ms = 0;
volatile uint8_t g_key_pressed = 0;
volatile uint32_t g_idle_counter = 0;

/* 系统时钟配置 */
void SystemClock_Config(void);

/* 主函数 */
int main(void)
{
    /* HAL初始化 */
    HAL_Init();
    
    /* 系统时钟配置 */
    SystemClock_Config();
    
    /* 外设初始化 */
    GPIO_Init_All();
    ADC_Init_All();
    Timer_Init_All();
    LCD_Init();
    
    /* 显示开机画面 */
    UI_ShowSplash();
    HAL_Delay(1000);
    
    /* 显示电池电压 */
    uint32_t vbat = ADC_ReadBatteryVoltage();
    UI_ShowBatteryVoltage(vbat);
    
    if (vbat < 6500) {  // 电池电压低于6.5V
        UI_ShowLowBattery();
        HAL_Delay(2000);
    }
    
    /* 主循环 */
    while (1)
    {
        /* 显示待机界面 */
        UI_ShowReady();
        
        /* 等待按键 */
        while (!g_key_pressed) {
            HAL_Delay(10);
            g_idle_counter += 10;
            
            /* 自动关机检测 */
            if (g_idle_counter >= AUTO_POWEROFF_SEC * 1000) {
                UI_ShowPowerOff();
                HAL_Delay(500);
                // 进入停机模式或关闭电源
                System_PowerOff();
            }
        }
        g_key_pressed = 0;
        g_idle_counter = 0;
        
        /* 显示测量中 */
        UI_ShowMeasuring();
        
        /* 清空上次结果 */
        memset(&g_result, 0, sizeof(MeasureResult_t));
        
        /* 开始测量 */
        Measure_Start();
        
        /* 分析结果 */
        Analyze_Component();
        
        /* 显示结果 */
        UI_ShowResult(&g_result);
        
        /* 等待按键继续 */
        HAL_Delay(500);  // 防抖
        while (!g_key_pressed) {
            HAL_Delay(10);
            g_idle_counter += 10;
            
            if (g_idle_counter >= AUTO_POWEROFF_SEC * 1000) {
                UI_ShowPowerOff();
                HAL_Delay(500);
                System_PowerOff();
            }
        }
        g_key_pressed = 0;
        g_idle_counter = 0;
    }
}

/**
 * @brief 系统时钟配置 - 72MHz
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    /* 配置HSE和PLL */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;  // 8MHz * 9 = 72MHz
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    /* 配置系统时钟 */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);

    /* ADC时钟配置 */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;  // 72/6 = 12MHz
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);
}

/**
 * @brief 系统关机
 */
void System_PowerOff(void)
{
    /* 关闭LCD背光 */
    HAL_GPIO_WritePin(LCD_BL_PORT, LCD_BL_PIN, GPIO_PIN_RESET);
    
    /* 进入停机模式 */
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
    
    /* 如果有外部电源控制，可以在这里关闭 */
    // HAL_GPIO_WritePin(POWER_CTRL_PORT, POWER_CTRL_PIN, GPIO_PIN_RESET);
}

/**
 * @brief SysTick中断回调
 */
void HAL_SYSTICK_Callback(void)
{
    g_systick_ms++;
}

/**
 * @brief 外部中断回调 (按键)
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == KEY_PIN) {
        g_key_pressed = 1;
        g_idle_counter = 0;
    }
}
```

### 2. gpio.c - GPIO驱动

```c
/**
 * @file gpio.c
 * @brief GPIO驱动实现
 */

#include "gpio.h"

/* 测试端口配置数组 */
TestPort_t TestPorts[3] = {
    {   // TP1
        .adc_port = TP1_PORT,
        .adc_pin = TP1_PIN,
        .adc_channel = TP1_ADC_CHANNEL,
        .rl_port = TP1_RL_PORT,
        .rl_pin = TP1_RL_PIN,
        .rh_port = TP1_RH_PORT,
        .rh_pin = TP1_RH_PIN
    },
    {   // TP2
        .adc_port = TP2_PORT,
        .adc_pin = TP2_PIN,
        .adc_channel = TP2_ADC_CHANNEL,
        .rl_port = TP2_RL_PORT,
        .rl_pin = TP2_RL_PIN,
        .rh_port = TP2_RH_PORT,
        .rh_pin = TP2_RH_PIN
    },
    {   // TP3
        .adc_port = TP3_PORT,
        .adc_pin = TP3_PIN,
        .adc_channel = TP3_ADC_CHANNEL,
        .rl_port = TP3_RL_PORT,
        .rl_pin = TP3_RL_PIN,
        .rh_port = TP3_RH_PORT,
        .rh_pin = TP3_RH_PIN
    }
};

/**
 * @brief 初始化所有GPIO
 */
void GPIO_Init_All(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    /* 使能GPIO时钟 */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    
    /* 初始化测试端口控制引脚 (RL和RH) */
    for (int i = 0; i < 3; i++) {
        /* RL引脚 - 680Ω控制 */
        GPIO_InitStruct.Pin = TestPorts[i].rl_pin;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(TestPorts[i].rl_port, &GPIO_InitStruct);
        HAL_GPIO_WritePin(TestPorts[i].rl_port, TestPorts[i].rl_pin, GPIO_PIN_RESET);
        
        /* RH引脚 - 470kΩ控制 */
        GPIO_InitStruct.Pin = TestPorts[i].rh_pin;
        HAL_GPIO_Init(TestPorts[i].rh_port, &GPIO_InitStruct);
        HAL_GPIO_WritePin(TestPorts[i].rh_port, TestPorts[i].rh_pin, GPIO_PIN_RESET);
    }
    
    /* 初始化按键引脚 */
    GPIO_InitStruct.Pin = KEY_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(KEY_PORT, &GPIO_InitStruct);
    
    /* 配置按键中断 */
    HAL_NVIC_SetPriority(EXTI1_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);
    
    /* 初始化LCD引脚 */
    GPIO_InitStruct.Pin = LCD_CS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_CS_PORT, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = LCD_DC_PIN;
    HAL_GPIO_Init(LCD_DC_PORT, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = LCD_RST_PIN;
    HAL_GPIO_Init(LCD_RST_PORT, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = LCD_BL_PIN;
    HAL_GPIO_Init(LCD_BL_PORT, &GPIO_InitStruct);
    
    /* 默认打开背光 */
    HAL_GPIO_WritePin(LCD_BL_PORT, LCD_BL_PIN, GPIO_PIN_SET);
}

/**
 * @brief 设置测试端口状态
 * @param port_num 端口号 (0-2)
 * @param state 端口状态
 */
void GPIO_SetPortState(uint8_t port_num, PortState_t state)
{
    if (port_num > 2) return;
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    TestPort_t *port = &TestPorts[port_num];
    
    GPIO_InitStruct.Pin = port->adc_pin;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    
    switch (state) {
        case PORT_HIZ:
            GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
            GPIO_InitStruct.Pull = GPIO_NOPULL;
            break;
            
        case PORT_LOW:
            GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
            GPIO_InitStruct.Pull = GPIO_NOPULL;
            HAL_GPIO_Init(port->adc_port, &GPIO_InitStruct);
            HAL_GPIO_WritePin(port->adc_port, port->adc_pin, GPIO_PIN_RESET);
            return;
            
        case PORT_HIGH:
            GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
            GPIO_InitStruct.Pull = GPIO_NOPULL;
            HAL_GPIO_Init(port->adc_port, &GPIO_InitStruct);
            HAL_GPIO_WritePin(port->adc_port, port->adc_pin, GPIO_PIN_SET);
            return;
            
        case PORT_PULLUP:
            GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
            GPIO_InitStruct.Pull = GPIO_PULLUP;
            break;
            
        case PORT_PULLDOWN:
            GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
            GPIO_InitStruct.Pull = GPIO_PULLDOWN;
            break;
    }
    
    HAL_GPIO_Init(port->adc_port, &GPIO_InitStruct);
}

/**
 * @brief 控制680Ω电阻开关
 * @param port_num 端口号 (0-2)
 * @param state 0=断开, 1=接地, 2=接VCC
 */
void GPIO_SetRL(uint8_t port_num, uint8_t state)
{
    if (port_num > 2) return;
    
    TestPort_t *port = &TestPorts[port_num];
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    GPIO_InitStruct.Pin = port->rl_pin;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    
    if (state == 0) {
        /* 高阻态 - 断开 */
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(port->rl_port, &GPIO_InitStruct);
    } else if (state == 1) {
        /* 输出低电平 - 接地 */
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(port->rl_port, &GPIO_InitStruct);
        HAL_GPIO_WritePin(port->rl_port, port->rl_pin, GPIO_PIN_RESET);
    } else {
        /* 输出高电平 - 接VCC */
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(port->rl_port, &GPIO_InitStruct);
        HAL_GPIO_WritePin(port->rl_port, port->rl_pin, GPIO_PIN_SET);
    }
}

/**
 * @brief 控制470kΩ电阻开关
 * @param port_num 端口号 (0-2)
 * @param state 0=断开, 1=接地, 2=接VCC
 */
void GPIO_SetRH(uint8_t port_num, uint8_t state)
{
    if (port_num > 2) return;
    
    TestPort_t *port = &TestPorts[port_num];
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    GPIO_InitStruct.Pin = port->rh_pin;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    
    if (state == 0) {
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(port->rh_port, &GPIO_InitStruct);
    } else if (state == 1) {
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(port->rh_port, &GPIO_InitStruct);
        HAL_GPIO_WritePin(port->rh_port, port->rh_pin, GPIO_PIN_RESET);
    } else {
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(port->rh_port, &GPIO_InitStruct);
        HAL_GPIO_WritePin(port->rh_port, port->rh_pin, GPIO_PIN_SET);
    }
}

/**
 * @brief 放电所有测试端口
 */
void GPIO_DischargeAll(void)
{
    /* 所有端口设为输出低电平 */
    for (int i = 0; i < 3; i++) {
        GPIO_SetPortState(i, PORT_LOW);
        GPIO_SetRL(i, 1);   // 680Ω接地
        GPIO_SetRH(i, 1);   // 470kΩ接地
    }
    
    /* 等待放电完成 */
    HAL_Delay(10);
    
    /* 恢复高阻态 */
    for (int i = 0; i < 3; i++) {
        GPIO_SetPortState(i, PORT_HIZ);
        GPIO_SetRL(i, 0);
        GPIO_SetRH(i, 0);
    }
}

/**
 * @brief 读取按键状态
 * @return 1=按下, 0=未按下
 */
uint8_t GPIO_ReadKey(void)
{
    return (HAL_GPIO_ReadPin(KEY_PORT, KEY_PIN) == GPIO_PIN_RESET) ? 1 : 0;
}
```

### 3. adc.c - ADC驱动

```c
/**
 * @file adc.c
 * @brief ADC驱动实现
 */

#include "adc.h"
#include "gpio.h"

/* ADC句柄 */
ADC_HandleTypeDef hadc1;

/**
 * @brief 初始化ADC
 */
void ADC_Init_All(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    
    /* 使能ADC时钟 */
    __HAL_RCC_ADC1_CLK_ENABLE();
    
    /* ADC基本配置 */
    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    HAL_ADC_Init(&hadc1);
    
    /* ADC校准 */
    HAL_ADCEx_Calibration_Start(&hadc1);
}

/**
 * @brief 读取ADC通道值
 * @param channel ADC通道
 * @return ADC原始值 (0-4095)
 */
uint16_t ADC_ReadChannel(uint32_t channel)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    
    sConfig.Channel = channel;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;  // 最长采样时间
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 10);
    uint16_t value = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    
    return value;
}

/**
 * @brief 读取ADC通道平均值
 * @param channel ADC通道
 * @param samples 采样次数
 * @return ADC平均值
 */
uint16_t ADC_ReadAverage(uint32_t channel, uint8_t samples)
{
    uint32_t sum = 0;
    
    for (uint8_t i = 0; i < samples; i++) {
        sum += ADC_ReadChannel(channel);
    }
    
    return (uint16_t)(sum / samples);
}

/**
 * @brief 读取ADC通道电压值
 * @param channel ADC通道
 * @return 电压值 (mV)
 */
uint32_t ADC_ReadVoltage_mV(uint32_t channel)
{
    uint16_t adc_value = ADC_ReadAverage(channel, 8);
    return (uint32_t)((uint64_t)adc_value * ADC_VREF_MV / ADC_RESOLUTION);
}

/**
 * @brief 读取测试端口电压
 * @param port_num 端口号 (0-2)
 * @return 电压值 (mV)
 */
uint32_t ADC_ReadTP_mV(uint8_t port_num)
{
    if (port_num > 2) return 0;
    
    /* 设置端口为模拟输入模式 */
    GPIO_SetPortState(port_num, PORT_HIZ);
    
    /* 短暂延时等待稳定 */
    for (volatile int i = 0; i < 100; i++);
    
    return ADC_ReadVoltage_mV(TestPorts[port_num].adc_channel);
}

/**
 * @brief 读取电池电压
 * @return 电池电压 (mV)
 */
uint32_t ADC_ReadBatteryVoltage(void)
{
    uint32_t adc_mv = ADC_ReadVoltage_mV(VBAT_ADC_CHANNEL);
    return adc_mv * VBAT_DIVIDER_RATIO;
}
```

### 4. timer.c - 定时器驱动

```c
/**
 * @file timer.c
 * @brief 定时器驱动实现
 */

#include "timer.h"

/* 定时器句柄 */
TIM_HandleTypeDef htim2;    // 微秒级计时
TIM_HandleTypeDef htim3;    // 通用定时

/* 微秒计数器 */
volatile uint32_t g_us_counter = 0;

/**
 * @brief 初始化所有定时器
 */
void Timer_Init_All(void)
{
    /* 使能定时器时钟 */
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_TIM3_CLK_ENABLE();
    
    /* TIM2配置 - 1μs计时 */
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 72 - 1;          // 72MHz / 72 = 1MHz
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 0xFFFF;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Base_Init(&htim2);
    HAL_TIM_Base_Start(&htim2);
    
    /* TIM3配置 - 1ms中断 */
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 7200 - 1;        // 72MHz / 7200 = 10kHz
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 10 - 1;             // 10kHz / 10 = 1kHz = 1ms
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Base_Init(&htim3);
    HAL_TIM_Base_Start_IT(&htim3);
    
    /* 配置TIM3中断 */
    HAL_NVIC_SetPriority(TIM3_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);
}

/**
 * @brief 获取微秒计数器值
 * @return 微秒计数值
 */
uint32_t Timer_GetMicros(void)
{
    return __HAL_TIM_GET_COUNTER(&htim2);
}

/**
 * @brief 重置微秒计数器
 */
void Timer_ResetMicros(void)
{
    __HAL_TIM_SET_COUNTER(&htim2, 0);
}

/**
 * @brief 微秒级延时
 * @param us 延时微秒数
 */
void Timer_DelayUs(uint32_t us)
{
    uint32_t start = Timer_GetMicros();
    while ((Timer_GetMicros() - start) < us);
}

/**
 * @brief 测量时间间隔 (微秒)
 * @param start_us 起始时间
 * @return 时间间隔 (μs)
 */
uint32_t Timer_GetElapsedUs(uint32_t start_us)
{
    uint32_t current = Timer_GetMicros();
    if (current >= start_us) {
        return current - start_us;
    } else {
        return (0xFFFF - start_us) + current + 1;
    }
}

/**
 * @brief TIM3中断处理
 */
void TIM3_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim3);
}

/**
 * @brief TIM3周期回调
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3) {
        // 1ms定时任务
        g_us_counter++;
    }
}
```

### 5. measure.c - 测量功能实现

```c
/**
 * @file measure.c
 * @brief 测量功能实现
 */

#include "measure.h"
#include "gpio.h"
#include "adc.h"
#include "timer.h"
#include <math.h>

/* 测量数据缓存 */
typedef struct {
    uint32_t voltage[3][3];     // [源端口][目标端口] 电压值
    uint8_t  diode_found;       // 二极管标志
    uint8_t  diode_anode[6];    // 二极管阳极
    uint8_t  diode_cathode[6];  // 二极管阴极
    uint32_t diode_vf[6];       // 二极管正向压降
    uint8_t  diode_count;       // 二极管数量
} MeasureData_t;

static MeasureData_t m_data;

/* 内部函数声明 */
static void Measure_ScanPorts(void);
static void Measure_CheckDiodes(void);
static void Measure_CheckResistance(void);
static void Measure_CheckCapacitance(void);
static void Measure_CheckTransistor(void);

/**
 * @brief 初始化测量模块
 */
void Measure_Init(void)
{
    memset(&m_data, 0, sizeof(MeasureData_t));
}

/**
 * @brief 开始测量
 */
void Measure_Start(void)
{
    /* 初始化测量数据 */
    Measure_Init();
    
    /* 放电所有端口 */
    GPIO_DischargeAll();
    HAL_Delay(10);
    
    /* 扫描端口 */
    Measure_ScanPorts();
    
    /* 检测二极管 */
    Measure_CheckDiodes();
    
    /* 根据二极管数量判断元件类型 */
    if (m_data.diode_count == 0) {
        /* 可能是电阻、电容或电感 */
        Measure_CheckResistance();
        if (g_result.type == COMP_NONE) {
            Measure_CheckCapacitance();
        }
    } else if (m_data.diode_count == 1) {
        /* 单个二极管 */
        g_result.type = COMP_DIODE;
        g_result.pin1 = m_data.diode_anode[0] + 1;
        g_result.pin2 = m_data.diode_cathode[0] + 1;
        g_result.pin1_func = PIN_ANODE;
        g_result.pin2_func = PIN_CATHODE;
        g_result.vf = m_data.diode_vf[0] / 1000.0f;
    } else if (m_data.diode_count >= 2) {
        /* 可能是三极管或MOSFET */
        Measure_CheckTransistor();
    }
}

/**
 * @brief 扫描所有端口组合
 */
static void Measure_ScanPorts(void)
{
    /* 使用680Ω电阻扫描 */
    for (uint8_t src = 0; src < 3; src++) {
        for (uint8_t dst = 0; dst < 3; dst++) {
            if (src == dst) continue;
            
            /* 放电 */
            GPIO_DischargeAll();
            Timer_DelayUs(100);
            
            /* src端口通过680Ω输出高电平 */
            GPIO_SetRL(src, 2);      // 680Ω接VCC
            GPIO_SetRL(dst, 1);      // 680Ω接GND
            
            /* 等待稳定 */
            Timer_DelayUs(500);
            
            /* 读取dst端口电压 */
            m_data.voltage[src][dst] = ADC_ReadTP_mV(dst);
            
            /* 恢复 */
            GPIO_SetRL(src, 0);
            GPIO_SetRL(dst, 0);
        }
    }
}

/**
 * @brief 检测二极管
 */
static void Measure_CheckDiodes(void)
{
    m_data.diode_count = 0;
    
    for (uint8_t src = 0; src < 3; src++) {
        for (uint8_t dst = 0; dst < 3; dst++) {
            if (src == dst) continue;
            
            uint32_t v_forward = m_data.voltage[src][dst];
            uint32_t v_reverse = m_data.voltage[dst][src];
            
            /* 正向导通且反向截止 */
            if (v_forward > THRESHOLD_DIODE_MV && 
                v_forward < (ADC_VREF_MV - 200) &&
                v_reverse < 100) {
                
                /* 检查是否已记录 */
                uint8_t found = 0;
                for (uint8_t i = 0; i < m_data.diode_count; i++) {
                    if (m_data.diode_anode[i] == src && 
                        m_data.diode_cathode[i] == dst) {
                        found = 1;
                        break;
                    }
                }
                
                if (!found && m_data.diode_count < 6) {
                    m_data.diode_anode[m_data.diode_count] = src;
                    m_data.diode_cathode[m_data.diode_count] = dst;
                    m_data.diode_vf[m_data.diode_count] = v_forward;
                    m_data.diode_count++;
                }
            }
        }
    }
}

/**
 * @brief 检测电阻
 */
static void Measure_CheckResistance(void)
{
    /* 尝试在TP1和TP3之间测量电阻 */
    float resistance = Calculate_Resistance(0, 2, 1);
    
    if (resistance > 0 && resistance < RES_MAX_OHM) {
        g_result.type = COMP_RESISTOR;
        g_result.resistance = resistance;
        g_result.pin1 = 1;
        g_result.pin2 = 3;
    }
}

/**
 * @brief 检测电容
 */
static void Measure_CheckCapacitance(void)
{
    float capacitance = Calculate_Capacitance(0, 2);
    
    if (capacitance > 0) {
        g_result.type = COMP_CAPACITOR;
        g_result.capacitance = capacitance;
        g_result.pin1 = 1;
        g_result.pin2 = 3;
        
        /* 测量ESR */
        Measure_ESR();
    }
}

/**
 * @brief 检测三极管
 */
static void Measure_CheckTransistor(void)
{
    /* 分析二极管连接关系，判断三极管类型 */
    
    /* 查找共同端点作为基极 */
    uint8_t base = 0xFF;
    uint8_t collector = 0xFF;
    uint8_t emitter = 0xFF;
    uint8_t is_npn = 0;
    
    for (uint8_t b = 0; b < 3; b++) {
        uint8_t anode_count = 0;
        uint8_t cathode_count = 0;
        uint8_t other_pins[2];
        uint8_t other_idx = 0;
        
        for (uint8_t i = 0; i < m_data.diode_count; i++) {
            if (m_data.diode_anode[i] == b) {
                anode_count++;
                if (other_idx < 2) other_pins[other_idx++] = m_data.diode_cathode[i];
            }
            if (m_data.diode_cathode[i] == b) {
                cathode_count++;
                if (other_idx < 2) other_pins[other_idx++] = m_data.diode_anode[i];
            }
        }
        
        if (anode_count == 2) {
            /* NPN: 基极是两个二极管的公共阳极 */
            base = b;
            is_npn = 1;
            collector = other_pins[0];
            emitter = other_pins[1];
            break;
        } else if (cathode_count == 2) {
            /* PNP: 基极是两个二极管的公共阴极 */
            base = b;
            is_npn = 0;
            collector = other_pins[0];
            emitter = other_pins[1];
            break;
        }
    }
    
    if (base != 0xFF) {
        /* 进一步测量hFE来确定C和E */
        float hfe1 = Measure_hFE(base, collector, emitter, is_npn);
        float hfe2 = Measure_hFE(base, emitter, collector, is_npn);
        
        if (hfe2 > hfe1) {
            /* 交换C和E */
            uint8_t temp = collector;
            collector = emitter;
            emitter = temp;
            g_result.hfe = hfe2;
        } else {
            g_result.hfe = hfe1;
        }
        
        g_result.type = is_npn ? COMP_BJT_NPN : COMP_BJT_PNP;
        g_result.pin1 = base + 1;
        g_result.pin2 = collector + 1;
        g_result.pin3 = emitter + 1;
        g_result.pin1_func = PIN_BASE;
        g_result.pin2_func = PIN_COLLECTOR;
        g_result.pin3_func = PIN_EMITTER;
        
        /* 测量Vbe */
        g_result.vbe = mode_vf[0] / 1000.0f;
    }
}

/**
 * @brief 计算电阻值
 * @param port_h 高电平端口
 * @param port_l 低电平端口
 * @param use_rl 1=使用680Ω, 0=使用470kΩ
 * @return 电阻值 (Ω)
 */
float Calculate_Resistance(uint8_t port_h, uint8_t port_l, uint8_t use_rl)
{
    GPIO_DischargeAll();
    Timer_DelayUs(100);
    
    uint32_t r_ref = use_rl ? R_LOW : R_HIGH;
    
    if (use_rl) {
        GPIO_SetRL(port_h, 2);  // 680Ω接VCC
        GPIO_SetRL(port_l, 1);  // 680Ω接GND
    } else {
        GPIO_SetRH(port_h, 2);  // 470kΩ接VCC
        GPIO_SetRH(port_l, 1);  // 470kΩ接GND
    }
    
    Timer_DelayUs(1000);
    
    uint32_t v_h = ADC_ReadTP_mV(port_h);
    uint32_t v_l = ADC_ReadTP_mV(port_l);
    
    /* 恢复 */
    if (use_rl) {
        GPIO_SetRL(port_h, 0);
        GPIO_SetRL(port_l, 0);
    } else {
        GPIO_SetRH(port_h, 0);
        GPIO_SetRH(port_l, 0);
    }
    
    /* 计算电阻 */
    /* Vcc --[R_ref]-- port_h --[Rx]-- port_l --[R_ref]-- GND */
    /* V_h = Vcc * (Rx + R_ref) / (2*R_ref + Rx) */
    /* V_l = Vcc * R_ref / (2*R_ref + Rx) */
    
    if (v_l < 10) return -1;  // 开路
    
    float v_diff = v_h - v_l;
    if (v_diff < 10) return 0;  // 短路
    
    /* Rx = R_ref * (V_h - V_l) / V_l */
    float rx = (float)r_ref * v_diff / v_l;
    
    return rx;
}

/**
 * @brief 计算电容值
 * @param port_h 充电端口
 * @param port_l 接地端口
 * @return 电容值 (F)
 */
float Calculate_Capacitance(uint8_t port_h, uint8_t port_l)
{
    /* 放电 */
    GPIO_DischargeAll();
    HAL_Delay(50);
    
    /* 设置接地端口 */
    GPIO_SetRL(port_l, 1);
    
    /* 测量充电时间 */
    uint32_t threshold_mv = (uint32_t)(ADC_VREF_MV * 0.632);  // 63.2% Vcc
    uint32_t charge_time = Measure_ChargeTime_us(port_h, port_l, threshold_mv);
    
    GPIO_SetRL(port_l, 0);
    
    if (charge_time == 0 || charge_time > 10000000) {
        return -1;  // 超时或无效
    }
    
    /* C = t / R */
    /* 使用470kΩ电阻充电 */
    float capacitance = (float)charge_time / (float)R_HIGH / 1000000.0f;
    
    return capacitance;
}

/**
 * @brief 测量充电时间
 * @param port_h 充电端口
 * @param port_l 接地端口
 * @param threshold_mv 阈值电压 (mV)
 * @return 充电时间 (μs)
 */
uint32_t Measure_ChargeTime_us(uint8_t port_h, uint8_t port_l, uint32_t threshold_mv)
{
    /* 开始充电 */
    GPIO_SetRH(port_h, 2);  // 470kΩ接VCC
    
    Timer_ResetMicros();
    uint32_t start_time = Timer_GetMicros();
    uint32_t timeout = 10000000;  // 10秒超时
    
    while (1) {
        uint32_t voltage = ADC_ReadTP_mV(port_h);
        
        if (voltage >= threshold_mv) {
            break;
        }
        
        if (Timer_GetElapsedUs(start_time) > timeout) {
            GPIO_SetRH(port_h, 0);
            return 0;  // 超时
        }
    }
    
    uint32_t charge_time = Timer_GetElapsedUs(start_time);
    GPIO_SetRH(port_h, 0);
    
    return charge_time;
}

/**
 * @brief 测量三极管hFE
 * @param base 基极端口
 * @param collector 集电极端口
 * @param emitter 发射极端口
 * @param is_npn NPN=1, PNP=0
 * @return hFE值
 */
float Measure_hFE(uint8_t base, uint8_t collector, uint8_t emitter, uint8_t is_npn)
{
    GPIO_DischargeAll();
    Timer_DelayUs(100);
    
    if (is_npn) {
        /* NPN: E接地, C通过680Ω接VCC, B通过470kΩ接VCC */
        GPIO_SetRL(emitter, 1);     // E接地
        GPIO_SetRL(collector, 2);   // C通过680Ω接VCC
        GPIO_SetRH(base, 2);        // B通过470kΩ接VCC
    } else {
        /* PNP: E接VCC, C通过680Ω接地, B通过470kΩ接地 */
        GPIO_SetRL(emitter, 2);     // E接VCC
        GPIO_SetRL(collector, 1);   // C通过680Ω接地
        GPIO_SetRH(base, 1);        // B通过470kΩ接地
    }
    
    Timer_DelayUs(1000);
    
    /* 读取电压 */
    uint32_t v_base = ADC_ReadTP_mV(base);
    uint32_t v_collector = ADC_ReadTP_mV(collector);
    
    /* 恢复 */
    GPIO_SetRL(emitter, 0);
    GPIO_SetRL(collector, 0);
    GPIO_SetRH(base, 0);
    
    /* 计算电流 */
    float i_base, i_collector;
    
    if (is_npn) {
        i_base = (ADC_VREF_MV - v_base) / (float)R_HIGH;         // mA
        i_collector = (ADC_VREF_MV - v_collector) / (float)R_LOW; // mA
    } else {
        i_base = v_base / (float)R_HIGH;
        i_collector = v_collector / (float)R_LOW;
    }
    
    if (i_base < 0.001) return 0;
    
    float hfe = i_collector / i_base;
    
    return hfe;
}

/**
 * @brief 测量ESR
 */
void Measure_ESR(void)
{
    /* ESR测量需要快速充放电 */
    /* 简化实现：通过测量充电初期的电压跳变来估算ESR */
    
    GPIO_DischargeAll();
    HAL_Delay(50);
    
    /* 快速充电脉冲 */
    GPIO_SetRL(2, 1);   // TP3接地
    GPIO_SetRL(0, 2);   // TP1通过680Ω接VCC
    
    Timer_DelayUs(10);  // 10μs脉冲
    
    uint32_t v_initial = ADC_ReadTP_mV(0);
    
    GPIO_SetRL(0, 0);
    GPIO_SetRL(2, 0);
    
    /* ESR ≈ V_initial / I_charge */
    /* I_charge ≈ Vcc / R_LOW */
    float i_charge = (float)ADC_VREF_MV / R_LOW;  // mA
    float esr = v_initial / i_charge;             // Ω
    
    g_result.esr = esr;
}
```

### 6. analyze.c - 分析功能实现

```c
/**
 * @file analyze.c
 * @brief 元件分析功能实现
 */

#include "analyze.h"
#include "measure.h"

/**
 * @brief 分析元件类型和参数
 */
void Analyze_Component(void)
{
    /* 主要分析工作已在measure.c中完成 */
    /* 这里进行额外的参数计算和验证 */
    
    switch (g_result.type) {
        case COMP_RESISTOR:
            Analyze_Resistor();
            break;
            
        case COMP_CAPACITOR:
            Analyze_Capacitor();
            break;
            
        case COMP_DIODE:
```





```
继续完成代码
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

已完成生成

继续完成剩余的代码：

### 6. analyze.c - 分析功能实现（续）

```c
/**
 * @file analyze.c
 * @brief 元件分析功能实现
 */

#include "analyze.h"
#include "measure.h"
#include "gpio.h"
#include "adc.h"
#include "timer.h"
#include <math.h>
#include <string.h>

/**
 * @brief 分析元件类型和参数
 */
void Analyze_Component(void)
{
    switch (g_result.type) {
        case COMP_RESISTOR:
            Analyze_Resistor();
            break;
            
        case COMP_CAPACITOR:
            Analyze_Capacitor();
            break;
            
        case COMP_DIODE:
            Analyze_Diode();
            break;
            
        case COMP_BJT_NPN:
        case COMP_BJT_PNP:
            Analyze_BJT();
            break;
            
        case COMP_MOSFET_N:
        case COMP_MOSFET_P:
            Analyze_MOSFET();
            break;
            
        default:
            break;
    }
}

/**
 * @brief 分析电阻
 */
void Analyze_Resistor(void)
{
    /* 根据电阻值选择合适的测量电阻 */
    if (g_result.resistance > 10000) {
        /* 大电阻使用470kΩ测量 */
        float r_new = Calculate_Resistance(0, 2, 0);
        if (r_new > 0) {
            g_result.resistance = r_new;
        }
    }
    
    /* 电阻值范围检查 */
    if (g_result.resistance < RES_MIN_OHM) {
        g_result.resistance = 0;  // 短路
    } else if (g_result.resistance > RES_MAX_OHM) {
        g_result.type = COMP_NONE;  // 开路
    }
}

/**
 * @brief 分析电容
 */
void Analyze_Capacitor(void)
{
    /* 计算损耗因子 VLoss */
    GPIO_DischargeAll();
    HAL_Delay(50);
    
    /* 充电到一定电压 */
    GPIO_SetRL(2, 1);   // TP3接地
    GPIO_SetRH(0, 2);   // TP1通过470kΩ充电
    
    HAL_Delay(100);     // 充电100ms
    
    uint32_t v_charge = ADC_ReadTP_mV(0);
    
    /* 断开充电，测量电压下降 */
    GPIO_SetRH(0, 0);
    HAL_Delay(10);
    
    uint32_t v_after = ADC_ReadTP_mV(0);
    
    GPIO_SetRL(2, 0);
    
    /* 计算损耗 */
    if (v_charge > 100) {
        g_result.vloss = (float)(v_charge - v_after) / v_charge * 100.0f;
    }
    
    /* 电容值单位转换和范围检查 */
    if (g_result.capacitance < CAP_MIN_PF * 1e-12) {
        g_result.type = COMP_NONE;
    }
}

/**
 * @brief 分析二极管
 */
void Analyze_Diode(void)
{
    /* 测量结电容 */
    g_result.cj = Measure_DiodeCapacitance(g_result.pin1 - 1, g_result.pin2 - 1);
    
    /* 测量反向漏电流 */
    g_result.ir = Measure_DiodeLeakage(g_result.pin1 - 1, g_result.pin2 - 1);
    
    /* 判断是否为稳压管 (Vf > 1.5V 可能是稳压管) */
    if (g_result.vf > 1.5) {
        /* 可能需要进一步测试稳压电压 */
    }
}

/**
 * @brief 分析三极管
 */
void Analyze_BJT(void)
{
    uint8_t base = g_result.pin1 - 1;
    uint8_t collector = g_result.pin2 - 1;
    uint8_t emitter = g_result.pin3 - 1;
    uint8_t is_npn = (g_result.type == COMP_BJT_NPN);
    
    /* 测量Iceo (集电极截止电流) */
    GPIO_DischargeAll();
    Timer_DelayUs(100);
    
    if (is_npn) {
        GPIO_SetRL(emitter, 1);     // E接地
        GPIO_SetRL(collector, 2);   // C通过680Ω接VCC
        /* B悬空 */
    } else {
        GPIO_SetRL(emitter, 2);     // E接VCC
        GPIO_SetRL(collector, 1);   // C通过680Ω接地
    }
    
    Timer_DelayUs(1000);
    
    uint32_t v_collector = ADC_ReadTP_mV(collector);
    
    GPIO_SetRL(emitter, 0);
    GPIO_SetRL(collector, 0);
    
    /* 计算Iceo */
    if (is_npn) {
        g_result.ice0 = (ADC_VREF_MV - v_collector) / (float)R_LOW / 1000.0f;  // A
    } else {
        g_result.ice0 = v_collector / (float)R_LOW / 1000.0f;
    }
    
    /* 多次测量hFE取平均值 */
    float hfe_sum = 0;
    for (int i = 0; i < 3; i++) {
        hfe_sum += Measure_hFE(base, collector, emitter, is_npn);
    }
    g_result.hfe = hfe_sum / 3.0f;
}

/**
 * @brief 分析MOSFET
 */
void Analyze_MOSFET(void)
{
    uint8_t gate = g_result.pin1 - 1;
    uint8_t drain = g_result.pin2 - 1;
    uint8_t source = g_result.pin3 - 1;
    uint8_t is_n_ch = (g_result.type == COMP_MOSFET_N);
    
    /* 测量阈值电压 Vth */
    g_result.vth = Measure_MOSFET_Vth(gate, drain, source, is_n_ch);
    
    /* 测量栅源电容 Cgs */
    g_result.cgs = Measure_MOSFET_Cgs(gate, source);
    
    /* 测量导通电阻 Rds(on) */
    g_result.rds_on = Measure_MOSFET_RdsOn(gate, drain, source, is_n_ch);
}

/**
 * @brief 测量二极管结电容
 */
float Measure_DiodeCapacitance(uint8_t anode, uint8_t cathode)
{
    /* 反向偏置测量结电容 */
    GPIO_DischargeAll();
    HAL_Delay(10);
    
    /* 反向充电 */
    GPIO_SetRH(cathode, 2);  // 阴极接VCC
    GPIO_SetRL(anode, 1);    // 阳极接地
    
    uint32_t threshold_mv = (uint32_t)(ADC_VREF_MV * 0.632);
    uint32_t charge_time = Measure_ChargeTime_us(cathode, anode, threshold_mv);
    
    GPIO_SetRH(cathode, 0);
    GPIO_SetRL(anode, 0);
    
    if (charge_time == 0) return 0;
    
    float capacitance = (float)charge_time / (float)R_HIGH / 1000000.0f;
    
    return capacitance;
}

/**
 * @brief 测量二极管反向漏电流
 */
float Measure_DiodeLeakage(uint8_t anode, uint8_t cathode)
{
    GPIO_DischargeAll();
    Timer_DelayUs(100);
    
    /* 反向偏置 */
    GPIO_SetRH(cathode, 2);  // 阴极通过470kΩ接VCC
    GPIO_SetRL(anode, 1);    // 阳极接地
    
    HAL_Delay(10);
    
    uint32_t v_cathode = ADC_ReadTP_mV(cathode);
    
    GPIO_SetRH(cathode, 0);
    GPIO_SetRL(anode, 0);
    
    /* 计算漏电流 */
    float i_leak = (ADC_VREF_MV - v_cathode) / (float)R_HIGH / 1000.0f;  // A
    
    return i_leak;
}

/**
 * @brief 测量MOSFET阈值电压
 */
float Measure_MOSFET_Vth(uint8_t gate, uint8_t drain, uint8_t source, uint8_t is_n_ch)
{
    GPIO_DischargeAll();
    Timer_DelayUs(100);
    
    float vth = 0;
    
    if (is_n_ch) {
        /* N沟道: S接地, D通过680Ω接VCC, G逐渐升压 */
        GPIO_SetRL(source, 1);
        GPIO_SetRL(drain, 2);
        
        /* 通过470kΩ给栅极充电 */
        GPIO_SetRH(gate, 2);
        
        /* 监测漏极电压变化 */
        uint32_t start_time = Timer_GetMicros();
        uint32_t v_drain_prev = ADC_ReadTP_mV(drain);
        
        while (Timer_GetElapsedUs(start_time) < 100000) {  // 100ms超时
            uint32_t v_gate = ADC_ReadTP_mV(gate);
            uint32_t v_drain = ADC_ReadTP_mV(drain);
            
            /* 当漏极电压开始下降时，记录栅极电压 */
            if (v_drain < v_drain_prev - 100) {
                vth = v_gate / 1000.0f;
                break;
            }
            v_drain_prev = v_drain;
            Timer_DelayUs(100);
        }
        
        GPIO_SetRH(gate, 0);
        GPIO_SetRL(source, 0);
        GPIO_SetRL(drain, 0);
    } else {
        /* P沟道: S接VCC, D通过680Ω接地, G逐渐降压 */
        GPIO_SetRL(source, 2);
        GPIO_SetRL(drain, 1);
        GPIO_SetRH(gate, 1);  // G通过470kΩ接地
        
        uint32_t start_time = Timer_GetMicros();
        uint32_t v_drain_prev = ADC_ReadTP_mV(drain);
        
        while (Timer_GetElapsedUs(start_time) < 100000) {
            uint32_t v_gate = ADC_ReadTP_mV(gate);
            uint32_t v_drain = ADC_ReadTP_mV(drain);
            
            if (v_drain > v_drain_prev + 100) {
                vth = (ADC_VREF_MV - v_gate) / 1000.0f;
                break;
            }
            v_drain_prev = v_drain;
            Timer_DelayUs(100);
        }
        
        GPIO_SetRH(gate, 0);
        GPIO_SetRL(source, 0);
        GPIO_SetRL(drain, 0);
    }
    
    return vth;
}

/**
 * @brief 测量MOSFET栅源电容
 */
float Measure_MOSFET_Cgs(uint8_t gate, uint8_t source)
{
    GPIO_DischargeAll();
    HAL_Delay(10);
    
    GPIO_SetRL(source, 1);  // S接地
    
    uint32_t threshold_mv = (uint32_t)(ADC_VREF_MV * 0.632);
    uint32_t charge_time = Measure_ChargeTime_us(gate, source, threshold_mv);
    
    GPIO_SetRL(source, 0);
    
    if (charge_time == 0) return 0;
    
    float cgs = (float)charge_time / (float)R_HIGH / 1000000.0f;
    
    return cgs;
}

/**
 * @brief 测量MOSFET导通电阻
 */
float Measure_MOSFET_RdsOn(uint8_t gate, uint8_t drain, uint8_t source, uint8_t is_n_ch)
{
    GPIO_DischargeAll();
    Timer_DelayUs(100);
    
    float rds_on = 0;
    
    if (is_n_ch) {
        /* N沟道: G接VCC使其完全导通 */
        GPIO_SetRL(gate, 2);    // G接VCC
        GPIO_SetRL(source, 1);  // S接地
        GPIO_SetRL(drain, 2);   // D通过680Ω接VCC
        
        Timer_DelayUs(1000);
        
        uint32_t v_drain = ADC_ReadTP_mV(drain);
        uint32_t v_source = ADC_ReadTP_mV(source);
        
        /* 计算Rds(on) */
        float i_ds = (ADC_VREF_MV - v_drain) / (float)R_LOW;  // mA
        if (i_ds > 0.1) {
            rds_on = (v_drain - v_source) / i_ds;  // Ω
        }
        
        GPIO_SetRL(gate, 0);
        GPIO_SetRL(source, 0);
        GPIO_SetRL(drain, 0);
    } else {
        /* P沟道: G接地使其完全导通 */
        GPIO_SetRL(gate, 1);    // G接地
        GPIO_SetRL(source, 2);  // S接VCC
        GPIO_SetRL(drain, 1);   // D通过680Ω接地
        
        Timer_DelayUs(1000);
        
        uint32_t v_drain = ADC_ReadTP_mV(drain);
        uint32_t v_source = ADC_ReadTP_mV(source);
        
        float i_ds = v_drain / (float)R_LOW;
        if (i_ds > 0.1) {
            rds_on = (v_source - v_drain) / i_ds;
        }
        
        GPIO_SetRL(gate, 0);
        GPIO_SetRL(source, 0);
        GPIO_SetRL(drain, 0);
    }
    
    return rds_on;
}
```

### 7. lcd.c - LCD显示驱动 (ST7735)

```c
/**
 * @file lcd.c
 * @brief ST7735 LCD驱动实现
 */

#include "lcd.h"
#include "font.h"
#include <string.h>

/* SPI句柄 */
SPI_HandleTypeDef hspi1;

/* 显示缓冲区 */
static uint16_t lcd_buffer[LCD_WIDTH];

/* 颜色定义 */
#define COLOR_BLACK     0x0000
#define COLOR_WHITE     0xFFFF
#define COLOR_RED       0xF800
#define COLOR_GREEN     0x07E0
#define COLOR_BLUE      0x001F
#define COLOR_YELLOW    0xFFE0
#define COLOR_CYAN      0x07FF
#define COLOR_MAGENTA   0xF81F
#define COLOR_ORANGE    0xFD20

/* ST7735命令定义 */
#define ST7735_NOP      0x00
#define ST7735_SWRESET  0x01
#define ST7735_SLPOUT   0x11
#define ST7735_NORON    0x13
#define ST7735_INVOFF   0x20
#define ST7735_INVON    0x21
#define ST7735_DISPOFF  0x28
#define ST7735_DISPON   0x29
#define ST7735_CASET    0x2A
#define ST7735_RASET    0x2B
#define ST7735_RAMWR    0x2C
#define ST7735_MADCTL   0x36
#define ST7735_COLMOD   0x3A
#define ST7735_FRMCTR1  0xB1
#define ST7735_FRMCTR2  0xB2
#define ST7735_FRMCTR3  0xB3
#define ST7735_INVCTR   0xB4
#define ST7735_PWCTR1   0xC0
#define ST7735_PWCTR2   0xC1
#define ST7735_PWCTR3   0xC2
#define ST7735_PWCTR4   0xC3
#define ST7735_PWCTR5   0xC4
#define ST7735_VMCTR1   0xC5
#define ST7735_GMCTRP1  0xE0
#define ST7735_GMCTRN1  0xE1

/* 内部函数声明 */
static void LCD_WriteCommand(uint8_t cmd);
static void LCD_WriteData(uint8_t data);
static void LCD_WriteData16(uint16_t data);
static void LCD_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

/**
 * @brief 初始化SPI
 */
static void LCD_SPI_Init(void)
{
    __HAL_RCC_SPI1_CLK_ENABLE();
    
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;  // 18MHz
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    HAL_SPI_Init(&hspi1);
    
    /* 配置SPI引脚 */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    /* PA5 = SCK, PA7 = MOSI */
    GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/**
 * @brief 写命令
 */
static void LCD_WriteCommand(uint8_t cmd)
{
    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_RESET);  // DC=0 命令
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);  // CS=0
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);    // CS=1
}

/**
 * @brief 写数据
 */
static void LCD_WriteData(uint8_t data)
{
    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);    // DC=1 数据
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, &data, 1, 100);
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
}

/**
 * @brief 写16位数据
 */
static void LCD_WriteData16(uint16_t data)
{
    uint8_t buf[2] = {data >> 8, data & 0xFF};
    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, buf, 2, 100);
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
}

/**
 * @brief 设置显示窗口
 */
static void LCD_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    LCD_WriteCommand(ST7735_CASET);
    LCD_WriteData16(x0);
    LCD_WriteData16(x1);
    
    LCD_WriteCommand(ST7735_RASET);
    LCD_WriteData16(y0);
    LCD_WriteData16(y1);
    
    LCD_WriteCommand(ST7735_RAMWR);
}

/**
 * @brief 初始化LCD
 */
void LCD_Init(void)
{
    LCD_SPI_Init();
    
    /* 硬件复位 */
    HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(100);
    
    /* 软件复位 */
    LCD_WriteCommand(ST7735_SWRESET);
    HAL_Delay(150);
    
    /* 退出睡眠模式 */
    LCD_WriteCommand(ST7735_SLPOUT);
    HAL_Delay(500);
    
    /* 帧率控制 */
    LCD_WriteCommand(ST7735_FRMCTR1);
    LCD_WriteData(0x01);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x2D);
    
    LCD_WriteCommand(ST7735_FRMCTR2);
    LCD_WriteData(0x01);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x2D);
    
    LCD_WriteCommand(ST7735_FRMCTR3);
    LCD_WriteData(0x01);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x2D);
    LCD_WriteData(0x01);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x2D);
    
    /* 反转控制 */
    LCD_WriteCommand(ST7735_INVCTR);
    LCD_WriteData(0x07);
    
    /* 电源控制 */
    LCD_WriteCommand(ST7735_PWCTR1);
    LCD_WriteData(0xA2);
    LCD_WriteData(0x02);
    LCD_WriteData(0x84);
    
    LCD_WriteCommand(ST7735_PWCTR2);
    LCD_WriteData(0xC5);
    
    LCD_WriteCommand(ST7735_PWCTR3);
    LCD_WriteData(0x0A);
    LCD_WriteData(0x00);
    
    LCD_WriteCommand(ST7735_PWCTR4);
    LCD_WriteData(0x8A);
    LCD_WriteData(0x2A);
    
    LCD_WriteCommand(ST7735_PWCTR5);
    LCD_WriteData(0x8A);
    LCD_WriteData(0x);
    
    LCD_WriteCommand(ST7735_VMCTR1);
    LCD_WriteData(0x0E);
    
    /* 显示方向 */
    LCD_WriteCommand(ST7735_MADCTL);
    LCD_WriteData(0xC8);  // RGB顺序，行列交换
    
    /* 颜色格式 16bit */
    LCD_WriteCommand(ST7735_COLMOD);
    LCD_WriteData(0x05);
    
    /* Gamma校正 */
    LCD_WriteCommand(ST7735_GMCTRP1);
    LCD_WriteData(0x02);
    LCD_WriteData(0x1C);
    LCD_WriteData(0x07);
    LCD_WriteData(0x12);
    LCD_WriteData(0x37);
    LCD_WriteData(0x32);
    LCD_WriteData(0x29);
    LCD_WriteData(0x2D);
    LCD_WriteData(0x29);
    LCD_WriteData(0x25);
    LCD_WriteData(0x2B);
    LCD_WriteData(0x39);
    LCD_WriteData(0x00);
    LCD_WriteData(0x01);
    LCD_WriteData(0x03);
    LCD_WriteData(0x10);
    
    LCD_WriteCommand(ST7735_GMCTRN1);
    LCD_WriteData(0x03);
    LCD_WriteData(0x1D);
    LCD_WriteData(0x07);
    LCD_WriteData(0x06);
    LCD_WriteData(0x2E);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x29);
    LCD_WriteData(0x2D);
    LCD_WriteData(0x2E);
    LCD_WriteData(0x2E);
    LCD_WriteData(0x37);
    LCD_WriteData(0x3F);
    LCD_WriteData(0x00);
    LCD_WriteData(0x00);
    LCD_WriteData(0x02);
    LCD_WriteData(0x10);
    
    /* 正常显示模式 */
    LCD_WriteCommand(ST7735_NORON);
    HAL_Delay(10);
    
    /* 开启显示 */
    LCD_WriteCommand(ST7735_DISPON);
    HAL_Delay(100);
    
    /* 清屏 */
    LCD_Clear(COLOR_BLACK);
    
    /* 打开背光 */
    HAL_GPIO_WritePin(LCD_BL_PORT, LCD_BL_PIN, GPIO_PIN_SET);
}

/**
 * @brief 清屏
 */
void LCD_Clear(uint16_t color)
{
    LCD_SetWindow(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
    
    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);
    
    uint8_t buf[2] = {color >> 8, color & 0xFF};
    for (uint32_t i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
        HAL_SPI_Transmit(&hspi1, buf, 2, 100);
    }
    
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
}

/**
 * @brief 画点
 */
void LCD_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    
    LCD_SetWindow(x, y, x, y);
    LCD_WriteData16(color);
}

/**
 * @brief 填充矩形
 */
void LCD_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    if (x + w > LCD_WIDTH) w = LCD_WIDTH - x;
    if (y + h > LCD_HEIGHT) h = LCD_HEIGHT - y;
    
    LCD_SetWindow(x, y, x + w - 1, y + h - 1);
    
    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);
    
    uint8_t buf[2] = {color >> 8, color & 0xFF};
    for (uint32_t i = 0; i < w * h; i++) {
        HAL_SPI_Transmit(&hspi1, buf, 2, 100);
    }
    
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
}

/**
 * @brief 显示字符
 */
void LCD_DrawChar(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg, uint8_t size)
{
    if (c < 32 || c > 126) c = '?';
    
    for (uint8_t i = 0; i < 6; i++) {
        uint8_t line = (i < 5) ? Font5x7[c - 32][i] : 0;
        for (uint8_t j = 0; j < 8; j++) {
            if (line & (1 << j)) {
                if (size == 1) {
                    LCD_DrawPixel(x + i, y + j, color);
                } else {
                    LCD_FillRect(x + i * size, y + j * size, size, size, color);
                }
            } else if (bg != color) {
                if (size == 1) {
                    LCD_DrawPixel(x + i, y + j, bg);
                } else {
                    LCD_FillRect(x + i * size, y + j * size, size, size, bg);
                }
            }
        }
    }
}

/**
 * @brief 显示字符串
 */
void LCD_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t size)
{
    while (*str) {
        LCD_DrawChar(x, y, *str++, color, bg, size);
        x += 6 * size;
        if (x + 6 * size > LCD_WIDTH) {
            x = 0;
            y += 8 * size;
        }
        if (y + 8 * size > LCD_HEIGHT) break;
    }
}

/**
 * @brief 显示整数
 */
void LCD_DrawInt(uint16_t x, uint16_t y, int32_t num, uint16_t color, uint16_t bg, uint8_t size)
{
    char buf[12];
    sprintf(buf, "%ld", num);
    LCD_DrawString(x, y, buf, color, bg, size);
}

/**
 * @brief 显示浮点数
 */
void LCD_DrawFloat(uint16_t x, uint16_t y, float num, uint8_t decimals, uint16_t color, uint16_t bg, uint8_t size)
{
    char buf[16];
    char fmt[8];
    sprintf(fmt, "%%.%df", decimals);
    sprintf(buf, fmt, num);
    LCD_DrawString(x, y, buf, color, bg, size);
}
```

### 8. ui.c - 用户界面

```c
/**
 * @file ui.c
 * @brief 用户界面实现
 */

#include "ui.h"
#include "lcd.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

/* 颜色定义 */
#define UI_COLOR_BG         0x0000      // 黑色背景
#define UI_COLOR_TEXT       0xFFFF      // 白色文字
#define UI_COLOR_TITLE      0x07E0      // 绿色标题
#define UI_COLOR_VALUE      0xFFE0      // 黄色数值
#define UI_COLOR_UNIT       0x07FF      // 青色单位
#define UI_COLOR_PIN        0xF800      // 红色引脚
#define UI_COLOR_WARN       0xFD20      // 橙色警告

/**
 * @brief 显示开机画面
 */
void UI_ShowSplash(void)
{
    LCD_Clear(UI_COLOR_BG);
    
    LCD_DrawString(20, 40, "Transistor", UI_COLOR_TITLE, UI_COLOR_BG, 2);
    LCD_DrawString(30, 65, "Tester", UI_COLOR_TITLE, UI_COLOR_BG, 2);
    LCD_DrawString(25, 100, "STM32 Version", UI_COLOR_TEXT, UI_COLOR_BG, 1);
    LCD_DrawString(40, 120, "v1.0.0", UI_COLOR_TEXT, UI_COLOR_BG, 1);
}

/**
 * @brief 显示电池电压
 */
void UI_ShowBatteryVoltage(uint32_t voltage_mv)
{
    char buf[16];
    sprintf(buf, "Bat: %ld.%02ldV", voltage_mv / 1000, (voltage_mv % 1000) / 10);
    LCD_DrawString(30, 140, buf, UI_COLOR_TEXT, UI_COLOR_BG, 1);
}

/**
 * @brief 显示低电量警告
 */
void UI_ShowLowBattery(void)
{
    LCD_FillRect(10, 60, 108, 40, UI_COLOR_WARN);
    LCD_DrawString(20, 70, "LOW BATTERY!", UI_COLOR_BG, UI_COLOR_WARN, 1);
    LCD_DrawString(15, 85, "Please charge", UI_COLOR_BG, UI_COLOR_WARN, 1);
}

/**
 * @brief 显示待机界面
 */
void UI_ShowReady(void)
{
    LCD_Clear(UI_COLOR_BG);
    
    LCD_DrawString(10, 10, "Component Tester", UI_COLOR_TITLE, UI_COLOR_BG, 1);
    LCD_DrawString(10, 30, "----------------", UI_COLOR_TEXT, UI_COLOR_BG, 1);
    
    /* 显示测试端口示意图 */
    LCD_DrawString(10, 50, "Test Ports:", UI_COLOR_TEXT, UI_COLOR_BG, 1);
    LCD_DrawString(20, 70, "1   2   3", UI_COLOR_PIN, UI_COLOR_BG, 2);
    LCD_DrawString(20, 95, "|   |   |", UI_COLOR_TEXT, UI_COLOR_BG, 1);
    
    LCD_DrawString(5, 120, "Insert component", UI_COLOR_TEXT, UI_COLOR_BG, 1);
    LCD_DrawString(5, 135, "Press key to test", UI_COLOR_VALUE, UI_COLOR_BG, 1);
}

/**
 * @brief 显示测量中
 */
void UI_ShowMeasuring(void)
{
    LCD_Clear(UI_COLOR_BG);
    LCD_DrawString(25, 70, "Measuring...", UI_COLOR_TITLE, UI_COLOR_BG, 1);
    LCD_DrawString(20, 90, "Please wait", UI_COLOR_TEXT, UI_COLOR_BG, 1);
}

/**
 * @brief 显示测量结果
 */
void UI_ShowResult(MeasureResult_t *result)
{
    LCD_Clear(UI_COLOR_BG);
    
    char buf[32];
    
    switch (result->type) {
        case COMP_NONE:
            UI_ShowNoComponent();
            break;
            
        case COMP_RESISTOR:
            UI_ShowResistor(result);
            break;
            
        case COMP_CAPACITOR:
            UI_ShowCapacitor(result);
            break;
            
        case COMP_INDUCTOR:
            UI_ShowInductor(result);
            break;
            
        case COMP_DIODE:
            UI_ShowDiode(result);
            break;
            
        case COMP_BJT_NPN:
        case COMP_BJT_PNP:
            UI_ShowBJT(result);
            break;
            
        case COMP_MOSFET_N:
        case COMP_MOSFET_P:
            UI_ShowMOSFET(result);
            break;
            
        default:
            UI_ShowUnknown();
            break;
    }
    
    /* 底部提示 */
    LCD_DrawString(5, 145, "Press key...", UI_COLOR_TEXT, UI_COLOR_BG, 1);
}

/**
 * @brief 显示未检测到元件
 */
void UI_ShowNoComponent(void)
{
    LCD_DrawString(10, 10, "No Component", UI_COLOR_WARN, UI_COLOR_BG, 1);
    LCD_DrawString(10, 40, "Please check:", UI_COLOR_TEXT, UI_COLOR_BG, 1);
    LCD_DrawString(10, 60, "- Connection", UI_COLOR_TEXT, UI_COLOR_BG, 1);
    LCD_DrawString(10, 75, "- Component", UI_COLOR_TEXT, UI_COLOR_BG, 1);
    LCD_DrawString(10, 90, "- Discharge cap", UI_COLOR_TEXT, UI_COLOR_BG, 1);
}

/**
 * @brief 显示电阻结果
 */
void UI_ShowResistor(MeasureResult_t *result)
{
    char buf[32];
    
    LCD_DrawString(10, 10, "RESISTOR", UI_COLOR_TITLE, UI_COLOR_BG, 2);
    
    /* 显示引脚 */
    sprintf(buf, "Pins: %d - %d", result->pin1, result->pin2);
    LCD_DrawString(10, 40, buf, UI_COLOR_PIN, UI_COLOR_BG, 1);
    
    /* 显示电阻值 */
    LCD_DrawString(10, 60, "R =", UI_COLOR_TEXT, UI_COLOR_BG, 1);
    
    if (result->resistance < 1.0) {
        sprintf(buf, "%.2f mOhm", result->resistance * 1000);
    } else if (result->resistance < 1000) {
        sprintf(buf, "%.2f Ohm", result->resistance);
    } else if (result->resistance < 1000000) {
        sprintf(buf, "%.3f kOhm", result->resistance / 1000);
    } else {
        sprintf(buf, "%.3f MOhm", result->resistance / 1000000);
    }
    LCD_DrawString(10, 80, buf, UI_COLOR_VALUE, UI_COLOR_BG, 2);
    
    /* 显示电阻符号 */
    LCD_DrawString(40, 115, "[===]", UI_COLOR_TEXT, UI_COLOR_BG, 1);
    sprintf(buf, "%d       %d", result->pin1, result->pin2);
    LCD_DrawString(35, 130, buf, UI_COLOR_PIN, UI_COLOR_BG, 1);
}

/**
 * @brief 显示电容结果
 */
void UI_ShowCapacitor(MeasureResult_t *result)
{
    char buf[32];
    
    LCD_DrawString(10, 10, "CAPACITOR", UI_COLOR_TITLE, UI_COLOR_BG, 2);
    
    /* 显示引脚 */
    sprintf(buf, "Pins: %d - %d", result->pin1, result->pin2);
    LCD_DrawString(10, 35, buf, UI_COLOR_PIN, UI_COLOR_BG, 1);
    
    /* 显示电容值 */
    LCD_DrawString(10, 50, "C =", UI_COLOR_TEXT, UI_COLOR_BG, 1);
    
    float cap = result->capacitance;
    if (cap < 1e-9) {
        sprintf(buf, "%.2f pF", cap * 1e12);
    } else if (cap < 1e-6) {
        sprintf(buf, "%.3f nF", cap * 1e9);
    } else if (cap < 1e-3) {
        sprintf(buf, "%.3f uF", cap * 1e6);
    } else {
        sprintf(buf, "%.2f mF", cap * 1e3);
    }
    LCD_DrawString(10, 65, buf, UI_COLOR_VALUE, UI_COLOR_BG, 2);
    
    /* 显示ESR */
    if (result->esr > 0) {
        sprintf(buf, "ESR: %.2f Ohm", result->esr);
        LCD_DrawString(10, 90, buf, UI_COLOR_TEXT, UI_COLOR_BG, 1);
    }
    
    /* 显示损耗 */
    if (result->vloss > 0) {
        sprintf(buf, "VLoss: %.1f%%", result->vloss);
        LCD_DrawString(10, 105, buf, UI_COLOR_TEXT, UI_COLOR_BG, 1);
    }
    
    /* 显示电容符号 */
    LCD_DrawString(40, 125, "| |", UI_COLOR_TEXT, UI_COLOR_BG, 1);
}

/**
 * @brief 显示电感结果
 */
void UI_ShowInductor(MeasureResult_t *result)
{
    char buf[32];
    
    LCD_DrawString(10, 10, "INDUCTOR", UI_COLOR_TITLE, UI_COLOR_BG, 2);
    
    sprintf(buf, "Pins: %d - %d", result->pin1, result->pin2);
    LCD_DrawString(10, 40, buf, UI_COLOR_PIN, UI_COLOR_BG, 1);
    
    LCD_DrawString(10, 60, "L =", UI_COLOR_TEXT, UI_COLOR_BG, 1);
    
    float ind = result->inductance;
    if (ind < 1e-3) {
        sprintf(buf, "%.2f uH", ind * 1e6);
    } else if (ind < 1) {
        sprintf(buf, "%.3f mH", ind * 1e3);
    } else {
        sprintf(buf, "%.3f H", ind);
    }
    LCD_DrawString(10, 80, buf, UI_COLOR_VALUE, UI_COLOR_BG, 2);
    
    /* 显示直流电阻 */
    if (result->dcr > 0) {
        sprintf(buf, "DCR: %.2f Ohm", result->dcr);
        LCD_DrawString(10, 105, buf, UI_COLOR_TEXT, UI_COLOR_BG, 1);
    }
}

/**
 * @brief 显示二极管结果
 */
void UI_ShowDiode(MeasureResult_t *result)
{
    char buf[32];
    
    LCD_DrawString(10, 10, "DIODE", UI_COLOR_TITLE, UI_COLOR_BG, 2);
    
    /* 显示引脚 */
    sprintf(buf, "A=%d  K=%d", result->pin1, result->pin2);
    LCD_DrawString(10, 40, buf, UI_COLOR_PIN, UI_COLOR_BG, 1);
    
    /* 显示正向压降 */
    sprintf(buf, "Vf = %.3f V", result->vf);
    LCD_DrawString(10, 60, buf, UI_COLOR_VALUE, UI_COLOR_BG, 1);
    
    /* 显示结电容 */
    if (result->cj > 0) {
        float cj = result->cj;
        if (cj < 1e-9) {
            sprintf(buf, "Cj = %.1f pF", cj * 1e12);
        } else {
            sprintf(buf, "Cj = %.2f nF", cj * 1e9);
        }
        LCD_DrawString(10, 80, buf, UI_COLOR_TEXT, UI_COLOR_BG, 1);
    }
    
    /* 显示反向漏电流 */
    if (result->ir > 0) {
        sprintf(buf, "Ir = %.2f uA", result->ir * 1e6);
        LCD_DrawString(10, 95, buf, UI_COLOR_TEXT, UI_COLOR_BG, 1);
    }
    
    /* 显示二极管符号 */
    LCD_DrawString(30, 115, "-->|--", UI_COLOR_TEXT, UI_COLOR_BG, 2);
    sprintf(buf, "%d      %d", result->pin1, result->pin2);
    LCD_DrawString(25, 140, buf, UI_COLOR_PIN, UI_COLOR_BG, 1);
}

/**
 * @brief 显示三极管结果
 */
void UI_ShowBJT(MeasureResult_t *result)
{
    char buf[32];
    
    /* 显示类型 */
    if (result->type == COMP_BJT_NPN) {
        LCD_DrawString(10, 5, "NPN", UI_COLOR_TITLE, UI_COLOR_BG, 2);
    } else {
        LCD_DrawString(10, 5, "PNP", UI_COLOR_TITLE, UI_COLOR_BG, 2);
    }
    
    /* 显示引脚定义 */
    sprintf(buf, "B=%d C=%d E=%d", result->pin1, result->pin2, result->pin3);
    LCD_DrawString(10, 30, buf, UI_COLOR_PIN, UI_COLOR_BG, 1);
    
    /* 显示hFE */
    sprintf(buf, "hFE = %.0f", result->hfe);
    LCD_DrawString(10, 50, buf, UI_COLOR_VALUE, UI_COLOR_BG, 1);
    
    /* 显示Vbe */
    sprintf(buf, "Vbe = %.3f V", result->vbe);
    LCD_DrawString(10, 70, buf, UI_COLOR_TEXT, UI_COLOR_BG, 1);
    
    /* 显示Iceo */
    if (result->ice0 > 0) {
        sprintf(buf, "Iceo= %.2f uA", result->ice0 * 1e6);
        LCD_DrawString(10, 90, buf, UI_COLOR_TEXT, UI_COLOR_BG, 1);
    }
    
    /* 显示三极管符号 */
    if (result->type == COMP_BJT_NPN) {
        LCD_DrawString(50, 110, "  |", UI_COLOR_TEXT, UI_COLOR_BG, 1);
        LCD_DrawString(50, 120, "--|", UI_COLOR_TEXT, UI_COLOR_BG, 1);
        LCD_DrawString(50, 130, "  |>", UI_COLOR_TEXT, UI_COLOR_BG, 1);
    } else {
        LCD_DrawString(50, 110, "  |", UI_COLOR_TEXT, UI_COLOR_BG, 1);
        LCD_DrawString(50, 120, "--|", UI_COLOR_TEXT, UI_COLOR_BG, 1);
        LCD_DrawString(50, 130, " <|", UI_COLOR_TEXT, UI_COLOR_BG, 1);
    }
}

/**
 * @brief 显示MOSFET结果
 */
void UI_ShowMOSFET(MeasureResult_t *result)
{
    char buf[32];
    
    /* 显示类型 */
    if (result->type == COMP_MOSFET_N) {
        LCD_DrawString(5, 5, "N-MOSFET", UI_COLOR_TITLE, UI_COLOR_BG, 2);
    } else {
        LCD_DrawString(5, 5, "P-MOSFET", UI_COLOR_TITLE, UI_COLOR_BG, 2);
    }
    
    /* 显示引脚定义 */
    sprintf(buf, "G=%d D=%d S=%d", result->pin1, result->pin2, result->pin3);
    LCD_DrawString(10, 30, buf, UI_COLOR_PIN, UI_COLOR_BG, 1);
    
    /* 显示阈值电压 */
    sprintf(buf, "Vth = %.2f V", result->vth);
    LCD_DrawString(10, 50, buf, UI_COLOR_VALUE, UI_COLOR_BG, 1);
    
    /* 显示栅源电容 */
    if (result->cgs > 0) {
        float cgs = result->cgs;
        if (cgs < 1e-9) {
            sprintf(buf, "Cgs = %.0f pF", cgs * 1e12);
        } else {
            sprintf(buf, "Cgs = %.2f nF", cgs * 1e9);
        }
        LCD_DrawString(10, 70, buf, UI_COLOR_TEXT, UI_COLOR_BG, 1);
    }
    
    /* 显示导通电阻 */
    if (result->rds_on > 0) {
        sprintf(buf, "Rds = %.2f Ohm", result->rds_on);
        LCD_DrawString(10, 90, buf, UI_COLOR_TEXT, UI_COLOR_BG, 1);
    }
    
    /* 显示MOSFET符号 */
    LCD_DrawString(50, 110, " |--", UI_COLOR_TEXT, UI_COLOR_BG, 1);
    LCD_DrawString(50, 120, "-|  ", UI_COLOR_TEXT, UI_COLOR_BG, 1);
    LCD_DrawString(50, 130, " |--", UI_COLOR_TEXT, UI_COLOR_BG, 1);
}

/**
 * @brief 显示未知元件
 */
void UI_ShowUnknown(void)
{
    LCD_DrawString(10, 10, "UNKNOWN", UI_COLOR_WARN, UI_COLOR_BG, 2);
    LCD_DrawString(10, 50, "Component type", UI_COLOR_TEXT, UI_COLOR_BG, 1);
    LCD_DrawString(10, 70, "not recognized", UI_COLOR_TEXT, UI_COLOR_BG, 1);
}

/**
 * @brief 显示关机界面
 */
void UI_ShowPowerOff(void)
{
    LCD_Clear(UI_COLOR_BG);
    LCD_DrawString(20, 70, "Power Off...", UI_COLOR_TEXT, UI_COLOR_BG, 1);
}
```

### 9. font.h - 字体数据

```c
/**
 * @file font.h
 * @brief 5x7 ASCII字体数据
 */

#ifndef __FONT_H
#define __FONT_H

#include <stdint.h>

/* 5x7 ASCII字体 (32-126) */
static const uint8_t Font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // 32 (space)
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // 33 !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // 34 "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // 35 #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // 36 $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // 37 %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // 38 &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // 39 '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // 40 (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // 41 )
    {0x08, 0x2A, 0x1C, 0x2A, 0x08}, // 42 *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // 43 +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // 44 ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // 45 -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // 46 .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // 47 /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 48 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 49 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 50 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 51 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 52 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 53 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 54 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 55 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 56 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 57 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // 58 :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // 59 ;
    {0x00, 0x08, 0x14, 0x22, 0x41}, // 60 <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // 61 =
    {0x41, 0x22, 0x14, 0x08, 0x00}, // 62 >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // 63 ?
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // 64 @
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // 65 A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // 66 B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // 67 C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 68 D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // 69 E
    {0x7F, 0x09, 0x09, 0x01, 0x01}, // 70 F
    {0x3E, 0x41, 0x41, 0x51, 0x32}, // 71 G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 72 H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // 73 I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // 74 J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // 75 K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // 76 L
    {0x7F, 0x02, 0x04, 0x02, 0x7F}, // 77 M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 78 N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 79 O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // 80 P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 81 Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // 82 R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // 83 S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // 84 T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 85 U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 86 V
    {0x7F, 0x20, 0x18, 0x20, 0x7F}, // 87 W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 88 X
    {0x03, 0x04, 0x78, 0x04, 0x03}, // 89 Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // 90 Z
    {0x00, 0x00, 0x7F, 0x41, 0x41}, // 91 [
    {0x02, 0x04, 0x08, 0x10, 0x20}, // 92 backslash
    {0x41, 0x41, 0x7F, 0x00, 0x00}, // 93 ]
    {0x04, 0x02, 0x01, 0x02, 0x04}, // 94 ^
    {0x40, 0x40, 0x40, 0x40, 0x40}, // 95 _
    {0x00, 0x01, 0x02, 0x04, 0x00}, // 96 `
    {0x20, 0x54, 0x54, 0x54, 0x78}, // 97 a
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // 98 b
    {0x38, 0x44, 0x44, 0x44, 0x20}, // 99 c
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // 100 d
    {0x38, 0x54, 0x54, 0x54, 0x18}, // 101 e
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // 102 f
    {0x08, 0x14, 0x54, 0x54, 0x3C}, // 103 g
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // 104 h
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // 105 i
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // 106 j
    {0x00, 0x7F, 0x10, 0x28, 0x44}, // 107 k
    {0x00,
```





```
继续完成代码
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

已完成生成

继续完成剩余的代码：

### 9. font.h - 字体数据（续）

```c
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // 108 l
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // 109 m
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // 110 n
    {0x38, 0x44, 0x44, 0x44, 0x38}, // 111 o
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // 112 p
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // 113 q
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // 114 r
    {0x48, 0x54, 0x54, 0x54, 0x20}, // 115 s
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // 116 t
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // 117 u
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // 118 v
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // 119 w
    {0x44, 0x28, 0x10, 0x28, 0x44}, // 120 x
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // 121 y
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // 122 z
    {0x00, 0x08, 0x36, 0x41, 0x00}, // 123 {
    {0x00, 0x00, 0x7F, 0x00, 0x00}, // 124 |
    {0x00, 0x41, 0x36, 0x08, 0x00}, // 125 }
    {0x08, 0x08, 0x2A, 0x1C, 0x08}, // 126 ~
};

#endif /* __FONT_H */
```

### 10. timer.h - 定时器头文件

```c
/**
 * @file timer.h
 * @brief 定时器驱动头文件
 */

#ifndef __TIMER_H
#define __TIMER_H

#include "config.h"

/* 函数声明 */
void Timer_Init_All(void);
uint32_t Timer_GetMicros(void);
void Timer_ResetMicros(void);
void Timer_DelayUs(uint32_t us);
uint32_t Timer_GetElapsedUs(uint32_t start_us);

/* 外部变量 */
extern volatile uint32_t g_us_counter;

#endif /* __TIMER_H */
```

### 11. lcd.h - LCD头文件

```c
/**
 * @file lcd.h
 * @brief LCD驱动头文件
 */

#ifndef __LCD_H
#define __LCD_H

#include "config.h"

/* 函数声明 */
void LCD_Init(void);
void LCD_Clear(uint16_t color);
void LCD_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void LCD_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void LCD_DrawChar(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg, uint8_t size);
void LCD_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t size);
void LCD_DrawInt(uint16_t x, uint16_t y, int32_t num, uint16_t color, uint16_t bg, uint8_t size);
void LCD_DrawFloat(uint16_t x, uint16_t y, float num, uint8_t decimals, uint16_t color, uint16_t bg, uint8_t size);

#endif /* __LCD_H */
```

### 12. ui.h - UI头文件

```c
/**
 * @file ui.h
 * @brief 用户界面头文件
 */

#ifndef __UI_H
#define __UI_H

#include "config.h"

/* 函数声明 */
void UI_ShowSplash(void);
void UI_ShowBatteryVoltage(uint32_t voltage_mv);
void UI_ShowLowBattery(void);
void UI_ShowReady(void);
void UI_ShowMeasuring(void);
void UI_ShowResult(MeasureResult_t *result);
void UI_ShowNoComponent(void);
void UI_ShowResistor(MeasureResult_t *result);
void UI_ShowCapacitor(MeasureResult_t *result);
void UI_ShowInductor(MeasureResult_t *result);
void UI_ShowDiode(MeasureResult_t *result);
void UI_ShowBJT(MeasureResult_t *result);
void UI_ShowMOSFET(MeasureResult_t *result);
void UI_ShowUnknown(void);
void UI_ShowPowerOff(void);

#endif /* __UI_H */
```

### 13. analyze.h - 分析功能头文件

```c
/**
 * @file analyze.h
 * @brief 元件分析功能头文件
 */

#ifndef __ANALYZE_H
#define __ANALYZE_H

#include "config.h"

/* 函数声明 */
void Analyze_Component(void);
void Analyze_Resistor(void);
void Analyze_Capacitor(void);
void Analyze_Diode(void);
void Analyze_BJT(void);
void Analyze_MOSFET(void);

/* 辅助测量函数 */
float Measure_DiodeCapacitance(uint8_t anode, uint8_t cathode);
float Measure_DiodeLeakage(uint8_t anode, uint8_t cathode);
float Measure_hFE(uint8_t base, uint8_t collector, uint8_t emitter, uint8_t is_npn);
float Measure_MOSFET_Vth(uint8_t gate, uint8_t drain, uint8_t source, uint8_t is_n_ch);
float Measure_MOSFET_Cgs(uint8_t gate, uint8_t source);
float Measure_MOSFET_RdsOn(uint8_t gate, uint8_t drain, uint8_t source, uint8_t is_n_ch);

#endif /* __ANALYZE_H */
```

### 14. main.h - 主程序头文件

```c
/**
 * @file main.h
 * @brief 主程序头文件
 */

#ifndef __MAIN_H
#define __MAIN_H

#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* 外部变量声明 */
extern volatile uint32_t g_systick_ms;
extern volatile uint8_t g_key_pressed;
extern volatile uint32_t g_idle_counter;

/* 函数声明 */
void SystemClock_Config(void);
void System_PowerOff(void);
void Error_Handler(void);

#endif /* __MAIN_H */
```

### 15. stm32f1xx_it.c - 中断处理

```c
/**
 * @file stm32f1xx_it.c
 * @brief 中断服务程序
 */

#include "main.h"
#include "stm32f1xx_it.h"

/* 外部变量 */
extern TIM_HandleTypeDef htim3;

/**
 * @brief NMI中断处理
 */
void NMI_Handler(void)
{
}

/**
 * @brief HardFault中断处理
 */
void HardFault_Handler(void)
{
    while (1) {
    }
}

/**
 * @brief MemManage中断处理
 */
void MemManage_Handler(void)
{
    while (1) {
    }
}

/**
 * @brief BusFault中断处理
 */
void BusFault_Handler(void)
{
    while (1) {
    }
}

/**
 * @brief UsageFault中断处理
 */
void UsageFault_Handler(void)
{
    while (1) {
    }
}

/**
 * @brief SVC中断处理
 */
void SVC_Handler(void)
{
}

/**
 * @brief DebugMon中断处理
 */
void DebugMon_Handler(void)
{
}

/**
 * @brief PendSV中断处理
 */
void PendSV_Handler(void)
{
}

/**
 * @brief SysTick中断处理
 */
void SysTick_Handler(void)
{
    HAL_IncTick();
    HAL_SYSTICK_IRQHandler();
}

/**
 * @brief EXTI1中断处理 (按键)
 */
void EXTI1_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
}

/**
 * @brief TIM3中断处理
 */
void TIM3_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim3);
}
```

### 16. stm32f1xx_it.h - 中断头文件

```c
/**
 * @file stm32f1xx_it.h
 * @brief 中断服务程序头文件
 */

#ifndef __STM32F1XX_IT_H
#define __STM32F1XX_IT_H

void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);
void EXTI1_IRQHandler(void);
void TIM3_IRQHandler(void);

#endif /* __STM32F1XX_IT_H */
```

### 17. eeprom.c - EEPROM模拟（使用Flash）

```c
/**
 * @file eeprom.c
 * @brief 使用Flash模拟EEPROM存储校准数据
 */

#include "eeprom.h"
#include "stm32f1xx_hal.h"

/* Flash页地址 (使用最后一页) */
#define EEPROM_START_ADDRESS    ((uint32_t)0x0800FC00)  // 64KB Flash的最后1KB
#define EEPROM_PAGE_SIZE        1024

/* 校准数据结构 */
typedef struct {
    uint32_t magic;             // 魔数，用于验证数据有效性
    uint16_t adc_offset[3];     // ADC偏移校准
    uint16_t r_low_cal;         // 680Ω校准值
    uint32_t r_high_cal;        // 470kΩ校准值
    uint16_t vref_cal;          // 参考电压校准值
    uint16_t checksum;          // 校验和
} CalibrationData_t;

#define CALIB_MAGIC     0xCAL1B328

static CalibrationData_t calib_data;

/**
 * @brief 计算校验和
 */
static uint16_t Calculate_Checksum(CalibrationData_t *data)
{
    uint16_t sum = 0;
    uint8_t *ptr = (uint8_t *)data;
    
    for (int i = 0; i < sizeof(CalibrationData_t) - 2; i++) {
        sum += ptr[i];
    }
    
    return sum;
}

/**
 * @brief 初始化EEPROM
 */
void EEPROM_Init(void)
{
    /* 读取校准数据 */
    memcpy(&calib_data, (void *)EEPROM_START_ADDRESS, sizeof(CalibrationData_t));
    
    /* 验证数据有效性 */
    if (calib_data.magic != CALIB_MAGIC || 
        Calculate_Checksum(&calib_data) != calib_data.checksum) {
        /* 数据无效，使用默认值 */
        EEPROM_SetDefaults();
    }
}

/**
 * @brief 设置默认校准值
 */
void EEPROM_SetDefaults(void)
{
    calib_data.magic = CALIB_MAGIC;
    calib_data.adc_offset[0] = 0;
    calib_data.adc_offset[1] = 0;
    calib_data.adc_offset[2] = 0;
    calib_data.r_low_cal = 680;
    calib_data.r_high_cal = 470000;
    calib_data.vref_cal = 3300;
    calib_data.checksum = Calculate_Checksum(&calib_data);
}

/**
 * @brief 保存校准数据到Flash
 */
HAL_StatusTypeDef EEPROM_Save(void)
{
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef erase_init;
    uint32_t page_error;
    
    /* 更新校验和 */
    calib_data.checksum = Calculate_Checksum(&calib_data);
    
    /* 解锁Flash */
    HAL_FLASH_Unlock();
    
    /* 擦除页 */
    erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
    erase_init.PageAddress = EEPROM_START_ADDRESS;
    erase_init.NbPages = 1;
    
    status = HAL_FLASHEx_Erase(&erase_init, &page_error);
    if (status != HAL_OK) {
        HAL_FLASH_Lock();
        return status;
    }
    
    /* 写入数据 */
    uint32_t *src = (uint32_t *)&calib_data;
    uint32_t addr = EEPROM_START_ADDRESS;
    
    for (int i = 0; i < sizeof(CalibrationData_t) / 4; i++) {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, src[i]);
        if (status != HAL_OK) {
            HAL_FLASH_Lock();
            return status;
        }
        addr += 4;
    }
    
    /* 锁定Flash */
    HAL_FLASH_Lock();
    
    return HAL_OK;
}

/**
 * @brief 获取ADC偏移校准值
 */
uint16_t EEPROM_GetADCOffset(uint8_t channel)
{
    if (channel < 3) {
        return calib_data.adc_offset[channel];
    }
    return 0;
}

/**
 * @brief 设置ADC偏移校准值
 */
void EEPROM_SetADCOffset(uint8_t channel, uint16_t offset)
{
    if (channel < 3) {
        calib_data.adc_offset[channel] = offset;
    }
}

/**
 * @brief 获取低阻值电阻校准值
 */
uint16_t EEPROM_GetRLowCal(void)
{
    return calib_data.r_low_cal;
}

/**
 * @brief 设置低阻值电阻校准值
 */
void EEPROM_SetRLowCal(uint16_t value)
{
    calib_data.r_low_cal = value;
}

/**
 * @brief 获取高阻值电阻校准值
 */
uint32_t EEPROM_GetRHighCal(void)
{
    return calib_data.r_high_cal;
}

/**
 * @brief 设置高阻值电阻校准值
 */
void EEPROM_SetRHighCal(uint32_t value)
{
    calib_data.r_high_cal = value;
}

/**
 * @brief 获取参考电压校准值
 */
uint16_t EEPROM_GetVrefCal(void)
{
    return calib_data.vref_cal;
}

/**
 * @brief 设置参考电压校准值
 */
void EEPROM_SetVrefCal(uint16_t value)
{
    calib_data.vref_cal = value;
}
```

### 18. eeprom.h - EEPROM头文件

```c
/**
 * @file eeprom.h
 * @brief EEPROM模拟头文件
 */

#ifndef __EEPROM_H
#define __EEPROM_H

#include "config.h"

/* 函数声明 */
void EEPROM_Init(void);
void EEPROM_SetDefaults(void);
HAL_StatusTypeDef EEPROM_Save(void);

uint16_t EEPROM_GetADCOffset(uint8_t channel);
void EEPROM_SetADCOffset(uint8_t channel, uint16_t offset);

uint16_t EEPROM_GetRLowCal(void);
void EEPROM_SetRLowCal(uint16_t value);

uint32_t EEPROM_GetRHighCal(void);
void EEPROM_SetRHighCal(uint32_t value);

uint16_t EEPROM_GetVrefCal(void);
void EEPROM_SetVrefCal(uint16_t value);

#endif /* __EEPROM_H */
```

### 19. calibration.c - 自校准功能

```c
/**
 * @file calibration.c
 * @brief 自校准功能实现
 */

#include "calibration.h"
#include "gpio.h"
#include "adc.h"
#include "timer.h"
#include "lcd.h"
#include "eeprom.h"
#include "ui.h"
#include <stdio.h>

/* 颜色定义 */
#define CAL_COLOR_BG        0x0000
#define CAL_COLOR_TEXT      0xFFFF
#define CAL_COLOR_OK        0x07E0
#define CAL_COLOR_ERR       0xF800

/**
 * @brief 执行自校准
 * @return 0=成功, 其他=错误代码
 */
int Calibration_Run(void)
{
    char buf[32];
    int result = 0;
    
    LCD_Clear(CAL_COLOR_BG);
    LCD_DrawString(10, 10, "SELF CALIBRATION", CAL_COLOR_TEXT, CAL_COLOR_BG, 1);
    LCD_DrawString(10, 30, "================", CAL_COLOR_TEXT, CAL_COLOR_BG, 1);
    
    /* 步骤1: 短接所有测试端口 */
    LCD_DrawString(10, 50, "Step 1:", CAL_COLOR_TEXT, CAL_COLOR_BG, 1);
    LCD_DrawString(10, 65, "Short all probes", CAL_COLOR_TEXT, CAL_COLOR_BG, 1);
    LCD_DrawString(10, 80, "Press key...", CAL_COLOR_TEXT, CAL_COLOR_BG, 1);
    
    /* 等待按键 */
    extern volatile uint8_t g_key_pressed;
    g_key_pressed = 0;
    while (!g_key_pressed) {
        HAL_Delay(10);
    }
    g_key_pressed = 0;
    
    /* 测量零点偏移 */
    LCD_DrawString(10, 95, "Measuring...", CAL_COLOR_TEXT, CAL_COLOR_BG, 1);
    
    uint32_t offset_sum[3] = {0, 0, 0};
    
    for (int i = 0; i < 10; i++) {
        GPIO_DischargeAll();
        HAL_Delay(10);
        
        for (int ch = 0; ch < 3; ch++) {
            offset_sum[ch] += ADC_ReadTP_mV(ch);
        }
    }
    
    /* 计算平均偏移 */
    for (int ch = 0; ch < 3; ch++) {
        uint16_t offset = offset_sum[ch] / 10;
        EEPROM_SetADCOffset(ch, offset);
        
        sprintf(buf, "TP%d offset: %dmV", ch + 1, offset);
        LCD_DrawString(10, 110 + ch * 12, buf, CAL_COLOR_TEXT, CAL_COLOR_BG, 1);
        
        /* 检查偏移是否在合理范围内 */
        if (offset > 50) {
            result = -1;
        }
    }
    
    HAL_Delay(1000);
    
    /* 步骤2: 断开所有测试端口 */
    LCD_Clear(CAL_COLOR_BG);
    LCD_DrawString(10, 10, "SELF CALIBRATION", CAL_COLOR_TEXT, CAL_COLOR_BG, 1);
    LCD_DrawString(10, 30, "================", CAL_COLOR_TEXT, CAL_COLOR_BG, 1);
    
    LCD_DrawString(10, 50, "Step 2:", CAL_COLOR_TEXT, CAL_COLOR_BG, 1);
    LCD_DrawString(10, 65, "Open all probes", CAL_COLOR_TEXT, CAL_COLOR_BG, 1);
    LCD_DrawString(10, 80, "Press key...", CAL_COLOR_TEXT, CAL_COLOR_BG, 1);
    
    g_key_pressed = 0;
    while (!g_key_pressed) {
        HAL_Delay(10);
    }
    g_key_pressed = 0;
    
    LCD_DrawString(10, 95, "Measuring...", CAL_COLOR_TEXT, CAL_COLOR_BG, 1);
    
    /* 测量开路电压 */
    GPIO_DischargeAll();
    HAL_Delay(100);
    
    uint8_t open_ok = 1;
    for (int ch = 0; ch < 3; ch++) {
        /* 通过470kΩ上拉 */
        GPIO_SetRH(ch, 2);
        HAL_Delay(10);
        
        uint32_t v_open = ADC_ReadTP_mV(ch);
        GPIO_SetRH(ch, 0);
        
        sprintf(buf, "TP%d open: %ldmV", ch + 1, v_open);
        LCD_DrawString(10, 110 + ch * 12, buf, CAL_COLOR_TEXT, CAL_COLOR_BG, 1);
        
        /* 开路时电压应该接近VCC */
        if (v_open < ADC_VREF_MV - 200) {
            open_ok = 0;
            result = -2;
        }
    }
    
    HAL_Delay(1000);
    
    /* 步骤3: 连接校准电容 (可选) */
    LCD_Clear(CAL_COLOR_BG);
    LCD_DrawString(10, 10, "SELF CALIBRATION", CAL_COLOR_TEXT, CAL_COLOR_BG, 1);
    LCD_DrawString(10, 30, "================", CAL_COLOR_TEXT, CAL_COLOR_BG, 1);
    
    LCD_DrawString(10, 50, "Step 3 (Optional):", CAL_COLOR_TEXT, CAL_COLOR_BG, 1);
    LCD_DrawString(10, 65, "Connect 100nF cap", CAL_COLOR_TEXT, CAL_COLOR_BG, 1);
    LCD_DrawString(10, 80, "between TP1-TP3", CAL_COLOR_TEXT, CAL_COLOR_BG, 1);
    LCD_DrawString(10, 95, "Press key...", CAL_COLOR_TEXT, CAL_COLOR_BG, 1);
    
    g_key_pressed = 0;
    uint32_t wait_start = HAL_GetTick();
    while (!g_key_pressed) {
        HAL_Delay(10);
        /* 5秒超时跳过 */
        if (HAL_GetTick() - wait_start > 5000) {
            break;
        }
    }
    
    if (g_key_pressed) {
        g_key_pressed = 0;
        
        LCD_DrawString(10, 110, "Measuring...", CAL_COLOR_TEXT, CAL_COLOR_BG, 1);
        
        /* 测量电容充电时间来校准 */
        GPIO_DischargeAll();
        HAL_Delay(100);
        
        uint32_t threshold_mv = (uint32_t)(ADC_VREF_MV * 0.632);
        uint32_t charge_time = Measure_ChargeTime_us(0, 2, threshold_mv);
        
        if (charge_time > 0) {
            /* 100nF @ 470kΩ 理论时间约47ms */
            float measured_r = (float)charge_time / 100e-9 / 1000000.0f;
            
            sprintf(buf, "R_high: %.0f Ohm", measured_r);
            LCD_DrawString(10, 125, buf, CAL_COLOR_TEXT, CAL_COLOR_BG, 1);
            
            /* 更新校准值 */
            EEPROM_SetRHighCal((uint32_t)measured_r);
        }
    }
    
    /* 保存校准数据 */
    LCD_Clear(CAL_COLOR_BG);
    LCD_DrawString(10, 60, "Saving...", CAL_COLOR_TEXT, CAL_COLOR_BG, 1);
    
    if (EEPROM_Save() == HAL_OK) {
        LCD_DrawString(10, 80, "Calibration OK!", CAL_COLOR_OK, CAL_COLOR_BG, 1);
    } else {
        LCD_DrawString(10, 80, "Save failed!", CAL_COLOR_ERR, CAL_COLOR_BG, 1);
        result = -3;
    }
    
    HAL_Delay(2000);
    
    return result;
}

/**
 * @brief 检查是否需要校准
 */
uint8_t Calibration_IsNeeded(void)
{
    /* 检查校准数据是否有效 */
    uint16_t vref = EEPROM_GetVrefCal();
    
    if (vref < 3000 || vref > 3600) {
        return 1;  // 需要校准
    }
    
    return 0;
}
```

### 20. calibration.h - 校准头文件

```c
/**
 * @file calibration.h
 * @brief 自校准功能头文件
 */

#ifndef __CALIBRATION_H
#define __CALIBRATION_H

#include "config.h"

/* 函数声明 */
int Calibration_Run(void);
uint8_t Calibration_IsNeeded(void);

#endif /* __CALIBRATION_H */
```

------

## 四、硬件原理图说明

```text
                          STM32F103C8T6 晶体管测试仪原理图
    
    ┌─────────────────────────────────────────────────────────────────────┐
    │                                                                     │
    │   电源部分:                                                          │
    │   ┌─────┐     ┌─────┐                                               │
    │   │9V   │────►│LDO  │────► 3.3V                                     │
    │   │电池 │     │3.3V │                                               │
    │   └─────┘     └─────┘                                               │
    │       │                                                             │
    │       └──[R1]──[R2]──► VBAT_ADC (PB1)                               │
    │                                                                     │
    │   测试端口部分:                                                       │
    │                                                                     │
    │   TP1 ──┬──────────────────────────────► PA0 (ADC_IN0)              │
    │         │                                                           │
    │         ├──[680Ω]──────────────────────► PA1 (RL1控制)              │
    │         │                                                           │
    │         └──[470kΩ]─────────────────────► PA2 (RH1控制)              │
    │                                                                     │
    │   TP2 ──┬──────────────────────────────► PA3 (ADC_IN3)              │
    │         │                                                           │
    │         ├──[680Ω]──────────────────────► PA4 (RL2控制)              │
    │         │                                                           │
    │         └──[470kΩ]─────────────────────► PA5 (RH2控制)              │
    │                                                                     │
    │   TP3 ──┬──────────────────────────────► PA6 (ADC_IN6)              │
    │         │                                                           │
    │         ├──[680Ω]──────────────────────► PA7 (RL3控制)              │
    │         │                                                           │
    │         └──[470kΩ]─────────────────────► PB0 (RH3控制)              │
    │                                                                     │
    │   LCD部分 (ST7735 SPI):                                              │
    │   ┌─────────┐                                                       │
    │   │ ST7735  │                                                       │
    │   │         │                                                       │
    │   │ SCK  ◄──┼─────────────────────────── PA5 (SPI1_SCK)             │
    │   │ MOSI ◄──┼─────────────────────────── PA7 (SPI1_MOSI)            │
    │   │ CS   ◄──┼─────────────────────────── PB12                       │
    │   │ DC   ◄──┼─────────────────────────── PB13                       │
    │   │ RST  ◄──┼─────────────────────────── PB14                       │
    │   │ BL   ◄──┼─────────────────────────── PB15                       │
    │   └─────────┘                                                       │
    │                                                                     │
    │   按键:                                                              │
    │   ┌───┐                                                             │
    │   │KEY│──────────────────────────────── PB1 (EXTI1)                 │
    │   └───┘                                                             │
    │     │                                                               │
    │    GND                                                              │
    │                                                                     │
    └─────────────────────────────────────────────────────────────────────┘
```

------

## 五、编译和烧录说明

### 1. 使用Keil MDK编译

```text
1. 创建新工程，选择STM32F103C8T6
2. 添加所有源文件到工程
3. 配置Include路径
4. 编译生成HEX文件
```

### 2. 使用STM32CubeIDE编译

```text
1. 创建STM32工程
2. 配置时钟和外设
3. 导入源代码
4. 编译生成BIN文件
```

### 3. 烧录方法

```bash
# 使用ST-Link
st-flash write firmware.bin 0x08000000

# 使用OpenOCD
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "program firmware.bin 0x08000000 verify reset exit"
```

------

## 六、使用说明

1. **开机**：按下按键开机
2. **测量**：将元件插入测试座，按下按键开始测量
3. **查看结果**：屏幕显示元件类型和参数
4. **自校准**：长按按键进入校准模式
5. **关机**：30秒无操作自动关机

------

这套代码提供了完整的M328晶体管测试仪STM32移植框架，包括：

- GPIO控制
- ADC采集
- 定时器
- LCD显示
- 元件测量算法
- 用户界面
- EEPROM存储
- 自校准功能

您可以根据实际硬件进行调整和优化。如有任何问题，欢迎继续询问！









推荐的开源项目与资源

如果您不想从零开始，可以参考以下已经完成移植的成熟方案：

1. **[mean00/stm32ComponentTester (GitHub)](https://github.com/mean00/stm32ComponentTester)**:
   - **特点**: 专门为 STM32F103/GD32 优化的版本。
   - **支持**: 支持常见的 Blue Pill 开发板，使用 CMake 构建。
2. **[madires/Transistortester-Warehouse](https://github.com/madires/Transistortester-Warehouse)**:
   - **特点**: 包含 Karl-Heinz Kübbeler 和 Markus Seidl 的原始代码库。虽然核心是 AVR，但其文档详细解释了测量逻辑，是移植时的核心参考。
3. **基于 Arduino 框架移植**:
   - 可以通过 **STM32Duino** 环境，将原有的 Arduino 兼容版 M328 代码移植到 STM32。这涉及将引脚编号映射到 STM32 的 PA/PB 端口。 ![GitHub](data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAAb1BMVEX///8kKS74+PgsMTY+Q0f8/Pzw8PGRk5ZNUVXr6+xDSEzCxMUpLjLV1tYuMzc/Q0i3ubuUl5lbX2NTV1uLjpFKTlKxs7Xh4uOFh4rn5+h3en1vcnZ9gIPc3d6/wcJzd3o3PECmqKpjZ2rOz9CfoaRP5W5KAAABU0lEQVQ4jW1T7aKCIAzdEEUTSUuzLMtuvf8zXtiQVDx/lJ3DvgEIEPU9l1rL/F4LiFEMCQYkj2JDC6VxhaRJl3x2xgin7MePMuYRD7cQftrjrcL7ELk9nNt8yXXtyRm5mquzXADKA6LpzgZRlgBPZ1WO7yn/yrmqyGdGn95Zjav2hbNghZHMV3uNG3DZCigEmhRq+pmyrUAcifj4CO+49xciWqAe6p3pCJrNH1ATp5gH6KghQLrDnoB8GxaYPYFkhpONipjrl+wI61hQ8tB5EvYb4eTL5Ebhd8u/2f4EYfhvvWJpw9ZJ0LQfb5uxbMKi3pp5xVp30miexZHbRriHvUnojrKt7isXqWRBFQQNF2w3aYCPUl8/EREWzxtuE+pVI/wbkSGp0a51Nyg1F+L5/nel56xXgt+zcMgeC0FK27gdTzVoPXtI9DBuO2tRhJj94m3/A1GiDZXoM3d5AAAAAElFTkSuQmCC)GitHub +4

硬件连接要点

| 组件                 | STM32 引脚示例 (Blue Pill) | 说明                                     |
| :------------------- | :------------------------- | :--------------------------------------- |
| **测试端子 (TP1-3)** | PA0, PA1, PA2              | 连接精密电阻网络（通常为 680Ω 和 470kΩ） |
| **显示屏 (SPI/I2C)** | SPI1 (PA5/6/7) 或 I2C1     | 根据屏幕类型（如 ST7735 或 OLED）配置    |
| **电源控制**         | 任意可用 GPIO              | 实现长按开机、自动关机功能               |