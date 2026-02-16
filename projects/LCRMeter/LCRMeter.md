## 基于STM32的高精密LCR测试仪设计

### 一、系统概述

LCR测试仪用于精准测量电感(L)、电容(C)、电阻(R)等无源元件参数，广泛应用于电子研发、生产质检、设备维修等场景。

### 二、推荐芯片选型

|    芯片型号     |         特点          |   适用场景    |
| :-------------: | :-------------------: | :-----------: |
|  **STM32F103**  |   72MHz主频，成本低   | 入门级/教学用 |
| **STM32F4系列** | 168MHz主频，高精度ADC |   中端应用    |
|  **STM32H750**  |  480MHz主频，24位ADC  | 高精度专业级  |

### 三、核心设计原理

#### 1. 交流电桥法 + 数字信号处理

```text
激励信号生成 → 交流电桥测量 → 信号采集 → 算法解算 → 参数输出
```

#### 2. 主要测量方法

- **谐振法**：利用LC谐振频率计算元件值
- **相敏检波法**：测量幅值与相位，解算阻抗参数
- **NE555振荡法**：将R/L/C转换为频率信号

### 四、硬件设计框架

```text
┌─────────────────────────────────────────────────────────┐
│                    STM32主控制器                         │
│  ┌─────┐  ┌─────┐  ┌─────┐  ┌─────┐  ┌─────┐           │
│  │ DAC │  │ ADC │  │ TIM │  │ GPIO│  │ SPI │           │
│  └──┬──┘  └──┬──┘  └──┬──┘  └──┬──┘  └──┬──┘           │
└─────┼───────┼───────┼───────┼───────┼──────────────────┘
      │       │       │       │       │
      ▼       ▼       ▼       ▼       ▼
┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐
│激励信号 │ │信号采集 │ │频率测量 │ │量程切换 │ │数据存储 │
│生成电路 │ │调理电路 │ │输入捕获 │ │继电器组 │ │Flash   │
└─────────┘ └─────────┘ └─────────┘ └─────────┘ └─────────┘
```

### 五、关键硬件模块

#### 1. 激励信号生成

- 使用STM32的DAC + DMA输出正弦波
- 频率范围：100Hz ~ 1MHz（可程控调节）
- 配合锁相环保证频率稳定性

#### 2. 信号采集电路

- 仪表放大器（如AD8421）进行差分放大
- 有源滤波电路提高信噪比
- 高精度ADC采样（过采样+平均滤波）

#### 3. 量程自动切换

- CD4052多路选择开关
- 程控继电器切换分压/分流电阻网络

#### 4. 显示与交互

- LCD触摸屏（推荐4.3寸或更大）
- 按键输入

### 六、软件架构

```c
// 主程序框架
int main(void)
{
    System_Init();           // 系统初始化
    ADC_Init_Config();       // ADC配置
    DAC_Init_Config();       // DAC配置
    TIM_InputCapture_Init(); // 定时器输入捕获
    
    while(1)
    {
        Generate_Excitation();    // 生成激励信号
        Sample_Signal();          // 采集响应信号
        Calculate_LCR();          // 计算L/C/R参数
        Display_Result();         // 显示结果
        Auto_Range_Switch();      // 自动量程切换
    }
}
```

### 七、关键算法

#### 1. 相敏检波算法

```c
// 计算阻抗幅值和相位
float amplitude = sqrt(I_real*I_real + I_imag*I_imag);
float phase = atan2(I_imag, I_real);

// 根据相位判断元件类型并计算参数
if(phase > 0) {
    // 感性负载，计算电感值
    L = Z * sin(phase) / (2 * PI * freq);
} else {
    // 容性负载，计算电容值
    C = 1 / (Z * sin(-phase) * 2 * PI * freq);
}
```

#### 2. 输入捕获测频

```c
// TIM4定时器配置 (84MHz系统时钟)
htim4.Init.Prescaler = 83;  // 84MHz/(83+1) = 1MHz → 每tick 1μs
htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
```

### 八、性能指标参考（STM32H750方案）

|       参数        |      指标       |
| :---------------: | :-------------: |
|   测量频率范围    |  100Hz ~ 1MHz   |
|   电阻测量精度    |     ≤ ±0.1%     |
| 电感/电容测量精度 |     ≤ ±0.2%     |
|     电阻量程      |   100Ω ~ 1MΩ    |
|     电容量程      | 100pF ~ 10000pF |
|     电感量程      |  100μH ~ 10mH   |

### 九、设计建议

1. **高精度要求**：选用STM32H750，配合24位外置ADC（如ADS1256）
2. **成本敏感**：选用STM32F103 + NE555振荡电路方案
3. **校准**：必须进行零点校准和增益校准，使用标准元件
4. **PCB设计**：模拟与数字部分分区，注意接地处理

如果您需要更详细的某个模块设计（如具体电路图、完整代码等），请告诉我！

参考资料





```
具体电路图和完整代码
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

根据搜索结果，我为您整理一套完整的基于STM32的高精密LCR测试仪设计方案，包括电路图和完整代码：

------

## 基于STM32的高精密LCR测试仪 - 完整设计

### 一、系统总体框图

```text
┌────────────────────────────────────────────────────────────────┐
│                         电源模块 (5V/3.3V)                      │
└────────────────────────────────────────────────────────────────┘
                                │
        ┌───────────────────────┼───────────────────────┐
        │                       │                       │
        ▼                       ▼                       ▼
┌───────────────┐      ┌───────────────┐      ┌───────────────┐
│  NE555振荡电路 │      │ STM32F103C8T6 │      │  LCD1602显示  │
│  (R/L/C转频率) │◄────►│   主控制器     │◄────►│    模块       │
└───────────────┘      └───────────────┘      └───────────────┘
        │                       │                       
        │                       │                       
        ▼                       ▼                       
┌───────────────┐      ┌───────────────┐      ┌───────────────┐
│ CD4052多路    │      │   按键模块    │      │  LED指示灯    │
│ 选择开关      │◄────►│  (模式切换)   │      │  (R/L/C指示)  │
└───────────────┘      └───────────────┘      └───────────────┘
```

------

### 二、硬件电路原理图

#### 1. 主控制器电路 (STM32F103C8T6)

```text
                          VDD(3.3V)
                              │
                         ┌────┴────┐
                         │  100nF  │
                         └────┬────┘
                              │
    ┌─────────────────────────┼─────────────────────────┐
    │                    STM32F103C8T6                   │
    │                                                    │
    │  PA0 ◄──── NE555输出(频率信号)                    │
    │  PA1 ◄──── ADC1(电压采样)                         │
    │  PA2 ◄──── ADC2(电流采样)                         │
    │                                                    │
    │  PB0 ────► CD4052_A (通道选择A)                   │
    │  PB1 ────► CD4052_B (通道选择B)                   │
    │                                                    │
    │  PB6 ────► LCD_RS                                 │
    │  PB7 ────► LCD_RW                                 │
    │  PB8 ────► LCD_EN                                 │
    │  PB9-PB12 ► LCD_D4-D7                             │
    │                                                    │
    │  PC13 ◄─── KEY1 (电容模式)                        │
    │  PC14 ◄─── KEY2 (电阻模式)                        │
    │  PC15 ◄─── KEY3 (电感模式)                        │
    │                                                    │
    │  PA8 ────► LED1 (蓝-电容)                         │
    │  PA9 ────► LED2 (红-电阻)                         │
    │  PA10 ───► LED3 (绿-电感)                         │
    │                                                    │
    │  NRST ◄─── 复位电路                               │
    │  OSC_IN/OUT ◄──► 8MHz晶振                         │
    └────────────────────────────────────────────────────┘
```

#### 2. NE555振荡电路 (核心测量电路)

```text
【电阻/电容测量 - RC振荡电路】

         VCC(5V)
            │
            ├────────────┬──────────────┐
            │            │              │
           ┌┴┐          ┌┴┐            │
           │ │R1        │ │Rx(被测)    │
           │ │10K       │ │            │
           └┬┘          └┬┘            │
            │            │              │
            ├────────────┤              │
            │            │              │
            │     ┌──────┴──────┐       │
            │     │    NE555    │       │
            │     │             │       │
            ├─────┤7(DIS)       │       │
            │     │             │       │
            │     │8(VCC)───────┼───────┤
            │     │             │       │
            │     │4(RST)───────┼───────┤
            │     │             │       │
            ├─────┤6(THR)       │       │
            │     │             │       │
            ├─────┤2(TRI)       │       │
            │     │             │       │
            │     │3(OUT)───────┼───────┼────► PA0(STM32)
            │     │             │       │
            │     │1(GND)───────┼───┐   │
            │     │             │   │   │
            │     │5(CV)────┐   │   │   │
            │     └─────────┼───┘   │   │
            │               │       │   │
           ┌┴┐             ┌┴┐      │   │
           │ │Cx(被测)     │ │10nF  │   │
           │ │             │ │      │   │
           └┬┘             └┬┘      │   │
            │               │       │   │
            └───────────────┴───────┴───┴──── GND


【电感测量 - 电容三点式振荡电路】

         VCC(5V)
            │
           ┌┴┐
           │ │R2
           │ │10K
           └┬┘
            │
            ├──────────────────────────────┐
            │                              │
            │     ┌──────────────┐         │
            │     │    NE555     │         │
            │     │              │         │
            ├─────┤7(DIS)        │         │
            │     │              │         │
            │     │8(VCC)────────┼─────────┤
            │     │              │         │
            │     │4(RST)────────┼─────────┤
            │     │              │         │
            ├─────┤6(THR)        │         │
            │     │              │         │
            ├─────┤2(TRI)        │         │
            │     │              │         │
            │     │3(OUT)────────┼─────────┼────► PA0
            │     │              │         │
            │     │1(GND)────────┼───┐     │
            │     │              │   │     │
            │     │5(CV)─────┐   │   │     │
            │     └──────────┼───┘   │     │
            │                │       │     │
           ┌┴┐              ┌┴┐      │     │
           │ │C1            │ │10nF  │     │
           │ │100nF         │ │      │     │
           └┬┘              └┬┘      │     │
            │                │       │     │
            ├────────────────┴───────┴─────┘
            │
          ──┴──
          ─┬─┬─  Lx(被测电感)
           │ │
           └─┘
            │
           GND
```

#### 3. CD4052多路选择开关电路

```text
                    VCC(5V)
                       │
                  ┌────┴────┐
                  │ CD4052  │
                  │         │
    PB0(STM32)───►│A     X0 │◄──── 电容测量电路
    PB1(STM32)───►│B     X1 │◄──── 电阻测量电路
                  │      X2 │◄──── 电感测量电路
                  │      X3 │◄──── (备用)
                  │         │
                  │   X(COM)│────► NE555输入
                  │         │
                  │   INH   │──┐
                  │         │  │
                  │   VSS   │──┴──── GND
                  │         │
                  │   VEE   │──────── GND
                  └─────────┘

通道选择真值表:
┌─────┬─────┬──────────┐
│  B  │  A  │ 选择通道  │
├─────┼─────┼──────────┤
│  0  │  0  │ X0(电容) │
│  0  │  1  │ X1(电阻) │
│  1  │  0  │ X2(电感) │
│  1  │  1  │ X3(备用) │
└─────┴─────┴──────────┘
```

#### 4. LCD1602显示电路

```text
         VCC(5V)
            │
    ┌───────┼───────────────────────────────┐
    │       │                               │
    │   ┌───┴───┐                           │
    │   │ 10K   │◄── 对比度调节             │
    │   │ 电位器 │                           │
    │   └───┬───┘                           │
    │       │                               │
    │   ┌───┴───────────────────────────┐   │
    │   │          LCD1602              │   │
    │   │                               │   │
    │   │ 1(VSS)────────────────GND     │   │
    │   │ 2(VDD)────────────────VCC     │   │
    │   │ 3(V0)─────────────────┘(电位器)   │
    │   │ 4(RS)◄────────────────PB6     │   │
    │   │ 5(RW)◄────────────────PB7     │   │
    │   │ 6(EN)◄────────────────PB8     │   │
    │   │ 7-10(D0-D3)───────────NC      │   │
    │   │ 11(D4)◄───────────────PB9     │   │
    │   │ 12(D5)◄───────────────PB10    │   │
    │   │ 13(D6)◄───────────────PB11    │   │
    │   │ 14(D7)◄───────────────PB12    │   │
    │   │ 15(LED+)──────────────VCC     │   │
    │   │ 16(LED-)──────────────GND     │   │
    │   └───────────────────────────────┘   │
    │                                       │
    └───────────────────────────────────────┘
```

#### 5. 电源电路

```text
    DC 9-12V输入
         │
         ▼
    ┌─────────┐
    │ AMS1117 │
    │  -5.0   │────► 5V输出 (NE555, CD4052, LCD)
    └─────────┘
         │
         ▼
    ┌─────────┐
    │ AMS1117 │
    │  -3.3   │────► 3.3V输出 (STM32)
    └─────────┘
         │
        GND

滤波电容配置:
- 输入端: 100μF电解 + 100nF陶瓷
- 输出端: 100μF电解 + 100nF陶瓷
```

------

### 三、完整程序代码

#### 1. 主程序 main.c

```c
/**
  ******************************************************************************
  * @file    main.c
  * @brief   基于STM32的高精密LCR测试仪 - 主程序
  * @author  
  * @version V1.0
  ******************************************************************************
  */

#include "stm32f10x.h"
#include "delay.h"
#include "lcd1602.h"
#include "key.h"
#include "led.h"
#include "timer.h"
#include "cd4052.h"
#include "lcr_measure.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* 全局变量 */
uint8_t g_MeasureMode = MODE_RESISTANCE;  // 默认电阻测量模式
uint32_t g_Frequency = 0;                  // 测量频率
float g_MeasureResult = 0;                 // 测量结果

/* 测量模式定义 */
#define MODE_CAPACITANCE  1
#define MODE_RESISTANCE   2
#define MODE_INDUCTANCE   3

/* 函数声明 */
void System_Init(void);
void Display_Result(uint8_t mode, float value);
void Key_Process(void);

/**
  * @brief  主函数
  */
int main(void)
{
    char displayBuf[17];
    
    /* 系统初始化 */
    System_Init();
    
    /* 默认显示 */
    LCD_Write_String(0, 0, " MEASURE Rx ");
    LCD_Write_String(0, 1, " 0000.000K  ");
    
    while(1)
    {
        /* 按键扫描处理 */
        Key_Process();
        
        /* 频率测量 */
        g_Frequency = Timer_GetFrequency();
        
        /* 根据模式计算LCR值 */
        switch(g_MeasureMode)
        {
            case MODE_CAPACITANCE:
                g_MeasureResult = Calculate_Capacitance(g_Frequency);
                break;
                
            case MODE_RESISTANCE:
                g_MeasureResult = Calculate_Resistance(g_Frequency);
                break;
                
            case MODE_INDUCTANCE:
                g_MeasureResult = Calculate_Inductance(g_Frequency);
                break;
        }
        
        /* 显示测量结果 */
        Display_Result(g_MeasureMode, g_MeasureResult);
        
        delay_ms(200);  // 刷新间隔
    }
}

/**
  * @brief  系统初始化
  */
void System_Init(void)
{
    /* 延时函数初始化 */
    delay_init();
    
    /* NVIC配置 */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    
    /* 外设初始化 */
    LED_Init();
    KEY_Init();
    CD4052_Init();
    LCD_Init();
    Timer_Init();
    
    /* 默认选择电阻测量通道 */
    CD4052_Select(MODE_RESISTANCE);
    LED_SetMode(MODE_RESISTANCE);
    
    delay_ms(100);
}

/**
  * @brief  按键处理函数
  */
void Key_Process(void)
{
    uint8_t key = KEY_Scan(0);
    
    if(key == KEY1_PRESS)  // 电容模式
    {
        g_MeasureMode = MODE_CAPACITANCE;
        CD4052_Select(MODE_CAPACITANCE);
        LED_SetMode(MODE_CAPACITANCE);
        LCD_Write_String(0, 0, " MEASURE Cx ");
        LCD_Write_String(0, 1, " 00000.0pF  ");
    }
    else if(key == KEY2_PRESS)  // 电阻模式
    {
        g_MeasureMode = MODE_RESISTANCE;
        CD4052_Select(MODE_RESISTANCE);
        LED_SetMode(MODE_RESISTANCE);
        LCD_Write_String(0, 0, " MEASURE Rx ");
        LCD_Write_String(0, 1, " 0000.000K  ");
    }
    else if(key == KEY3_PRESS)  // 电感模式
    {
        g_MeasureMode = MODE_INDUCTANCE;
        CD4052_Select(MODE_INDUCTANCE);
        LED_SetMode(MODE_INDUCTANCE);
        LCD_Write_String(0, 0, " MEASURE Lx ");
        LCD_Write_String(0, 1, " 000000uH   ");
    }
}

/**
  * @brief  显示测量结果
  * @param  mode: 测量模式
  * @param  value: 测量值
  */
void Display_Result(uint8_t mode, float value)
{
    char buf[17];
    
    switch(mode)
    {
        case MODE_CAPACITANCE:
            if(value < 1000)
                sprintf(buf, " %7.1fpF  ", value);
            else if(value < 1000000)
                sprintf(buf, " %7.2fnF  ", value/1000);
            else
                sprintf(buf, " %7.3fuF  ", value/1000000);
            break;
            
        case MODE_RESISTANCE:
            if(value < 1000)
                sprintf(buf, " %7.1f Ohm ", value);
            else if(value < 1000000)
                sprintf(buf, " %7.3fK   ", value/1000);
            else
                sprintf(buf, " %7.3fM   ", value/1000000);
            break;
            
        case MODE_INDUCTANCE:
            if(value < 1000)
                sprintf(buf, " %7.1fuH  ", value);
            else
                sprintf(buf, " %7.2fmH  ", value/1000);
            break;
    }
    
    LCD_Write_String(0, 1, buf);
}
```

#### 2. LCR测量算法 lcr_measure.c

```c
/**
  ******************************************************************************
  * @file    lcr_measure.c
  * @brief   LCR测量算法实现
  ******************************************************************************
  */

#include "lcr_measure.h"
#include <math.h>

/* 校准参数 (需要根据实际电路调整) */
#define R_REF           10000.0f    // 参考电阻 10K
#define C_REF           100e-12f    // 参考电容 100pF
#define L_REF           100e-6f     // 参考电感 100uH

/* NE555振荡频率计算常数 */
#define NE555_CONST     1.44f       // NE555频率公式常数

/* 滤波参数 */
#define FILTER_SIZE     16          // 滑动平均滤波窗口大小

static float filterBuffer[FILTER_SIZE] = {0};
static uint8_t filterIndex = 0;

/**
  * @brief  滑动平均滤波
  * @param  newValue: 新采样值
  * @retval 滤波后的值
  */
static float SlidingAverageFilter(float newValue)
{
    float sum = 0;
    uint8_t i;
    
    filterBuffer[filterIndex] = newValue;
    filterIndex = (filterIndex + 1) % FILTER_SIZE;
    
    for(i = 0; i < FILTER_SIZE; i++)
    {
        sum += filterBuffer[i];
    }
    
    return sum / FILTER_SIZE;
}

/**
  * @brief  计算电阻值
  * @param  freq: 测量频率 (Hz)
  * @retval 电阻值 (Ohm)
  * @note   基于NE555 RC振荡电路
  *         f = 1.44 / ((R1 + 2*Rx) * C)
  *         Rx = (1.44 / (f * C) - R1) / 2
  */
float Calculate_Resistance(uint32_t freq)
{
    float resistance;
    float C = 100e-9f;   // 100nF定时电容
    float R1 = 10000.0f; // 10K固定电阻
    
    if(freq == 0) return 0;
    
    // 计算电阻值
    resistance = (NE555_CONST / (freq * C) - R1) / 2.0f;
    
    // 限幅处理
    if(resistance < 0) resistance = 0;
    if(resistance > 10000000) resistance = 10000000;  // 最大10M
    
    // 滤波处理
    resistance = SlidingAverageFilter(resistance);
    
    return resistance;
}

/**
  * @brief  计算电容值
  * @param  freq: 测量频率 (Hz)
  * @retval 电容值 (pF)
  * @note   基于NE555 RC振荡电路
  *         f = 1.44 / ((R1 + 2*R2) * Cx)
  *         Cx = 1.44 / (f * (R1 + 2*R2))
  */
float Calculate_Capacitance(uint32_t freq)
{
    float capacitance;
    float R1 = 10000.0f;  // 10K
    float R2 = 10000.0f;  // 10K
    
    if(freq == 0) return 0;
    
    // 计算电容值 (单位: F)
    capacitance = NE555_CONST / (freq * (R1 + 2 * R2));
    
    // 转换为pF
    capacitance = capacitance * 1e12f;
    
    // 限幅处理
    if(capacitance < 0) capacitance = 0;
    if(capacitance > 100000000) capacitance = 100000000;  // 最大100uF
    
    // 滤波处理
    capacitance = SlidingAverageFilter(capacitance);
    
    return capacitance;
}

/**
  * @brief  计算电感值
  * @param  freq: 测量频率 (Hz)
  * @retval 电感值 (uH)
  * @note   基于LC振荡电路
  *         f = 1 / (2 * PI * sqrt(L * C))
  *         L = 1 / (4 * PI^2 * f^2 * C)
  */
float Calculate_Inductance(uint32_t freq)
{
    float inductance;
    float C = 100e-9f;   // 100nF谐振电容
    float PI_SQ = 9.8696f;  // PI^2
    
    if(freq == 0) return 0;
    
    // 计算电感值 (单位: H)
    inductance = 1.0f / (4.0f * PI_SQ * (float)freq * (float)freq * C);
    
    // 转换为uH
    inductance = inductance * 1e6f;
    
    // 限幅处理
    if(inductance < 0) inductance = 0;
    if(inductance > 100000) inductance = 100000;  // 最大100mH
    
    // 滤波处理
    inductance = SlidingAverageFilter(inductance);
    
    return inductance;
}

/**
  * @brief  自动识别元件类型
  * @param  freq1: 频率1 (1kHz激励)
  * @param  freq2: 频率2 (10kHz激励)
  * @retval 元件类型: 1-电容, 2-电阻, 3-电感
  */
uint8_t Auto_Identify_Component(uint32_t freq1, uint32_t freq2)
{
    float ratio = (float)freq1 / (float)freq2;
    
    // 电阻: 阻抗不随频率变化, ratio ≈ 1
    if(ratio > 0.95f && ratio < 1.05f)
    {
        return 2;  // 电阻
    }
    // 电容: 高频阻抗小, freq2 > freq1, ratio < 1
    else if(ratio < 0.95f)
    {
        return 1;  // 电容
    }
    // 电感: 高频阻抗大, freq2 < freq1, ratio > 1
    else
    {
        return 3;  // 电感
    }
}
```

#### 3. 定时器配置 (频率测量) timer.c

```c
/**
  ******************************************************************************
  * @file    timer.c
  * @brief   定时器配置 - 输入捕获测频
  ******************************************************************************
  */

#include "timer.h"
#include "stm32f10x.h"

/* 全局变量 */
static volatile uint32_t g_CaptureValue1 = 0;
static volatile uint32_t g_CaptureValue2 = 0;
static volatile uint8_t g_CaptureFlag = 0;
static volatile uint32_t g_Frequency = 0;
static volatile uint32_t g_OverflowCount = 0;

/**
  * @brief  定时器初始化 - 输入捕获模式
  * @note   使用TIM2_CH1 (PA0) 进行频率测量
  */
void Timer_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_ICInitTypeDef TIM_ICInitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    /* 使能时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    
    /* 配置PA0为浮空输入 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    /* 定时器基础配置 */
    // 72MHz / 72 = 1MHz, 每个计数周期1us
    TIM_TimeBaseStructure.TIM_Period = 0xFFFF;
    TIM_TimeBaseStructure.TIM_Prescaler = 71;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    
    /* 输入捕获配置 */
    TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
    TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;  // 上升沿捕获
    TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
    TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
    TIM_ICInitStructure.TIM_ICFilter = 0x03;  // 滤波
    TIM_ICInit(TIM2, &TIM_ICInitStructure);
    
    /* NVIC配置 */
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    /* 使能捕获中断和更新中断 */
    TIM_ITConfig(TIM2, TIM_IT_CC1 | TIM_IT_Update, ENABLE);
    
    /* 使能定时器 */
    TIM_Cmd(TIM2, ENABLE);
}

/**
  * @brief  TIM2中断服务函数
  */
void TIM2_IRQHandler(void)
{
    /* 更新中断 (溢出) */
    if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        g_OverflowCount++;
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
    
    /* 捕获中断 */
    if(TIM_GetITStatus(TIM2, TIM_IT_CC1) != RESET)
    {
        if(g_CaptureFlag == 0)
        {
            // 第一次捕获
            g_CaptureValue1 = TIM_GetCapture1(TIM2);
            g_OverflowCount = 0;
            g_CaptureFlag = 1;
        }
        else if(g_CaptureFlag == 1)
        {
            // 第二次捕获
            g_CaptureValue2 = TIM_GetCapture1(TIM2);
            
            // 计算周期 (单位: us)
            uint32_t period;
            if(g_OverflowCount == 0)
            {
                period = g_CaptureValue2 - g_CaptureValue1;
            }
            else
            {
                period = (g_OverflowCount * 65536) + g_CaptureValue2 - g_CaptureValue1;
            }
            
            // 计算频率 (Hz)
            if(period > 0)
            {
                g_Frequency = 1000000 / period;
            }
            
            g_CaptureFlag = 0;
        }
        
        TIM_ClearITPendingBit(TIM2, TIM_IT_CC1);
    }
}

/**
  * @brief  获取测量频率
  * @retval 频率值 (Hz)
  */
uint32_t Timer_GetFrequency(void)
{
    return g_Frequency;
}

/**
  * @brief  复位频率测量
  */
void Timer_ResetMeasure(void)
{
    g_CaptureFlag = 0;
    g_Frequency = 0;
    g_OverflowCount = 0;
}
```

#### 4. CD4052多路选择开关 cd4052.c

```c
/**
  ******************************************************************************
  * @file    cd4052.c
  * @brief   CD4052多路选择开关驱动
  ******************************************************************************
  */

#include "cd4052.h"
#include "stm32f10x.h"

/* 引脚定义 */
#define CD4052_A_PIN    GPIO_Pin_0
#define CD4052_B_PIN    GPIO_Pin_1
#define CD4052_PORT     GPIOB

#define CD4052_A_HIGH   GPIO_SetBits(CD4052_PORT, CD4052_A_PIN)
#define CD4052_A_LOW    GPIO_ResetBits(CD4052_PORT, CD4052_A_PIN)
#define CD4052_B_HIGH   GPIO_SetBits(CD4052_PORT, CD4052_B_PIN)
#define CD4052_B_LOW    GPIO_ResetBits(CD4052_PORT, CD4052_B_PIN)

/**
  * @brief  CD4052初始化
  */
void CD4052_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    /* 使能GPIOB时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    /* 配置PB0, PB1为推挽输出 */
    GPIO_InitStructure.GPIO_Pin = CD4052_A_PIN | CD4052_B_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(CD4052_PORT, &GPIO_InitStructure);
    
    /* 默认选择通道1 (电阻) */
    CD4052_A_HIGH;
    CD4052_B_LOW;
}

/**
  * @brief  选择测量通道
  * @param  channel: 通道号
  *         @arg 1: 电容测量 (B=0, A=0)
  *         @arg 2: 电阻测量 (B=0, A=1)
  *         @arg 3: 电感测量 (B=1, A=0)
  */
void CD4052_Select(uint8_t channel)
{
    switch(channel)
    {
        case 1:  // 电容
            CD4052_B_LOW;
            CD4052_A_LOW;
            break;
            
        case 2:  // 电阻
            CD4052_B_LOW;
            CD4052_A_HIGH;
            break;
            
        case 3:  // 电感
            CD4052_B_HIGH;
            CD4052_A_LOW;
            break;
            
        default:
            CD4052_B_LOW;
            CD4052_A_HIGH;
            break;
    }
}
```

#### 5. LCD1602驱动 lcd1602.c

```c
/**
  ******************************************************************************
  * @file    lcd1602.c
  * @brief   LCD1602液晶显示驱动 (4位模式)
  ******************************************************************************
  */

#include "lcd1602.h"
#include "delay.h"
#include "stm32f10x.h"

/* 引脚定义 */
#define LCD_RS_PIN      GPIO_Pin_6
#define LCD_RW_PIN      GPIO_Pin_7
#define LCD_EN_PIN      GPIO_Pin_8
#define LCD_D4_PIN      GPIO_Pin_9
#define LCD_D5_PIN      GPIO_Pin_10
#define LCD_D6_PIN      GPIO_Pin_11
#define LCD_D7_PIN      GPIO_Pin_12
#define LCD_PORT        GPIOB

/* 控制引脚操作宏 */
#define LCD_RS_HIGH     GPIO_SetBits(LCD_PORT, LCD_RS_PIN)
#define LCD_RS_LOW      GPIO_ResetBits(LCD_PORT, LCD_RS_PIN)
#define LCD_RW_HIGH     GPIO_SetBits(LCD_PORT, LCD_RW_PIN)
#define LCD_RW_LOW      GPIO_ResetBits(LCD_PORT, LCD_RW_PIN)
#define LCD_EN_HIGH     GPIO_SetBits(LCD_PORT, LCD_EN_PIN)
#define LCD_EN_LOW      GPIO_ResetBits(LCD_PORT, LCD_EN_PIN)

/**
  * @brief  写4位数据到LCD
  */
static void LCD_Write_4Bits(uint8_t data)
{
    GPIO_ResetBits(LCD_PORT, LCD_D4_PIN | LCD_D5_PIN | LCD_D6_PIN | LCD_D7_PIN);
    
    if(data & 0x01) GPIO_SetBits(LCD_PORT, LCD_D4_PIN);
    if(data & 0x02) GPIO_SetBits(LCD_PORT, LCD_D5_PIN);
    if(data & 0x04) GPIO_SetBits(LCD_PORT, LCD_D6_PIN);
    if(data & 0x08) GPIO_SetBits(LCD_PORT, LCD_D7_PIN);
    
    LCD_EN_HIGH;
    delay_us(1);
    LCD_EN_LOW;
    delay_us(50);
}

/**
  * @brief  写命令到LCD
  */
void LCD_Write_Cmd(uint8_t cmd)
{
    LCD_RS_LOW;
    LCD_RW_LOW;
    
    LCD_Write_4Bits(cmd >> 4);    // 高4位
    LCD_Write_4Bits(cmd & 0x0F);  // 低4位
    
    delay_us(50);
}

/**
  * @brief  写数据到LCD
  */
void LCD_Write_Data(uint8_t data)
{
    LCD_RS_HIGH;
    LCD_RW_LOW;
    
    LCD_Write_4Bits(data >> 4);    // 高4位
    LCD_Write_4Bits(data & 0x0F);  // 低4位
    
    delay_us(50);
}

/**
  * @brief  LCD初始化
  */
void LCD_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    /* 使能GPIOB时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    /* 配置LCD引脚为推挽输出 */
    GPIO_InitStructure.GPIO_Pin = LCD_RS_PIN | LCD_RW_PIN | LCD_EN_PIN |
                                   LCD_D4_PIN | LCD_D5_PIN | LCD_D6_PIN | LCD_D7_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LCD_PORT, &GPIO_InitStructure);
    
    /* LCD初始化序列 */
    delay_ms(50);
    
    LCD_RS_LOW;
    LCD_RW_LOW;
    
    // 4位模式初始化
    LCD_Write_4Bits(0x03);
    delay_ms(5);
    LCD_Write_4Bits(0x03);
    delay_ms(1);
    LCD_Write_4Bits(0x03);
    delay_ms(1);
    LCD_Write_4Bits(0x02);  // 设置4位模式
    delay_ms(1);
    
    LCD_Write_Cmd(0x28);    // 4位, 2行, 5x8点阵
    LCD_Write_Cmd(0x0C);    // 显示开, 光标关
    LCD_Write_Cmd(0x06);    // 地址自增, 不移屏
    LCD_Write_Cmd(0x01);    // 清屏
    delay_ms(2);
}

/**
  * @brief  设置光标位置
  * @param  x: 列 (0-15)
  * @param  y: 行 (0-1)
  */
void LCD_Set_Cursor(uint8_t x, uint8_t y)
{
    uint8_t addr;
    
    if(y == 0)
        addr = 0x80 + x;
    else
        addr = 0xC0 + x;
    
    LCD_Write_Cmd(addr);
}

/**
  * @brief  在指定位置显示字符串
  * @param  x: 列 (0-15)
  * @param  y: 行 (0-1)
  * @param  str: 字符串指针
  */
void LCD_Write_String(uint8_t x, uint8_t y, char *str)
{
    LCD_Set_Cursor(x, y);
    
    while(*str)
    {
        LCD_Write_Data(*str++);
    }
}

/**
  * @brief  清屏
  */
void LCD_Clear(void)
{
    LCD_Write_Cmd(0x01);
    delay_ms(2);
}
```

#### 6. 按键驱动 key.c

```c
/**
  ******************************************************************************
  * @file    key.c
  * @brief   按键驱动
  ******************************************************************************
  */

#include "key.h"
#include "delay.h"
#include "stm32f10x.h"

/* 引脚定义 */
#define KEY1_PIN    GPIO_Pin_13
#define KEY2_PIN    GPIO_Pin_14
#define KEY3_PIN    GPIO_Pin_15
#define KEY_PORT    GPIOC

#define KEY1_READ   GPIO_ReadInputDataBit(KEY_PORT, KEY1_PIN)
#define KEY2_READ   GPIO_ReadInputDataBit(KEY_PORT, KEY2_PIN)
#define KEY3_READ   GPIO_ReadInputDataBit(KEY_PORT, KEY3_PIN)

/**
  * @brief  按键初始化
  */
void KEY_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    /* 使能GPIOC时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    
    /* 配置按键引脚为上拉输入 */
    GPIO_InitStructure.GPIO_Pin = KEY1_PIN | KEY2_PIN | KEY3_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(KEY_PORT, &GPIO_InitStructure);
}

/**
  * @brief  按键扫描
  * @param  mode: 0-不支持连按, 1-支持连按
  * @retval 按键值: 0-无按键, 1-KEY1, 2-KEY2, 3-KEY3
  */
uint8_t KEY_Scan(uint8_t mode)
{
    static uint8_t key_up = 1;
    
    if(mode) key_up = 1;
    
    if(key_up && (KEY1_READ == 0 || KEY2_READ == 0 || KEY3_READ == 0))
    {
        delay_ms(10);  // 消抖
        key_up = 0;
        
        if(KEY1_READ == 0) return KEY1_PRESS;
        if(KEY2_READ == 0) return KEY2_PRESS;
        if(KEY3_READ == 0) return KEY3_PRESS;
    }
    else if(KEY1_READ == 1 && KEY2_READ == 1 && KEY3_READ == 1)
    {
        key_up = 1;
    }
    
    return 0;
}
```

#### 7. LED指示灯驱动 led.c

```c
/**
  ******************************************************************************
  * @file    led.c
  * @brief   LED指示灯驱动
  ******************************************************************************
  */

#include "led.h"
#include "stm32f10x.h"

/* 引脚定义 */
#define LED1_PIN    GPIO_Pin_8   // 蓝色 - 电容
#define LED2_PIN    GPIO_Pin_9   // 红色 - 电阻
#define LED3_PIN    GPIO_Pin_10  // 绿色 - 电感
#define LED_PORT    GPIOA

/**
  * @brief  LED初始化
  */
void LED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    /* 使能GPIOA时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    /* 配置LED引脚为推挽输出 */
    GPIO_InitStructure.GPIO_Pin = LED1_PIN | LED2_PIN | LED3_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LED_PORT, &GPIO_InitStructure);
    
    /* 默认全灭 */
    GPIO_SetBits(LED_PORT, LED1_PIN | LED2_PIN | LED3_PIN);
}

/**
  * @brief  设置LED显示模式
  * @param  mode: 1-电容(蓝), 2-电阻(红), 3-电感(绿)
  */
void LED_SetMode(uint8_t mode)
{
    /* 先全灭 */
    GPIO_SetBits(LED_PORT, LED1_PIN | LED2_PIN | LED3_PIN);
    
    /* 点亮对应LED */
    switch(mode)
    {
        case 1:  // 电容 - 蓝色
            GPIO_ResetBits(LED_PORT, LED1_PIN);
            break;
            
        case 2:  // 电阻 - 红色
            GPIO_ResetBits(LED_PORT, LED2_PIN);
            break;
            
        case 3:  // 电感 - 绿色
            GPIO_ResetBits(LED_PORT, LED3_PIN);
            break;
    }
}
```

#### 8. 头文件定义

```c
/* lcr_measure.h */
#ifndef __LCR_MEASURE_H
#define __LCR_MEASURE_H

#include "stm32f10x.h"

float Calculate_Resistance(uint32_t freq);
float Calculate_Capacitance(uint32_t freq);
float Calculate_Inductance(uint32_t freq);
uint8_t Auto_Identify_Component(uint32_t freq1, uint32_t freq2);

#endif

/* timer.h */
#ifndef __TIMER_H
#define __TIMER_H

#include "stm32f10x.h"

void Timer_Init(void);
uint32_t Timer_GetFrequency(void);
void Timer_ResetMeasure(void);

#endif

/* cd4052.h */
#ifndef __CD4052_H
#define __CD4052_H

#include "stm32f10x.h"

void CD4052_Init(void);
void CD4052_Select(uint8_t channel);

#endif

/* lcd1602.h */
#ifndef __LCD1602_H
#define __LCD1602_H

#include "stm32f10x.h"

void LCD_Init(void);
void LCD_Write_Cmd(uint8_t cmd);
void LCD_Write_Data(uint8_t data);
void LCD_Set_Cursor(uint8_t x, uint8_t y);
void LCD_Write_String(uint8_t x, uint8_t y, char *str);
void LCD_Clear(void);

#endif

/* key.h */
#ifndef __KEY_H
#define __KEY_H

#include "stm32f10x.h"

#define KEY1_PRESS  1
#define KEY2_PRESS  2
#define KEY3_PRESS  3

void KEY_Init(void);
uint8_t KEY_Scan(uint8_t mode);

#endif

/* led.h */
#ifndef __LED_H
#define __LED_H

#include "stm32f10x.h"

void LED_Init(void);
void LED_SetMode(uint8_t mode);

#endif
```

------

### 四、PCB设计要点

```text
┌─────────────────────────────────────────────────────────────┐
│                        PCB布局建议                          │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────┐    ┌─────────────────┐    ┌─────────────────┐ │
│  │ 电源区  │    │    数字区       │    │    模拟区       │ │
│  │         │    │                 │    │                 │ │
│  │ AMS1117 │    │  STM32F103     │    │  NE555振荡电路  │ │
│  │ 滤波电容│    │  LCD1602       │    │  CD4052        │ │
│  │         │    │  按键/LED      │    │  测量接口      │ │
│  └─────────┘    └─────────────────┘    └─────────────────┘ │
│                                                             │
│  注意事项:                                                  │
│  1. 模拟区与数字区分开布局                                  │
│  2. 模拟地与数字地单点连接                                  │
│  3. NE555周围加屏蔽                                         │
│  4. 测量接口使用屏蔽线                                      │
│  5. 电源滤波电容靠近芯片放置                                │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

------

### 五、测量量程与精度

| 参数 |      量程       | 典型精度 |
| :--: | :-------------: | :------: |
| 电阻 |   100Ω ~ 1MΩ    |   ±1%    |
| 电容 | 100pF ~ 10000pF |   ±2%    |
| 电感 |  100μH ~ 10mH   |   ±3%    |

------

### 六、校准方法

```c
/**
  * @brief  校准程序
  * @note   使用标准元件进行校准
  */
void Calibration_Process(void)
{
    // 1. 电阻校准: 使用1K, 10K, 100K标准电阻
    // 2. 电容校准: 使用100pF, 1nF, 10nF标准电容
    // 3. 电感校准: 使用100uH, 1mH标准电感
    
    // 记录校准系数并存储到Flash
}
```

------

如需更详细的某个模块说明或有其他问题，请随时告诉我！

参考资料





```
具体校准实现，以及一定要用 555嚒，STM32 内置的定时器可否
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

已完成生成

您问得非常好！**STM32内置的定时器完全可以替代NE555**，而且精度更高、电路更简洁。下面我详细介绍两种方案以及完整的校准实现。

------

## 一、为什么可以不用NE555？

|   对比项   |     NE555方案      | STM32 DAC+定时器方案 |
| :--------: | :----------------: | :------------------: |
| 电路复杂度 |  需要外部振荡电路  |   仅需简单调理电路   |
|  频率精度  | 受温漂影响大(±2%)  |   晶振精度(±0.01%)   |
|  频率范围  |    受限于RC参数    |  软件可调(1Hz~1MHz)  |
|    成本    | 需要NE555+外围元件 |     无需额外芯片     |
|   灵活性   |      固定电路      |      软件可编程      |

------

## 二、纯STM32方案 - 硬件电路

### 1. 系统框图

```text
                    ┌─────────────────────────────────────┐
                    │           STM32F103C8T6             │
                    │                                     │
  ┌──────────┐      │  ┌─────┐    ┌─────┐    ┌─────┐    │
  │ 被测元件 │◄────►│  │ DAC │───►│ ADC │───►│ DMA │    │
  │ (R/L/C) │      │  └─────┘    └─────┘    └─────┘    │
  └──────────┘      │      │          ▲                  │
       │           │      │          │                  │
       │           │      ▼          │                  │
       │           │  ┌─────────────────┐               │
       │           │  │  运算放大电路   │               │
       │           │  │  (电压/电流采样) │               │
       │           │  └─────────────────┘               │
       │           │                                     │
       │           │  ┌─────┐                           │
       └───────────┼─►│ TIM │ (相位测量)                │
                    │  └─────┘                           │
                    └─────────────────────────────────────┘
```

### 2. 核心测量电路

```text
【阻抗测量电路 - 交流电桥法】

                STM32 DAC输出
                     │
                     ▼
              ┌──────────────┐
              │   运放缓冲   │
              │   (TL072)    │
              └──────┬───────┘
                     │
                     │ Vout (正弦波激励)
                     │
         ┌───────────┼───────────┐
         │           │           │
         │      ┌────┴────┐      │
         │      │  Rref   │      │
         │      │  1KΩ    │      │
         │      └────┬────┘      │
         │           │           │
         │           ├───────────┼────► ADC1 (电流采样点)
         │           │           │
         │      ┌────┴────┐      │
         │      │   Zx    │      │
         │      │ (被测)  │      │
         │      └────┬────┘      │
         │           │           │
         │           ├───────────┼────► ADC2 (电压采样点)
         │           │           │
         └───────────┴───────────┘
                     │
                    GND


【信号调理电路】

  ADC采样点 ──────┬──────────────────────────────────► STM32 ADC
                  │
             ┌────┴────┐
             │ 100nF   │  (隔直电容)
             └────┬────┘
                  │
            ┌─────┴─────┐
            │           │
           ┌┴┐         ┌┴┐
           │ │10K      │ │10K
           └┬┘         └┬┘
            │           │
           VCC/2       GND
            │
            └──────────────────────────────────────► 偏置到1.65V
```

### 3. 量程切换电路

```text
                              VCC
                               │
                          ┌────┴────┐
                          │ CD4051  │ (8选1模拟开关)
                          │         │
    STM32 GPIO ──────────►│ A B C   │
    (PA4,PA5,PA6)         │         │
                          │    X0   │◄─── 10Ω    (小电阻档)
                          │    X1   │◄─── 100Ω
                          │    X2   │◄─── 1KΩ    (默认档)
                          │    X3   │◄─── 10KΩ
                          │    X4   │◄─── 100KΩ  (大电阻档)
                          │    X5   │◄─── 1MΩ
                          │    X6   │◄─── (备用)
                          │    X7   │◄─── (备用)
                          │         │
                          │   COM   │────► 接入测量电路Rref位置
                          └─────────┘
```

------

## 三、完整代码实现 (纯STM32方案)

### 1. DAC正弦波发生器

```c
/**
  ******************************************************************************
  * @file    dac_signal.c
  * @brief   DAC正弦波信号发生器
  ******************************************************************************
  */

#include "dac_signal.h"
#include "stm32f10x.h"
#include <math.h>

/* 正弦波查找表 - 256点 */
#define SINE_TABLE_SIZE  256
static uint16_t SineTable[SINE_TABLE_SIZE];

/* 当前激励频率 */
static uint32_t g_ExciteFreq = 1000;  // 默认1kHz

/**
  * @brief  生成正弦波查找表
  */
static void Generate_SineTable(void)
{
    uint16_t i;
    float angle;
    
    for(i = 0; i < SINE_TABLE_SIZE; i++)
    {
        angle = 2.0f * 3.14159f * i / SINE_TABLE_SIZE;
        // DAC 12位, 范围0-4095, 中心值2048, 幅度±1800
        SineTable[i] = (uint16_t)(2048 + 1800 * sin(angle));
    }
}

/**
  * @brief  DAC + DMA + TIM初始化
  * @note   使用TIM6触发DAC, DMA自动传输
  */
void DAC_Signal_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    DAC_InitTypeDef DAC_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    
    /* 生成正弦波表 */
    Generate_SineTable();
    
    /* 使能时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC | RCC_APB1Periph_TIM6, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, ENABLE);
    
    /* 配置PA4为模拟输出 (DAC_OUT1) */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    /* 配置TIM6 - 控制DAC更新频率 */
    // 输出频率 = 72MHz / (PSC+1) / (ARR+1) / 256
    // 1kHz: 72MHz / 1 / 281 / 256 ≈ 1kHz
    TIM_TimeBaseStructure.TIM_Period = 280;
    TIM_TimeBaseStructure.TIM_Prescaler = 0;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStructure);
    TIM_SelectOutputTrigger(TIM6, TIM_TRGOSource_Update);
    
    /* 配置DMA */
    DMA_DeInit(DMA2_Channel3);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&DAC->DHR12R1;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)SineTable;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize = SINE_TABLE_SIZE;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA2_Channel3, &DMA_InitStructure);
    DMA_Cmd(DMA2_Channel3, ENABLE);
    
    /* 配置DAC */
    DAC_InitStructure.DAC_Trigger = DAC_Trigger_T6_TRGO;
    DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
    DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
    DAC_Init(DAC_Channel_1, &DAC_InitStructure);
    
    /* 使能DAC DMA */
    DAC_DMACmd(DAC_Channel_1, ENABLE);
    
    /* 使能DAC通道 */
    DAC_Cmd(DAC_Channel_1, ENABLE);
    
    /* 启动TIM6 */
    TIM_Cmd(TIM6, ENABLE);
}

/**
  * @brief  设置激励信号频率
  * @param  freq: 目标频率 (Hz), 范围: 10Hz ~ 100kHz
  */
void DAC_SetFrequency(uint32_t freq)
{
    uint32_t arr;
    
    if(freq < 10) freq = 10;
    if(freq > 100000) freq = 100000;
    
    g_ExciteFreq = freq;
    
    // ARR = 72MHz / freq / 256 - 1
    arr = 72000000 / freq / SINE_TABLE_SIZE - 1;
    
    TIM_Cmd(TIM6, DISABLE);
    TIM6->ARR = arr;
    TIM_Cmd(TIM6, ENABLE);
}

/**
  * @brief  获取当前激励频率
  */
uint32_t DAC_GetFrequency(void)
{
    return g_ExciteFreq;
}
```

### 2. ADC双通道同步采样

```c
/**
  ******************************************************************************
  * @file    adc_sample.c
  * @brief   ADC双通道同步采样 (电压+电流)
  ******************************************************************************
  */

#include "adc_sample.h"
#include "stm32f10x.h"
#include <math.h>

/* 采样缓冲区 */
#define ADC_BUFFER_SIZE  256
static uint16_t ADC_Buffer[ADC_BUFFER_SIZE * 2];  // 双通道交替存储

/* 采样完成标志 */
static volatile uint8_t g_SampleComplete = 0;

/**
  * @brief  ADC + DMA初始化 (双通道同步采样)
  * @note   ADC1-CH1(PA1): 电压采样
  *         ADC1-CH2(PA2): 电流采样
  */
void ADC_Sample_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    
    /* 使能时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    
    /* ADC时钟配置 (最大14MHz) */
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);  // 72MHz/6 = 12MHz
    
    /* 配置PA1, PA2为模拟输入 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    /* 配置TIM3触发ADC采样 */
    // 采样率 = 72MHz / (PSC+1) / (ARR+1)
    // 256kHz采样率: 72MHz / 1 / 281 ≈ 256kHz
    TIM_TimeBaseStructure.TIM_Period = 280;
    TIM_TimeBaseStructure.TIM_Prescaler = 0;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
    TIM_SelectOutputTrigger(TIM3, TIM_TRGOSource_Update);
    
    /* 配置DMA */
    DMA_DeInit(DMA1_Channel1);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)ADC_Buffer;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = ADC_BUFFER_SIZE * 2;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel1, &DMA_InitStructure);
    
    /* 使能DMA传输完成中断 */
    DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);
    
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    DMA_Cmd(DMA1_Channel1, ENABLE);
    
    /* 配置ADC1 */
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = ENABLE;           // 扫描模式
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;    // 单次转换
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T3_TRGO;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 2;
    ADC_Init(ADC1, &ADC_InitStructure);
    
    /* 配置ADC通道 */
    ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_7Cycles5);  // 电压
    ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 2, ADC_SampleTime_7Cycles5);  // 电流
    
    /* 使能ADC DMA */
    ADC_DMACmd(ADC1, ENABLE);
    
    /* 使能外部触发 */
    ADC_ExternalTrigConvCmd(ADC1, ENABLE);
    
    /* 使能ADC */
    ADC_Cmd(ADC1, ENABLE);
    
    /* ADC校准 */
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
}

/**
  * @brief  启动ADC采样
  */
void ADC_StartSample(void)
{
    g_SampleComplete = 0;
    TIM_Cmd(TIM3, ENABLE);
}

/**
  * @brief  停止ADC采样
  */
void ADC_StopSample(void)
{
    TIM_Cmd(TIM3, DISABLE);
}

/**
  * @brief  检查采样是否完成
  */
uint8_t ADC_IsSampleComplete(void)
{
    return g_SampleComplete;
}

/**
  * @brief  DMA传输完成中断
  */
void DMA1_Channel1_IRQHandler(void)
{
    if(DMA_GetITStatus(DMA1_IT_TC1))
    {
        g_SampleComplete = 1;
        DMA_ClearITPendingBit(DMA1_IT_TC1);
    }
}

/**
  * @brief  获取采样缓冲区指针
  */
uint16_t* ADC_GetBuffer(void)
{
    return ADC_Buffer;
}
```

### 3. LCR计算算法 (核心)

```c
/**
  ******************************************************************************
  * @file    lcr_calculate.c
  * @brief   LCR参数计算算法 - 基于DFT相敏检波
  ******************************************************************************
  */

#include "lcr_calculate.h"
#include "adc_sample.h"
#include "dac_signal.h"
#include "calibration.h"
#include <math.h>

#define PI  3.14159265358979f
#define ADC_VREF        3.3f        // ADC参考电压
#define ADC_RESOLUTION  4096.0f     // 12位ADC

/* 测量结果结构体 */
typedef struct {
    float impedance;    // 阻抗幅值 (Ohm)
    float phase;        // 相位角 (rad)
    float resistance;   // 电阻分量 (Ohm)
    float reactance;    // 电抗分量 (Ohm)
} ImpedanceResult_t;

/* 校准系数 (从Flash加载) */
static CalibrationData_t g_CalData;

/**
  * @brief  DFT计算基波分量 (单频点)
  * @param  data: 采样数据
  * @param  size: 数据长度
  * @param  real: 实部输出
  * @param  imag: 虚部输出
  */
static void DFT_SingleFreq(uint16_t *data, uint16_t size, float *real, float *imag)
{
    float sumReal = 0, sumImag = 0;
    float angle;
    uint16_t i;
    
    for(i = 0; i < size; i++)
    {
        angle = 2.0f * PI * i / size;
        sumReal += (data[i] - 2048) * cos(angle);
        sumImag += (data[i] - 2048) * sin(angle);
    }
    
    *real = sumReal * 2.0f / size;
    *imag = sumImag * 2.0f / size;
}

/**
  * @brief  计算阻抗
  * @param  result: 结果输出
  */
void Calculate_Impedance(ImpedanceResult_t *result)
{
    uint16_t *buffer = ADC_GetBuffer();
    uint16_t voltageData[256], currentData[256];
    float vReal, vImag, iReal, iImag;
    float vMag, iMag, vPhase, iPhase;
    float Rref;
    uint16_t i;
    
    /* 分离电压和电流数据 */
    for(i = 0; i < 256; i++)
    {
        voltageData[i] = buffer[i * 2];      // 偶数索引: 电压
        currentData[i] = buffer[i * 2 + 1];  // 奇数索引: 电流
    }
    
    /* DFT计算基波分量 */
    DFT_SingleFreq(voltageData, 256, &vReal, &vImag);
    DFT_SingleFreq(currentData, 256, &iReal, &iImag);
    
    /* 计算幅值和相位 */
    vMag = sqrt(vReal * vReal + vImag * vImag);
    iMag = sqrt(iReal * iReal + iImag * iImag);
    vPhase = atan2(vImag, vReal);
    iPhase = atan2(iImag, iReal);
    
    /* 获取当前量程的参考电阻 */
    Rref = g_CalData.refResistance[g_CalData.currentRange];
    
    /* 计算阻抗 */
    // I = Vref / Rref, Zx = Vx / I = Vx * Rref / Vref
    // 这里Vref对应currentData, Vx对应voltageData
    if(iMag > 0.001f)
    {
        result->impedance = (vMag / iMag) * Rref;
        result->phase = vPhase - iPhase;
        
        /* 计算实部和虚部 */
        result->resistance = result->impedance * cos(result->phase);
        result->reactance = result->impedance * sin(result->phase);
    }
    else
    {
        result->impedance = 0;
        result->phase = 0;
        result->resistance = 0;
        result->reactance = 0;
    }
    
    /* 应用校准系数 */
    result->impedance *= g_CalData.gainFactor;
    result->impedance -= g_CalData.offsetFactor;
}

/**
  * @brief  计算电阻值
  * @retval 电阻值 (Ohm)
  */
float Calculate_Resistance_Value(void)
{
    ImpedanceResult_t result;
    Calculate_Impedance(&result);
    
    // 电阻 = 阻抗的实部
    return fabs(result.resistance);
}

/**
  * @brief  计算电容值
  * @retval 电容值 (F)
  */
float Calculate_Capacitance_Value(void)
{
    ImpedanceResult_t result;
    float freq = DAC_GetFrequency();
    float capacitance;
    
    Calculate_Impedance(&result);
    
    // 容抗 Xc = 1/(2πfC), 所以 C = 1/(2πf|Xc|)
    // 电容的电抗为负值
    if(result.reactance < -0.001f)
    {
        capacitance = 1.0f / (2.0f * PI * freq * fabs(result.reactance));
    }
    else
    {
        capacitance = 0;
    }
    
    return capacitance;
}

/**
  * @brief  计算电感值
  * @retval 电感值 (H)
  */
float Calculate_Inductance_Value(void)
{
    ImpedanceResult_t result;
    float freq = DAC_GetFrequency();
    float inductance;
    
    Calculate_Impedance(&result);
    
    // 感抗 XL = 2πfL, 所以 L = XL/(2πf)
    // 电感的电抗为正值
    if(result.reactance > 0.001f)
    {
        inductance = result.reactance / (2.0f * PI * freq);
    }
    else
    {
        inductance = 0;
    }
    
    return inductance;
}

/**
  * @brief  自动识别元件类型
  * @retval 1-电阻, 2-电容, 3-电感
  */
uint8_t Auto_Identify_Component_Type(void)
{
    ImpedanceResult_t result1, result2;
    float phaseDeg;
    
    /* 使用1kHz测量 */
    DAC_SetFrequency(1000);
    delay_ms(50);
    ADC_StartSample();
    while(!ADC_IsSampleComplete());
    Calculate_Impedance(&result1);
    
    /* 使用10kHz测量 */
    DAC_SetFrequency(10000);
    delay_ms(50);
    ADC_StartSample();
    while(!ADC_IsSampleComplete());
    Calculate_Impedance(&result2);
    
    /* 根据相位判断 */
    phaseDeg = result1.phase * 180.0f / PI;
    
    if(phaseDeg > -10.0f && phaseDeg < 10.0f)
    {
        return 1;  // 电阻: 相位接近0°
    }
    else if(phaseDeg < -45.0f)
    {
        return 2;  // 电容: 相位为负 (电流超前电压)
    }
    else if(phaseDeg > 45.0f)
    {
        return 3;  // 电感: 相位为正 (电流滞后电压)
    }
    
    /* 根据阻抗随频率变化判断 */
    if(result2.impedance < result1.impedance * 0.9f)
    {
        return 2;  // 电容: 高频阻抗小
    }
    else if(result2.impedance > result1.impedance * 1.1f)
    {
        return 3;  // 电感: 高频阻抗大
    }
    
    return 1;  // 默认电阻
}

/**
  * @brief  设置校准数据
  */
void LCR_SetCalibrationData(CalibrationData_t *calData)
{
    memcpy(&g_CalData, calData, sizeof(CalibrationData_t));
}
```

------

## 四、完整校准系统实现

### 1. 校准数据结构

```c
/**
  ******************************************************************************
  * @file    calibration.h
  * @brief   校准系统头文件
  ******************************************************************************
  */

#ifndef __CALIBRATION_H
#define __CALIBRATION_H

#include "stm32f10x.h"

/* 量程定义 */
#define RANGE_10R       0
#define RANGE_100R      1
#define RANGE_1K        2
#define RANGE_10K       3
#define RANGE_100K      4
#define RANGE_1M        5
#define RANGE_COUNT     6

/* 频率点定义 */
#define FREQ_100HZ      0
#define FREQ_1KHZ       1
#define FREQ_10KHZ      2
#define FREQ_100KHZ     3
#define FREQ_COUNT      4

/* 校准数据结构 */
typedef struct {
    /* 基本校准参数 */
    float gainFactor;           // 增益校准系数
    float offsetFactor;         // 偏移校准系数
    
    /* 各量程参考电阻值 */
    float refResistance[RANGE_COUNT];
    
    /* 开路校准数据 (各频率点) */
    float openReal[FREQ_COUNT];
    float openImag[FREQ_COUNT];
    
    /* 短路校准数据 (各频率点) */
    float shortReal[FREQ_COUNT];
    float shortImag[FREQ_COUNT];
    
    /* 负载校准数据 (使用标准电阻) */
    float loadReal[FREQ_COUNT];
    float loadImag[FREQ_COUNT];
    float loadRefValue;         // 标准负载值
    
    /* 当前量程 */
    uint8_t currentRange;
    
    /* 校准有效标志 */
    uint32_t validFlag;         // 0x5A5A5A5A表示有效
    
} CalibrationData_t;

/* 校准状态 */
typedef enum {
    CAL_STATE_IDLE = 0,
    CAL_STATE_OPEN,
    CAL_STATE_SHORT,
    CAL_STATE_LOAD,
    CAL_STATE_COMPLETE
} CalibrationState_t;

/* 函数声明 */
void Calibration_Init(void);
void Calibration_Open(void);
void Calibration_Short(void);
void Calibration_Load(float refValue);
void Calibration_Save(void);
void Calibration_Load_FromFlash(void);
void Calibration_Reset(void);
CalibrationData_t* Calibration_GetData(void);

#endif
```

### 2. 校准算法实现

```c
/**
  ******************************************************************************
  * @file    calibration.c
  * @brief   校准系统实现 - 开路/短路/负载三点校准
  ******************************************************************************
  */

#include "calibration.h"
#include "adc_sample.h"
#include "dac_signal.h"
#include "flash_storage.h"
#include <math.h>
#include <string.h>

#define PI  3.14159265358979f

/* 校准数据 */
static CalibrationData_t g_CalData;
static CalibrationState_t g_CalState = CAL_STATE_IDLE;

/* 测量频率表 */
static const uint32_t FreqTable[FREQ_COUNT] = {100, 1000, 10000, 100000};

/* 默认参考电阻值 */
static const float DefaultRefRes[RANGE_COUNT] = {
    10.0f,      // 10Ω档
    100.0f,     // 100Ω档
    1000.0f,    // 1KΩ档
    10000.0f,   // 10KΩ档
    100000.0f,  // 100KΩ档
    1000000.0f  // 1MΩ档
};

/**
  * @brief  测量原始阻抗 (未校准)
  * @param  real: 实部输出
  * @param  imag: 虚部输出
  */
static void Measure_RawImpedance(float *real, float *imag)
{
    uint16_t *buffer = ADC_GetBuffer();
    uint16_t voltageData[256], currentData[256];
    float vReal, vImag, iReal, iImag;
    float Rref;
    uint16_t i;
    
    /* 启动采样 */
    ADC_StartSample();
    while(!ADC_IsSampleComplete());
    
    /* 分离数据 */
    for(i = 0; i < 256; i++)
    {
        voltageData[i] = buffer[i * 2];
        currentData[i] = buffer[i * 2 + 1];
    }
    
    /* DFT计算 */
    DFT_SingleFreq(voltageData, 256, &vReal, &vImag);
    DFT_SingleFreq(currentData, 256, &iReal, &iImag);
    
    /* 计算复阻抗 */
    Rref = g_CalData.refResistance[g_CalData.currentRange];
    
    float iMagSq = iReal * iReal + iImag * iImag;
    if(iMagSq > 0.0001f)
    {
        // Zx = Vx / I = Vx * Rref / Vref
        // 复数除法: (a+jb)/(c+jd) = [(ac+bd)+j(bc-ad)]/(c²+d²)
        *real = (vReal * iReal + vImag * iImag) * Rref / iMagSq;
        *imag = (vImag * iReal - vReal * iImag) * Rref / iMagSq;
    }
    else
    {
        *real = 0;
        *imag = 0;
    }
}

/**
  * @brief  校准初始化
  */
void Calibration_Init(void)
{
    uint8_t i;
    
    /* 尝试从Flash加载校准数据 */
    Calibration_Load_FromFlash();
    
    /* 如果无效,使用默认值 */
    if(g_CalData.validFlag != 0x5A5A5A5A)
    {
        memset(&g_CalData, 0, sizeof(CalibrationData_t));
        
        g_CalData.gainFactor = 1.0f;
        g_CalData.offsetFactor = 0.0f;
        g_CalData.currentRange = RANGE_1K;
        
        for(i = 0; i < RANGE_COUNT; i++)
        {
            g_CalData.refResistance[i] = DefaultRefRes[i];
        }
    }
    
    g_CalState = CAL_STATE_IDLE;
}

/**
  * @brief  开路校准
  * @note   测量端子开路,测量杂散电容和泄漏
  */
void Calibration_Open(void)
{
    uint8_t freqIdx;
    float real, imag;
    
    g_CalState = CAL_STATE_OPEN;
    
    /* 在各频率点测量开路阻抗 */
    for(freqIdx = 0; freqIdx < FREQ_COUNT; freqIdx++)
    {
        DAC_SetFrequency(FreqTable[freqIdx]);
        delay_ms(100);  // 等待稳定
        
        /* 多次测量取平均 */
        float sumReal = 0, sumImag = 0;
        for(int i = 0; i < 8; i++)
        {
            Measure_RawImpedance(&real, &imag);
            sumReal += real;
            sumImag += imag;
            delay_ms(10);
        }
        
        g_CalData.openReal[freqIdx] = sumReal / 8.0f;
        g_CalData.openImag[freqIdx] = sumImag / 8.0f;
    }
    
    g_CalState = CAL_STATE_IDLE;
}

/**
  * @brief  短路校准
  * @note   测量端子短路,测量引线电阻和电感
  */
void Calibration_Short(void)
{
    uint8_t freqIdx;
    float real, imag;
    
    g_CalState = CAL_STATE_SHORT;
    
    /* 在各频率点测量短路阻抗 */
    for(freqIdx = 0; freqIdx < FREQ_COUNT; freqIdx++)
    {
        DAC_SetFrequency(FreqTable[freqIdx]);
        delay_ms(100);
        
        float sumReal = 0, sumImag = 0;
        for(int i = 0; i < 8; i++)
        {
            Measure_RawImpedance(&real, &imag);
            sumReal += real;
            sumImag += imag;
            delay_ms(10);
        }
        
        g_CalData.shortReal[freqIdx] = sumReal / 8.0f;
        g_CalData.shortImag[freqIdx] = sumImag / 8.0f;
    }
    
    g_CalState = CAL_STATE_IDLE;
}

/**
  * @brief  负载校准
  * @param  refValue: 标准负载值 (Ohm)
  * @note   使用已知精密电阻进行增益校准
  */
void Calibration_Load(float refValue)
{
    uint8_t freqIdx;
    float real, imag;
    float measuredZ, errorSum = 0;
    
    g_CalState = CAL_STATE_LOAD;
    g_CalData.loadRefValue = refValue;
    
    /* 在各频率点测量标准负载 */
    for(freqIdx = 0; freqIdx < FREQ_COUNT; freqIdx++)
    {
        DAC_SetFrequency(FreqTable[freqIdx]);
        delay_ms(100);
        
        float sumReal = 0, sumImag = 0;
        for(int i = 0; i < 8; i++)
        {
            Measure_RawImpedance(&real, &imag);
            sumReal += real;
            sumImag += imag;
            delay_ms(10);
        }
        
        g_CalData.loadReal[freqIdx] = sumReal / 8.0f;
        g_CalData.loadImag[freqIdx] = sumImag / 8.0f;
        
        /* 计算测量值与标准值的误差 */
        measuredZ = sqrt(g_CalData.loadReal[freqIdx] * g_CalData.loadReal[freqIdx] +
                        g_CalData.loadImag[freqIdx] * g_CalData.loadImag[freqIdx]);
        
        if(measuredZ > 0.001f)
        {
            errorSum += refValue / measuredZ;
        }
    }
    
    /* 计算增益校准系数 */
    g_CalData.gainFactor = errorSum / FREQ_COUNT;
    
    g_CalState = CAL_STATE_COMPLETE;
}

/**
  * @brief  应用校准补偿
  * @param  rawReal: 原始实部
  * @param  rawImag: 原始虚部
  * @param  calReal: 校准后实部
  * @param  calImag: 校准后虚部
  * @param  freqIdx: 频率索引
  */
void Calibration_Apply(float rawReal, float rawImag, 
                       float *calReal, float *calImag, uint8_t freqIdx)
{
    float Zo_real, Zo_imag;  // 开路阻抗
    float Zs_real, Zs_imag;  // 短路阻抗
    float Zm_real, Zm_imag;  // 测量阻抗
    float temp_real, temp_imag;
    float denom;
    
    /* 获取校准数据 */
    Zo_real = g_CalData.openReal[freqIdx];
    Zo_imag = g_CalData.openImag[freqIdx];
    Zs_real = g_CalData.shortReal[freqIdx];
    Zs_imag = g_CalData.shortImag[freqIdx];
    Zm_real = rawReal;
    Zm_imag = rawImag;
    
    /*
     * 三点校准公式:
     * Zx = (Zm - Zs) * Zo / (Zo - Zm)
     * 
     * 其中:
     * Zo = 开路阻抗 (理想为无穷大)
     * Zs = 短路阻抗 (理想为0)
     * Zm = 测量阻抗
     * Zx = 实际阻抗
     */
    
    /* 计算 Zm - Zs */
    float num_real = Zm_real - Zs_real;
    float num_imag = Zm_imag - Zs_imag;
    
    /* 计算 Zo - Zm */
    float den_real = Zo_real - Zm_real;
    float den_imag = Zo_imag - Zm_imag;
    
    /* 计算 (Zm - Zs) * Zo */
    temp_real = num_real * Zo_real - num_imag * Zo_imag;
    temp_imag = num_real * Zo_imag + num_imag * Zo_real;
    
    /* 计算 [(Zm - Zs) * Zo] / (Zo - Zm) */
    denom = den_real * den_real + den_imag * den_imag;
    
    if(denom > 0.0001f)
    {
        *calReal = (temp_real * den_real + temp_imag * den_imag) / denom;
        *calImag = (temp_imag * den_real - temp_real * den_imag) / denom;
    }
    else
    {
        /* 分母太小,直接使用原始值 */
        *calReal = rawReal;
        *calImag = rawImag;
    }
    
    /* 应用增益校准 */
    *calReal *= g_CalData.gainFactor;
    *calImag *= g_CalData.gainFactor;
}

/**
  * @brief  保存校准数据到Flash
  */
void Calibration_Save(void)
{
    g_CalData.validFlag = 0x5A5A5A5A;
    Flash_Write(FLASH_CAL_ADDR, (uint8_t*)&g_CalData, sizeof(CalibrationData_t));
}

/**
  * @brief  从Flash加载校准数据
  */
void Calibration_Load_FromFlash(void)
{
    Flash_Read(FLASH_CAL_ADDR, (uint8_t*)&g_CalData, sizeof(CalibrationData_t));
}

/**
  * @brief  重置校准数据
  */
void Calibration_Reset(void)
{
    memset(&g_CalData, 0, sizeof(CalibrationData_t));
    g_CalData.gainFactor = 1.0f;
    g_CalData.offsetFactor = 0.0f;
    g_CalData.validFlag = 0;
    
    for(int i = 0; i < RANGE_COUNT; i++)
    {
        g_CalData.refResistance[i] = DefaultRefRes[i];
    }
}

/**
  * @brief  获取校准数据指针
  */
CalibrationData_t* Calibration_GetData(void)
{
    return &g_CalData;
}
```

### 3. Flash存储

```c
/**
  ******************************************************************************
  * @file    flash_storage.c
  * @brief   Flash存储驱动 - 保存校准数据
  ******************************************************************************
  */

#include "flash_storage.h"
#include "stm32f10x.h"

/* Flash存储地址 (使用最后一页) */
#define FLASH_CAL_ADDR  0x0800FC00  // STM32F103C8T6最后1KB

/**
  * @brief  写入Flash
  * @param  addr: 起始地址
  * @param  数据指针
  * @param  len: 数据长度
  */
void Flash_Write(uint32_t addr, uint8_t *data, uint16_t len)
{
    uint16_t i;
    uint16_t *pData = (uint16_t*)data;
    uint16_t wordLen = (len + 1) / 2;
    
    /* 解锁Flash */
    FLASH_Unlock();
    
    /* 擦除页 */
    FLASH_ErasePage(addr);
    
    /* 写入数据 (半字写入) */
    for(i = 0; i < wordLen; i++)
    {
        FLASH_ProgramHalfWord(addr + i * 2, pData[i]);
    }
    
    /* 锁定Flash */
    FLASH_Lock();
}

/**
  * @brief  读取Flash
  * @param  addr: 起始地址
  * @param  数据缓冲区
  * @param  len: 数据长度
  */
void Flash_Read(uint32_t addr, uint8_t *data, uint16_t len)
{
    uint16_t i;
    
    for(i = 0; i < len; i++)
    {
        data[i] = *(__IO uint8_t*)(addr + i);
    }
}
```

### 4. 校准流程主程序

```c
/**
  ******************************************************************************
  * @file    calibration_menu.c
  * @brief   校准菜单界面
  ******************************************************************************
  */

#include "calibration_menu.h"
#include "calibration.h"
#include "lcd1602.h"
#include "key.h"
#include "delay.h"
#include <stdio.h>

/* 校准菜单状态 */
typedef enum {
    CAL_MENU_MAIN = 0,
    CAL_MENU_OPEN,
    CAL_MENU_SHORT,
    CAL_MENU_LOAD,
    CAL_MENU_SAVE,
    CAL_MENU_EXIT
} CalMenuState_t;

static CalMenuState_t menuState = CAL_MENU_MAIN;

/**
  * @brief  显示校准主菜单
  */
static void Show_CalMenu_Main(void)
{
    LCD_Clear();
    LCD_Write_String(0, 0, "=CALIBRATION=");
    LCD_Write_String(0, 1, "1:OPEN 2:SHORT");
}

/**
  * @brief  执行开路校准
  */
static void Do_OpenCalibration(void)
{
    LCD_Clear();
    LCD_Write_String(0, 0, "OPEN CAL...");
    LCD_Write_String(0, 1, "Keep OPEN!");
    
    delay_ms(2000);  // 等待用户准备
    
    LCD_Write_String(0, 1, "Measuring...");
    
    Calibration_Open();
    
    LCD_Write_String(0, 1, "OPEN CAL OK!");
    delay_ms(1000);
}

/**
  * @brief  执行短路校准
  */
static void Do_ShortCalibration(void)
{
    LCD_Clear();
    LCD_Write_String(0, 0, "SHORT CAL...");
    LCD_Write_String(0, 1, "Short probes!");
    
    delay_ms(2000);
    
    LCD_Write_String(0, 1, "Measuring...");
    
    Calibration_Short();
    
    LCD_Write_String(0, 1, "SHORT CAL OK!");
    delay_ms(1000);
}

/**
  * @brief  执行负载校准
  */
static void Do_LoadCalibration(void)
{
    char buf[17];
    float refValue = 1000.0f;  // 默认1K标准电阻
    
    LCD_Clear();
    LCD_Write_String(0, 0, "LOAD CAL...");
    sprintf(buf, "Ref:%6.1fOhm", refValue);
    LCD_Write_String(0, 1, buf);
    
    delay_ms(2000);
    
    LCD_Write_String(0, 1, "Measuring...");
    
    Calibration_Load(refValue);
    
    LCD_Write_String(0, 1, "LOAD CAL OK!");
    delay_ms(1000);
}

/**
  * @brief  校准菜单处理
  */
void Calibration_Menu_Process(void)
{
    uint8_t key;
    
    Show_CalMenu_Main();
    
    while(1)
    {
        key = KEY_Scan(0);
        
        switch(key)
        {
            case KEY1_PRESS:  // 开路校准
                Do_OpenCalibration();
                Show_CalMenu_Main();
                break;
                
            case KEY2_PRESS:  // 短路校准
                Do_ShortCalibration();
                Show_CalMenu_Main();
                break;
                
            case KEY3_PRESS:  // 负载校准
                Do_LoadCalibration();
                Show_CalMenu_Main();
                break;
                
            case KEY4_PRESS:  // 保存并退出
                LCD_Clear();
                LCD_Write_String(0, 0, "Saving...");
                Calibration_Save();
                LCD_Write_String(0, 1, "Saved!");
                delay_ms(1000);
                return;
        }
    }
}

/**
  * @brief  完整校准流程 (引导式)
  */
void Calibration_FullProcess(void)
{
    char buf[17];
    
    /* 步骤1: 开路校准 */
    LCD_Clear();
    LCD_Write_String(0, 0, "Step 1/3: OPEN");
    LCD_Write_String(0, 1, "Press KEY1...");
    
    while(KEY_Scan(0) != KEY1_PRESS);
    
    LCD_Write_String(0, 1, "Calibrating...");
    Calibration_Open();
    LCD_Write_String(0, 1, "Done!");
    delay_ms(500);
    
    /* 步骤2: 短路校准 */
    LCD_Clear();
    LCD_Write_String(0, 0, "Step 2/3: SHORT");
    LCD_Write_String(0, 1, "Short & KEY1");
    
    while(KEY_Scan(0) != KEY1_PRESS);
    
    LCD_Write_String(0, 1, "Calibrating...");
    Calibration_Short();
    LCD_Write_String(0, 1, "Done!");
    delay_ms(500);
    
    /* 步骤3: 负载校准 */
    LCD_Clear();
    LCD_Write_String(0, 0, "Step 3/3: LOAD");
    LCD_Write_String(0, 1, "1K Ref & KEY1");
    
    while(KEY_Scan(0) != KEY1_PRESS);
    
    LCD_Write_String(0, 1, "Calibrating...");
    Calibration_Load(1000.0f);  // 使用1K标准电阻
    LCD_Write_String(0, 1, "Done!");
    delay_ms(500);
    
    /* 保存校准数据 */
    LCD_Clear();
    LCD_Write_String(0, 0, "Saving to Flash");
    Calibration_Save();
    LCD_Write_String(0, 1, "CAL Complete!");
    delay_ms(1000);
}
```

------

## 五、校准原理详解

### 1. 三点校准模型

```text
【误差模型】

实际测量电路存在以下误差源:
┌─────────────────────────────────────────────────────────┐
│                                                         │
│    Zs (短路残余)     Zx (被测)      Zo (开路泄漏)      │
│    ┌───┐            ┌───┐          ┌───┐               │
│ ───┤   ├────────────┤   ├──────────┤   ├───            │
│    └───┘            └───┘          └───┘               │
│    (引线电阻+电感)   (实际阻抗)    (杂散电容)          │
│                                                         │
└─────────────────────────────────────────────────────────┘

【校准公式推导】

测量阻抗 Zm 与实际阻抗 Zx 的关系:
    Zm = Zs + Zx // Zo
    
其中 // 表示并联:
    Zx // Zo = (Zx * Zo) / (Zx + Zo)

解出 Zx:
    Zx = (Zm - Zs) * Zo / (Zo - Zm + Zs)
    
简化 (当 Zs << Zo 时):
    Zx ≈ (Zm - Zs) * Zo / (Zo - Zm)
```

### 2. 校准步骤说明

```text
┌─────────────────────────────────────────────────────────┐
│                    校准流程图                           │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  ┌─────────────┐                                        │
│  │ 1. 开路校准 │  测量端子悬空                          │
│  │   (OPEN)    │  → 获取杂散电容 Zo                     │
│  └──────┬──────┘                                        │
│         │                                               │
│         ▼                                               │
│  ┌─────────────┐                                        │
│  │ 2. 短路校准 │  测量端子短接                          │
│  │   (SHORT)   │  → 获取引线阻抗 Zs                     │
│  └──────┬──────┘                                        │
│         │                                               │
│         ▼                                               │
│  ┌─────────────┐                                        │
│  │ 3. 负载校准 │  接入标准电阻 (如1KΩ 0.1%)            │
│  │   (LOAD)    │  → 获取增益系数                        │
│  └──────┬──────┘                                        │
│         │                                               │
│         ▼                                               │
│  ┌─────────────┐                                        │
│  │ 4. 保存数据 │  写入Flash                             │
│  └─────────────┘                                        │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### 3. 校准所需标准件

|  校准项  | 所需标准件  |  精度要求  |     用途     |
| :------: | :---------: | :--------: | :----------: |
| 开路校准 |     无      |     -      | 测量杂散参数 |
| 短路校准 | 短路片/导线 | 电阻<0.1Ω  | 测量引线阻抗 |
| 负载校准 |  精密电阻   | 0.1%或更高 |   增益校准   |

**推荐标准电阻:**

- 100Ω ±0.1% (小阻值校准)
- 1KΩ ±0.1% (中阻值校准)
- 10KΩ ±0.1% (大阻值校准)

------

## 六、完整主程序

```c
/**
  ******************************************************************************
  * @file    main.c
  * @brief   高精密LCR测试仪主程序 (纯STM32方案)
  ******************************************************************************
  */

#include "stm32f10x.h"
#include "delay.h"
#include "lcd1602.h"
#include "key.h"
#include "led.h"
#include "dac_signal.h"
#include "adc_sample.h"
#include "lcr_calculate.h"
#include "calibration.h"
#include "calibration_menu.h"
#include "range_switch.h"
#include <stdio.h>
#include <math.h>

/* 测量模式 */
#define MODE_AUTO       0
#define MODE_RESISTANCE 1
#define MODE_CAPACITANCE 2
#define MODE_INDUCTANCE 3

static uint8_t g_MeasureMode = MODE_AUTO;
static uint32_t g_TestFreq = 1000;  // 默认1kHz

/**
  * @brief  系统初始化
  */
void System_Init(void)
{
    delay_init();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    
    LED_Init();
    KEY_Init();
    LCD_Init();
    Range_Switch_Init();
    DAC_Signal_Init();
    ADC_Sample_Init();
    Calibration_Init();
    
    /* 设置默认测试频率 */
    DAC_SetFrequency(g_TestFreq);
    
    /* 加载校准数据到计算模块 */
    LCR_SetCalibrationData(Calibration_GetData());
}

/**
  * @brief  显示测量结果
  */
void Display_Measurement(uint8_t type, float value)
{
    char line1[17], line2[17];
    
    switch(type)
    {
        case MODE_RESISTANCE:
            sprintf(line1, "R Mode  %5luHz", g_TestFreq);
            if(value < 1000)
                sprintf(line2, "R=%7.2f Ohm", value);
            else if(value < 1000000)
                sprintf(line2, "R=%7.3f KOhm", value/1000);
            else
                sprintf(line2, "R=%7.3f MOhm", value/1000000);
            LED_SetMode(MODE_RESISTANCE);
            break;
            
        case MODE_CAPACITANCE:
            sprintf(line1, "C Mode  %5luHz", g_TestFreq);
            if(value < 1e-9f)
                sprintf(line2, "C=%7.2f pF", value*1e12f);
            else if(value < 1e-6f)
                sprintf(line2, "C=%7.3f nF", value*1e9f);
            else
                sprintf(line2, "C=%7.3f uF", value*1e6f);
            LED_SetMode(MODE_CAPACITANCE);
            break;
            
        case MODE_INDUCTANCE:
            sprintf(line1, "L Mode  %5luHz", g_TestFreq);
            if(value < 1e-3f)
                sprintf(line2, "L=%7.2f uH", value*1e6f);
            else
                sprintf(line2, "L=%7.3f mH", value*1e3f);
            LED_SetMode(MODE_INDUCTANCE);
            break;
            
        case MODE_AUTO:
            sprintf(line1, "AUTO    %5luHz", g_TestFreq
```





```
继续完成代码
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

已完成生成

继续完成代码：

```c
/**
  * @brief  显示测量结果 (续)
  */
void Display_Measurement(uint8_t type, float value)
{
    char line1[17], line2[17];
    
    switch(type)
    {
        case MODE_RESISTANCE:
            sprintf(line1, "R Mode  %5luHz", g_TestFreq);
            if(value < 1000)
                sprintf(line2, "R=%7.2f Ohm", value);
            else if(value < 1000000)
                sprintf(line2, "R=%7.3f KOhm", value/1000);
            else
                sprintf(line2, "R=%7.3f MOhm", value/1000000);
            LED_SetMode(MODE_RESISTANCE);
            break;
            
        case MODE_CAPACITANCE:
            sprintf(line1, "C Mode  %5luHz", g_TestFreq);
            if(value < 1e-9f)
                sprintf(line2, "C=%7.2f pF", value*1e12f);
            else if(value < 1e-6f)
                sprintf(line2, "C=%7.3f nF", value*1e9f);
            else
                sprintf(line2, "C=%7.3f uF", value*1e6f);
            LED_SetMode(MODE_CAPACITANCE);
            break;
            
        case MODE_INDUCTANCE:
            sprintf(line1, "L Mode  %5luHz", g_TestFreq);
            if(value < 1e-3f)
                sprintf(line2, "L=%7.2f uH", value*1e6f);
            else
                sprintf(line2, "L=%7.3f mH", value*1e3f);
            LED_SetMode(MODE_INDUCTANCE);
            break;
            
        case MODE_AUTO:
            sprintf(line1, "AUTO    %5luHz", g_TestFreq);
            sprintf(line2, "Detecting...   ");
            break;
            
        default:
            sprintf(line1, "LCR Meter v1.0");
            sprintf(line2, "Ready...       ");
            break;
    }
    
    LCD_Write_String(0, 0, line1);
    LCD_Write_String(0, 1, line2);
}

/**
  * @brief  按键处理
  */
void Key_Process(void)
{
    uint8_t key = KEY_Scan(0);
    
    switch(key)
    {
        case KEY1_PRESS:  // 切换测量模式
            g_MeasureMode++;
            if(g_MeasureMode > MODE_INDUCTANCE)
                g_MeasureMode = MODE_AUTO;
            break;
            
        case KEY2_PRESS:  // 切换测试频率
            if(g_TestFreq == 100)
                g_TestFreq = 1000;
            else if(g_TestFreq == 1000)
                g_TestFreq = 10000;
            else if(g_TestFreq == 10000)
                g_TestFreq = 100000;
            else
                g_TestFreq = 100;
            DAC_SetFrequency(g_TestFreq);
            break;
            
        case KEY3_PRESS:  // 进入校准菜单
            Calibration_Menu_Process();
            break;
            
        case KEY4_PRESS:  // 自动量程切换
            Range_AutoSwitch();
            break;
    }
}

/**
  * @brief  执行一次完整测量
  */
void Do_Measurement(void)
{
    float result = 0;
    uint8_t componentType;
    
    /* 启动ADC采样 */
    ADC_StartSample();
    
    /* 等待采样完成 */
    while(!ADC_IsSampleComplete())
    {
        delay_ms(1);
    }
    
    /* 根据模式计算结果 */
    if(g_MeasureMode == MODE_AUTO)
    {
        /* 自动识别元件类型 */
        componentType = Auto_Identify_Component_Type();
        
        switch(componentType)
        {
            case 1:  // 电阻
                result = Calculate_Resistance_Value();
                Display_Measurement(MODE_RESISTANCE, result);
                break;
            case 2:  // 电容
                result = Calculate_Capacitance_Value();
                Display_Measurement(MODE_CAPACITANCE, result);
                break;
            case 3:  // 电感
                result = Calculate_Inductance_Value();
                Display_Measurement(MODE_INDUCTANCE, result);
                break;
        }
    }
    else
    {
        switch(g_MeasureMode)
        {
            case MODE_RESISTANCE:
                result = Calculate_Resistance_Value();
                break;
            case MODE_CAPACITANCE:
                result = Calculate_Capacitance_Value();
                break;
            case MODE_INDUCTANCE:
                result = Calculate_Inductance_Value();
                break;
        }
        Display_Measurement(g_MeasureMode, result);
    }
    
    /* 自动量程调整 */
    Range_AutoAdjust(result, g_MeasureMode);
}

/**
  * @brief  主函数
  */
int main(void)
{
    /* 系统初始化 */
    System_Init();
    
    /* 显示开机画面 */
    LCD_Clear();
    LCD_Write_String(0, 0, " LCR Meter V1.0");
    LCD_Write_String(0, 1, "  STM32 Based  ");
    delay_ms(1500);
    
    /* 检查是否需要校准 */
    CalibrationData_t *calData = Calibration_GetData();
    if(calData->validFlag != 0x5A5A5A5A)
    {
        LCD_Clear();
        LCD_Write_String(0, 0, "No Calibration!");
        LCD_Write_String(0, 1, "KEY3 to Calib");
        delay_ms(2000);
    }
    
    /* 主循环 */
    while(1)
    {
        /* 按键处理 */
        Key_Process();
        
        /* 执行测量 */
        Do_Measurement();
        
        /* 测量间隔 */
        delay_ms(200);
    }
}
```

------

## 七、量程自动切换模块

```c
/**
  ******************************************************************************
  * @file    range_switch.c
  * @brief   量程自动切换模块
  ******************************************************************************
  */

#include "range_switch.h"
#include "stm32f10x.h"
#include "calibration.h"

/* 量程切换引脚定义 (控制CD4051) */
#define RANGE_A_PIN     GPIO_Pin_4
#define RANGE_B_PIN     GPIO_Pin_5
#define RANGE_C_PIN     GPIO_Pin_6
#define RANGE_PORT      GPIOA

/* 当前量程 */
static uint8_t g_CurrentRange = RANGE_1K;

/* 量程阈值表 (单位: Ohm) */
static const float RangeThreshold[RANGE_COUNT][2] = {
    {0.1f,      50.0f},       // RANGE_10R:    0.1Ω ~ 50Ω
    {10.0f,     500.0f},      // RANGE_100R:   10Ω ~ 500Ω
    {100.0f,    5000.0f},     // RANGE_1K:     100Ω ~ 5KΩ
    {1000.0f,   50000.0f},    // RANGE_10K:    1KΩ ~ 50KΩ
    {10000.0f,  500000.0f},   // RANGE_100K:   10KΩ ~ 500KΩ
    {100000.0f, 5000000.0f}   // RANGE_1M:     100KΩ ~ 5MΩ
};

/**
  * @brief  量程切换初始化
  */
void Range_Switch_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = RANGE_A_PIN | RANGE_B_PIN | RANGE_C_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(RANGE_PORT, &GPIO_InitStructure);
    
    /* 默认1K量程 */
    Range_Set(RANGE_1K);
}

/**
  * @brief  设置量程
  * @param  range: 量程索引 (0-5)
  */
void Range_Set(uint8_t range)
{
    if(range >= RANGE_COUNT)
        range = RANGE_1K;
    
    g_CurrentRange = range;
    
    /* 更新校准数据中的当前量程 */
    CalibrationData_t *calData = Calibration_GetData();
    calData->currentRange = range;
    
    /* 设置CD4051选择引脚 */
    GPIO_ResetBits(RANGE_PORT, RANGE_A_PIN | RANGE_B_PIN | RANGE_C_PIN);
    
    if(range & 0x01) GPIO_SetBits(RANGE_PORT, RANGE_A_PIN);
    if(range & 0x02) GPIO_SetBits(RANGE_PORT, RANGE_B_PIN);
    if(range & 0x04) GPIO_SetBits(RANGE_PORT, RANGE_C_PIN);
}

/**
  * @brief  获取当前量程
  */
uint8_t Range_Get(void)
{
    return g_CurrentRange;
}

/**
  * @brief  自动量程调整
  * @param  measuredValue: 测量值
  * @param  mode: 测量模式
  */
void Range_AutoAdjust(float measuredValue, uint8_t mode)
{
    float impedance;
    uint8_t newRange = g_CurrentRange;
    
    /* 根据模式转换为阻抗值 */
    switch(mode)
    {
        case MODE_RESISTANCE:
            impedance = measuredValue;
            break;
        case MODE_CAPACITANCE:
            // Xc = 1/(2πfC), 假设1kHz
            impedance = 1.0f / (6.2832f * 1000.0f * measuredValue);
            break;
        case MODE_INDUCTANCE:
            // XL = 2πfL, 假设1kHz
            impedance = 6.2832f * 1000.0f * measuredValue;
            break;
        default:
            return;
    }
    
    /* 判断是否需要切换量程 */
    if(impedance < RangeThreshold[g_CurrentRange][0] && g_CurrentRange > RANGE_10R)
    {
        /* 测量值过小,切换到更小量程 */
        newRange = g_CurrentRange - 1;
    }
    else if(impedance > RangeThreshold[g_CurrentRange][1] && g_CurrentRange < RANGE_1M)
    {
        /* 测量值过大,切换到更大量程 */
        newRange = g_CurrentRange + 1;
    }
    
    if(newRange != g_CurrentRange)
    {
        Range_Set(newRange);
    }
}

/**
  * @brief  手动切换量程 (循环)
  */
void Range_AutoSwitch(void)
{
    uint8_t newRange = g_CurrentRange + 1;
    if(newRange >= RANGE_COUNT)
        newRange = RANGE_10R;
    Range_Set(newRange);
}

/**
  * @brief  获取当前量程的参考电阻值
  */
float Range_GetRefResistance(void)
{
    CalibrationData_t *calData = Calibration_GetData();
    return calData->refResistance[g_CurrentRange];
}
```

------

## 八、高精度DFT算法优化

```c
/**
  ******************************************************************************
  * @file    dft_algorithm.c
  * @brief   优化的DFT算法 - 支持多频点分析
  ******************************************************************************
  */

#include "dft_algorithm.h"
#include <math.h>

#define PI  3.14159265358979f

/* 预计算的正弦/余弦表 (256点) */
static float SinTable[256];
static float CosTable[256];
static uint8_t TableInitialized = 0;

/**
  * @brief  初始化三角函数查找表
  */
void DFT_Init(void)
{
    uint16_t i;
    float angle;
    
    for(i = 0; i < 256; i++)
    {
        angle = 2.0f * PI * i / 256.0f;
        SinTable[i] = sinf(angle);
        CosTable[i] = cosf(angle);
    }
    
    TableInitialized = 1;
}

/**
  * @brief  快速单频点DFT (使用查找表)
  * @param  data: 输入数据 (12位ADC值)
  * @param  size: 数据长度 (必须为256)
  * @param  real: 实部输出
  * @param  imag: 虚部输出
  */
void DFT_SingleFreq_Fast(uint16_t *data, uint16_t size, float *real, float *imag)
{
    float sumReal = 0, sumImag = 0;
    float sample;
    uint16_t i;
    
    if(!TableInitialized)
        DFT_Init();
    
    for(i = 0; i < size; i++)
    {
        /* 转换为浮点并去除直流偏置 */
        sample = (float)(data[i]) - 2048.0f;
        
        /* 使用查找表计算 */
        sumReal += sample * CosTable[i];
        sumImag += sample * SinTable[i];
    }
    
    *real = sumReal * 2.0f / size;
    *imag = sumImag * 2.0f / size;
}

/**
  * @brief  计算指定谐波的DFT
  *数据
  * @param  size: 数据长度
  * @param  harmonic: 谐波次数 (1=基波, 2=二次谐波...)
  * @param  real: 实部输出
  * @param  imag: 虚部输出
  */
void DFT_Harmonic(uint16_t *data, uint16_t size, uint8_t harmonic, 
                  float *real, float *imag)
{
    float sumReal = 0, sumImag = 0;
    float sample, angle;
    uint16_t i;
    
    for(i = 0; i < size; i++)
    {
        sample = (float)(data[i]) - 2048.0f;
        angle = 2.0f * PI * harmonic * i / size;
        
        sumReal += sample * cosf(angle);
        sumImag += sample * sinf(angle);
    }
    
    *real = sumReal * 2.0f / size;
    *imag = sumImag * 2.0f / size;
}

/**
  * @brief  计算信号的THD (总谐波失真)输入数据
  * @param  size: 数据长度
  * @retval THD百分比
  */
float DFT_Calculate_THD(uint16_t *data, uint16_t size)
{
    float fundReal, fundImag, fundMag;
    float harmReal, harmImag, harmMagSq;
    float totalHarmSq = 0;
    uint8_t h;
    
    /* 计算基波幅值 */
    DFT_Harmonic(data, size, 1, &fundReal, &fundImag);
    fundMag = sqrtf(fundReal * fundReal + fundImag * fundImag);
    
    if(fundMag < 0.001f)
        return 0;
    
    /* 计算2~10次谐波 */
    for(h = 2; h <= 10; h++)
    {
        DFT_Harmonic(data, size, h, &harmReal, &harmImag);
        harmMagSq = harmReal * harmReal + harmImag * harmImag;
        totalHarmSq += harmMagSq;
    }
    
    /* THD = sqrt(V2² + V3² + ...) / V1 * 100% */
    return sqrtf(totalHarmSq) / fundMag * 100.0f;
}

/**
  * @brief  Goertzel算法 - 更高效的单频点DFT
  *数据
  * @param  size: 数据长度
  * @param  targetFreq: 目标频率 (Hz)
  * @param  sampleRate: 采样率 (Hz)
  * @param  real: 实部输出
  * @param  imag: 虚部输出
  */
void Goertzel_SingleFreq(uint16_t *data, uint16_t size, 
                         float targetFreq, float sampleRate,
                         float *real, float *imag)
{
    float k, w, cosw, sinw, coeff;
    float s0, s1, s2;
    float sample;
    uint16_t i;
    
    /* 计算系数 */
    k = (targetFreq / sampleRate) * size;
    w = 2.0f * PI * k / size;
    cosw = cosf(w);
    sinw = sinf(w);
    coeff = 2.0f * cosw;
    
    /* Goertzel迭代 */
    s1 = 0;
    s2 = 0;
    
    for(i = 0; i < size; i++)
    {
        sample = (float)(data[i]) - 2048.0f;
        s0 = sample + coeff * s1 - s2;
        s2 = s1;
        s1 = s0;
    }
    
    /* 计算结果 */
    *real = (s1 - s2 * cosw) * 2.0f / size;
    *imag = (s2 * sinw) * 2.0f / size;
}
```

------

## 九、完整的头文件汇总

```c
/**
  ******************************************************************************
  * @file    lcr_meter.h
  * @brief   LCR测试仪总头文件
  ******************************************************************************
  */

#ifndef __LCR_METER_H
#define __LCR_METER_H

#include "stm32f10x.h"

/*============================================================================
 * 系统配置
 *============================================================================*/
#define SYSTEM_CLOCK        72000000    // 72MHz
#define ADC_SAMPLE_RATE     256000      // 256kHz采样率
#define DAC_SAMPLE_POINTS   256         // DAC每周期采样点数

/*============================================================================
 * 测量模式定义
 *============================================================================*/
#define MODE_AUTO           0
#define MODE_RESISTANCE     1
#define MODE_CAPACITANCE    2
#define MODE_INDUCTANCE     3

/*============================================================================
 * 量程定义
 *============================================================================*/
#define RANGE_10R           0   // 10Ω档
#define RANGE_100R          1   // 100Ω档
#define RANGE_1K            2   // 1KΩ档
#define RANGE_10K           3   // 10KΩ档
#define RANGE_100K          4   // 100KΩ档
#define RANGE_1M            5   // 1MΩ档
#define RANGE_COUNT         6

/*============================================================================
 * 频率点定义
 *============================================================================*/
#define FREQ_100HZ          0
#define FREQ_1KHZ           1
#define FREQ_10KHZ          2
#define FREQ_100KHZ         3
#define FREQ_COUNT          4

/*============================================================================
 * 按键定义
 *============================================================================*/
#define KEY1_PRESS          1
#define KEY2_PRESS          2
#define KEY3_PRESS          3
#define KEY4_PRESS          4

/*============================================================================
 * Flash存储地址
 *============================================================================*/
#define FLASH_CAL_ADDR      0x0800FC00  // 校准数据存储地址

/*============================================================================
 * 数据结构定义
 *============================================================================*/

/* 阻抗测量结果 */
typedef struct {
    float impedance;    // 阻抗幅值 (Ohm)
    float phase;        // 相位角 (rad)
    float resistance;   // 电阻分量 (Ohm)
    float reactance;    // 电抗分量 (Ohm)
    float Q;            // 品质因数
    float D;            // 损耗因数
} ImpedanceResult_t;

/* 校准数据 */
typedef struct {
    float gainFactor;
    float offsetFactor;
    float refResistance[RANGE_COUNT];
    float openReal[FREQ_COUNT];
    float openImag[FREQ_COUNT];
    float shortReal[FREQ_COUNT];
    float shortImag[FREQ_COUNT];
    float loadReal[FREQ_COUNT];
    float loadImag[FREQ_COUNT];
    float loadRefValue;
    uint8_t currentRange;
    uint32_t validFlag;
} CalibrationData_t;

/*============================================================================
 * 函数声明
 *============================================================================*/

/* 系统初始化 */
void System_Init(void);

/* DAC信号发生 */
void DAC_Signal_Init(void);
void DAC_SetFrequency(uint32_t freq);
uint32_t DAC_GetFrequency(void);

/* ADC采样 */
void ADC_Sample_Init(void);
void ADC_StartSample(void);
void ADC_StopSample(void);
uint8_t ADC_IsSampleComplete(void);
uint16_t* ADC_GetBuffer(void);

/* LCR计算 */
void Calculate_Impedance(ImpedanceResult_t *result);
float Calculate_Resistance_Value(void);
float Calculate_Capacitance_Value(void);
float Calculate_Inductance_Value(void);
uint8_t Auto_Identify_Component_Type(void);
void LCR_SetCalibrationData(CalibrationData_t *calData);

/* 校准 */
void Calibration_Init(void);
void Calibration_Open(void);
void Calibration_Short(void);
void Calibration_Load(float refValue);
void Calibration_Apply(float rawReal, float rawImag, 
                       float *calReal, float *calImag, uint8_t freqIdx);
void Calibration_Save(void);
void Calibration_Load_FromFlash(void);
void Calibration_Reset(void);
CalibrationData_t* Calibration_GetData(void);

/* 量程切换 */
void Range_Switch_Init(void);
void Range_Set(uint8_t range);
uint8_t Range_Get(void);
void Range_AutoAdjust(float measuredValue, uint8_t mode);
void Range_AutoSwitch(void);
float Range_GetRefResistance(void);

/* DFT算法 */
void DFT_Init(void);
void DFT_SingleFreq_Fast(uint16_t *data, uint16_t size, float *real, float *imag);
void DFT_Harmonic(uint16_t *data, uint16_t size, uint8_t harmonic, 
                  float *real, float *imag);
float DFT_Calculate_THD(uint16_t *data, uint16_t size);
void Goertzel_SingleFreq(uint16_t *data, uint16_t size, 
                         float targetFreq, float sampleRate,
                         float *real, float *imag);

/* Flash存储 */
void Flash_Write(uint32_t addr, uint8_t *data, uint16_t len);
void Flash_Read(uint32_t addr, uint8_t *data, uint16_t len);

/* 外设驱动 */
void LCD_Init(void);
void LCD_Clear(void);
void LCD_Write_String(uint8_t x, uint8_t y, char *str);
void LED_Init(void);
void LED_SetMode(uint8_t mode);
void KEY_Init(void);
uint8_t KEY_Scan(uint8_t mode);
void delay_init(void);
void delay_ms(uint16_t ms);
void delay_us(uint32 __LCR_METER_H */
```

------

## 十、性能指标与调试建议

### 1. 预期性能指标

|       参数        |  纯STM32方案   |  NE555方案   |
| :---------------: | :------------: | :----------: |
|     测量频率      |  100Hz~100kHz  | 固定(由RC定) |
|     电阻精度      | ±0.5% (校准后) |    ±1~2%     |
|     电容精度      |  ±1% (校准后)  |    ±2~3%     |
|     电感精度      |  ±1% (校准后)  |    ±3~5%     |
|     测量速度      |    ~5次/秒     |   ~3次/秒    |
| 自动识 需手动切换 |                |              |

### 2. 调试要点

```text
┌─────────────────────────────────────────────────────────┐
│                     调试检查清单                        │
├─────────────────────────────────────────────────────────┤
│                                                         │
│ □ 1. DAC输出波形检查                                   │
│      - 用示波器观察PA4输出                │
│      - 确认正弦波频率和幅度正确                         │
│                                                         │
│ □ 2. ADC采样检查                                       │
│      - 确认采样率与DAC频率匹配                          │
│      - 每周期采样256点                                  │
│                                                         │
│ □ 3. 相位测量验证                                      │
│      - 纯电阻: 相≈0°                                 │
│      - 纯电容: 相位≈-90°                               │
│      - 纯电感: 相位≈+90°                               │
│                                                         │
│ □ 4. 校准验证                                          │
│      - 开路阻抗应>1MΩ                                  │
│      - 短路阻抗应<1Ω                                   │
│      - 标准电阻误差<0.5%                               │
│ □ 5. 量程切换测试                                      │
│      - 各量程参考电阻准确                               │
│      - 自动切换阈值合理                                 │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### 3. 常见问题解决

|      问题      |   可能原因   |        解决方法         |
| :------------: | :----------: | :---------------------: |
|    测跳动大    |   采样噪声   |  增加滤波次数,检查接地  |
|  相位测量不准  | ADC采样延迟  |   校准时补偿固定相移    |
|  小电容测不准  | 杂散电容影响 |      做好开路校准       |
|  大电阻测不准  |              |    提高输入阻抗,屏蔽    |
| 高频测量误差大 |   寄生参数   | 缩短测量引线,使用同轴线 |

------

## 十一、总结

**纯STM32方案的优势：**

1. **精度更高** - 利用度,避免NE555温漂
2. **灵活性强** - 频率软件可调,支持多频点测量
3. **成本更低** - 省去NE555及外围元件
4. **功能更全** - 支持自动识别、THD分析等
5. **校准完善** - 三点校准消除系统误差

**建议的开发顺序：**

1. 先调通DAC正弦波输出
2. 再调通ADC双通道采样
3. 验证DFT算法正确性
4. 实现基本阻抗测量
5. 添加校准功能
6. 完善量程切换和自动识别

如需进一步的技节或有其他问题，请随时告诉我！





```
进一步技节
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

已完成生成

好的，我来深入讲解更多技术细节，包括信号链设计、噪声抑制、高精度算法优化、PCB布局等关键内容。

------

## 一、信号链深度分析

### 1. 完整信号链路图

```text
┌─────────────────────────────────────────────────────────────────────────────┐
│                            完整信号链路                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────┐    ┌─────────┐    ┌─────────┐    ┌─────────┐                  │
│  │  DAC    │───►│ 缓冲放大 │───►│ 低通滤波│───►│ 功率驱动│──┐              │
│  │ 12-bit  │    │ (运放)  │    │ (抗混叠)│    │ (运放)  │  │              │
│  └─────────┘    └─────────┘    └─────────┘    └─────────┘  │              │
│       ▲                                                     │              │
│       │                                                     ▼              │
│  ┌─────────┐                                          ┌──────────┐        │
│  │  DMA    │                                          │ 激励信号 │        │
│  │ 自动传输│                                          │ Vexc     │        │
│  └─────────┘                                          └────┬─────┘        │
│                                                            │              │
│                              ┌──────────────────────────────┤              │
│                              │                              │              │
│                              ▼                              ▼              │
│                        ┌──────────┐                  ┌──────────┐         │
│                        │   Rref   │                  │    Zx    │         │
│                        │ (参考)   │                  │ (被测)   │         │
│                        └────┬─────┘                  └────┬─────┘         │
│                              │                              │              │
│                              │ Vi (电流信号)                │ Vx (电压信号)│
│                              ▼                              ▼              │
│                        ┌──────────┐                  ┌──────────┐         │
│                        │ 差分放大 │                  │ 差分放大 │         │
│                        │ (INA128) │                  │ (INA128) │         │
│                        └────┬─────┘                  └────┬─────┘         │
│                              │                              │              │
│                              ▼                              ▼              │
│                        ┌──────────┐                  ┌──────────┐         │
│                        │ 抗混叠   │                  │ 抗混叠   │         │
│                        │ 滤波器   │                  │ 滤波器   │         │
│                        └────┬─────┘                  └────┬─────┘         │
│                              │                              │              │
│                              ▼                              ▼              │
│                        ┌──────────┐                  ┌──────────┐         │
│                        │  ADC1    │                  │  ADC2    │         │
│                        │ (电流)   │                  │ (电压)   │         │
│                        └────┬─────┘                  └────┬─────┘         │
│                              │                              │              │
│                              └──────────┬───────────────────┘              │
│                                         │                                  │
│                                         ▼                                  │
│                                   ┌──────────┐                             │
│                                   │   DMA    │                             │
│                                   │ 双缓冲   │                             │
│                                   └────┬─────┘                             │
│                                         │                                  │
│                                         ▼                                  │
│                                   ┌──────────┐                             │
│                                   │ DSP处理  │                             │
│                                   │ DFT/校准 │                             │
│                                   └──────────┘                             │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 2. 各环节详细电路

#### 2.1 DAC输出缓冲与滤波电路

```text
【DAC输出级 - 二阶Sallen-Key低通滤波器】

                          VCC(3.3V)
                              │
                         ┌────┴────┐
                         │  100nF  │
                         └────┬────┘
                              │
    STM32 DAC ────┬──────────┤
    (PA4)        │          │
                 │     ┌────┴────┐
            ┌────┴────┐│         │
            │  1KΩ    ││  OPA365 │───┬──────────────────┐
            │   R1    │├─────────┤   │                  │
            └────┬────┘│    +    │   │                  │
                 │     └────┬────┘   │                  │
                 │          │        │                  │
            ┌────┴────┐     │   ┌────┴────┐             │
            │  1KΩ    │     │   │  100nF  │             │
            │   R2    │     │   │   C2    │             │
            └────┬────┘     │   └────┬────┘             │
                 │          │        │                  │
                 ├──────────┴────────┤                  │
                 │                   │                  │
            ┌────┴────┐              │                  │
            │  100nF  │              │                  │
            │   C1    │             GND                 │
            └────┬────┘                                 │
                 │                                      │
                GND                                     │
                                                        │
                                                        ▼
                                                   Vout (滤波后)

【设计参数】
截止频率: fc = 1/(2π√(R1×R2×C1×C2)) ≈ 160kHz
Q值: Q = √(R1×R2×C1×C2)/(C2×(R1+R2)) ≈ 0.707 (Butterworth)

【元件选择】
R1 = R2 = 1KΩ (精密电阻 0.1%)
C1 = C2 = 100nF (NPO/C0G电容)
运放: OPA365 (低噪声, 50MHz带宽, 轨到轨)
```

#### 2.2 功率驱动级

```text
【电压跟随器 + 电流增强】

                              VCC(5V)
                                 │
                            ┌────┴────┐
                            │  10uF   │
                            └────┬────┘
                                 │
    滤波后信号 ──────┬───────────┤
                    │           │
               ┌────┴────┐ ┌────┴────┐
               │         │ │         │
               │  OPA548 │─┤  OPA548 │──────► 激励输出
               │    +    │ │    -    │        (可驱动50mA)
               └────┬────┘ └────┬────┘
                    │           │
                    └─────┬─────┘
                          │
                     ┌────┴────┐
                     │  10Ω   │ (限流电阻)
                     └────┬────┘
                          │
                         GND

【或使用分立元件方案】

                         VCC(5V)
                            │
                       ┌────┴────┐
                       │  BD139  │ (NPN)
                       │    E    │
    输入信号 ──┬───────┤    B    │
              │       │    C    ├────────┬────► 输出
              │       └─────────┘        │
              │                          │
              │       ┌─────────┐        │
              │       │  BD140  │ (PNP)  │
              │       │    E    │        │
              └───────┤    B    │        │
                      │    C    ├────────┘
                      └────┬────┘
                           │
                          GND
```

#### 2.3 高精度差分放大电路

```text
【仪表放大器电路 - INA128】

                                    VCC(5V)
                                       │
                                  ┌────┴────┐
                                  │  100nF  │
                                  └────┬────┘
                                       │
                              ┌────────┴────────┐
                              │     INA128      │
                              │                 │
    Vin+ ─────────────────────┤ 3(+IN)          │
    (被测元件高端)             │                 │
                              │           6(OUT)├────────► ADC输入
    Vin- ─────────────────────┤ 2(-IN)          │
    (被测元件低端)             │                 │
                              │                 │
                              │    1(RG) 8(RG)  │
                              └──────┬───┬──────┘
                                     │   │
                                ┌────┴───┴────┐
                                │    Rg       │
                                │  (增益设置) │
                                └─────────────┘

【增益计算】
G = 1 + 50KΩ/Rg

Rg = 50KΩ/(G-1)

推荐增益设置:
┌────────┬────────┬──────────┐
│  增益  │   Rg   │  应用    │
├────────┼────────┼──────────┤
│   1    │   ∞    │ 大信号  │
│   10   │  5.56K │ 中信号  │
│   100  │  505Ω  │ 小信号  │
│   1000 │  50Ω   │ 微信号  │
└────────┴────────┴──────────┘

【可编程增益放大器 (PGA)】

使用CD4051切换不同Rg实现自动增益调整:

                    ┌─────────────┐
    INA128 Pin1 ────┤ CD4051 COM  │
                    │             │
                    │    X0 ──────┼──── ∞ (G=1)
                    │    X1 ──────┼──── 5.56K (G=10)
                    │    X2 ──────┼──── 505Ω (G=100)
                    │    X3 ──────┼──── 50Ω (G=1000)
                    │             │
    INA128 Pin8 ────┤   (接地)    │
                    │             │
    STM32 GPIO ─────┤ A B C       │
                    └─────────────┘
```

#### 2.4 抗混叠滤波器 (AAF)

```text
【四阶Butterworth低通滤波器】

                    第一级                      第二级
              ┌─────────────────┐        ┌─────────────────┐
              │                 │        │                 │
    输入 ─────┤  Sallen-Key     ├────────┤  Sallen-Key     ├────► ADC
              │  fc=200kHz      │        │  fc=200kHz      │
              │  Q=0.541        │        │  Q=1.307        │
              │                 │        │                 │
              └─────────────────┘        └─────────────────┘

【第一级详细电路】

              R1=1K        R2=1K
    输入 ────┬────/\/\/────┬────/\/\/────┬────────────────┐
             │             │             │                │
             │        C1=680pF          │                │
             │             │             │           ┌────┴────┐
             │            GND            │           │    +    │
             │                           │           │  OPA365 │───► 输出
             │                      C2=1.2nF        │    -    │
             │                           │           └────┬────┘
             │                          GND               │
             │                                            │
             └────────────────────────────────────────────┘

【设计要点】
1. 截止频率设为采样率的1/2以下 (Nyquist)
2. 采样率256kHz时, fc应<128kHz, 推荐100kHz
3. 使用NPO/C0G电容保证温度稳定性
4. 运放带宽应>10×fc
```

------

## 二、高精度ADC采样技术

### 1. 双ADC同步采样配置

```c
/**
  ******************************************************************************
  * @file    adc_dual_sync.c
  * @brief   双ADC同步采样 - 精确相位测量
  ******************************************************************************
  */

#include "stm32f10x.h"

#define ADC_BUFFER_SIZE     512     // 每通道256点
#define ADC_SAMPLE_CYCLES   256     // 每周期采样点数

/* 双缓冲区 */
static uint32_t ADC_DualBuffer[2][ADC_BUFFER_SIZE];
static volatile uint8_t g_ActiveBuffer = 0;
static volatile uint8_t g_BufferReady = 0;

/**
  * @brief  双ADC同步模式初始化
  * @note   ADC1(主) + ADC2(从) 同步采样
  *         确保电压和电流信号同时采样,相位误差<10ns
  */
void ADC_DualSync_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    /* 使能时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_ADC1 | RCC_APB2Periph_ADC2, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    
    /* ADC时钟: 72MHz/6 = 12MHz */
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);
    
    /* 配置ADC输入引脚 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2;  // PA1=电压, PA2=电流
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    /*========== 配置TIM2作为ADC触发源 ==========*/
    // 采样率 = 72MHz / (PSC+1) / (ARR+1)
    // 256kHz: 72MHz / 1 / 281 ≈ 256kHz
    TIM_TimeBaseStructure.TIM_Period = 280;
    TIM_TimeBaseStructure.TIM_Prescaler = 0;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    TIM_SelectOutputTrigger(TIM2, TIM_TRGOSource_Update);
    
    /*========== 配置DMA ==========*/
    DMA_DeInit(DMA1_Channel1);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)ADC_DualBuffer[0];
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = ADC_BUFFER_SIZE;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;  // 32位
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel1, &DMA_InitStructure);
    
    /* 使能DMA半传输和传输完成中断 (双缓冲) */
    DMA_ITConfig(DMA1_Channel1, DMA_IT_TC | DMA_IT_HT, ENABLE);
    
    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    DMA_Cmd(DMA1_Channel1, ENABLE);
    
    /*========== 配置ADC1 (主) ==========*/
    ADC_InitStructure.ADC_Mode = ADC_Mode_RegSimult;  // 规则同步模式
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T2_TRGO;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure);
    
    /* ADC1通道配置: PA1 (电压) */
    ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_7Cycles5);
    
    /* 使能ADC1 DMA */
    ADC_DMACmd(ADC1, ENABLE);
    
    /* 使能外部触发 */
    ADC_ExternalTrigConvCmd(ADC1, ENABLE);
    
    /*========== 配置ADC2 (从) ==========*/
    ADC_InitStructure.ADC_Mode = ADC_Mode_RegSimult;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;  // 由ADC1触发
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC2, &ADC_InitStructure);
    
    /* ADC2通道配置: PA2 (电流) */
    ADC_RegularChannelConfig(ADC2, ADC_Channel_2, 1, ADC_SampleTime_7Cycles5);
    
    /* 使能外部触发 */
    ADC_ExternalTrigConvCmd(ADC2, ENABLE);
    
    /*========== 使能ADC ==========*/
    ADC_Cmd(ADC1, ENABLE);
    ADC_Cmd(ADC2, ENABLE);
    
    /* ADC校准 */
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
    
    ADC_ResetCalibration(ADC2);
    while(ADC_GetResetCalibrationStatus(ADC2));
    ADC_StartCalibration(ADC2);
    while(ADC_GetCalibrationStatus(ADC2));
}

/**
  * @brief  启动ADC采样
  */
void ADC_DualSync_Start(void)
{
    g_BufferReady = 0;
    TIM_Cmd(TIM2, ENABLE);
}

/**
  * @brief  停止ADC采样
  */
void ADC_DualSync_Stop(void)
{
    TIM_Cmd(TIM2, DISABLE);
}

/**
  * @brief  DMA中断处理 - 双缓冲切换
  */
void DMA1_Channel1_IRQHandler(void)
{
    if(DMA_GetITStatus(DMA1_IT_HT1))  // 半传输完成
    {
        g_ActiveBuffer = 0;
        g_BufferReady = 1;
        DMA_ClearITPendingBit(DMA1_IT_HT1);
    }
    
    if(DMA_GetITStatus(DMA1_IT_TC1))  // 传输完成
    {
        g_ActiveBuffer = 1;
        g_BufferReady = 1;
        DMA_ClearITPendingBit(DMA1_IT_TC1);
    }
}

/**
  * @brief  获取采样数据
  * @param  voltageData: 电压数据输出
  * @param  currentData: 电流数据输出
  * @param  size: 数据长度
  */
void ADC_GetSampleData(uint16_t *voltageData, uint16_t *currentData, uint16_t size)
{
    uint32_t *pBuffer;
    uint16_t i;
    uint16_t offset;
    
    /* 选择已完成的缓冲区 */
    offset = (g_ActiveBuffer == 0) ? (ADC_BUFFER_SIZE / 2) : 0;
    pBuffer = &ADC_DualBuffer[0][offset];
    
    /* 分离ADC1和ADC2数据 */
    // 同步模式下, DR寄存器: [31:16]=ADC2, [15:0]=ADC1
    for(i = 0; i < size; i++)
    {
        voltageData[i] = (uint16_t)(pBuffer[i] & 0xFFFF);         // ADC1 (电压)
        currentData[i] = (uint16_t)((pBuffer[i] >> 16) & 0xFFFF); // ADC2 (电流)
    }
}

/**
  * @brief  检查数据是否就绪
  */
uint8_t ADC_IsDataReady(void)
{
    if(g_BufferReady)
    {
        g_BufferReady = 0;
        return 1;
    }
    return 0;
}
```

### 2. 过采样与抽取 (提高分辨率)

```c
/**
  ******************************************************************************
  * @file    oversampling.c
  * @brief   过采样技术 - 将12位ADC提升到16位有效分辨率
  ******************************************************************************
  */

#include "oversampling.h"
#include <math.h>

/*
 * 过采样原理:
 * 每增加4倍采样率,可提高1位有效分辨率
 * 
 * 目标: 12位 → 16位 (增加4位)
 * 需要: 4^4 = 256倍过采样
 * 
 * 实现: 采样256点,求和后右移4位
 */

#define OVERSAMPLE_RATIO    256     // 过采样倍数
#define EXTRA_BITS          4       // 额外分辨率位数

/**
  * @brief  过采样处理
  * @param  rawData: 原始12位ADC数据
  * @param  size: 数据长度 (必须是OVERSAMPLE_RATIO的倍数)
  * @param  result: 16位结果输出
  * @param  resultSize: 结果长度
  */
void Oversampling_Process(uint16_t *rawData, uint16_t size, 
                          uint16_t *result, uint16_t *resultSize)
{
    uint32_t sum;
    uint16_t i, j;
    uint16_t groups = size / OVERSAMPLE_RATIO;
    
    for(i = 0; i < groups; i++)
    {
        sum = 0;
        
        /* 累加256个采样值 */
        for(j = 0; j < OVERSAMPLE_RATIO; j++)
        {
            sum += rawData[i * OVERSAMPLE_RATIO + j];
        }
        
        /* 右移4位 (除以16) 得到16位结果 */
        result[i] = (uint16_t)(sum >> EXTRA_BITS);
    }
    
    *resultSize = groups;
}

/**
  * @brief  带抽取的过采样 (Decimation)
  * @param  rawData: 原始数据
  * @param  size: 数据长度
  * @param  oversampleRatio: 过采样比
  * @param  result: 结果输出
  * @retval 结果数据长度
  */
uint16_t Oversampling_Decimate(uint16_t *rawData, uint16_t size, 
                                uint8_t oversampleRatio, float *result)
{
    uint32_t sum;
    uint16_t i, j;
    uint16_t groups = size / oversampleRatio;
    uint8_t shift;
    
    /* 计算移位数 */
    shift = 0;
    uint8_t temp = oversampleRatio;
    while(temp > 1)
    {
        temp >>= 1;
        shift++;
    }
    shift >>= 1;  // 每4倍采样增加1位
    
    for(i = 0; i < groups; i++)
    {
        sum = 0;
        for(j = 0; j < oversampleRatio; j++)
        {
            sum += rawData[i * oversampleRatio + j];
        }
        
        /* 转换为电压值 */
        result[i] = (float)(sum >> shift) * 3.3f / 4096.0f / (1 << shift);
    }
    
    return groups;
}

/**
  * @brief  Sinc滤波器 (用于Sigma-Delta过采样)
  * @param  data: 输入数据
  * @param  size: 数据长度
  * @param  order: 滤波器阶数 (1-4)
  * @param  decimation: 抽取比
  * @param  output: 输出数据
  * @retval 输出数据长度
  */
uint16_t Sinc_Filter(uint16_t *data, uint16_t size, uint8_t order, 
                     uint8_t decimation, uint32_t *output)
{
    static int32_t integrator[4] = {0};
    static int32_t differentiator[4] = {0};
    static int32_t diffDelay[4] = {0};
    
    uint16_t i, j;
    uint16_t outIdx = 0;
    int32_t temp;
    
    for(i = 0; i < size; i++)
    {
        /* 积分器级联 */
        temp = data[i];
        for(j = 0; j < order; j++)
        {
            integrator[j] += temp;
            temp = integrator[j];
        }
        
        /* 抽取 */
        if((i + 1) % decimation == 0)
        {
            /* 微分器级联 */
            temp = integrator[order - 1];
            for(j = 0; j < order; j++)
            {
                differentiator[j] = temp - diffDelay[j];
                diffDelay[j] = temp;
                temp = differentiator[j];
            }
            
            output[outIdx++] = (temp > 0) ? temp : 0;
        }
    }
    
    return outIdx;
}
```

------

## 三、高精度DFT/FFT算法

### 1. 优化的Goertzel算法

```c
/**
  ******************************************************************************
  * @file    goertzel_optimized.c
  * @brief   优化的Goertzel算法 - 单频点高效计算
  ******************************************************************************
  */

#include "goertzel_optimized.h"
#include <math.h>

#define PI  3.14159265358979323846f

/**
  * @brief  标准Goertzel算法
  * @param  data: 输入数据 (已去除直流)
  * @param  N: 数据长度
  * @param  k: 目标频率bin (k = f_target * N / f_sample)
  * @param  magnitude: 幅值输出
  * @param  phase: 相位输出 (弧度)
  */
void Goertzel_Standard(float *data, uint16_t N, float k, 
                       float *magnitude, float *phase)
{
    float omega = 2.0f * PI * k / N;
    float coeff = 2.0f * cosf(omega);
    float s0, s1 = 0, s2 = 0;
    uint16_t i;
    float real, imag;
    
    /* 迭代计算 */
    for(i = 0; i < N; i++)
    {
        s0 = data[i] + coeff * s1 - s2;
        s2 = s1;
        s1 = s0;
    }
    
    /* 计算实部和虚部 */
    real = s1 - s2 * cosf(omega);
    imag = s2 * sinf(omega);
    
    /* 计算幅值和相位 */
    *magnitude = sqrtf(real * real + imag * imag) * 2.0f / N;
    *phase = atan2f(imag, real);
}

/**
  * @brief  定点数Goertzel算法 (适用于无FPU的MCU)
  * @param  (Q15格式)
  * @param  N: 数据长度
  * @param  k: 目标频率bin
  * @param  magnitude: 幅值输出 (Q15)
  * @param  phase: 相位输出 (Q15, 范围-32768~32767对应-π~π)
  */
void Goertzel_FixedPoint(int16_t *data, uint16_t N, uint16_t k,
                         int32_t *magnitude, int16_t *phase)
{
    /* 预计算系数 (Q14格式) */
    // coeff = 2*cos(2πk/N) * 16384
    int32_t omega_q14 = (int32_t)(2.0f * PI * k / N * 16384.0f);
    int16_t coeff = (int16_t)(2.0f * cosf(2.0f * PI * k / N) * 16384.0f);
    
    int32_t s0, s1 = 0, s2 = 0;
    uint16_t i;
    int32_t real, imag;
    
    /* 迭代计算 */
    for(i = 0; i < N; i++)
    {
        s0 = (int32_t)data[i] + ((coeff * s1) >> 14) - s2;
        s2 = s1;
        s1 = s0;
    }
    
    /* 计算实部和虚部 */
    int16_t cos_omega = (int16_t)(cosf(2.0f * PI * k / N) * 32767.0f);
    int16_t sin_omega = (int16_t)(sinf(2.0f * PI * k / N) * 32767.0f);
    
    real = s1 - ((s2 * cos_omega) >> 15);
    imag = (s2 * sin_omega) >> 15;
    
    /* 计算幅值 (使用快速平方根近似) */
    *magnitude = fast_sqrt_i32(real * real + imag * imag);
    
    /* 计算相位 (使用CORDIC或查表) */
    *phase = fast_atan2_i16(imag, real);
}

/**
  * @brief  滑动Goertzel算法 (实时流处理)
  * @note   适用于连续数据流的实时频率检测
  */
typedef struct {
    float coeff;
    float s1, s2;
    float cos_omega, sin_omega;
    uint16_t N;
    uint16_t count;
} SlidingGoertzel_t;

void SlidingGoertzel_Init(SlidingGoertzel_t *sg, uint16_t N, float k)
{
    float omega = 2.0f * PI * k / N;
    sg->coeff = 2.0f * cosf(omega);
    sg->cos_omega = cosf(omega);
    sg->sin_omega = sinf(omega);
    sg->s1 = 0;
    sg->s2 = 0;
    sg->N = N;
    sg->count = 0;
}

void SlidingGoertzel_Process(SlidingGoertzel_t *sg, float sample,
                             float *magnitude, float *phase)
{
    float s0 = sample + sg->coeff * sg->s1 - sg->s2;
    sg->s2 = sg->s1;
    sg->s1 = s0;
    sg->count++;
    
    /* 每N个采样点输出一次结果 */
    if(sg->count >= sg->N)
    {
        float real = sg->s1 - sg->s2 * sg->cos_omega;
        float imag = sg->s2 * sg->sin_omega;
        
        *magnitude = sqrtf(real * real + imag * imag) * 2.0f / sg->N;
        *phase = atan2f(imag, real);
        
        /* 重置 */
        sg->s1 = 0;
        sg->s2 = 0;
        sg->count = 0;
    }
}
```

### 2. 窗函数与频谱泄漏抑制

```c
/**
  ******************************************************************************
  * @file    window_functions.c
  * @brief   窗函数实现 - 减少频谱泄漏
  ******************************************************************************
  */

#include "window_functions.h"
#include <math.h>

#define PI  3.14159265358979f

/* 预计算的窗函数系数表 */
static float HannWindow[256];
static float HammingWindow[256];
static float BlackmanWindow[256];
static float FlatTopWindow[256];
static uint8_t WindowInitialized = 0;

/**
  * @brief  初始化窗函数表
  * @param  size: 窗口大小
  */
void Window_Init(uint16_t size)
{
    uint16_t i;
    float n;
    
    for(i = 0; i < size; i++)
    {
        n = (float)i / (size - 1);
        
        /* Hann窗 (余弦窗) */
        // 主瓣宽度: 4bins, 旁瓣衰减: -31.5dB
        HannWindow[i] = 0.5f * (1.0f - cosf(2.0f * PI * n));
        
        /* Hamming窗 */
        // 主瓣宽度: 4bins, 旁瓣衰减: -42.7dB
        HammingWindow[i] = 0.54f - 0.46f * cosf(2.0f * PI * n);
        
        /* Blackman窗 */
        // 主瓣宽度: 6bins, 旁瓣衰减: -58dB
        BlackmanWindow[i] = 0.42f - 0.5f * cosf(2.0f * PI * n) 
                          + 0.08f * cosf(4.0f * PI * n);
        
        /* Flat-Top窗 (幅值精度最高) */
        // 主瓣宽度: 10bins, 旁瓣衰减: -93dB
        // 幅值误差: <0.01%
        FlatTopWindow[i] = 0.21557895f 
                         - 0.41663158f * cosf(2.0f * PI * n)
                         + 0.277263158f * cosf(4.0f * PI * n)
                         - 0.083578947f * cosf(6.0f * PI * n)
                         + 0.006947368f * cosf(8.0f * PI * n);
    }
    
    WindowInitialized = 1;
}

/**
  * @brief  应用窗函数
  */输出数据
  * @param  size: 数据长度
  * @param  windowType: 窗函数类型
  */
typedef enum {
    WINDOW_RECT = 0,    // 矩形窗 (无窗)
    WINDOW_HANN,        // Hann窗
    WINDOW_HAMMING,     // Hamming窗
    WINDOW_BLACKMAN,    // Blackman窗
    WINDOW_FLATTOP      // Flat-Top窗
} WindowType_t;

void Window_Apply(float *data, uint16_t size, WindowType_t windowType)
{
    uint16_t i;
    float *window;
    
    if(!WindowInitialized)
        Window_Init(size);
    
    switch(windowType)
    {
        case WINDOW_HANN:
            window = HannWindow;
            break;
        case WINDOW_HAMMING:
            window = HammingWindow;
            break;
        case WINDOW_BLACKMAN:
            window = BlackmanWindow;
            break;
        case WINDOW_FLATTOP:
            window = FlatTopWindow;
            break;
        case WINDOW_RECT:
        default:
            return;  // 矩形窗不需要处理
    }
    
    for(i = 0; i < size; i++)
    {
        data[i] *= window[i];
    }
}

/**
  * @brief  获取窗函数的相干增益 (用于幅值校正)
  * @param  windowType: 窗函数类型
  * @retval 相干增益
  */
float Window_GetCoherentGain(WindowType_t windowType)
{
    switch(windowType)
    {
        case WINDOW_RECT:     return 1.0f;
        case WINDOW_HANN:     return 0.5f;
        case WINDOW_HAMMING:  return 0.54f;
        case WINDOW_BLACKMAN: return 0.42f;
        case WINDOW_FLATTOP:  return 0.22f;
        default:              return 1.0f;
    }
}

/**
  * @brief  频谱插值 (提高频率分辨率)
  * @param  mag: 幅值谱
  * @param  peakBin: 峰值bin索引
  * @param  N: FFT长度
  * @param  fs: 采样率
  * @retval 精确频率
  */
float Spectrum_Interpolate(float *mag, uint16_t peakBin, uint16_t N, float fs)
{
    float alpha, beta, gamma;
    float delta;
    
    if(peakBin == 0 || peakBin >= N/2 - 1)
        return (float)peakBin * fs / N;
    
    /* 抛物线插值 */
    alpha = mag[peakBin - 1];
    beta = mag[peakBin];
    gamma = mag[peakBin + 1];
    
    delta = 0.5f * (alpha - gamma) / (alpha - 2.0f * beta + gamma);
    
    return ((float)peakBin + delta) * fs / N;
}
```

------

## 四、相位测量与校正

### 1. 精确相位计算

```c
/**
  ******************************************************************************
  * @file    phase_measurement.c
  * @brief   高精度相位测量
  ******************************************************************************
  */

#include "phase_measurement.h"
#include <math.h>

#define PI  3.14159265358979f

/**
  * @brief  计算两路信号的相位差
  * @param  signal1: 信号1 (参考)
  * @param  signal2: 信号2 (被测)
  * @param  size: 数据长度
  * @param  sampleRate: 采样率
  * @param  signalFreq: 信号频率
  * @retval 相位差 (弧度, -π ~ +π)
  */
float Phase_Calculate(float *signal1, float *signal2, uint16_t size,
                      float sampleRate, float signalFreq)
{
    float real1, imag1, real2, imag2;
    float phase1, phase2, phaseDiff;
    float k;
    
    /* 计算目标频率对应的bin */
    k = signalFreq * size / sampleRate;
    
    /* 使用Goertzel算法计算两路信号的相位 */
    float omega = 2.0f * PI * k / size;
    float coeff = 2.0f * cosf(omega);
    float s0, s1, s2;
    uint16_t i;
    
    /* 信号1 */
    s1 = s2 = 0;
    for(i = 0; i < size; i++)
    {
        s0 = signal1[i] + coeff * s1 - s2;
        s2 = s1;
        s1 = s0;
    }
    real1 = s1 - s2 * cosf(omega);
    imag1 = s2 * sinf(omega);
    phase1 = atan2f(imag1, real1);
    
    /* 信号2 */
    s1 = s2 = 0;
    for(i = 0; i < size; i++)
    {
        s0 = signal2[i] + coeff * s1 - s2;
        s2 = s1;
        s1 = s0;
    }
    real2 = s1 - s2 * cosf(omega);
    imag2 = s2 * sinf(omega);
    phase2 = atan2f(imag2, real2);
    
    /* 计算相位差 */
    phaseDiff = phase2 - phase1;
    
    /* 归一化到 -π ~ +π */
    while(phaseDiff > PI) phaseDiff -= 2.0f * PI;
    while(phaseDiff < -PI) phaseDiff += 2.0f * PI;
    
    return phaseDiff;
}

/**
  * @brief  相位展开 (处理相位跳变)
  * @param  phase: 相位数组
  * @param  size: 数据长度
  */
void Phase_Unwrap(float *phase, uint16_t size)
{
    uint16_t i;
    float diff;
    float offset = 0;
    
    for(i = 1; i < size; i++)
    {
        diff = phase[i] - phase[i-1];
        
        if(diff > PI)
            offset -= 2.0f * PI;
        else if(diff < -PI)
            offset += 2.0f * PI;
        
        phase[i] += offset;
    }
}

/**
  * @brief  系统相位校正
  * @note   补偿ADC采样延迟、运放相移等系统误差
  */
typedef struct {
    float phaseOffset[4];   // 各频率点的相位偏移
    float freqPoints[4];    // 频率点
    uint8_t numPoints;
} PhaseCalibration_t;

static PhaseCalibration_t g_PhaseCal;

/**
  * @brief  相位校准 (使用纯电阻)
  * @param  freq: 测试频率
  * @param  measuredPhase: 测量的相位
  * @note   纯电阻的理论相位为0,测量值即为系统相移
  */
void Phase_Calibrate(float freq, float measuredPhase)
{
    uint8_t i;
    
    /* 找到对应的频率点 */
    for(i = 0; i < g_PhaseCal.numPoints; i++)
    {
        if(fabsf(freq - g_PhaseCal.freqPoints[i]) < 1.0f)
        {
            g_PhaseCal.phaseOffset[i] = measuredPhase;
            return;
        }
    }
    
    /* 添加新的频率点 */
    if(g_PhaseCal.numPoints < 4)
    {
        g_PhaseCal.freqPoints[g_PhaseCal.numPoints] = freq;
        g_PhaseCal.phaseOffset[g_PhaseCal.numPoints] = measuredPhase;
        g_PhaseCal.numPoints++;
    }
}

/**
  * @brief  应用相位校正
  * @param  freq: 测量频率
  * @param  rawPhase: 原始相位
  * @retval 校正后的相位
  */
float Phase_ApplyCorrection(float freq, float rawPhase)
{
    uint8_t i;
    float offset = 0;
    
    /* 线性插值获取相位偏移 */
    for(i = 0; i < g_PhaseCal.numPoints - 1; i++)
    {
        if(freq >= g_PhaseCal.freqPoints[i] && freq <= g_PhaseCal.freqPoints[i+1])
        {
            float ratio = (freq - g_PhaseCal.freqPoints[i]) / 
                         (g_PhaseCal.freqPoints[i+1] - g_PhaseCal.freqPoints[i]);
            offset = g_PhaseCal.phaseOffset[i] + 
                    ratio * (g_PhaseCal.phaseOffset[i+1] - g_PhaseCal.phaseOffset[i]);
            break;
        }
    }
    
    return rawPhase - offset;
}
```

### 2. 复阻抗计算

```c
/**
  ******************************************************************************
  * @file    impedance_calc.c
  * @brief   复阻抗计算与参数提取
  ******************************************************************************
  */

#include "impedance_calc.h"
#include <math.h>

#define PI  3.14159265358979f

/**
  * @brief  完整阻抗测量结果
  */
typedef struct {
    /* 基本参数 */
    float Z;            // 阻抗幅值 |Z| (Ω)
    float theta;        // 相位角 θ (rad)
    float R;            // 电阻 (实部) (Ω)
    float X;            // 电抗 (虚部) (Ω)
    
    /* 串联等效模型 */
    float Rs;           // 串联电阻 (Ω)
    float Ls;           // 串联电感 (H)
    float Cs;           // 串联电容 (F)
    
    /* 并联等效模型 */
    float Rp;           // 并联电阻 (Ω)
    float Lp;           // 并联电感 (H)
    float Cp;           // 并联电容 (F)
    
    /* 品质因数 */
    float Q;            // 品质因数
    float D;            // 损耗因数 (tanδ)
    float ESR;          // 等效串联电阻
    
    /* 测量条件 */
    float frequency;    // 测量频率 (Hz)
    uint8_t componentType;  // 元件类型: 1=R, 2=C, 3=L
    
} ImpedanceFullResult_t;

/**
  * @brief  计算完整阻抗参数
  * @param  vMag: 电压幅值
  * @param  iMag: 电流幅值
  * @param  phaseDiff: 相位差 (电压超前电流为正)
  * @param  freq: 测量频率
  * @param  Rref: 参考电阻
  * @param  result: 结果输出
  */
void Impedance_Calculate(float vMag, float iMag, float phaseDiff, 
                         float freq, float Rref, ImpedanceFullResult_t *result)
{
    float omega = 2.0f * PI * freq;
    
    /* 基本阻抗计算 */
    // Z = V/I, 但实际电路中 I = Vref/Rref
    // 所以 Z = V * Rref / Vref
    if(iMag > 0.0001f)
    {
        result->Z = (vMag / iMag) * Rref;
    }
    else
    {
        result->Z = 1e9f;  // 开路
    }
    
    result->theta = phaseDiff;
    
    /* 实部和虚部 */
    result->R = result->Z * cosf(phaseDiff);
    result->X = result->Z * sinf(phaseDiff);
    
    /* 判断元件类型 */
    if(fabsf(phaseDiff) < 0.1f)  // |θ| < 5.7°
    {
        result->componentType = 1;  // 电阻
    }
    else if(phaseDiff < -0.1f)
    {
        result->componentType = 2;  // 电容 (电流超前)
    }
    else
    {
        result->componentType = 3;  // 电感 (电流滞后)
    }
    
    /* 串联等效模型 */
    result->Rs = result->R;
    
    if(result->X > 0)  // 感性
    {
        result->Ls = result->X / omega;
        result->Cs = 0;
    }
    else  // 容性
    {
        result->Ls = 0;
        result->Cs = -1.0f / (omega * result->X);
    }
    
    /* 并联等效模型 */
    // Zp = Rp // jXp
    // 1/Zp = 1/Rp + 1/jXp = (jXp + Rp)/(Rp*jXp)
    // Zp = Rp*jXp/(Rp + jXp)
    // 
    // 已知 Z = R + jX (串联)
    // 转换为并联: Rp = (R² + X²)/R, Xp = (R² + X²)/X
    
    float Zsq = result->R * result->R + result->X * result->X;
    
    if(fabsf(result->R) > 0.001f)
        result->Rp = Zsq / result->R;
    else
        result->Rp = 1e9f;
    
    if(fabsf(result->X) > 0.001f)
    {
        float Xp = Zsq / result->X;
        if(Xp > 0)
        {
            result->Lp = Xp / omega;
            result->Cp = 0;
        }
        else
        {
            result->Lp = 0;
            result->Cp = -1.0f / (omega * Xp);
        }
    }
    
    /* 品质因数和损耗因数 */
    if(fabsf(result->R) > 0.001f)
    {
        result->Q = fabsf(result->X / result->R);
        result->D = fabsf(result->R / result->X);
    }
    else
    {
        result->Q = 1e6f;
        result->D = 0;
    }
    
    /* ESR (等效串联电阻) */
    result->ESR = result->R;
    
    result->frequency = freq;
}

/**
  * @brief  多频点阻抗分析 (提取寄生参数)
  * @param  results: 多个频率点的测量结果
  * @param  numPoints: 频率点数量
  * @param  L: 提取的电感值
  * @param  C: 提取的电容值
  * @param  R: 提取的电阻值
  * @param  SRF: 自谐振频率
  */
void Impedance_MultiFreqAnalysis(ImpedanceFullResult_t *results, uint8_t numPoints,
                                  float *L, float *C, float *R, float *SRF)
{
    uint8_t i;
    float sumL = 0, sumC = 0, sumR = 0;
    uint8_t countL = 0, countC = 0;
    float minZ = 1e9f, maxZ = 0;
    float freqAtMinZ = 0;
    
    for(i = 0; i < numPoints; i++)
    {
        /* 累加电阻值 */
        sumR += results[i].R;
        
        /* 根据相位判断并累加L或C */
        if(results[i].theta > 0.1f)  // 感性
        {
            sumL += results[i].Ls;
            countL++;
        }
        else if(results[i].theta < -0.1f)  // 容性
        {
            sumC += results[i].Cs;
            countC++;
        }
        
        /* 找自谐振点 (阻抗最小或最大) */
        if(results[i].Z < minZ)
        {
            minZ = results[i].Z;
            freqAtMinZ = results[i].frequency;
        }
        if(results[i].Z > maxZ)
        {
            maxZ = results[i].Z;
        }
    }
    
    /* 计算平均值 */
    *R = sumR / numPoints;
    *L = (countL > 0) ? (sumL / countL) : 0;
    *C = (countC > 0) ? (sumC / countC) : 0;
    
    /* 自谐振频率 */
    *SRF = freqAtMinZ;
    
    /* 如果没有直接测到SRF,可以通过L和C计算 */
    if(*SRF == 0 && *L > 0 && *C > 0)
    {
        *SRF = 1.0f / (2.0f * PI * sqrtf(*L * *C));
    }
}
```

------

## 五、自动量程与增益控制

### 1. 智能量程切换算法

```c
/**
  ******************************************************************************
  * @file    auto_range.c
  * @brief   自动量程切换算法
  ******************************************************************************
  */

#include "auto_range.h"
#include "range_switch.h"
#include "adc_sample.h"

/* 量程配置表 */
typedef struct {
    float refResistance;    // 参考电阻值
    float minImpedance;     // 最小可测阻抗
    float maxImpedance;     // 最大可测阻抗
    float optimalMin;       // 最佳测量范围下限
    float optimalMax;       // 最佳测量范围上限
    uint8_t pgaGain;        // PGA增益设置
} RangeConfig_t;

static const RangeConfig_t RangeTable[RANGE_COUNT] = {
    /* Rref,    Zmin,    Zmax,     OptMin,   OptMax,  Gain */
    {  10.0f,   0.1f,    100.0f,   1.0f,     50.0f,   100 },  // 10Ω档
    {  100.0f,  1.0f,    1000.0f,  10.0f,    500.0f,  10  },  // 100Ω档
    {  1000.0f, 10.0f,   10000.0f, 100.0f,   5000.0f, 1   },  // 1KΩ档
    {  10000.0f,100.0f,  100000.0f,1000.0f,  50000.0f,1   },  // 10KΩ档
    {  100000.0f,1000.0f,1000000.0f,10000.0f,500000.0f,1  },  // 100KΩ档
    {  1000000.0f,10000.0f,10000000.0f,100000.0f,5000000.0f,1} // 1MΩ档
};

/* 当前量程状态 */
static uint8_t g_CurrentRange = RANGE_1K;
static uint8_t g_RangeStable = 0;
static uint8_t g_RangeChangeCount = 0;

/**
  * @brief  检查ADC是否饱和
  * @param  adcValue: ADC值
  * @retval 1=饱和, 0=正常
  */
static uint8_t Check_ADC_Saturation(uint16_t adcValue)
{
    /* ADC接近满量程或接近0 */
    if(adcValue > 4000 || adcValue < 100)
        return 1;
    return 0;
}

/**
  * @brief  检查信号幅度是否合适
  * @param  peakToPeak: 峰峰值 (ADC单位)
  * @retval -1=太小, 0=合适, 1=太大
  */
static int8_t Check_Signal_Level(uint16_t peakToPeak)
{
    if(peakToPeak < 200)
        return -1;  // 信号太小
    else if(peakToPeak > 3800)
        return 1;   // 信号太大
    else
        return 0;   // 合适
}

/**
  * @brief  自动量程调整
  * @param  measuredZ: 测量的阻抗值
  * @param  adcVoltage: 电压通道ADC值
  * @param  adcCurrent: 电流通道ADC值
  * @retval 1=量程已改变, 0=量程未变
  */
uint8_t AutoRange_
```





```
继续
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

已完成生成

继续完成代码：

```c
/**
  * @brief  自动量程调整
  * @param  measuredZ: 测量的阻抗值
  * @param  adcVoltage: 电压通道ADC值
  * @param  adcCurrent: 电流通道ADC值
  * @retval 1=量程已改变, 0=量程未变
  */
uint8_t AutoRange_Adjust(float measuredZ, uint16_t *adcVoltage, uint16_t *adcCurrent, uint16_t size)
{
    uint8_t newRange = g_CurrentRange;
    uint16_t vMax = 0, vMin = 4095, iMax = 0, iMin = 4095;
    uint16_t vPP, iPP;
    int8_t vLevel, iLevel;
    uint16_t i;
    
    /* 计算峰峰值 */
    for(i = 0; i < size; i++)
    {
        if(adcVoltage[i] > vMax) vMax = adcVoltage[i];
        if(adcVoltage[i] < vMin) vMin = adcVoltage[i];
        if(adcCurrent[i] > iMax) iMax = adcCurrent[i];
        if(adcCurrent[i] < iMin) iMin = adcCurrent[i];
    }
    
    vPP = vMax - vMin;
    iPP = iMax - iMin;
    
    /* 检查信号电平 */
    vLevel = Check_Signal_Level(vPP);
    iLevel = Check_Signal_Level(iPP);
    
    /* 检查ADC饱和 */
    if(Check_ADC_Saturation(vMax) || Check_ADC_Saturation(iMax))
    {
        /* 信号过大,切换到更大量程 */
        if(g_CurrentRange < RANGE_1M)
            newRange = g_CurrentRange + 1;
    }
    else if(vLevel == -1 && iLevel == -1)
    {
        /* 信号过小,切换到更小量程 */
        if(g_CurrentRange > RANGE_10R)
            newRange = g_CurrentRange - 1;
    }
    else
    {
        /* 根据阻抗值判断最佳量程 */
        for(i = 0; i < RANGE_COUNT; i++)
        {
            if(measuredZ >= RangeTable[i].optimalMin && 
               measuredZ <= RangeTable[i].optimalMax)
            {
                newRange = i;
                break;
            }
        }
    }
    
    /* 防止量程频繁切换 (滞回) */
    if(newRange != g_CurrentRange)
    {
        g_RangeChangeCount++;
        
        /* 连续3次判断需要切换才真正切换 */
        if(g_RangeChangeCount >= 3)
        {
            g_CurrentRange = newRange;
            Range_Set(newRange);
            g_RangeChangeCount = 0;
            g_RangeStable = 0;
            return 1;
        }
    }
    else
    {
        g_RangeChangeCount = 0;
        g_RangeStable = 1;
    }
    
    return 0;
}

/**
  * @brief  快速量程搜索 (开机或大范围变化时)
  * @param  estimatedZ: 估计的阻抗值 (可以为0表示未知)
  */
void AutoRange_FastSearch(float estimatedZ)
{
    uint8_t range;
    uint16_t adcV[64], adcI[64];
    uint16_t vPP, iPP;
    uint16_t vMax, vMin, iMax, iMin;
    uint16_t i;
    
    if(estimatedZ > 0)
    {
        /* 根据估计值直接选择量程 */
        for(range = 0; range < RANGE_COUNT; range++)
        {
            if(estimatedZ >= RangeTable[range].minImpedance &&
               estimatedZ <= RangeTable[range].maxImpedance)
            {
                g_CurrentRange = range;
                Range_Set(range);
                return;
            }
        }
    }
    
    /* 从中间量程开始二分搜索 */
    range = RANGE_1K;
    
    for(int iter = 0; iter < 4; iter++)
    {
        Range_Set(range);
        delay_ms(10);
        
        /* 快速采样 */
        ADC_DualSync_Start();
        delay_ms(5);
        ADC_DualSync_Stop();
        ADC_GetSampleData(adcV, adcI, 64);
        
        /* 计算峰峰值 */
        vMax = vMin = adcV[0];
        iMax = iMin = adcI[0];
        for(i = 1; i < 64; i++)
        {
            if(adcV[i] > vMax) vMax = adcV[i];
            if(adcV[i] < vMin) vMin = adcV[i];
            if(adcI[i] > iMax) iMax = adcI[i];
            if(adcI[i] < iMin) iMin = adcI[i];
        }
        vPP = vMax - vMin;
        iPP = iMax - iMin;
        
        /* 判断并调整 */
        if(vPP > 3500 || iPP > 3500)
        {
            /* 信号太大,增大量程 */
            if(range < RANGE_1M) range++;
        }
        else if(vPP < 300 && iPP < 300)
        {
            /* 信号太小,减小量程 */
            if(range > RANGE_10R) range--;
        }
        else
        {
            /* 找到合适量程 */
            break;
        }
    }
    
    g_CurrentRange = range;
}

/**
  * @brief  获取当前量程信息
  */
void AutoRange_GetInfo(uint8_t *range, float *refRes, float *minZ, float *maxZ)
{
    *range = g_CurrentRange;
    *refRes = RangeTable[g_CurrentRange].refResistance;
    *minZ = RangeTable[g_CurrentRange].minImpedance;
    *maxZ = RangeTable[g_CurrentRange].maxImpedance;
}

/**
  * @brief  量程是否稳定
  */
uint8_t AutoRange_IsStable(void)
{
    return g_RangeStable;
}
```

### 2. 可编程增益放大器控制

```c
/**
  ******************************************************************************
  * @file    pga_control.c
  * @brief   可编程增益放大器控制
  ******************************************************************************
  */

#include "pga_control.h"
#include "stm32f10x.h"

/* PGA增益档位 */
typedef enum {
    PGA_GAIN_1 = 0,
    PGA_GAIN_2,
    PGA_GAIN_5,
    PGA_GAIN_10,
    PGA_GAIN_20,
    PGA_GAIN_50,
    PGA_GAIN_100,
    PGA_GAIN_200
} PGA_Gain_t;

/* 增益值表 */
static const float GainValues[] = {1.0f, 2.0f, 5.0f, 10.0f, 20.0f, 50.0f, 100.0f, 200.0f};

/* 当前增益 */
static PGA_Gain_t g_CurrentGain_V = PGA_GAIN_1;  // 电压通道
static PGA_Gain_t g_CurrentGain_I = PGA_GAIN_1;  // 电流通道

/* 控制引脚定义 (使用CD4051) */
#define PGA_V_A_PIN     GPIO_Pin_0
#define PGA_V_B_PIN     GPIO_Pin_1
#define PGA_V_C_PIN     GPIO_Pin_2
#define PGA_I_A_PIN     GPIO_Pin_3
#define PGA_I_B_PIN     GPIO_Pin_4
#define PGA_I_C_PIN     GPIO_Pin_5
#define PGA_PORT        GPIOB

/**
  * @brief  PGA初始化
  */
void PGA_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = PGA_V_A_PIN | PGA_V_B_PIN | PGA_V_C_PIN |
                                   PGA_I_A_PIN | PGA_I_B_PIN | PGA_I_C_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(PGA_PORT, &GPIO_InitStructure);
    
    /* 默认增益1 */
    PGA_SetGain(PGA_CHANNEL_V, PGA_GAIN_1);
    PGA_SetGain(PGA_CHANNEL_I, PGA_GAIN_1);
}

/**
  * @brief  设置PGA增益
  * @param  channel: 通道 (0=电压, 1=电流)
  * @param  gain: 增益档位
  */
void PGA_SetGain(uint8_t channel, PGA_Gain_t gain)
{
    uint16_t pinA, pinB, pinC;
    
    if(channel == 0)  // 电压通道
    {
        pinA = PGA_V_A_PIN;
        pinB = PGA_V_B_PIN;
        pinC = PGA_V_C_PIN;
        g_CurrentGain_V = gain;
    }
    else  // 电流通道
    {
        pinA = PGA_I_A_PIN;
        pinB = PGA_I_B_PIN;
        pinC = PGA_I_C_PIN;
        g_CurrentGain_I = gain;
    }
    
    /* 清除所有位 */
    GPIO_ResetBits(PGA_PORT, pinA | pinB | pinC);
    
    /* 设置选择位 */
    if(gain & 0x01) GPIO_SetBits(PGA_PORT, pinA);
    if(gain & 0x02) GPIO_SetBits(PGA_PORT, pinB);
    if(gain & 0x04) GPIO_SetBits(PGA_PORT, pinC);
}

/**
  * @brief  获取当前增益值
  */
float PGA_GetGainValue(uint8_t channel)
{
    if(channel == 0)
        return GainValues[g_CurrentGain_V];
    else
        return GainValues[g_CurrentGain_I];
}

/**
  * @brief  自动增益调整
  * @param  adcPeakToPeak: ADC峰峰值
  * @param  channel: 通道
  * @retval 1=增益已改变, 0=未改变
  */
uint8_t PGA_AutoAdjust(uint16_t adcPeakToPeak, uint8_t channel)
{
    PGA_Gain_t currentGain = (channel == 0) ? g_CurrentGain_V : g_CurrentGain_I;
    PGA_Gain_t newGain = currentGain;
    
    /* 目标: 使ADC峰峰值在1000~3000之间 */
    if(adcPeakToPeak > 3500)
    {
        /* 信号太大,降低增益 */
        if(currentGain > PGA_GAIN_1)
            newGain = currentGain - 1;
    }
    else if(adcPeakToPeak < 500)
    {
        /* 信号太小,提高增益 */
        if(currentGain < PGA_GAIN_200)
            newGain = currentGain + 1;
    }
    
    if(newGain != currentGain)
    {
        PGA_SetGain(channel, newGain);
        return 1;
    }
    
    return 0;
}
```

------

## 六、完整校准系统 (增强版)

### 1. 多点校准与温度补偿

```c
/**
  ******************************************************************************
  * @file    calibration_advanced.c
  * @brief   高级校准系统 - 多点校准、温度补偿
  ******************************************************************************
  */

#include "calibration_advanced.h"
#include "flash_storage.h"
#include "temperature_sensor.h"
#include <math.h>
#include <string.h>

#define PI  3.14159265358979f

/* 校准点数量 */
#define CAL_FREQ_POINTS     8   // 频率点
#define CAL_RANGE_POINTS    6   // 量程点
#define CAL_TEMP_POINTS     5   // 温度点

/* 高级校准数据结构 */
typedef struct {
    /* 开路校准矩阵 [频率][量程] */
    float openReal[CAL_FREQ_POINTS][CAL_RANGE_POINTS];
    float openImag[CAL_FREQ_POINTS][CAL_RANGE_POINTS];
    
    /* 短路校准矩阵 [频率][量程] */
    float shortReal[CAL_FREQ_POINTS][CAL_RANGE_POINTS];
    float shortImag[CAL_FREQ_POINTS][CAL_RANGE_POINTS];
    
    /* 负载校准矩阵 [频率][量程] */
    float loadReal[CAL_FREQ_POINTS][CAL_RANGE_POINTS];
    float loadImag[CAL_FREQ_POINTS][CAL_RANGE_POINTS];
    float loadRefValue[CAL_RANGE_POINTS];
    
    /* 温度补偿系数 */
    float tempCoeff[CAL_TEMP_POINTS];
    float tempPoints[CAL_TEMP_POINTS];
    
    /* 频率点 */
    float freqPoints[CAL_FREQ_POINTS];
    
    /* 相位校正 */
    float phaseOffset[CAL_FREQ_POINTS];
    
    /* 校准温度 */
    float calibrationTemp;
    
    /* 校准日期 */
    uint32_t calibrationDate;
    
    /* 有效标志 */
    uint32_t validFlag;
    
} AdvancedCalData_t;

static AdvancedCalData_t g_AdvCalData;

/* 默认频率点 */
static const float DefaultFreqPoints[CAL_FREQ_POINTS] = {
    100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 100000.0f
};

/**
  * @brief  初始化高级校准系统
  */
void AdvCal_Init(void)
{
    /* 从Flash加载校准数据 */
    Flash_Read(FLASH_ADV_CAL_ADDR, (uint8_t*)&g_AdvCalData, sizeof(AdvancedCalData_t));
    
    /* 检查有效性 */
    if(g_AdvCalData.validFlag != 0xCAL1DATA)
    {
        /* 使用默认值 */
        memset(&g_AdvCalData, 0, sizeof(AdvancedCalData_t));
        memcpy(g_AdvCalData.freqPoints, DefaultFreqPoints, sizeof(DefaultFreqPoints));
        
        /* 设置默认温度补偿 */
        g_AdvCalData.tempPoints[0] = 0.0f;
        g_AdvCalData.tempPoints[1] = 15.0f;
        g_AdvCalData.tempPoints[2] = 25.0f;
        g_AdvCalData.tempPoints[3] = 35.0f;
        g_AdvCalData.tempPoints[4] = 50.0f;
        
        for(int i = 0; i < CAL_TEMP_POINTS; i++)
        {
            g_AdvCalData.tempCoeff[i] = 1.0f;  // 默认无补偿
        }
    }
}

/**
  * @brief  查找最近的频率索引
  */
static uint8_t FindFreqIndex(float freq)
{
    uint8_t i;
    float minDiff = 1e9f;
    uint8_t bestIdx = 0;
    
    for(i = 0; i < CAL_FREQ_POINTS; i++)
    {
        float diff = fabsf(freq - g_AdvCalData.freqPoints[i]);
        if(diff < minDiff)
        {
            minDiff = diff;
            bestIdx = i;
        }
    }
    
    return bestIdx;
}

/**
  * @brief  双线性插值获取校准值
  * @param  matrix: 校准矩阵
  * @param  freq: 频率
  * @param  range: 量程
  */
static float BilinearInterpolate(float matrix[CAL_FREQ_POINTS][CAL_RANGE_POINTS],
                                  float freq, uint8_t range)
{
    uint8_t fIdx1, fIdx2;
    float fRatio;
    float val1, val2;
    
    /* 找到频率区间 */
    fIdx1 = 0;
    for(uint8_t i = 0; i < CAL_FREQ_POINTS - 1; i++)
    {
        if(freq >= g_AdvCalData.freqPoints[i] && freq <= g_AdvCalData.freqPoints[i+1])
        {
            fIdx1 = i;
            break;
        }
    }
    fIdx2 = fIdx1 + 1;
    if(fIdx2 >= CAL_FREQ_POINTS) fIdx2 = CAL_FREQ_POINTS - 1;
    
    /* 计算插值比例 */
    if(g_AdvCalData.freqPoints[fIdx2] != g_AdvCalData.freqPoints[fIdx1])
    {
        fRatio = (freq - g_AdvCalData.freqPoints[fIdx1]) / 
                 (g_AdvCalData.freqPoints[fIdx2] - g_AdvCalData.freqPoints[fIdx1]);
    }
    else
    {
        fRatio = 0;
    }
    
    /* 线性插值 */
    val1 = matrix[fIdx1][range];
    val2 = matrix[fIdx2][range];
    
    return val1 + fRatio * (val2 - val1);
}

/**
  * @brief  执行开路校准
  * @param  freq: 校准频率
  * @param  range: 校准量程
  */
void AdvCal_Open(float freq, uint8_t range)
{
    float real, imag;
    uint8_t fIdx = FindFreqIndex(freq);
    
    /* 多次测量取平均 */
    float sumReal = 0, sumImag = 0;
    for(int i = 0; i < 16; i++)
    {
        Measure_RawImpedance(freq, range, &real, &imag);
        sumReal += real;
        sumImag += imag;
        delay_ms(50);
    }
    
    g_AdvCalData.openReal[fIdx][range] = sumReal / 16.0f;
    g_AdvCalData.openImag[fIdx][range] = sumImag / 16.0f;
}

/**
  * @brief  执行短路校准
  */
void AdvCal_Short(float freq, uint8_t range)
{
    float real, imag;
    uint8_t fIdx = FindFreqIndex(freq);
    
    float sumReal = 0, sumImag = 0;
    for(int i = 0; i < 16; i++)
    {
        Measure_RawImpedance(freq, range, &real, &imag);
        sumReal += real;
        sumImag += imag;
        delay_ms(50);
    }
    
    g_AdvCalData.shortReal[fIdx][range] = sumReal / 16.0f;
    g_AdvCalData.shortImag[fIdx][range] = sumImag / 16.0f;
}

/**
  * @brief  执行负载校准
  * @param  refValue: 标准负载值
  */
void AdvCal_Load(float freq, uint8_t range, float refValue)
{
    float real, imag;
    uint8_t fIdx = FindFreqIndex(freq);
    
    float sumReal = 0, sumImag = 0;
    for(int i = 0; i < 16; i++)
    {
        Measure_RawImpedance(freq, range, &real, &imag);
        sumReal += real;
        sumImag += imag;
        delay_ms(50);
    }
    
    g_AdvCalData.loadReal[fIdx][range] = sumReal / 16.0f;
    g_AdvCalData.loadImag[fIdx][range] = sumImag / 16.0f;
    g_AdvCalData.loadRefValue[range] = refValue;
}

/**
  * @brief  应用完整校准补偿
  * @param  rawReal: 原始实部
  * @param  rawImag: 原始虚部
  * @param  freq: 测量频率
  * @param  range: 当前量程
  * @param  calReal: 校准后实部
  * @param  calImag: 校准后虚部
  */
void AdvCal_Apply(float rawReal, float rawImag, float freq, uint8_t range,
                  float *calReal, float *calImag)
{
    /* 获取插值后的校准参数 */
    float Zo_real = BilinearInterpolate(g_AdvCalData.openReal, freq, range);
    float Zo_imag = BilinearInterpolate(g_AdvCalData.openImag, freq, range);
    float Zs_real = BilinearInterpolate(g_AdvCalData.shortReal, freq, range);
    float Zs_imag = BilinearInterpolate(g_AdvCalData.shortImag, freq, range);
    
    /* 三点校准公式: Zx = (Zm - Zs) * Zo / (Zo - Zm) */
    
    /* 复数运算: Zm - Zs */
    float num_real = rawReal - Zs_real;
    float num_imag = rawImag - Zs_imag;
    
    /* 复数运算: Zo - Zm */
    float den_real = Zo_real - rawReal;
    float den_imag = Zo_imag - rawImag;
    
    /* 复数运算: (Zm - Zs) * Zo */
    float temp_real = num_real * Zo_real - num_imag * Zo_imag;
    float temp_imag = num_real * Zo_imag + num_imag * Zo_real;
    
    /* 复数除法: temp / den */
    float denom = den_real * den_real + den_imag * den_imag;
    
    if(denom > 1e-10f)
    {
        *calReal = (temp_real * den_real + temp_imag * den_imag) / denom;
        *calImag = (temp_imag * den_real - temp_real * den_imag) / denom;
    }
    else
    {
        *calReal = rawReal;
        *calImag = rawImag;
    }
    
    /* 应用温度补偿 */
    float currentTemp = Temperature_Read();
    float tempFactor = AdvCal_GetTempFactor(currentTemp);
    
    *calReal *= tempFactor;
    *calImag *= tempFactor;
}

/**
  * @brief  获取温度补偿系数
  */
float AdvCal_GetTempFactor(float temperature)
{
    uint8_t i;
    float factor = 1.0f;
    
    /* 线性插值 */
    for(i = 0; i < CAL_TEMP_POINTS - 1; i++)
    {
        if(temperature >= g_AdvCalData.tempPoints[i] && 
           temperature <= g_AdvCalData.tempPoints[i+1])
        {
            float ratio = (temperature - g_AdvCalData.tempPoints[i]) /
                         (g_AdvCalData.tempPoints[i+1] - g_AdvCalData.tempPoints[i]);
            factor = g_AdvCalData.tempCoeff[i] + 
                    ratio * (g_AdvCalData.tempCoeff[i+1] - g_AdvCalData.tempCoeff[i]);
            break;
        }
    }
    
    return factor;
}

/**
  * @brief  执行温度校准
  * @param  temperature: 当前温度
  * @param  refValue: 标准电阻值
  * @param  measuredValue: 测量值
  */
void AdvCal_Temperature(float temperature, float refValue, float measuredValue)
{
    uint8_t i;
    
    /* 找到最近的温度点 */
    for(i = 0; i < CAL_TEMP_POINTS; i++)
    {
        if(fabsf(temperature - g_AdvCalData.tempPoints[i]) < 3.0f)
        {
            /* 计算补偿系数 */
            g_AdvCalData.tempCoeff[i] = refValue / measuredValue;
            break;
        }
    }
}

/**
  * @brief  保存校准数据
  */
void AdvCal_Save(void)
{
    g_AdvCalData.validFlag = 0xCAL1DATA;
    g_AdvCalData.calibrationTemp = Temperature_Read();
    g_AdvCalData.calibrationDate = RTC_GetCounter();  // 假设有RTC
    
    Flash_Write(FLASH_ADV_CAL_ADDR, (uint8_t*)&g_AdvCalData, sizeof(AdvancedCalData_t));
}

/**
  * @brief  完整校准流程
  */
void AdvCal_FullCalibration(void)
{
    uint8_t range, fIdx;
    float freq;
    char msg[32];
    
    LCD_Clear();
    LCD_Write_String(0, 0, "Full Calibration");
    
    /* 对每个量程和频率点进行校准 */
    for(range = 0; range < CAL_RANGE_POINTS; range++)
    {
        /* 开路校准 */
        sprintf(msg, "OPEN R%d...", range);
        LCD_Write_String(0, 1, msg);
        LCD_Write_String(0, 1, "Keep OPEN, KEY1");
        while(KEY_Scan(0) != KEY1_PRESS);
        
        for(fIdx = 0; fIdx < CAL_FREQ_POINTS; fIdx++)
        {
            freq = g_AdvCalData.freqPoints[fIdx];
            AdvCal_Open(freq, range);
            
            sprintf(msg, "OPEN %dHz", (int)freq);
            LCD_Write_String(0, 1, msg);
        }
        
        /* 短路校准 */
        LCD_Write_String(0, 1, "SHORT, KEY1");
        while(KEY_Scan(0) != KEY1_PRESS);
        
        for(fIdx = 0; fIdx < CAL_FREQ_POINTS; fIdx++)
        {
            freq = g_AdvCalData.freqPoints[fIdx];
            AdvCal_Short(freq, range);
        }
        
        /* 负载校准 */
        float refValues[] = {10.0f, 100.0f, 1000.0f, 10000.0f, 100000.0f, 1000000.0f};
        sprintf(msg, "LOAD %.0fR, KEY1", refValues[range]);
        LCD_Write_String(0, 1, msg);
        while(KEY_Scan(0) != KEY1_PRESS);
        
        for(fIdx = 0; fIdx < CAL_FREQ_POINTS; fIdx++)
        {
            freq = g_AdvCalData.freqPoints[fIdx];
            AdvCal_Load(freq, range, refValues[range]);
        }
    }
    
    /* 保存 */
    AdvCal_Save();
    
    LCD_Clear();
    LCD_Write_String(0, 0, "Calibration");
    LCD_Write_String(0, 1, "Complete!");
    delay_ms(2000);
}
```

------

## 七、数字滤波与信号处理

### 1. 高级数字滤波器

```c
/**
  ******************************************************************************
  * @file    digital_filter.c
  * @brief   数字滤波器实现
  ******************************************************************************
  */

#include "digital_filter.h"
#include <math.h>
#include <string.h>

/*============================================================================
 * IIR滤波器 - 二阶节 (Biquad)
 *============================================================================*/

typedef struct {
    float b0, b1, b2;   // 分子系数
    float a1, a2;       // 分母系数 (a0=1)
    float z1, z2;       // 状态变量
} BiquadFilter_t;

/**
  * @brief  初始化低通Biquad滤波器
  * @param  filter: 滤波器结构体
  * @param  fc: 截止频率
  * @param  fs: 采样频率
  * @param  Q: 品质因数
  */
void Biquad_InitLowpass(BiquadFilter_t *filter, float fc, float fs, float Q)
{
    float omega = 2.0f * PI * fc / fs;
    float sinOmega = sinf(omega);
    float cosOmega = cosf(omega);
    float alpha = sinOmega / (2.0f * Q);
    
    float a0 = 1.0f + alpha;
    
    filter->b0 = (1.0f - cosOmega) / 2.0f / a0;
    filter->b1 = (1.0f - cosOmega) / a0;
    filter->b2 = (1.0f - cosOmega) / 2.0f / a0;
    filter->a1 = -2.0f * cosOmega / a0;
    filter->a2 = (1.0f - alpha) / a0;
    
    filter->z1 = 0;
    filter->z2 = 0;
}

/**
  * @brief  初始化带通Biquad滤波器
  */
void Biquad_InitBandpass(BiquadFilter_t *filter, float fc, float fs, float Q)
{
    float omega = 2.0f * PI * fc / fs;
    float sinOmega = sinf(omega);
    float cosOmega = cosf(omega);
    float alpha = sinOmega / (2.0f * Q);
    
    float a0 = 1.0f + alpha;
    
    filter->b0 = alpha / a0;
    filter->b1 = 0;
    filter->b2 = -alpha / a0;
    filter->a1 = -2.0f * cosOmega / a0;
    filter->a2 = (1.0f - alpha) / a0;
    
    filter->z1 = 0;
    filter->z2 = 0;
}

/**
  * @brief  初始化陷波滤波器 (Notch)
  */
void Biquad_InitNotch(BiquadFilter_t *filter, float fc, float fs, float Q)
{
    float omega = 2.0f * PI * fc / fs;
    float sinOmega = sinf(omega);
    float cosOmega = cosf(omega);
    float alpha = sinOmega / (2.0f * Q);
    
    float a0 = 1.0f + alpha;
    
    filter->b0 = 1.0f / a0;
    filter->b1 = -2.0f * cosOmega / a0;
    filter->b2 = 1.0f / a0;
    filter->a1 = -2.0f * cosOmega / a0;
    filter->a2 = (1.0f - alpha) / a0;
    
    filter->z1 = 0;
    filter->z2 = 0;
}

/**
  * @brief  Biquad滤波处理 (Direct Form II)
  */
float Biquad_Process(BiquadFilter_t *filter, float input)
{
    float output;
    float w;
    
    w = input - filter->a1 * filter->z1 - filter->a2 * filter->z2;
    output = filter->b0 * w + filter->b1 * filter->z1 + filter->b2 * filter->z2;
    
    filter->z2 = filter->z1;
    filter->z1 = w;
    
    return output;
}

/**
  * @brief  批量滤波处理
  */
void Biquad_ProcessBlock(BiquadFilter_t *filter, float *input, float *output, uint16_t size)
{
    for(uint16_t i = 0; i < size; i++)
    {
        output[i] = Biquad_Process(filter, input[i]);
    }
}

/*============================================================================
 * 中值滤波器
 *============================================================================*/

#define MEDIAN_FILTER_SIZE  9

typedef struct {
    float buffer[MEDIAN_FILTER_SIZE];
    uint8_t index;
    uint8_t filled;
} MedianFilter_t;

void MedianFilter_Init(MedianFilter_t *filter)
{
    memset(filter->buffer, 0, sizeof(filter->buffer));
    filter->index = 0;
    filter->filled = 0;
}

/**
  * @brief  快速选择算法找中值
  */
static float QuickSelect(float *arr, uint8_t n, uint8_t k)
{
    float temp[MEDIAN_FILTER_SIZE];
    memcpy(temp, arr, n * sizeof(float));
    
    /* 简单排序 (数据量小) */
    for(uint8_t i = 0; i < n - 1; i++)
    {
        for(uint8_t j = i + 1; j < n; j++)
        {
            if(temp[i] > temp[j])
            {
                float t = temp[i];
                temp[i] = temp[j];
                temp[j] = t;
            }
        }
    }
    
    return temp[k];
}

float MedianFilter_Process(MedianFilter_t *filter, float input)
{
    filter->buffer[filter->index] = input;
    filter->index = (filter->index + 1) % MEDIAN_FILTER_SIZE;
    
    if(filter->filled < MEDIAN_FILTER_SIZE)
        filter->filled++;
    
    return QuickSelect(filter->buffer, filter->filled, filter->filled / 2);
}

/*============================================================================
 * 卡尔曼滤波器 (一维)
 *============================================================================*/

typedef struct {
    float Q;    // 过程噪声协方差
    float R;    // 测量噪声协方差
    float x;    // 估计值
    float P;    // 估计误差协方差
    float K;    // 卡尔曼增益
} KalmanFilter_t;

void Kalman_Init(KalmanFilter_t *kf, float Q, float R, float initialValue)
{
    kf->Q = Q;
    kf->R = R;
    kf->x = initialValue;
    kf->P = 1.0f;
}

float Kalman_Update(KalmanFilter_t *kf, float measurement)
{
    /* 预测 */
    // x_pred = x (假设状态不变)
    // P_pred = P + Q
    kf->P = kf->P + kf->Q;
    
    /* 更新 */
    // K = P_pred / (P_pred + R)
    kf->K = kf->P / (kf->P + kf->R);
    
    // x = x_pred + K * (measurement - x_pred)
    kf->x = kf->x + kf->K * (measurement - kf->x);
    
    // P = (1 - K) * P_pred
    kf->P = (1.0f - kf->K) * kf->P;
    
    return kf->x;
}

/*============================================================================
 * 自适应滤波器
 *============================================================================*/

typedef struct {
    float alpha;        // 当前平滑系数
    float alphaMin;     // 最小平滑系数
    float alphaMax;     // 最大平滑系数
    float threshold;    // 变化阈值
    float lastValue;    // 上次值
    float filteredValue;// 滤波后的值
} AdaptiveFilter_t;

void AdaptiveFilter_Init(AdaptiveFilter_t *filter, float alphaMin, float alphaMax, float threshold)
{
    filter->alphaMin = alphaMin;
    filter->alphaMax = alphaMax;
    filter->threshold = threshold;
    filter->alpha = alphaMax;
    filter->lastValue = 0;
    filter->filteredValue = 0;
}

float AdaptiveFilter_Process(AdaptiveFilter_t *filter, float input)
{
    float change = fabsf(input - filter->lastValue);
    
    /* 根据变化量调整平滑系数 */
    if(change > filter->threshold)
    {
        /* 变化大,快速响应 */
        filter->alpha = filter->alphaMax;
    }
    else
    {
        /* 变化小,强滤波 */
        filter->alpha = filter->alphaMin;
    }
    
    /* 指数平滑 */
    filter->filteredValue = filter->alpha * input + (1.0f - filter->alpha) * filter->filteredValue;
    filter->lastValue = input;
    
    return filter->filteredValue;
}

/*============================================================================
 * 50Hz/60Hz工频陷波器
 *============================================================================*/

static BiquadFilter_t g_NotchFilter50Hz;
static BiquadFilter_t g_NotchFilter60Hz;

void PowerLineFilter_Init(float sampleRate)
{
    /* 50Hz陷波器, Q=30 */
    Biquad_InitNotch(&g_NotchFilter50Hz, 50.0f, sampleRate, 30.0f);
    
    /* 60Hz陷波器, Q=30 */
    Biquad_InitNotch(&g_NotchFilter60Hz, 60.0f, sampleRate, 30.0f);
}

float PowerLineFilter_Process(float input)
{
    float output;
    
    output = Biquad_Process(&g_NotchFilter50Hz, input);
    output = Biquad_Process(&g_NotchFilter60Hz, output);
    
    return output;
}
```

------

## 八、PCB设计详细指南

### 1. 分层与布局

```text
┌─────────────────────────────────────────────────────────────────────────────┐
│                           四层PCB堆叠结构                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ 顶层 (Top Layer) - 信号层                                           │   │
│  │ - 模拟信号走线                                                      │   │
│  │ - 高频信号走线                                                      │   │
│  │ - 元件放置                                                          │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                              │ 0.2mm                                        │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ 内层1 (Inner Layer 1) - 地平面                                      │   │
│  │ - 完整的接地平面                                                    │   │
│  │ - 模拟地和数字地分区                                                │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                              │ 1.0mm (核心)                                 │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ 内层2 (Inner Layer 2) - 电源平面                                    │   │
│  │ - 3.3V数字电源区                                                    │   │
│  │ - 5V模拟电源区                                                      │   │
│  │ - 分割清晰                                                          │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                              │ 0.2mm                                        │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ 底层 (Bottom Layer) - 信号层                                        │   │
│  │ - 数字信号走线                                                      │   │
│  │ - 电源走线                                                          │   │
│  │ - 元件放置                                                          │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 2. 布局分区

```text
┌─────────────────────────────────────────────────────────────────────────────┐
│                              PCB布局分区图                                   │
│                           (100mm x 80mm 参考)                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌──────────────────┬──────────────────┬──────────────────┐                │
│  │                  │                  │                  │                │
│  │   电源区域       │   数字区域       │   接口区域       │                │
│  │                  │                  │                  │                │
│  │  ┌────────────┐  │  ┌────────────┐  │  ┌────────────┐  │                │
│  │  │ DC-DC      │  │  │ STM32      │  │  │ USB/UART   │  │                │
│  │  │ LDO        │  │  │ MCU        │  │  │ 接口       │  │                │
│  │  │ 滤波电容   │  │  │            │  │  │            │  │                │
│  │  └────────────┘  │  └────────────┘  │  └────────────┘  │                │
│  │                  │                  │                  │                │
│  │  ┌────────────┐  │  ┌────────────┐  │  ┌────────────┐  │                │
│  │  │ 电源指示   │  │  │ 晶振       │  │  │ 按键       │  │                │
│  │  │ LED        │  │  │ 复位电路   │  │  │ LED        │  │                │
│  │  └────────────┘  │  └────────────┘  │  └────────────┘  │                │
│  │                  │                  │                  │                │
│  ├──────────────────┴──────────────────┴──────────────────┤                │
│  │                                                        │                │
│  │                      模拟区域                          │                │
│  │                                                        │                │
│  │  ┌────────────┐  ┌────────────┐  ┌────────────┐       │                │
│  │  │ DAC输出    │  │ 信号调理   │  │ ADC输入    │       │                │
│  │  │ 滤波电路   │  │ 运放电路   │  │ 滤波电路   │       │                │
│  │  └────────────┘  └────────────┘  └────────────┘       │                │
│  │                                                        │                │
│  │  ┌────────────┐  ┌────────────┐  ┌────────────┐       │                │
│  │  │ 量程切换   │  │ 测量接口   │  │ 屏蔽/保护  │       │                │
│  │  │ 继电器/开关│  │ BNC/香蕉头 │  │ 电路       │       │                │
│  │  └────────────┘  └────────────┘  └────────────┘       │                │
│  │                                                        │                │
│  └────────────────────────────────────────────────────────┘                │
│                                                                             │
│  ════════════════════════════════════════════════════════                  │
│                         模拟地与数字地单点连接                              │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 3. 关键布线规则

```c
/**
  ******************************************************************************
  * PCB设计规则检查清单
  ******************************************************************************
  */

/*
┌─────────────────────────────────────────────────────────────────────────────┐
│                           布线规则                                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│ 1. 模拟信号走线:                                                            │
│    □ 线宽: ≥0.3mm (12mil)                                                  │
│    □ 间距: ≥0.3mm                                                          │
│    □ 避免锐角, 使用45°或圆弧                                               │
│    □ 远离数字信号线 (≥2mm)                                                 │
│    □ 使用地线护送 (Guard Ring)                                             │
│                                                                             │
│ 2. 电源走线:                                                                │
│    □ 3.3V数字电源: ≥0.5mm                                                  │
│    □ 5V模拟电源: ≥0.5mm                                                    │
│    □ 大电流路径: ≥1.0mm                                                    │
│    □ 星型拓扑, 避免菊花链                                                  │
│                                                                             │
│ 3. 接地设计:                                                                │
│    □ 模拟地和数字地分开                                                    │
│    □ 单点连接 (在ADC参考地附近)                                            │
│    □ 地平面完整, 避免分割                                                  │
│    □ 过孔缝合 (每10mm一个接地过孔)                                         │
│                                                                             │
│ 4. 去耦电容:                                                                │
│    □ 每个IC电源引脚: 100nF                                                 │
│    □ 电源入口: 10uF + 100nF + 10nF                                         │
│    □ ADC参考电压: 10uF + 1uF + 100nF                                       │
│    □ 放置尽量靠近引脚                                                      │
│                                                                             │
│ 5. 高频信号:                                                                │
│    □ 晶振走线: ≤10mm, 包地                                                 │
│    □ DAC输出: 控制阻抗50Ω                                                  │
│    □ ADC输入: 差分走线, 等长                                               │
│                                                                             │
│ 6. EMC考虑:                                                                 │
│    □ 边缘不走高频信号                                                      │
│    □ 接口处加TVS保护                                                       │
│    □ 测量端口加共模电感                                                    │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
*/
```

### 4. 测量端口设计

```text
【四线测量接口 (Kelvin连接)】

                    ┌─────────────────────────────────────┐
                    │           测量端口设计              │
                    └─────────────────────────────────────┘

    ┌─────────────────────────────────────────────────────────────┐
    │                                                             │
    │      Hc (电流高端)    Hp (电压高端)                         │
    │         ○               ○                                   │
    │         │               │                                   │
    │         │    ┌─────┐    │                                   │
    │         └────┤ Zx  ├────┘                                   │
    │              │被测 │                                        │
    │         ┌────┤元件 ├────┐                                   │
    │         │    └─────┘    │                                   │
    │         │               │                                   │
    │         ○               ○                                   │
    │      Lc (电流低端)    Lp (电压低端)                         │
    │                                                             │
    └─────────────────────────────────────────────────────────────┘

    优点:
    - 消除引线电阻影响
    - 提高小电阻测量精度
    - 适合低阻抗测量 (<10Ω)


【BNC接口电路】

                        ┌─────────────────┐
                        │    BNC接口      │
                        │                 │
    测量信号 ───────────┤ 中心导体        │
                        │                 │
                   ┌────┤ 外壳 (屏蔽)     │
                   │    │                 │
                   │    └─────────────────┘
                   │
                  ┌┴┐
                  │ │ 0Ω (或磁珠)
                  └┬┘
                   │
                  GND (模拟地)


【ESD保护电路】

                    VCC
                     │
                ┌────┴────┐
                │  TVS    │
                │ (双向)  │
    测量输入 ───┼─────────┼─────► 内部电路
                │         │
                │  TVS    │
                └────┬────┘
                     │
                    GND

    推荐器件: PESD5V0S2BT (5V双向TVS)
```

------

## 九、温度传感器与补偿

```c
/**
  ******************************************************************************
  * @file    temperature_sensor.c
  * @brief   温度传感器驱动 (使用STM32内部温度传感器或外部NTC)
  ******************************************************************************
  */

#include "temperature_sensor.h"
#include "stm32f10x.h"

/* 使用内部温度传感器 */
#define USE_INTERNAL_TEMP_SENSOR    1

/* NTC参数 (如果使用外部NTC) */
#define NTC_R25         10000.0f    // 25°C时的电阻值
#define NTC_B           3950.0f     // B值
#define NTC_T25         298.15f     // 25°C (开尔文)

#if USE_INTERNAL_TEMP_SENSOR

/**
  * @brief  初始化内部温度传感器
  */
void Temperature_Init(void)
{
    ADC_InitTypeDef ADC_InitStructure;
    
    /* 使能ADC1时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    
    /* 使能内部温度传感器 */
    ADC_TempSensorVrefintCmd(ENABLE);
    
    /* ADC配置 */
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure);
    
    /* 配置温度传感器通道 (CH16) */
    ADC_RegularChannelConfig(ADC1, ADC_Channel_16, 1, ADC_SampleTime_239Cycles5);
    
    ADC_Cmd(ADC1, ENABLE);
    
    /* 校准 */
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
}

/**
  * @brief  读取温度
  * @retval 温度值 (°C)
  */
float Temperature_Read(void)
{
    uint16_t adcValue;
    float voltage;
    float temperature;
    
    /* 启动转换 */
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
    
    adcValue = ADC_GetConversionValue(ADC1);
    
    /* 转换为电压 */
    voltage = (float)adcValue * 3.3f / 4096.0f;
    
    /* 转换为温度 */
    // STM32F103: V25 = 1.43V, Avg_Slope = 4.3mV/°C
    temperature = (1.43f - voltage) / 0.0043f + 25.0f;
    
    return temperature;
}

#else

/**
  * @brief  初始化外部NTC温度传感器
  */
void Temperature_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);
    
    /* 配置NTC输入引脚 (PA3) */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    /* ADC配置 */
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure);
    
    ADC_RegularChannelConfig(ADC1, ADC_Channel_3, 1, ADC_SampleTime_55Cycles5);
    
    ADC_Cmd(ADC1, ENABLE);
    
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
}

/**
  * @brief  读取NTC温度
  * @retval 温度值 (°C)
  * @note   电路: VCC -- 10K -- ADC -- NTC -- GND
  */
float Temperature_Read(void)
{
    uint16_t adcValue;
    float voltage;
    float ntcResistance;
    float temperature;
    
    /* 启动转换 */
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
    
    adcValue = ADC_GetConversionValue(ADC1);
    
    /* 转换为电压 */
    voltage = (float)adcValue * 3.3f / 4096.0f;
    
    /* 计算NTC电阻值 */
    // Vout = VCC * NTC / (R + NTC)
    // NTC = R * Vout / (VCC - Vout)
    ntcResistance = 10000.0f * voltage / (3.3f - voltage);
    
    /* 使用B值公式计算温度 */
    // 1/T = 1/T25 + (1/B) * ln(R/R25)
    // T = 1 / (1/T25 + (1/B) * ln(R/R25))
    temperature = 1.0f / (1.0f / NTC_T25 + (1.0f / NTC_B) * logf(ntcResistance / NTC_R25));
    temperature -= 273.15f;  // 转换为摄氏度
    
    return temperature;
}

#endif

/**
  * @brief  获取温度补偿系数
  * @param  currentTemp: 当前温度
  * @param  calTemp: 校准温度
  * @retval 补偿系数
  */
float Temperature_GetCompensation(float currentTemp, float calTemp)
{
    /* 典型温度系数: 50ppm/°C */
    float tempCoeff = 50e-6f;
    float deltaT = currentTemp - calTemp;
    
    return 1.0f + tempCoeff * deltaT;
}
```

------

## 十、完整测量流程

```c
/**
  ******************************************************************************
  * @file    measurement_process.c
  * @brief   完整测量流程实现
  ******************************************************************************
  */

#include "measurement_process.h"
#include "dac_signal.h"
#include "adc_sample.h"
#include "auto_range.h"
#include "calibration_advanced.h"
#include "digital_filter.h"
#include "impedance_calc.h"
#include "temperature_sensor.h"

/* 测量配置 */
typedef struct {
    float frequency;        // 测量频率
    uint8_t averageCount;   // 平均次数
    uint8_t autoRange;      // 自动量程
    uint8_t autoIdentify;   // 自动识别
    WindowType_t window;    // 窗函数类型
} MeasureConfig_t;

/* 默认配置 */
static MeasureConfig_t g_Config = {
    .frequency = 1000.0f,
    .averageCount = 4,
    .autoRange = 1,
    .autoIdentify = 1,
    .window = WINDOW_HANN
};

/* 滤波器实例 */
static KalmanFilter_t g_KalmanZ;
static KalmanFilter_
```





```
继续
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

已完成生成

继续完成代码：

```c
/* 滤波器实例 */
static KalmanFilter_t g_KalmanZ;
static KalmanFilter_t g_KalmanPhase;
static MedianFilter_t g_MedianFilter;
static AdaptiveFilter_t g_AdaptiveFilter;

/* 测量状态 */
static uint8_t g_MeasureState = 0;
static uint8_t g_StableCount = 0;

/**
  * @brief  初始化测量系统
  */
void Measurement_Init(void)
{
    /* 初始化各模块 */
    DAC_Signal_Init();
    ADC_DualSync_Init();
    AutoRange_FastSearch(0);
    AdvCal_Init();
    Temperature_Init();
    PGA_Init();
    
    /* 初始化滤波器 */
    Kalman_Init(&g_KalmanZ, 0.01f, 0.1f, 1000.0f);
    Kalman_Init(&g_KalmanPhase, 0.001f, 0.05f, 0.0f);
    MedianFilter_Init(&g_MedianFilter);
    AdaptiveFilter_Init(&g_AdaptiveFilter, 0.05f, 0.5f, 10.0f);
    
    /* 初始化窗函数 */
    Window_Init(256);
    
    /* 初始化DFT */
    DFT_Init();
    
    /* 初始化工频陷波器 */
    PowerLineFilter_Init(256000.0f);
    
    /* 设置默认频率 */
    DAC_SetFrequency((uint32_t)g_Config.frequency);
}

/**
  * @brief  执行单次原始测量
  * @param  result: 测量结果
  */
static void Measurement_SingleRaw(ImpedanceFullResult_t *result)
{
    uint16_t voltageData[256], currentData[256];
    float voltageFloat[256], currentFloat[256];
    float vReal, vImag, iReal, iImag;
    float vMag, iMag, vPhase, iPhase;
    float phaseDiff;
    float Rref;
    uint16_t i;
    
    /* 启动ADC采样 */
    ADC_DualSync_Start();
    
    /* 等待数据就绪 */
    while(!ADC_IsDataReady())
    {
        delay_us(10);
    }
    
    ADC_DualSync_Stop();
    
    /* 获取采样数据 */
    ADC_GetSampleData(voltageData, currentData, 256);
    
    /* 转换为浮点并去除直流 */
    float vSum = 0, iSum = 0;
    for(i = 0; i < 256; i++)
    {
        vSum += voltageData[i];
        iSum += currentData[i];
    }
    float vDC = vSum / 256.0f;
    float iDC = iSum / 256.0f;
    
    for(i = 0; i < 256; i++)
    {
        voltageFloat[i] = (float)voltageData[i] - vDC;
        currentFloat[i] = (float)currentData[i] - iDC;
        
        /* 应用工频陷波 */
        voltageFloat[i] = PowerLineFilter_Process(voltageFloat[i]);
        currentFloat[i] = PowerLineFilter_Process(currentFloat[i]);
    }
    
    /* 应用窗函数 */
    Window_Apply(voltageFloat, 256, g_Config.window);
    Window_Apply(currentFloat, 256, g_Config.window);
    
    /* DFT计算基波分量 */
    float k = g_Config.frequency * 256.0f / 256000.0f;  // 目标bin
    
    Goertzel_Standard(voltageFloat, 256, k, &vMag, &vPhase);
    Goertzel_Standard(currentFloat, 256, k, &iMag, &iPhase);
    
    /* 窗函数幅值校正 */
    float coherentGain = Window_GetCoherentGain(g_Config.window);
    vMag /= coherentGain;
    iMag /= coherentGain;
    
    /* 计算相位差 */
    phaseDiff = vPhase - iPhase;
    while(phaseDiff > PI) phaseDiff -= 2.0f * PI;
    while(phaseDiff < -PI) phaseDiff += 2.0f * PI;
    
    /* 应用相位校正 */
    phaseDiff = Phase_ApplyCorrection(g_Config.frequency, phaseDiff);
    
    /* 获取参考电阻 */
    Rref = Range_GetRefResistance();
    
    /* 计算阻抗 */
    Impedance_Calculate(vMag, iMag, phaseDiff, g_Config.frequency, Rref, result);
}

/**
  * @brief  执行完整测量 (带平均和滤波)
  * @param  result: 测量结果
  * @retval 测量状态: 0=进行中, 1=稳定, 2=超量程
  */
uint8_t Measurement_Execute(ImpedanceFullResult_t *result)
{
    ImpedanceFullResult_t rawResult;
    float sumZ = 0, sumPhase = 0, sumR = 0, sumX = 0;
    float rawReal, rawImag, calReal, calImag;
    uint8_t i;
    uint8_t rangeChanged = 0;
    
    /* 多次测量取平均 */
    for(i = 0; i < g_Config.averageCount; i++)
    {
        Measurement_SingleRaw(&rawResult);
        
        /* 应用校准补偿 */
        rawReal = rawResult.R;
        rawImag = rawResult.X;
        AdvCal_Apply(rawReal, rawImag, g_Config.frequency, Range_Get(), &calReal, &calImag);
        
        sumR += calReal;
        sumX += calImag;
        sumZ += sqrtf(calReal * calReal + calImag * calImag);
        sumPhase += atan2f(calImag, calReal);
        
        delay_ms(10);
    }
    
    /* 计算平均值 */
    result->R = sumR / g_Config.averageCount;
    result->X = sumX / g_Config.averageCount;
    result->Z = sumZ / g_Config.averageCount;
    result->theta = sumPhase / g_Config.averageCount;
    
    /* 应用卡尔曼滤波 */
    result->Z = Kalman_Update(&g_KalmanZ, result->Z);
    result->theta = Kalman_Update(&g_KalmanPhase, result->theta);
    
    /* 重新计算R和X */
    result->R = result->Z * cosf(result->theta);
    result->X = result->Z * sinf(result->theta);
    
    /* 应用中值滤波 (去除异常值) */
    result->Z = MedianFilter_Process(&g_MedianFilter, result->Z);
    
    /* 应用自适应滤波 */
    result->Z = AdaptiveFilter_Process(&g_AdaptiveFilter, result->Z);
    
    /* 计算其他参数 */
    float omega = 2.0f * PI * g_Config.frequency;
    
    /* 串联等效 */
    result->Rs = result->R;
    if(result->X > 0)
    {
        result->Ls = result->X / omega;
        result->Cs = 0;
    }
    else
    {
        result->Ls = 0;
        result->Cs = (result->X != 0) ? (-1.0f / (omega * result->X)) : 0;
    }
    
    /* 并联等效 */
    float Zsq = result->R * result->R + result->X * result->X;
    result->Rp = (fabsf(result->R) > 0.001f) ? (Zsq / result->R) : 1e9f;
    if(result->X != 0)
    {
        float Xp = Zsq / result->X;
        if(Xp > 0)
        {
            result->Lp = Xp / omega;
            result->Cp = 0;
        }
        else
        {
            result->Lp = 0;
            result->Cp = -1.0f / (omega * Xp);
        }
    }
    
    /* Q和D */
    result->Q = (fabsf(result->R) > 0.001f) ? fabsf(result->X / result->R) : 9999.0f;
    result->D = (fabsf(result->X) > 0.001f) ? fabsf(result->R / result->X) : 9999.0f;
    result->ESR = result->R;
    
    /* 元件类型识别 */
    if(g_Config.autoIdentify)
    {
        float phaseDeg = result->theta * 180.0f / PI;
        if(phaseDeg > -10.0f && phaseDeg < 10.0f)
            result->componentType = 1;  // 电阻
        else if(phaseDeg < -10.0f)
            result->componentType = 2;  // 电容
        else
            result->componentType = 3;  // 电感
    }
    
    result->frequency = g_Config.frequency;
    
    /* 自动量程调整 */
    if(g_Config.autoRange)
    {
        uint16_t vData[256], iData[256];
        ADC_GetSampleData(vData, iData, 256);
        rangeChanged = AutoRange_Adjust(result->Z, vData, iData, 256);
    }
    
    /* 判断测量稳定性 */
    static float lastZ = 0;
    float changeRate = fabsf(result->Z - lastZ) / (lastZ + 0.001f);
    lastZ = result->Z;
    
    if(changeRate < 0.01f && !rangeChanged)
    {
        g_StableCount++;
        if(g_StableCount > 5)
            return 1;  // 稳定
    }
    else
    {
        g_StableCount = 0;
    }
    
    /* 检查超量程 */
    float minZ, maxZ, refRes;
    uint8_t range;
    AutoRange_GetInfo(&range, &refRes, &minZ, &maxZ);
    
    if(result->Z < minZ * 0.1f || result->Z > maxZ * 10.0f)
    {
        return 2;  // 超量程
    }
    
    return 0;  // 进行中
}

/**
  * @brief  设置测量频率
  */
void Measurement_SetFrequency(float freq)
{
    g_Config.frequency = freq;
    DAC_SetFrequency((uint32_t)freq);
    
    /* 重置滤波器 */
    Kalman_Init(&g_KalmanZ, 0.01f, 0.1f, 1000.0f);
    Kalman_Init(&g_KalmanPhase, 0.001f, 0.05f, 0.0f);
    g_StableCount = 0;
}

/**
  * @brief  设置平均次数
  */
void Measurement_SetAveraging(uint8_t count)
{
    if(count < 1) count = 1;
    if(count > 32) count = 32;
    g_Config.averageCount = count;
}

/**
  * @brief  设置自动量程
  */
void Measurement_SetAutoRange(uint8_t enable)
{
    g_Config.autoRange = enable;
}

/**
  * @brief  多频点扫描测量
  * @param  freqList: 频率列表
  * @param  numFreq: 频率点数
  * @param  results: 结果数组
  */
void Measurement_FrequencySweep(float *freqList, uint8_t numFreq, ImpedanceFullResult_t *results)
{
    uint8_t i;
    
    for(i = 0; i < numFreq; i++)
    {
        Measurement_SetFrequency(freqList[i]);
        delay_ms(100);  // 等待稳定
        
        /* 多次测量直到稳定 */
        uint8_t status;
        uint8_t retry = 0;
        do
        {
            status = Measurement_Execute(&results[i]);
            retry++;
        } while(status == 0 && retry < 10);
    }
}

/**
  * @brief  获取测量配置
  */
MeasureConfig_t* Measurement_GetConfig(void)
{
    return &g_Config;
}
```

------

## 十一、用户界面与显示

```c
/**
  ******************************************************************************
  * @file    user_interface.c
  * @brief   用户界面实现 (LCD显示 + 按键菜单)
  ******************************************************************************
  */

#include "user_interface.h"
#include "lcd1602.h"
#include "key.h"
#include "measurement_process.h"
#include "calibration_advanced.h"
#include <stdio.h>
#include <string.h>

/* 界面状态 */
typedef enum {
    UI_STATE_MEASURE = 0,   // 测量界面
    UI_STATE_MENU,          // 主菜单
    UI_STATE_FREQ_SET,      // 频率设置
    UI_STATE_MODE_SET,      // 模式设置
    UI_STATE_CAL_MENU,      // 校准菜单
    UI_STATE_INFO           // 信息显示
} UI_State_t;

static UI_State_t g_UIState = UI_STATE_MEASURE;
static uint8_t g_MenuIndex = 0;
static uint8_t g_DisplayMode = 0;  // 0=主参数, 1=串联等效, 2=并联等效

/* 菜单项 */
static const char* MainMenuItems[] = {
    "1.Frequency",
    "2.Mode",
    "3.Calibration",
    "4.Info",
    "5.Exit"
};
#define MENU_ITEM_COUNT  5

/* 频率选项 */
static const float FreqOptions[] = {100, 120, 1000, 10000, 100000};
static const char* FreqLabels[] = {"100Hz", "120Hz", "1kHz", "10kHz", "100kHz"};
#define FREQ_OPTION_COUNT  5
static uint8_t g_FreqIndex = 2;  // 默认1kHz

/**
  * @brief  格式化阻抗值显示
  */
static void Format_Impedance(float value, char *buf, const char *unit)
{
    if(value < 1e-9f)
        sprintf(buf, "0.000 %s", unit);
    else if(value < 1e-6f)
        sprintf(buf, "%.3fn%s", value * 1e9f, unit);
    else if(value < 1e-3f)
        sprintf(buf, "%.3fu%s", value * 1e6f, unit);
    else if(value < 1.0f)
        sprintf(buf, "%.3fm%s", value * 1e3f, unit);
    else if(value < 1e3f)
        sprintf(buf, "%.4f%s", value, unit);
    else if(value < 1e6f)
        sprintf(buf, "%.3fK%s", value / 1e3f, unit);
    else
        sprintf(buf, "%.3fM%s", value / 1e6f, unit);
}

/**
  * @brief  格式化电容值显示
  */
static void Format_Capacitance(float value, char *buf)
{
    if(value <= 0)
        sprintf(buf, "----");
    else if(value < 1e-12f)
        sprintf(buf, "%.2ffF", value * 1e15f);
    else if(value < 1e-9f)
        sprintf(buf, "%.3fpF", value * 1e12f);
    else if(value < 1e-6f)
        sprintf(buf, "%.3fnF", value * 1e9f);
    else if(value < 1e-3f)
        sprintf(buf, "%.3fuF", value * 1e6f);
    else
        sprintf(buf, "%.3fmF", value * 1e3f);
}

/**
  * @brief  格式化电感值显示
  */
static void Format_Inductance(float value, char *buf)
{
    if(value <= 0)
        sprintf(buf, "----");
    else if(value < 1e-9f)
        sprintf(buf, "%.2fnH", value * 1e9f);
    else if(value < 1e-6f)
        sprintf(buf, "%.3fuH", value * 1e6f);
    else if(value < 1e-3f)
        sprintf(buf, "%.3fmH", value * 1e3f);
    else
        sprintf(buf, "%.4fH", value);
}

/**
  * @brief  显示测量结果
  */
static void UI_DisplayMeasurement(ImpedanceFullResult_t *result)
{
    char line1[17], line2[17];
    char valueBuf[12];
    
    switch(g_DisplayMode)
    {
        case 0:  // 主参数显示
            switch(result->componentType)
            {
                case 1:  // 电阻
                    Format_Impedance(result->Z, valueBuf, "");
                    sprintf(line1, "R=%s Ohm", valueBuf);
                    sprintf(line2, "Q=%.3f D=%.4f", result->Q, result->D);
                    break;
                    
                case 2:  // 电容
                    Format_Capacitance(result->Cs, valueBuf);
                    sprintf(line1, "C=%s", valueBuf);
                    sprintf(line2, "D=%.4f ESR=%.2f", result->D, result->ESR);
                    break;
                    
                case 3:  // 电感
                    Format_Inductance(result->Ls, valueBuf);
                    sprintf(line1, "L=%s", valueBuf);
                    sprintf(line2, "Q=%.2f Rs=%.3f", result->Q, result->Rs);
                    break;
            }
            break;
            
        case 1:  // 串联等效
            Format_Impedance(result->Rs, valueBuf, "");
            sprintf(line1, "Rs=%sOhm", valueBuf);
            if(result->Ls > 0)
            {
                Format_Inductance(result->Ls, valueBuf);
                sprintf(line2, "Ls=%s", valueBuf);
            }
            else
            {
                Format_Capacitance(result->Cs, valueBuf);
                sprintf(line2, "Cs=%s", valueBuf);
            }
            break;
            
        case 2:  // 并联等效
            Format_Impedance(result->Rp, valueBuf, "");
            sprintf(line1, "Rp=%sOhm", valueBuf);
            if(result->Lp > 0)
            {
                Format_Inductance(result->Lp, valueBuf);
                sprintf(line2, "Lp=%s", valueBuf);
            }
            else
            {
                Format_Capacitance(result->Cp, valueBuf);
                sprintf(line2, "Cp=%s", valueBuf);
            }
            break;
    }
    
    LCD_Write_String(0, 0, line1);
    LCD_Write_String(0, 1, line2);
}

/**
  * @brief  显示状态栏
  */
static void UI_DisplayStatus(uint8_t measureStatus)
{
    char statusChar;
    
    switch(measureStatus)
    {
        case 0: statusChar = '*'; break;  // 测量中
        case 1: statusChar = ' '; break;  // 稳定
        case 2: statusChar = '!'; break;  // 超量程
        default: statusChar = '?'; break;
    }
    
    /* 在LCD右上角显示状态 */
    LCD_Set_Cursor(15, 0);
    LCD_Write_Data(statusChar);
}

/**
  * @brief  显示主菜单
  */
static void UI_DisplayMenu(void)
{
    LCD_Clear();
    LCD_Write_String(0, 0, "=== MENU ===");
    LCD_Write_String(0, 1, MainMenuItems[g_MenuIndex]);
}

/**
  * @brief  显示频率设置
  */
static void UI_DisplayFreqSet(void)
{
    char buf[17];
    LCD_Clear();
    LCD_Write_String(0, 0, "Set Frequency:");
    sprintf(buf, "> %s", FreqLabels[g_FreqIndex]);
    LCD_Write_String(0, 1, buf);
}

/**
  * @brief  显示校准菜单
  */
static void UI_DisplayCalMenu(void)
{
    static const char* calItems[] = {
        "1.Open Cal",
        "2.Short Cal",
        "3.Load Cal",
        "4.Full Cal",
        "5.Back"
    };
    static uint8_t calIndex = 0;
    
    LCD_Clear();
    LCD_Write_String(0, 0, "=CALIBRATION=");
    LCD_Write_String(0, 1, calItems[calIndex]);
}

/**
  * @brief  显示系统信息
  */
static void UI_DisplayInfo(void)
{
    char buf[17];
    float temp = Temperature_Read();
    
    LCD_Clear();
    sprintf(buf, "Temp: %.1f C", temp);
    LCD_Write_String(0, 0, buf);
    
    uint8_t range;
    float refRes, minZ, maxZ;
    AutoRange_GetInfo(&range, &refRes, &minZ, &maxZ);
    sprintf(buf, "Range: %.0fOhm", refRes);
    LCD_Write_String(0, 1, buf);
}

/**
  * @brief  按键处理 - 测量界面
  */
static void UI_KeyHandler_Measure(uint8_t key)
{
    switch(key)
    {
        case KEY1_PRESS:  // 切换显示模式
            g_DisplayMode = (g_DisplayMode + 1) % 3;
            break;
            
        case KEY2_PRESS:  // 切换频率
            g_FreqIndex = (g_FreqIndex + 1) % FREQ_OPTION_COUNT;
            Measurement_SetFrequency(FreqOptions[g_FreqIndex]);
            break;
            
        case KEY3_PRESS:  // 进入菜单
            g_UIState = UI_STATE_MENU;
            g_MenuIndex = 0;
            UI_DisplayMenu();
            break;
            
        case KEY4_PRESS:  // 手动触发测量/保持
            // 可以添加保持功能
            break;
    }
}

/**
  * @brief  按键处理 - 主菜单
  */
static void UI_KeyHandler_Menu(uint8_t key)
{
    switch(key)
    {
        case KEY1_PRESS:  // 上
            if(g_MenuIndex > 0) g_MenuIndex--;
            UI_DisplayMenu();
            break;
            
        case KEY2_PRESS:  // 下
            if(g_MenuIndex < MENU_ITEM_COUNT - 1) g_MenuIndex++;
            UI_DisplayMenu();
            break;
            
        case KEY3_PRESS:  // 确认
            switch(g_MenuIndex)
            {
                case 0:  // Frequency
                    g_UIState = UI_STATE_FREQ_SET;
                    UI_DisplayFreqSet();
                    break;
                case 1:  // Mode
                    g_UIState = UI_STATE_MODE_SET;
                    break;
                case 2:  // Calibration
                    g_UIState = UI_STATE_CAL_MENU;
                    UI_DisplayCalMenu();
                    break;
                case 3:  // Info
                    g_UIState = UI_STATE_INFO;
                    UI_DisplayInfo();
                    break;
                case 4:  // Exit
                    g_UIState = UI_STATE_MEASURE;
                    break;
            }
            break;
            
        case KEY4_PRESS:  // 返回
            g_UIState = UI_STATE_MEASURE;
            break;
    }
}

/**
  * @brief  按键处理 - 频率设置
  */
static void UI_KeyHandler_FreqSet(uint8_t key)
{
    switch(key)
    {
        case KEY1_PRESS:  // 上
            if(g_FreqIndex > 0) g_FreqIndex--;
            UI_DisplayFreqSet();
            break;
            
        case KEY2_PRESS:  // 下
            if(g_FreqIndex < FREQ_OPTION_COUNT - 1) g_FreqIndex++;
            UI_DisplayFreqSet();
            break;
            
        case KEY3_PRESS:  // 确认
            Measurement_SetFrequency(FreqOptions[g_FreqIndex]);
            g_UIState = UI_STATE_MEASURE;
            break;
            
        case KEY4_PRESS:  // 取消
            g_UIState = UI_STATE_MENU;
            UI_DisplayMenu();
            break;
    }
}

/**
  * @brief  用户界面主循环
  */
void UI_Process(void)
{
    uint8_t key;
    static ImpedanceFullResult_t result;
    static uint8_t measureStatus = 0;
    static uint32_t lastUpdateTime = 0;
    
    /* 按键扫描 */
    key = KEY_Scan(0);
    
    /* 根据状态处理按键 */
    if(key)
    {
        switch(g_UIState)
        {
            case UI_STATE_MEASURE:
                UI_KeyHandler_Measure(key);
                break;
            case UI_STATE_MENU:
                UI_KeyHandler_Menu(key);
                break;
            case UI_STATE_FREQ_SET:
                UI_KeyHandler_FreqSet(key);
                break;
            case UI_STATE_CAL_MENU:
                // 校准菜单按键处理
                if(key == KEY4_PRESS)
                {
                    g_UIState = UI_STATE_MENU;
                    UI_DisplayMenu();
                }
                break;
            case UI_STATE_INFO:
                if(key == KEY4_PRESS)
                {
                    g_UIState = UI_STATE_MENU;
                    UI_DisplayMenu();
                }
                break;
            default:
                break;
        }
    }
    
    /* 测量界面更新 */
    if(g_UIState == UI_STATE_MEASURE)
    {
        /* 执行测量 */
        measureStatus = Measurement_Execute(&result);
        
        /* 更新显示 (限制刷新率) */
        uint32_t currentTime = SysTick_GetTick();
        if(currentTime - lastUpdateTime > 200)  // 200ms刷新一次
        {
            UI_DisplayMeasurement(&result);
            UI_DisplayStatus(measureStatus);
            lastUpdateTime = currentTime;
        }
    }
}

/**
  * @brief  用户界面初始化
  */
void UI_Init(void)
{
    LCD_Init();
    KEY_Init();
    
    /* 显示开机画面 */
    LCD_Clear();
    LCD_Write_String(0, 0, " LCR Meter V2.0");
    LCD_Write_String(0, 1, "  Initializing ");
    delay_ms(1000);
    
    /* 初始化测量系统 */
    Measurement_Init();
    
    LCD_Write_String(0, 1, "     Ready     ");
    delay_ms(500);
    
    g_UIState = UI_STATE_MEASURE;
}
```

------

## 十二、完整main.c

```c
/**
  ******************************************************************************
  * @file    main.c
  * @brief   高精密LCR测试仪 - 主程序
  * @version V2.0
  ******************************************************************************
  */

#include "stm32f10x.h"
#include "system_config.h"
#include "user_interface.h"
#include "measurement_process.h"
#include "calibration_advanced.h"
#include "serial_comm.h"

/**
  * @brief  系统时钟配置
  */
static void SystemClock_Config(void)
{
    /* 使用外部8MHz晶振,PLL倍频到72MHz */
    RCC_DeInit();
    RCC_HSEConfig(RCC_HSE_ON);
    
    if(RCC_WaitForHSEStartUp() == SUCCESS)
    {
        FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
        FLASH_SetLatency(FLASH_Latency_2);
        
        RCC_HCLKConfig(RCC_SYSCLK_Div1);
        RCC_PCLK2Config(RCC_HCLK_Div1);
        RCC_PCLK1Config(RCC_HCLK_Div2);
        
        RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);
        RCC_PLLCmd(ENABLE);
        
        while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);
        
        RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
        while(RCC_GetSYSCLKSource() != 0x08);
    }
}

/**
  * @brief  NVIC配置
  */
static void NVIC_Config(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
}

/**
  * @brief  主函数
  */
int main(void)
{
    /* 系统初始化 */
    SystemClock_Config();
    NVIC_Config();
    delay_init();
    
    /* 用户界面初始化 (包含测量系统初始化) */
    UI_Init();
    
    /* 串口通信初始化 (可选,用于PC连接) */
    Serial_Init(115200);
    
    /* 检查校准状态 */
    CalibrationData_t *calData = Calibration_GetData();
    if(calData->validFlag != 0x5A5A5A5A)
    {
        LCD_Clear();
        LCD_Write_String(0, 0, "No Calibration!");
        LCD_Write_String(0, 1, "Press KEY3...");
        
        /* 等待用户选择 */
        while(1)
        {
            uint8_t key = KEY_Scan(0);
            if(key == KEY3_PRESS)
            {
                AdvCal_FullCalibration();
                break;
            }
            else if(key == KEY1_PRESS)
            {
                /* 跳过校准 */
                break;
            }
        }
    }
    
    /* 主循环 */
    while(1)
    {
        /* 用户界面处理 */
        UI_Process();
        
        /* 串口命令处理 */
        Serial_Process();
    }
}

/**
  * @brief  硬件错误处理
  */
void HardFault_Handler(void)
{
    LCD_Clear();
    LCD_Write_String(0, 0, "System Error!");
    LCD_Write_String(0, 1, "Please Reset");
    
    while(1);
}

/**
  * @brief  SysTick中断处理
  */
static volatile uint32_t g_SysTick = 0;

void SysTick_Handler(void)
{
    g_SysTick++;
}

uint32_t SysTick_GetTick(void)
{
    return g_SysTick;
}
```

------

## 十三、性能优化总结

### 1. 精度提升措施

```text
┌─────────────────────────────────────────────────────────────────────────────┐
│                          精度提升措施总结                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  【硬件层面】                                                               │
│  ├─ 使用高精度参考电阻 (0.1%精度, 低温漂)                                  │
│  ├─ 仪表放大器提高共模抑制比 (INA128: CMRR>100dB)                          │
│  ├─ 四线测量消除引线电阻                                                   │
│  ├─ 屏蔽设计减少外部干扰                                                   │
│  ├─ 低噪声电源设计 (LDO + 多级滤波)                                        │
│  └─ 温度传感器实现温度补偿                                                 │
│                                                                             │
│  【软件层面】                                                               │
│  ├─ 双ADC同步采样消除相位误差                                              │
│  ├─ 过采样技术提高ADC分辨率 (12位→16位)                                    │
│  ├─ Goertzel算法精确提取基波                                               │
│  ├─ Flat-Top窗函数保证幅值精度                                             │
│  ├─ 三点校准 (开路/短路/负载) 消除系统误差                                 │
│  ├─ 多频点校准覆盖全频段                                                   │
│  ├─ 卡尔曼滤波平滑测量结果                                                 │
│  ├─ 中值滤波去除异常值                                                     │
│  ├─ 50/60Hz陷波器抑制工频干扰                                              │
│  └─ 自适应滤波平衡响应速度和稳定性                                         │
│                                                                             │
│  【校准层面】                                                               │
│  ├─ 多量程独立校准                                                         │
│  ├─ 多频点插值校准                                                         │
│  ├─ 相位系统误差补偿                                                       │
│  └─ 温度补偿校准                                                           │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 2. 预期性能指标

|     参数     |     指标     |       条件       |
| :----------: | :----------: | :--------------: |
| 电阻测量精度 |    ±0.1%     | 100Ω~100KΩ, 1kHz |
| 电容测量精度 |    ±0.2%     | 100pF~10μF, 1kHz |
| 电感测量精度 |    ±0.3%     | 10μH~100mH, 1kHz |
|   相位精度   |    ±0.1°     |      全量程      |
| 测量频率范围 | 100Hz~100kHz |        -         |
| 阻抗测量范围 |  0.1Ω~10MΩ   |     自动量程     |
|   测量速度   |    5次/秒    |     快速模式     |
|   温度漂移   |  <50ppm/°C   |    带温度补偿    |

### 3. 文件结构

```text
LCR_Meter/
├── Core/
│   ├── Inc/
│   │   ├── main.h
│   │   ├── stm32f10x_conf.h
│   │   └── system_config.h
│   └── Src/
│       ├── main.c
│       ├── stm32f10x_it.c
│       └── system_stm32f10x.c
├── Drivers/
│   ├── BSP/
│   │   ├── lcd1602.c/h
│   │   ├── key.c/h
│   │   ├── led.c/h
│   │   └── delay.c/h
│   └── STM32F10x_StdPeriph_Driver/
├── App/
│   ├── Measurement/
│   │   ├── dac_signal.c/h
│   │   ├── adc_sample.c/h
│   │   ├── impedance_calc.c/h
│   │   ├── measurement_process.c/h
│   │   └── auto_range.c/h
│   ├── DSP/
│   │   ├── dft_algorithm.c/h
│   │   ├── goertzel_optimized.c/h
│   │   ├── window_functions.c/h
│   │   ├── digital_filter.c/h
│   │   └── phase_measurement.c/h
│   ├── Calibration/
│   │   ├── calibration.c/h
│   │   ├── calibration_advanced.c/h
│   │   ├── temperature_sensor.c/h
│   │   └── flash_storage.c/h
│   └── UI/
│       ├── user_interface.c/h
│       └── serial_comm.c/h
├── Doc/
│   ├── schematic.pdf
│   ├── pcb_layout.pdf
│   └── user_manual.pdf
└── Project/
    ├── MDK-ARM/
    └── Makefile
```

------

如果您需要更多细节，比如：

1. **串口通信协议** (与PC上位机通信)
2. **OLED/TFT彩屏显示** (替代LCD1602)
3. **USB虚拟串口** 实现
4. **数据记录与导出** 功能
5. **更高精度方案** (外置24位ADC如ADS1256)

请告诉我，我可以继续补充！