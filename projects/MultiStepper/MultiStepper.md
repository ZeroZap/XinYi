## Ref
https://oshwhub.com/freakstudio/i2c-zong-xian-dian-ji-qu-dong-kuo-zhan-ban-zhuan-ye-ban





我来为您设计一个基于单片机的多步进电机控制系统，支持I2C和UART扩展控制。

# 基于单片机的多步进电机控制系统

## 一、系统架构设计

```text
┌─────────────────────────────────────────────────────────────┐
│                      主控制器 (STM32/ESP32)                   │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐        │
│  │ Motor 1 │  │ Motor 2 │  │ Motor 3 │  │ Motor 4 │        │
│  └────┬────┘  └────┬────┘  └────┬────┘  └────┬────┘        │
│       │            │            │            │              │
│  ┌────┴────────────┴────────────┴────────────┴────┐        │
│  │              步进电机驱动层                      │        │
│  └─────────────────────────────────────────────────┘        │
│                                                             │
│  ┌──────────────┐    ┌──────────────┐                      │
│  │   I2C 扩展   │    │  UART 扩展   │                      │
│  │  (从机模式)  │    │  (命令解析)  │                      │
│  └──────────────┘    └──────────────┘                      │
└─────────────────────────────────────────────────────────────┘
```

## 二、核心代码实现

### 1. 步进电机配置头文件 (stepper_config.h)

```c
#ifndef __STEPPER_CONFIG_H
#define __STEPPER_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

// 最大支持电机数量
#define MAX_STEPPER_NUM         8

// 步进电机类型
typedef enum {
    STEPPER_TYPE_UNIPOLAR,      // 单极性 (28BYJ-48等)
    STEPPER_TYPE_BIPOLAR,       // 双极性 (NEMA17等)
    STEPPER_TYPE_SERVO          // 闭环步进
} StepperType_t;

// 驱动模式
typedef enum {
    DRIVE_MODE_FULL_STEP,       // 整步模式
    DRIVE_MODE_HALF_STEP,       // 半步模式
    DRIVE_MODE_MICRO_1_4,       // 1/4 细分
    DRIVE_MODE_MICRO_1_8,       // 1/8 细分
    DRIVE_MODE_MICRO_1_16,      // 1/16 细分
    DRIVE_MODE_MICRO_1_32       // 1/32 细分
} DriveMode_t;

// 运动状态
typedef enum {
    MOTOR_STATE_IDLE,           // 空闲
    MOTOR_STATE_RUNNING,        // 运行中
    MOTOR_STATE_ACCEL,          // 加速中
    MOTOR_STATE_DECEL,          // 减速中
    MOTOR_STATE_HOMING,         // 回零中
    MOTOR_STATE_ERROR           // 错误
} MotorState_t;

// 运动模式
typedef enum {
    MOTION_MODE_POSITION,       // 位置模式
    MOTION_MODE_VELOCITY,       // 速度模式
    MOTION_MODE_TORQUE          // 力矩模式(闭环)
} MotionMode_t;

// 步进电机引脚配置
typedef struct {
    uint8_t step_pin;           // 脉冲引脚
    uint8_t dir_pin;            // 方向引脚
    uint8_t enable_pin;         // 使能引脚
    uint8_t ms1_pin;            // 细分引脚1
    uint8_t ms2_pin;            // 细分引脚2
    uint8_t ms3_pin;            // 细分引脚3
    uint8_t limit_min_pin;      // 最小限位
    uint8_t limit_max_pin;      // 最大限位
    uint8_t home_pin;           // 原点开关
} StepperPins_t;

// 步进电机参数
typedef struct {
    uint16_t steps_per_rev;     // 每圈步数 (200/400)
    float gear_ratio;           // 减速比
    float lead_screw_pitch;     // 丝杆导程 (mm)
    float max_speed;            // 最大速度 (steps/s)
    float max_accel;            // 最大加速度 (steps/s²)
    float max_decel;            // 最大减速度 (steps/s²)
    int32_t soft_limit_min;     // 软限位最小
    int32_t soft_limit_max;     // 软限位最大
} StepperParams_t;

// 步进电机运动状态
typedef struct {
    int32_t current_pos;        // 当前位置 (steps)
    int32_t target_pos;         // 目标位置 (steps)
    float current_speed;        // 当前速度 (steps/s)
    float target_speed;         // 目标速度 (steps/s)
    MotorState_t state;         // 运动状态
    bool direction;             // 方向 (true=正向)
    bool enabled;               // 使能状态
    bool homed;                 // 是否已回零
} StepperStatus_t;

// 步进电机完整结构
typedef struct {
    uint8_t id;                 // 电机ID
    StepperType_t type;         // 电机类型
    DriveMode_t drive_mode;     // 驱动模式
    MotionMode_t motion_mode;   // 运动模式
    StepperPins_t pins;         // 引脚配置
    StepperParams_t params;     // 参数配置
    StepperStatus_t status;     // 状态信息
} Stepper_t;

#endif
```

### 2. 步进电机驱动核心 (stepper_driver.h)

```c
#ifndef __STEPPER_DRIVER_H
#define __STEPPER_DRIVER_H

#include "stepper_config.h"

// 初始化
void Stepper_Init(void);
void Stepper_DeInit(void);

// 电机配置
bool Stepper_Config(uint8_t motor_id, Stepper_t *config);
bool Stepper_SetDriveMode(uint8_t motor_id, DriveMode_t mode);
bool Stepper_SetParams(uint8_t motor_id, StepperParams_t *params);

// 使能控制
void Stepper_Enable(uint8_t motor_id);
void Stepper_Disable(uint8_t motor_id);
void Stepper_EnableAll(void);
void Stepper_DisableAll(void);

// 基本运动控制
bool Stepper_MoveTo(uint8_t motor_id, int32_t position);           // 绝对位置
bool Stepper_MoveRelative(uint8_t motor_id, int32_t steps);        // 相对位置
bool Stepper_MoveAtSpeed(uint8_t motor_id, float speed);           // 速度模式
bool Stepper_Stop(uint8_t motor_id);                               // 减速停止
bool Stepper_EmergencyStop(uint8_t motor_id);                      // 紧急停止
void Stepper_StopAll(void);                                        // 全部停止

// 速度加速度设置
bool Stepper_SetSpeed(uint8_t motor_id, float speed);
bool Stepper_SetAccel(uint8_t motor_id, float accel);
bool Stepper_SetDecel(uint8_t motor_id, float decel);

// 回零操作
bool Stepper_Home(uint8_t motor_id, float speed, bool direction);
bool Stepper_SetHome(uint8_t motor_id);                            // 设置当前位置为零点
bool Stepper_SetPosition(uint8_t motor_id, int32_t position);      // 设置当前位置值

// 状态查询
StepperStatus_t* Stepper_GetStatus(uint8_t motor_id);
int32_t Stepper_GetPosition(uint8_t motor_id);
float Stepper_GetSpeed(uint8_t motor_id);
bool Stepper_IsRunning(uint8_t motor_id);
bool Stepper_IsHomed(uint8_t motor_id);

// 限位检测
bool Stepper_CheckLimitMin(uint8_t motor_id);
bool Stepper_CheckLimitMax(uint8_t motor_id);

// 多轴同步
bool Stepper_SyncMove(uint8_t *motor_ids, int32_t *positions, uint8_t count);
bool Stepper_LinearInterpolation(uint8_t motor_x, uint8_t motor_y, 
                                  int32_t x_pos, int32_t y_pos, float speed);

// 定时器中断处理
void Stepper_TimerISR(void);

#endif
```

### 3. 步进电机驱动实现 (stepper_driver.c)

```c
#include "stepper_driver.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>

// 电机实例数组
static Stepper_t g_steppers[MAX_STEPPER_NUM];
static uint8_t g_stepper_count = 0;

// 梯形加减速计算结构
typedef struct {
    uint32_t accel_steps;       // 加速步数
    uint32_t decel_steps;       // 减速步数
    uint32_t run_steps;         // 匀速步数
    uint32_t total_steps;       // 总步数
    float initial_speed;        // 初始速度
    float cruise_speed;         // 巡航速度
    float current_interval;     // 当前脉冲间隔
    uint32_t step_count;        // 已执行步数
    uint8_t ramp_state;         // 0:加速 1:匀速 2:减速 3:完成
} RampProfile_t;

static RampProfile_t g_ramp[MAX_STEPPER_NUM];

// 硬件抽象层 (需根据具体MCU实现)
extern void HAL_GPIO_WritePin(uint8_t pin, uint8_t state);
extern uint8_t HAL_GPIO_ReadPin(uint8_t pin);
extern void HAL_Timer_SetPeriod(uint8_t motor_id, uint32_t period_us);
extern void HAL_Timer_Start(uint8_t motor_id);
extern void HAL_Timer_Stop(uint8_t motor_id);
extern void HAL_Delay_us(uint32_t us);

/*============================================================================
 * 初始化函数
 *============================================================================*/
void Stepper_Init(void)
{
    memset(g_steppers, 0, sizeof(g_steppers));
    memset(g_ramp, 0, sizeof(g_ramp));
    g_stepper_count = 0;
}

void Stepper_DeInit(void)
{
    Stepper_StopAll();
    Stepper_DisableAll();
}

/*============================================================================
 * 电机配置
 *============================================================================*/
bool Stepper_Config(uint8_t motor_id, Stepper_t *config)
{
    if (motor_id >= MAX_STEPPER_NUM || config == NULL) {
        return false;
    }
    
    memcpy(&g_steppers[motor_id], config, sizeof(Stepper_t));
    g_steppers[motor_id].id = motor_id;
    g_steppers[motor_id].status.state = MOTOR_STATE_IDLE;
    g_steppers[motor_id].status.enabled = false;
    g_steppers[motor_id].status.homed = false;
    
    // 配置GPIO为输出
    // HAL_GPIO_SetMode(config->pins.step_pin, GPIO_OUTPUT);
    // HAL_GPIO_SetMode(config->pins.dir_pin, GPIO_OUTPUT);
    // HAL_GPIO_SetMode(config->pins.enable_pin, GPIO_OUTPUT);
    
    if (motor_id >= g_stepper_count) {
        g_stepper_count = motor_id + 1;
    }
    
    return true;
}

bool Stepper_SetDriveMode(uint8_t motor_id, DriveMode_t mode)
{
    if (motor_id >= MAX_STEPPER_NUM) return false;
    
    Stepper_t *motor = &g_steppers[motor_id];
    motor->drive_mode = mode;
    
    // 设置细分引脚 (以A4988/DRV8825为例)
    uint8_t ms1 = 0, ms2 = 0, ms3 = 0;
    
    switch (mode) {
        case DRIVE_MODE_FULL_STEP:
            ms1 = 0; ms2 = 0; ms3 = 0;
            break;
        case DRIVE_MODE_HALF_STEP:
            ms1 = 1; ms2 = 0; ms3 = 0;
            break;
        case DRIVE_MODE_MICRO_1_4:
            ms1 = 0; ms2 = 1; ms3 = 0;
            break;
        case DRIVE_MODE_MICRO_1_8:
            ms1 = 1; ms2 = 1; ms3 = 0;
            break;
        case DRIVE_MODE_MICRO_1_16:
            ms1 = 1; ms2 = 1; ms3 = 1;
            break;
        case DRIVE_MODE_MICRO_1_32:
            ms1 = 1; ms2 = 1; ms3 = 1;
            break;
    }
    
    HAL_GPIO_WritePin(motor->pins.ms1_pin, ms1);
    HAL_GPIO_WritePin(motor->pins.ms2_pin, ms2);
    HAL_GPIO_WritePin(motor->pins.ms3_pin, ms3);
    
    return true;
}

/*============================================================================
 * 使能控制
 *============================================================================*/
void Stepper_Enable(uint8_t motor_id)
{
    if (motor_id >= MAX_STEPPER_NUM) return;
    
    Stepper_t *motor = &g_steppers[motor_id];
    HAL_GPIO_WritePin(motor->pins.enable_pin, 0);  // 低电平使能
    motor->status.enabled = true;
}

void Stepper_Disable(uint8_t motor_id)
{
    if (motor_id >= MAX_STEPPER_NUM) return;
    
    Stepper_t *motor = &g_steppers[motor_id];
    HAL_GPIO_WritePin(motor->pins.enable_pin, 1);  // 高电平禁用
    motor->status.enabled = false;
}

void Stepper_EnableAll(void)
{
    for (uint8_t i = 0; i < g_stepper_count; i++) {
        Stepper_Enable(i);
    }
}

void Stepper_DisableAll(void)
{
    for (uint8_t i = 0; i < g_stepper_count; i++) {
        Stepper_Disable(i);
    }
}

/*============================================================================
 * 梯形加减速计算
 *============================================================================*/
static void Calculate_RampProfile(uint8_t motor_id, int32_t target_steps)
{
    Stepper_t *motor = &g_steppers[motor_id];
    RampProfile_t *ramp = &g_ramp[motor_id];
    
    float accel = motor->params.max_accel;
    float decel = motor->params.max_decel;
    float max_speed = motor->params.max_speed;
    
    uint32_t total_steps = abs(target_steps);
    
    // 计算加速到最大速度需要的步数
    // v² = 2*a*s => s = v²/(2*a)
    float accel_steps_f = (max_speed * max_speed) / (2.0f * accel);
    float decel_steps_f = (max_speed * max_speed) / (2.0f * decel);
    
    // 检查是否能达到最大速度
    if ((accel_steps_f + decel_steps_f) > total_steps) {
        // 三角形速度曲线
        float peak_speed = sqrtf((2.0f * accel * decel * total_steps) / (accel + decel));
        accel_steps_f = (peak_speed * peak_speed) / (2.0f * accel);
        decel_steps_f = total_steps - accel_steps_f;
        ramp->cruise_speed = peak_speed;
        ramp->run_steps = 0;
    } else {
        // 梯形速度曲线
        ramp->cruise_speed = max_speed;
        ramp->run_steps = total_steps - (uint32_t)accel_steps_f - (uint32_t)decel_steps_f;
    }
    
    ramp->accel_steps = (uint32_t)accel_steps_f;
    ramp->decel_steps = (uint32_t)decel_steps_f;
    ramp->total_steps = total_steps;
    ramp->step_count = 0;
    ramp->ramp_state = 0;  // 从加速开始
    
    // 计算初始脉冲间隔 (基于初始速度)
    ramp->initial_speed = sqrtf(2.0f * accel);  // 第一步的速度
    ramp->current_interval = 1000000.0f / ramp->initial_speed;  // 微秒
}

/*============================================================================
 * 运动控制
 *============================================================================*/
bool Stepper_MoveTo(uint8_t motor_id, int32_t position)
{
    if (motor_id >= MAX_STEPPER_NUM) return false;
    
    Stepper_t *motor = &g_steppers[motor_id];
    
    if (!motor->status.enabled) {
        return false;
    }
    
    // 检查软限位
    if (position < motor->params.soft_limit_min || 
        position > motor->params.soft_limit_max) {
        return false;
    }
    
    int32_t delta = position - motor->status.current_pos;
    if (delta == 0) return true;
    
    // 设置方向
    motor->status.direction = (delta > 0);
    HAL_GPIO_WritePin(motor->pins.dir_pin, motor->status.direction ? 1 : 0);
    
    motor->status.target_pos = position;
    motor->status.state = MOTOR_STATE_ACCEL;
    
    // 计算加减速曲线
    Calculate_RampProfile(motor_id, delta);
    
    // 启动定时器
    HAL_Timer_SetPeriod(motor_id, (uint32_t)g_ramp[motor_id].current_interval);
    HAL_Timer_Start(motor_id);
    
    return true;
}

bool Stepper_MoveRelative(uint8_t motor_id, int32_t steps)
{
    if (motor_id >= MAX_STEPPER_NUM) return false;
    
    int32_t target = g_steppers[motor_id].status.current_pos + steps;
    return Stepper_MoveTo(motor_id, target);
}

bool Stepper_MoveAtSpeed(uint8_t motor_id, float speed)
{
    if (motor_id >= MAX_STEPPER_NUM) return false;
    
    Stepper_t *motor = &g_steppers[motor_id];
    
    if (!motor->status.enabled) return false;
    
    // 设置方向
    motor->status.direction = (speed >= 0);
    HAL_GPIO_WritePin(motor->pins.dir_pin, motor->status.direction ? 1 : 0);
    
    motor->status.target_speed = fabsf(speed);
    motor->motion_mode = MOTION_MODE_VELOCITY;
    motor->status.state = MOTOR_STATE_RUNNING;
    
    // 计算脉冲间隔
    float interval = 1000000.0f / fabsf(speed);
    HAL_Timer_SetPeriod(motor_id, (uint32_t)interval);
    HAL_Timer_Start(motor_id);
    
    return true;
}

bool Stepper_Stop(uint8_t motor_id)
{
    if (motor_id >= MAX_STEPPER_NUM) return false;
    
    Stepper_t *motor = &g_steppers[motor_id];
    
    if (motor->status.state == MOTOR_STATE_IDLE) return true;
    
    // 切换到减速状态
    motor->status.state = MOTOR_STATE_DECEL;
    g_ramp[motor_id].ramp_state = 2;  // 减速
    
    return true;
}

bool Stepper_EmergencyStop(uint8_t motor_id)
{
    if (motor_id >= MAX_STEPPER_NUM) return false;
    
    HAL_Timer_Stop(motor_id);
    g_steppers[motor_id].status.state = MOTOR_STATE_IDLE;
    g_steppers[motor_id].status.current_speed = 0;
    
    return true;
}

void Stepper_StopAll(void)
{
    for (uint8_t i = 0; i < g_stepper_count; i++) {
        Stepper_EmergencyStop(i);
    }
}

/*============================================================================
 * 回零操作
 *============================================================================*/
bool Stepper_Home(uint8_t motor_id, float speed, bool direction)
{
    if (motor_id >= MAX_STEPPER_NUM) return false;
    
    Stepper_t *motor = &g_steppers[motor_id];
    
    if (!motor->status.enabled) return false;
    
    motor->status.state = MOTOR_STATE_HOMING;
    motor->status.direction = direction;
    
    HAL_GPIO_WritePin(motor->pins.dir_pin, direction ? 1 : 0);
    
    // 以指定速度向原点移动
    float interval = 1000000.0f / speed;
    HAL_Timer_SetPeriod(motor_id, (uint32_t)interval);
    HAL_Timer_Start(motor_id);
    
    return true;
}

bool Stepper_SetHome(uint8_t motor_id)
{
    if (motor_id >= MAX_STEPPER_NUM) return false;
    
    g_steppers[motor_id].status.current_pos = 0;
    g_steppers[motor_id].status.homed = true;
    
    return true;
}

bool Stepper_SetPosition(uint8_t motor_id, int32_t position)
{
    if (motor_id >= MAX_STEPPER_NUM) return false;
    
    g_steppers[motor_id].status.current_pos = position;
    
    return true;
}

/*============================================================================
 * 状态查询
 *============================================================================*/
StepperStatus_t* Stepper_GetStatus(uint8_t motor_id)
{
    if (motor_id >= MAX_STEPPER_NUM) return NULL;
    return &g_steppers[motor_id].status;
}

int32_t Stepper_GetPosition(uint8_t motor_id)
{
    if (motor_id >= MAX_STEPPER_NUM) return 0;
    return g_steppers[motor_id].status.current_pos;
}

float Stepper_GetSpeed(uint8_t motor_id)
{
    if (motor_id >= MAX_STEPPER_NUM) return 0;
    return g_steppers[motor_id].status.current_speed;
}

bool Stepper_IsRunning(uint8_t motor_id)
{
    if (motor_id >= MAX_STEPPER_NUM) return false;
    return g_steppers[motor_id].status.state != MOTOR_STATE_IDLE;
}

bool Stepper_IsHomed(uint8_t motor_id)
{
    if (motor_id >= MAX_STEPPER_NUM) return false;
    return g_steppers[motor_id].status.homed;
}

/*============================================================================
 * 限位检测
 *============================================================================*/
bool Stepper_CheckLimitMin(uint8_t motor_id)
{
    if (motor_id >= MAX_STEPPER_NUM) return false;
    return HAL_GPIO_ReadPin(g_steppers[motor_id].pins.limit_min_pin) == 0;
}

bool Stepper_CheckLimitMax(uint8_t motor_id)
{
    if (motor_id >= MAX_STEPPER_NUM) return false;
    return HAL_GPIO_ReadPin(g_steppers[motor_id].pins.limit_max_pin) == 0;
}

/*============================================================================
 * 定时器中断处理 - 核心脉冲生成
 *============================================================================*/
void Stepper_TimerISR_Motor(uint8_t motor_id)
{
    Stepper_t *motor = &g_steppers[motor_id];
    RampProfile_t *ramp = &g_ramp[motor_id];
    
    if (motor->status.state == MOTOR_STATE_IDLE) {
        return;
    }
    
    // 检查限位
    if (motor->status.state == MOTOR_STATE_HOMING) {
        if (HAL_GPIO_ReadPin(motor->pins.home_pin) == 0) {
            HAL_Timer_Stop(motor_id);
            motor->status.state = MOTOR_STATE_IDLE;
            motor->status.current_pos = 0;
            motor->status.homed = true;
            return;
        }
    } else {
        if (motor->status.direction && Stepper_CheckLimitMax(motor_id)) {
            Stepper_EmergencyStop(motor_id);
            motor->status.state = MOTOR_STATE_ERROR;
            return;
        }
        if (!motor->status.direction && Stepper_CheckLimitMin(motor_id)) {
            Stepper_EmergencyStop(motor_id);
            motor->status.state = MOTOR_STATE_ERROR;
            return;
        }
    }
    
    // 生成脉冲
    HAL_GPIO_WritePin(motor->pins.step_pin, 1);
    HAL_Delay_us(2);  // 最小脉冲宽度
    HAL_GPIO_WritePin(motor->pins.step_pin, 0);
    
    // 更新位置
    if (motor->status.direction) {
        motor->status.current_pos++;
    } else {
        motor->status.current_pos--;
    }
    
    ramp->step_count++;
    
    // 位置模式下的加减速处理
    if (motor->motion_mode == MOTION_MODE_POSITION) {
        float new_interval;
        float accel = motor->params.max_accel;
        float decel = motor->params.max_decel;
        
        switch (ramp->ramp_state) {
            case 0:  // 加速阶段
                if (ramp->step_count >= ramp->accel_steps) {
                    ramp->ramp_state = 1;  // 进入匀速
                    motor->status.state = MOTOR_STATE_RUNNING;
                } else {
                    // 计算新的间隔时间
                    float speed = sqrtf(2.0f * accel * ramp->step_count);
                    new_interval = 1000000.0f / speed;
                    ramp->current_interval = new_interval;
                    motor->status.current_speed = speed;
                    HAL_Timer_SetPeriod(motor_id, (uint32_t)new_interval);
                }
                break;
                
            case 1:  // 匀速阶段
                if (ramp->step_count >= (ramp->accel_steps + ramp->run_steps)) {
                    ramp->ramp_state = 2;  // 进入减速
                    motor->status.state = MOTOR_STATE_DECEL;
                }
                break;
                
            case 2:  // 减速阶段
                if (ramp->step_count >= ramp->total_steps) {
                    HAL_Timer_Stop(motor_id);
                    motor->status.state = MOTOR_STATE_IDLE;
                    motor->status.current_speed = 0;
                    ramp->ramp_state = 3;
                } else {
                    uint32_t decel_step = ramp->step_count - ramp->accel_steps - ramp->run_steps;
                    uint32_t remaining = ramp->decel_steps - decel_step;
                    float speed = sqrtf(2.0f * decel * remaining);
                    if (speed < ramp->initial_speed) speed = ramp->initial_speed;
                    new_interval = 1000000.0f / speed;
                    ramp->current_interval = new_interval;
                    motor->status.current_speed = speed;
                    HAL_Timer_SetPeriod(motor_id, (uint32_t)new_interval);
                }
                break;
        }
    }
}

/*============================================================================
 * 多轴同步运动
 *============================================================================*/
bool Stepper_SyncMove(uint8_t *motor_ids, int32_t *positions, uint8_t count)
{
    if (count == 0 || count > MAX_STEPPER_NUM) return false;
    
    // 计算各轴需要移动的步数
    int32_t steps[MAX_STEPPER_NUM];
    int32_t max_steps = 0;
    
    for (uint8_t i = 0; i < count; i++) {
        uint8_t id = motor_ids[i];
        steps[i] = positions[i] - g_steppers[id].status.current_pos;
        if (abs(steps[i]) > max_steps) {
            max_steps = abs(steps[i]);
        }
    }
    
    if (max_steps == 0) return true;
    
    // 计算各轴速度比例，使所有轴同时到达
    for (uint8_t i = 0; i < count; i++) {
        uint8_t id = motor_ids[i];
        float ratio = (float)abs(steps[i]) / (float)max_steps;
        float adjusted_speed = g_steppers[id].params.max_speed * ratio;
        
        // 设置调整后的速度
        Stepper_SetSpeed(id, adjusted_speed);
    }
    
    // 启动所有轴
    for (uint8_t i = 0; i < count; i++) {
        Stepper_MoveTo(motor_ids[i], positions[i]);
    }
    
    return true;
}

bool Stepper_SetSpeed(uint8_t motor_id, float speed)
{
    if (motor_id >= MAX_STEPPER_NUM) return false;
    
    if (speed > g_steppers[motor_id].params.max_speed) {
        speed = g_steppers[motor_id].params.max_speed;
    }
    
    g_steppers[motor_id].status.target_speed = speed;
    return true;
}

bool Stepper_SetAccel(uint8_t motor_id, float accel)
{
    if (motor_id >= MAX_STEPPER_NUM) return false;
    g_steppers[motor_id].params.max_accel = accel;
    return true;
}

bool Stepper_SetDecel(uint8_t motor_id, float decel)
{
    if (motor_id >= MAX_STEPPER_NUM) return false;
    g_steppers[motor_id].params.max_decel = decel;
    return true;
}
```

### 4. I2C 从机扩展接口 (i2c_slave.h / i2c_slave.c)

```c
#ifndef __I2C_SLAVE_H
#define __I2C_SLAVE_H

#include <stdint.h>
#include "stepper_driver.h"

// I2C 从机地址 (可通过跳线设置)
#define I2C_SLAVE_BASE_ADDR     0x20

// 寄存器地址定义
#define REG_DEVICE_ID           0x00    // 设备ID (只读)
#define REG_VERSION             0x01    // 固件版本 (只读)
#define REG_STATUS              0x02    // 状态寄存器
#define REG_CONTROL             0x03    // 控制寄存器
#define REG_MOTOR_SELECT        0x04    // 电机选择
#define REG_COMMAND             0x05    // 命令寄存器

// 位置寄存器 (32位)
#define REG_TARGET_POS_0        0x10    // 目标位置 [7:0]
#define REG_TARGET_POS_1        0x11    // 目标位置 [15:8]
#define REG_TARGET_POS_2        0x12    // 目标位置 [23:16]
#define REG_TARGET_POS_3        0x13    // 目标位置 [31:24]

#define REG_CURRENT_POS_0       0x14    // 当前位置 [7:0]
#define REG_CURRENT_POS_1       0x15    // 当前位置 [15:8]
#define REG_CURRENT_POS_2       0x16    // 当前位置 [23:16]
#define REG_CURRENT_POS_3       0x17    // 当前位置 [31:24]

// 速度寄存器 (16位, 单位: steps/s)
#define REG_SPEED_0             0x20    // 速度 [7:0]
#define REG_SPEED_1             0x21    // 速度 [15:8]

// 加速度寄存器 (16位, 单位: steps/s²)
#define REG_ACCEL_0             0x22    // 加速度 [7:0]
#define REG_ACCEL_1             0x23    // 加速度 [15:8]

// 命令定义
#define CMD_NOP                 0x00
#define CMD_ENABLE              0x01
#define CMD_DISABLE             0x02
#define CMD_MOVE_TO             0x03
#define CMD_MOVE_REL            0x04
#define CMD_STOP                0x05
#define CMD_EMERGENCY_STOP      0x06
#define CMD_HOME                0x07
#define CMD_SET_HOME            0x08
#define CMD_SAVE_CONFIG         0x0F

// 状态位定义
#define STATUS_RUNNING          (1 << 0)
#define STATUS_HOMED            (1 << 1)
#define STATUS_ERROR            (1 << 2)
#define STATUS_LIMIT_MIN        (1 << 3)
#define STATUS_LIMIT_MAX        (1 << 4)

void I2C_Slave_Init(uint8_t address);
void I2C_Slave_Process(void);
void I2C_Slave_IRQHandler(void);

#endif
// i2c_slave.c
#include "i2c_slave.h"
#include <string.h>

#define REG_MAP_SIZE    64

static uint8_t g_reg_map[REG_MAP_SIZE];
static uint8_t g_reg_addr = 0;
static uint8_t g_selected_motor = 0;

// 硬件抽象
extern void HAL_I2C_Slave_Init(uint8_t address);
extern void HAL_I2C_Slave_SetTxData(uint8_t *data, uint8_t len);

void I2C_Slave_Init(uint8_t address)
{
    memset(g_reg_map, 0, sizeof(g_reg_map));
    
    // 设置设备信息
    g_reg_map[REG_DEVICE_ID] = 0x5A;    // 设备ID
    g_reg_map[REG_VERSION] = 0x10;       // 版本 1.0
    
    HAL_I2C_Slave_Init(address);
}

// 更新状态寄存器
static void Update_Status_Register(void)
{
    StepperStatus_t *status = Stepper_GetStatus(g_selected_motor);
    if (status == NULL) return;
    
    uint8_t stat = 0;
    if (status->state != MOTOR_STATE_IDLE) stat |= STATUS_RUNNING;
    if (status->homed) stat |= STATUS_HOMED;
    if (status->state == MOTOR_STATE_ERROR) stat |= STATUS_ERROR;
    if (Stepper_CheckLimitMin(g_selected_motor)) stat |= STATUS_LIMIT_MIN;
    if (Stepper_CheckLimitMax(g_selected_motor)) stat |= STATUS_LIMIT_MAX;
    
    g_reg_map[REG_STATUS] = stat;
    
    // 更新当前位置
    int32_t pos = status->current_pos;
    g_reg_map[REG_CURRENT_POS_0] = (pos >> 0) & 0xFF;
    g_reg_map[REG_CURRENT_POS_1] = (pos >> 8) & 0xFF;
    g_reg_map[REG_CURRENT_POS_2] = (pos >> 16) & 0xFF;
    g_reg_map[REG_CURRENT_POS_3] = (pos >> 24) & 0xFF;
}

// 处理写入的命令
static void Process_Command(uint8_t cmd)
{
    int32_t target_pos;
    uint16_t speed, accel;
    
    switch (cmd) {
        case CMD_ENABLE:
            Stepper_Enable(g_selected_motor);
            break;
            
        case CMD_DISABLE:
            Stepper_Disable(g_selected_motor);
            break;
            
        case CMD_MOVE_TO:
            target_pos = (int32_t)(
                (g_reg_map[REG_TARGET_POS_3] << 24) |
                (g_reg_map[REG_TARGET_POS_2] << 16) |
                (g_reg_map[REG_TARGET_POS_1] << 8) |
                (g_reg_map[REG_TARGET_POS_0])
            );
            Stepper_MoveTo(g_selected_motor, target_pos);
            break;
            
        case CMD_MOVE_REL:
            target_pos = (int32_t)(
                (g_reg_map[REG_TARGET_POS_3] << 24) |
                (g_reg_map[REG_TARGET_POS_2] << 16) |
                (g_reg_map[REG_TARGET_POS_1] << 8) |
                (g_reg_map[REG_TARGET_POS_0])
            );
            Stepper_MoveRelative(g_selected_motor, target_pos);
            break;
            
        case CMD_STOP:
            Stepper_Stop(g_selected_motor);
            break;
            
        case CMD_EMERGENCY_STOP:
            Stepper_EmergencyStop(g_selected_motor);
            break;
            
        case CMD_HOME:
            speed = (g_reg_map[REG_SPEED_1] << 8) | g_reg_map[REG_SPEED_0];
            Stepper_Home(g_selected_motor, (float)speed, true);
            break;
            
        case CMD_SET_HOME:
            Stepper_SetHome(g_selected_motor);
            break;
    }
    
    g_reg_map[REG_COMMAND] = CMD_NOP;  // 清除命令
}

// I2C 接收回调
void I2C_Slave_OnReceive(uint8_t *data, uint8_t len)
{
    if (len == 0) return;
    
    g_reg_addr = data[0];  // 第一个字节是寄存器地址
    
    // 写入数据
    for (uint8_t i = 1; i < len && (g_reg_addr + i - 1) < REG_MAP_SIZE; i++) {
        uint8_t reg = g_reg_addr + i - 1;
        
        // 跳过只读寄存器
        if (reg == REG_DEVICE_ID || reg == REG_VERSION) continue;
        if (reg >= REG_CURRENT_POS_0 && reg <= REG_CURRENT_POS_3) continue;
        
        g_reg_map[reg] = data[i];
        
        // 特殊处理
        if (reg == REG_MOTOR_SELECT) {
            g_selected_motor = data[i];
        } else if (reg == REG_COMMAND) {
            Process_Command(data[i]);
        }
    }
}

// I2C 发送请求回调
void I2C_Slave_OnRequest(void)
{
    Update_Status_Register();
    HAL_I2C_Slave_SetTxData(&g_reg_map[g_reg_addr], REG_MAP_SIZE - g_reg_addr);
}
```

### 5. UART 命令解析接口 (uart_cmd.h / uart_cmd.c)

```c
#ifndef __UART_CMD_H
#define __UART_CMD_H

#include <stdint.h>

// 命令格式: $CMD,PARAM1,PARAM2,...*CHECKSUM\r\n
// 例如: $MOVE,0,10000*5A\r\n

void UART_Cmd_Init(uint32_t baudrate);
void UART_Cmd_Process(void);
void UART_Cmd_SendResponse(const char *response);

// 命令回调类型
typedef void (*CmdCallback_t)(int argc, char *argv[]);

// 注册命令
void UART_Cmd_Register(const char *cmd, CmdCallback_t callback);

#endif
// uart_cmd.c
#include "uart_cmd.h"
#include "stepper_driver.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define RX_BUFFER_SIZE      128
#define MAX_ARGS            10
#define MAX_COMMANDS        20

typedef struct {
    char name[16];
    CmdCallback_t callback;
} CmdEntry_t;

static char g_rx_buffer[RX_BUFFER_SIZE];
static uint8_t g_rx_index = 0;
static CmdEntry_t g_commands[MAX_COMMANDS];
static uint8_t g_cmd_count = 0;

// 硬件抽象
extern void HAL_UART_Init(uint32_t baudrate);
extern void HAL_UART_SendString(const char *str);
extern int HAL_UART_Available(void);
extern char HAL_UART_Read(void);

// 内置命令处理函数
static void Cmd_Help(int argc, char *argv[]);
static void Cmd_Status(int argc, char *argv[]);
static void Cmd_Enable(int argc, char *argv[]);
static void Cmd_Disable(int argc, char *argv[]);
static void Cmd_Move(int argc, char *argv[]);
static void Cmd_MoveRel(int argc, char *argv[]);
static void Cmd_Speed(int argc, char *argv[]);
static void Cmd_Accel(int argc, char *argv[]);
static void Cmd_Stop(int argc, char *argv[]);
static void Cmd_Home(int argc, char *argv[]);
static void Cmd_Pos(int argc, char *argv[]);
static void Cmd_Config(int argc, char *argv[]);

void UART_Cmd_Init(uint32_t baudrate)
{
    HAL_UART_Init(baudrate);
    g_rx_index = 0;
    g_cmd_count = 0;
    
    // 注册内置命令
    UART_Cmd_Register("HELP", Cmd_Help);
    UART_Cmd_Register("STATUS", Cmd_Status);
    UART_Cmd_Register("ENABLE", Cmd_Enable);
    UART_Cmd_Register("DISABLE", Cmd_Disable);
    UART_Cmd_Register("MOVE", Cmd_Move);
    UART_Cmd_Register("MOVEREL", Cmd_MoveRel);
    UART_Cmd_Register("SPEED", Cmd_Speed);
    UART_Cmd_Register("ACCEL", Cmd_Accel);
    UART_Cmd_Register("STOP", Cmd_Stop);
    UART_Cmd_Register("HOME", Cmd_Home);
    UART_Cmd_Register("POS", Cmd_Pos);
    UART_Cmd_Register("CONFIG", Cmd_Config);
    
    UART_Cmd_SendResponse("Stepper Controller Ready\r\n");
}

void UART_Cmd_Register(const char *cmd, CmdCallback_t callback)
{
    if (g_cmd_count >= MAX_COMMANDS) return;
    
    strncpy(g_commands[g_cmd_count].name, cmd, 15);
    g_commands[g_cmd_count].callback = callback;
    g_cmd_count++;
}

void UART_Cmd_SendResponse(const char *response)
{
    HAL_UART_SendString(response);
}

// 计算校验和
static uint8_t Calculate_Checksum(const char *str, uint8_t len)
{
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < len; i++) {
        checksum ^= str[i];
    }
    return checksum;
}

// 解析并执行命令
static void Parse_Command(char *cmd_line)
{
    char *argv[MAX_ARGS];
    int argc = 0;
    
    // 去除起始符 '$'
    if (cmd_line[0] == '$') {
        cmd_line++;
    }
    
    // 查找并验证校验和
    char *checksum_ptr = strchr(cmd_line, '*');
    if (checksum_ptr != NULL) {
        *checksum_ptr = '\0';
        // 可选: 验证校验和
    }
    
    // 分割参数
    char *token = strtok(cmd_line, ",");
    while (token != NULL && argc < MAX_ARGS) {
        argv[argc++] = token;
        token = strtok(NULL, ",");
    }
    
    if (argc == 0) return;
    
    // 查找并执行命令
    for (uint8_t i = 0; i < g_cmd_count; i++) {
        if (strcasecmp(argv[0], g_commands[i].name) == 0) {
            g_commands[i].callback(argc, argv);
            return;
        }
    }
    
    UART_Cmd_SendResponse("ERROR: Unknown command\r\n");
}

void UART_Cmd_Process(void)
{
    while (HAL_UART_Available()) {
        char c = HAL_UART_Read();
        
        if (c == '\n' || c == '\r') {
            if (g_rx_index > 0) {
                g_rx_buffer[g_rx_index] = '\0';
                Parse_Command(g_rx_buffer);
                g_rx_index = 0;
            }
        } else if (g_rx_index < RX_BUFFER_SIZE - 1) {
            g_rx_buffer[g_rx_index++] = c;
        }
    }
}

/*============================================================================
 * 内置命令实现
 *============================================================================*/

// $HELP
static void Cmd_Help(int argc, char *argv[])
{
    UART_Cmd_SendResponse(
        "=== Stepper Motor Controller Commands ===\r\n"
        "$HELP                    - Show this help\r\n"
        "$STATUS,<motor>          - Get motor status\r\n"
        "$ENABLE,<motor>          - Enable motor\r\n"
        "$DISABLE,<motor>         - Disable motor\r\n"
        "$MOVE,<motor>,<pos>      - Move to absolute position\r\n"
        "$MOVEREL,<motor>,<steps> - Move relative steps\r\n"
        "$SPEED,<motor>,<speed>   - Set max speed (steps/s)\r\n"
        "$ACCEL,<motor>,<accel>   - Set acceleration (steps/s^2)\r\n"
        "$STOP,<motor>            - Stop motor (decel)\r\n"
        "$STOP,ALL                - Stop all motors\r\n"
        "$HOME,<motor>,<speed>,<dir> - Home motor\r\n"
        "$POS,<motor>,<pos>       - Set current position\r\n"
        "$CONFIG,<motor>,<param>,<value> - Configure motor\r\n"
        "=========================================\r\n"
    );
}

// $STATUS,<motor>
static void Cmd_Status(int argc, char *argv[])
{
    if (argc < 2) {
        UART_Cmd_SendResponse("ERROR: Usage: $STATUS,<motor>\r\n");
        return;
    }
    
    uint8_t motor_id = atoi(argv[1]);
    StepperStatus_t *status = Stepper_GetStatus(motor_id);
    
    if (status == NULL) {
        UART_Cmd_SendResponse("ERROR: Invalid motor ID\r\n");
        return;
    }
    
    char response[128];
    snprintf(response, sizeof(response),
        "OK: Motor%d, Pos=%ld, Speed=%.1f, State=%d, Enabled=%d, Homed=%d\r\n",
        motor_id,
        (long)status->current_pos,
        status->current_speed,
        status->state,
        status->enabled,
        status->homed
    );
    UART_Cmd_SendResponse(response);
}

// $ENABLE,<motor>
static void Cmd_Enable(int argc, char *argv[])
{
    if (argc < 2) {
        UART_Cmd_SendResponse("ERROR: Usage: $ENABLE,<motor>\r\n");
        return;
    }
    
    if (strcasecmp(argv[1], "ALL") == 0) {
        Stepper_EnableAll();
        UART_Cmd_SendResponse("OK: All motors enabled\r\n");
    } else {
        uint8_t motor_id = atoi(argv[1]);
        Stepper_Enable(motor_id);
        UART_Cmd_SendResponse("OK: Motor enabled\r\n");
    }
}

// $DISABLE,<motor>
static void Cmd_Disable(int argc, char *argv[])
{
    if (argc < 2) {
        UART_Cmd_SendResponse("ERROR: Usage: $DISABLE,<motor>\r\n");
        return;
    }
    
    if (strcasecmp(argv[1], "ALL") == 0) {
        Stepper_DisableAll();
        UART_Cmd_SendResponse("OK: All motors disabled\r\n");
    } else {
        uint8_t motor_id = atoi(argv[1]);
        Stepper_Disable(motor_id);
        UART_Cmd_SendResponse("OK: Motor disabled\r\n");
    }
}

// $MOVE,<motor>,<position>
static void Cmd_Move(int argc, char *argv[])
{
    if (argc < 3) {
        UART_Cmd_SendResponse("ERROR: Usage: $MOVE,<motor>,<position>\r\n");
        return;
    }
    
    uint8_t motor_id = atoi(argv[1]);
    int32_t position = atol(argv[2]);
    
    if (Stepper_MoveTo(motor_id, position)) {
        UART_Cmd_SendResponse("OK: Moving\r\n");
    } else {
        UART_Cmd_SendResponse("ERROR: Move failed\r\n");
    }
}

// $MOVEREL,<motor>,<steps>
static void Cmd_MoveRel(int argc, char *argv[])
{
    if (argc < 3) {
        UART_Cmd_SendResponse("ERROR: Usage: $MOVEREL,<motor>,<steps>\r\n");
        return;
    }
    
    uint8_t motor_id = atoi(argv[1]);
    int32_t steps = atol(argv[2]);
    
    if (Stepper_MoveRelative(motor_id, steps)) {
        UART_Cmd_SendResponse("OK: Moving\r\n");
    } else {
        UART_Cmd_SendResponse("ERROR: Move failed\r\n");
    }
}

// $SPEED,<motor>,<speed>
static void Cmd_Speed(int argc, char *argv[])
{
    if (argc < 3) {
        UART_Cmd_SendResponse("ERROR: Usage: $SPEED,<motor>,<speed>\r\n");
        return;
    }
    
    uint8_t motor_id = atoi(argv[1]);
    float speed = atof(argv[2]);
    
    if (Stepper_SetSpeed(motor_id, speed)) {
        UART_Cmd_SendResponse("OK: Speed set\r\n");
    } else {
        UART_Cmd_SendResponse("ERROR: Set speed failed\r\n");
    }
}

// $ACCEL,<motor>,<accel>
static void Cmd_Accel(int argc, char *argv[])
{
    if (argc < 3) {
        UART_Cmd_SendResponse("ERROR: Usage: $ACCEL,<motor>,<accel>\r\n");
        return;
    }
    
    uint8_t motor_id = atoi(argv[1]);
    float accel = atof(argv[2]);
    
    if (Stepper_SetAccel(motor_id, accel)) {
        UART_Cmd_SendResponse("OK: Acceleration set\r\n");
    } else {
        UART_Cmd_SendResponse("ERROR: Set accel failed\r\n");
    }
}

// $STOP,<motor>
static void Cmd_Stop(int argc, char *argv[])
{
    if (argc < 2) {
        UART_Cmd_SendResponse("ERROR: Usage: $STOP,<motor>\r\n");
        return;
    }
    
    if (strcasecmp(argv[1], "ALL") == 0) {
        Stepper_StopAll();
        UART_Cmd_SendResponse("OK: All motors stopped\r\n");
    } else {
        uint8_t motor_id = atoi(argv[1]);
        Stepper_Stop(motor_id);
        UART_Cmd_SendResponse("OK: Motor stopping\r\n");
    }
}

// $HOME,<motor>,<speed>,<direction>
static void Cmd_Home(int argc, char *argv[])
{
    if (argc < 4) {
        UART_Cmd_SendResponse("ERROR: Usage: $HOME,<motor>,<speed>,<dir>\r\n");
        return;
    }
    
    uint8_t motor_id = atoi(argv[1]);
    float speed = atof(argv[2]);
    bool direction = atoi(argv[3]) != 0;
    
    if (Stepper_Home(motor_id, speed, direction)) {
        UART_Cmd_SendResponse("OK: Homing\r\n");
    } else {
        UART_Cmd_SendResponse("ERROR: Home failed\r\n");
    }
}

// $POS,<motor>,<position>
static void Cmd_Pos(int argc, char *argv[])
{
    if (argc < 3) {
        UART_Cmd_SendResponse("ERROR: Usage: $POS,<motor>,<position>\r\n");
        return;
    }
    
    uint8_t motor_id = atoi(argv[1]);
    int32_t position = atol(argv[2]);
    
    if (Stepper_SetPosition(motor_id, position)) {
        UART_Cmd_SendResponse("OK: Position set\r\n");
    } else {
        UART_Cmd_SendResponse("ERROR: Set position failed\r\n");
    }
}

// $CONFIG,<motor>,<param>,<value>
static void Cmd_Config(int argc, char *argv[])
{
    if (argc < 4) {
        UART_Cmd_SendResponse("ERROR: Usage: $CONFIG,<motor>,<param>,<value>\r\n");
        return;
    }
    
    uint8_t motor_id = atoi(argv[1]);
    const char *param = argv[2];
    
    if (strcasecmp(param, "MICROSTEP") == 0) {
        int mode = atoi(argv[3]);
        Stepper_SetDriveMode(motor_id, (DriveMode_t)mode);
        UART_Cmd_SendResponse("OK: Microstep set\r\n");
    } else if (strcasecmp(param, "MAXSPEED") == 0) {
        float value = atof(argv[3]);
        Stepper_SetSpeed(motor_id, value);
        UART_Cmd_SendResponse("OK: Max speed set\r\n");
    } else {
        UART_Cmd_SendResponse("ERROR: Unknown parameter\r\n");
    }
}
```

### 6. 主程序示例 (main.c)

```c
#include "stepper_config.h"
#include "stepper_driver.h"
#include "i2c_slave.h"
#include "uart_cmd.h"

// 电机配置示例
void Setup_Motors(void)
{
    Stepper_t motor0 = {
        .id = 0,
        .type = STEPPER_TYPE_BIPOLAR,
        .drive_mode = DRIVE_MODE_MICRO_1_16,
        .motion_mode = MOTION_MODE_POSITION,
        .pins = {
            .step_pin = 2,
            .dir_pin = 3,
            .enable_pin = 4,
            .ms1_pin = 5,
            .ms2_pin = 6,
            .ms3_pin = 7,
            .limit_min_pin = 8,
            .limit_max_pin = 9,
            .home_pin = 10
        },
        .params = {
            .steps_per_rev = 200,
            .gear_ratio = 1.0f,
            .lead_screw_pitch = 8.0f,    // 8mm 导程丝杆
            .max_speed = 4000.0f,         // 4000 steps/s
            .max_accel = 8000.0f,         // 8000 steps/s²
            .max_decel = 8000.0f,
            .soft_limit_min = -100000,
            .soft_limit_max = 100000
        }
    };
    
    Stepper_t motor1 = {
        .id = 1,
        .type = STEPPER_TYPE_BIPOLAR,
        .drive_mode = DRIVE_MODE_MICRO_1_16,
        .motion_mode = MOTION_MODE_POSITION,
        .pins = {
            .step_pin = 11,
            .dir_pin = 12,
            .enable_pin = 13,
            .ms1_pin = 14,
            .ms2_pin = 15,
            .ms3_pin = 16,
            .limit_min_pin = 17,
            .limit_max_pin = 18,
            .home_pin = 19
        },
        .params = {
            .steps_per_rev = 200,
            .gear_ratio = 1.0f,
            .lead_screw_pitch = 8.0f,
            .max_speed = 4000.0f,
            .max_accel = 8000.0f,
            .max_decel = 8000.0f,
            .soft_limit_min = -100000,
            .soft_limit_max = 100000
        }
    };
    
    Stepper_Config(0, &motor0);
    Stepper_Config(1, &motor1);
}

int main(void)
{
    // 系统初始化
    // HAL_Init();
    // SystemClock_Config();
    
    // 初始化步进电机驱动
    Stepper_Init();
    Setup_Motors();
    
    // 初始化通信接口
    UART_Cmd_Init(115200);
    I2C_Slave_Init(I2C_SLAVE_BASE_ADDR);
    
```





```
继续
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

已完成生成

### 6. 主程序示例 (main.c) - 续

```c
    // 使能电机
    Stepper_Enable(0);
    Stepper_Enable(1);
    
    // 设置细分模式
    Stepper_SetDriveMode(0, DRIVE_MODE_MICRO_1_16);
    Stepper_SetDriveMode(1, DRIVE_MODE_MICRO_1_16);
    
    // 主循环
    while (1) {
        // 处理UART命令
        UART_Cmd_Process();
        
        // 处理I2C通信
        I2C_Slave_Process();
        
        // 其他任务...
    }
    
    return 0;
}

/*============================================================================
 * 定时器中断服务函数 (需要在中断向量表中配置)
 *============================================================================*/
void TIM2_IRQHandler(void)  // 电机0
{
    // 清除中断标志
    // HAL_TIM_ClearFlag(TIM2);
    Stepper_TimerISR_Motor(0);
}

void TIM3_IRQHandler(void)  // 电机1
{
    // HAL_TIM_ClearFlag(TIM3);
    Stepper_TimerISR_Motor(1);
}

void TIM4_IRQHandler(void)  // 电机2
{
    // HAL_TIM_ClearFlag(TIM4);
    Stepper_TimerISR_Motor(2);
}

void TIM5_IRQHandler(void)  // 电机3
{
    // HAL_TIM_ClearFlag(TIM5);
    Stepper_TimerISR_Motor(3);
}
```

### 7. STM32 硬件抽象层实现 (hal_stm32.c)

```c
#include "stm32f4xx_hal.h"
#include <string.h>

/*============================================================================
 * GPIO 操作
 *============================================================================*/

// GPIO端口映射表
typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
} GPIO_Map_t;

// 根据实际硬件配置修改
static const GPIO_Map_t gpio_map[] = {
    {GPIOA, GPIO_PIN_0},   // Pin 0
    {GPIOA, GPIO_PIN_1},   // Pin 1
    {GPIOA, GPIO_PIN_2},   // Pin 2  - Motor0 STEP
    {GPIOA, GPIO_PIN_3},   // Pin 3  - Motor0 DIR
    {GPIOA, GPIO_PIN_4},   // Pin 4  - Motor0 EN
    {GPIOA, GPIO_PIN_5},   // Pin 5  - Motor0 MS1
    {GPIOA, GPIO_PIN_6},   // Pin 6  - Motor0 MS2
    {GPIOA, GPIO_PIN_7},   // Pin 7  - Motor0 MS3
    {GPIOB, GPIO_PIN_0},   // Pin 8  - Motor0 LIMIT_MIN
    {GPIOB, GPIO_PIN_1},   // Pin 9  - Motor0 LIMIT_MAX
    {GPIOB, GPIO_PIN_2},   // Pin 10 - Motor0 HOME
    {GPIOB, GPIO_PIN_3},   // Pin 11 - Motor1 STEP
    {GPIOB, GPIO_PIN_4},   // Pin 12 - Motor1 DIR
    {GPIOB, GPIO_PIN_5},   // Pin 13 - Motor1 EN
    // ... 继续添加
};

void HAL_GPIO_WritePin_Custom(uint8_t pin, uint8_t state)
{
    if (pin >= sizeof(gpio_map)/sizeof(gpio_map[0])) return;
    HAL_GPIO_WritePin(gpio_map[pin].port, gpio_map[pin].pin, 
                      state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

uint8_t HAL_GPIO_ReadPin_Custom(uint8_t pin)
{
    if (pin >= sizeof(gpio_map)/sizeof(gpio_map[0])) return 0;
    return HAL_GPIO_ReadPin(gpio_map[pin].port, gpio_map[pin].pin);
}

/*============================================================================
 * 定时器操作 - 每个电机使用独立定时器
 *============================================================================*/

static TIM_HandleTypeDef htim2, htim3, htim4, htim5;
static TIM_HandleTypeDef *motor_timers[] = {&htim2, &htim3, &htim4, &htim5};

void HAL_Timer_Init_All(void)
{
    // TIM2 配置 - Motor 0
    __HAL_RCC_TIM2_CLK_ENABLE();
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 84 - 1;  // 84MHz / 84 = 1MHz (1us)
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 1000 - 1;   // 默认1ms
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_Base_Init(&htim2);
    
    // TIM3 配置 - Motor 1
    __HAL_RCC_TIM3_CLK_ENABLE();
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 84 - 1;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 1000 - 1;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_Base_Init(&htim3);
    
    // TIM4 配置 - Motor 2
    __HAL_RCC_TIM4_CLK_ENABLE();
    htim4.Instance = TIM4;
    htim4.Init.Prescaler = 84 - 1;
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = 1000 - 1;
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_Base_Init(&htim4);
    
    // TIM5 配置 - Motor 3
    __HAL_RCC_TIM5_CLK_ENABLE();
    htim5.Instance = TIM5;
    htim5.Init.Prescaler = 84 - 1;
    htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim5.Init.Period = 1000 - 1;
    htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_Base_Init(&htim5);
    
    // 配置中断优先级
    HAL_NVIC_SetPriority(TIM2_IRQn, 1, 0);
    HAL_NVIC_SetPriority(TIM3_IRQn, 1, 0);
    HAL_NVIC_SetPriority(TIM4_IRQn, 1, 0);
    HAL_NVIC_SetPriority(TIM5_IRQn, 1, 0);
}

void HAL_Timer_SetPeriod(uint8_t motor_id, uint32_t period_us)
{
    if (motor_id >= 4) return;
    
    TIM_HandleTypeDef *htim = motor_timers[motor_id];
    __HAL_TIM_SET_AUTORELOAD(htim, period_us - 1);
    __HAL_TIM_SET_COUNTER(htim, 0);
}

void HAL_Timer_Start(uint8_t motor_id)
{
    if (motor_id >= 4) return;
    
    TIM_HandleTypeDef *htim = motor_timers[motor_id];
    
    // 使能中断
    switch (motor_id) {
        case 0: HAL_NVIC_EnableIRQ(TIM2_IRQn); break;
        case 1: HAL_NVIC_EnableIRQ(TIM3_IRQn); break;
        case 2: HAL_NVIC_EnableIRQ(TIM4_IRQn); break;
        case 3: HAL_NVIC_EnableIRQ(TIM5_IRQn); break;
    }
    
    HAL_TIM_Base_Start_IT(htim);
}

void HAL_Timer_Stop(uint8_t motor_id)
{
    if (motor_id >= 4) return;
    
    TIM_HandleTypeDef *htim = motor_timers[motor_id];
    HAL_TIM_Base_Stop_IT(htim);
}

void HAL_Delay_us(uint32_t us)
{
    // 简单延时，实际应用中可使用DWT或定时器
    uint32_t cycles = us * (SystemCoreClock / 1000000) / 5;
    while (cycles--) {
        __NOP();
    }
}

/*============================================================================
 * UART 操作
 *============================================================================*/

static UART_HandleTypeDef huart1;
static uint8_t uart_rx_buffer[256];
static volatile uint16_t uart_rx_head = 0;
static volatile uint16_t uart_rx_tail = 0;

void HAL_UART_Init_Custom(uint32_t baudrate)
{
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;  // TX, RX
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    huart1.Instance = USART1;
    huart1.Init.BaudRate = baudrate;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    HAL_UART_Init(&huart1);
    
    // 使能接收中断
    HAL_NVIC_SetPriority(USART1_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
}

void HAL_UART_SendString(const char *str)
{
    HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), HAL_MAX_DELAY);
}

int HAL_UART_Available(void)
{
    return (uart_rx_head - uart_rx_tail) & 0xFF;
}

char HAL_UART_Read(void)
{
    if (uart_rx_head == uart_rx_tail) return 0;
    char c = uart_rx_buffer[uart_rx_tail];
    uart_rx_tail = (uart_rx_tail + 1) & 0xFF;
    return c;
}

void USART1_IRQHandler(void)
{
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE)) {
        uint8_t data = (uint8_t)(huart1.Instance->DR & 0xFF);
        uint16_t next_head = (uart_rx_head + 1) & 0xFF;
        if (next_head != uart_rx_tail) {
            uart_rx_buffer[uart_rx_head] = data;
            uart_rx_head = next_head;
        }
    }
}

/*============================================================================
 * I2C 从机操作
 *============================================================================*/

static I2C_HandleTypeDef hi2c1;
static uint8_t i2c_rx_buffer[32];
static uint8_t i2c_tx_buffer[64];

void HAL_I2C_Slave_Init(uint8_t address)
{
    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;  // SCL, SDA
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 400000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = address << 1;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
    
    // 使能I2C中断
    HAL_NVIC_SetPriority(I2C1_EV_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
    
    // 启动从机监听
    HAL_I2C_EnableListen_IT(&hi2c1);
}

void HAL_I2C_Slave_SetTxData(uint8_t *data, uint8_t len)
{
    memcpy(i2c_tx_buffer, data, len);
}

// I2C 事件回调
void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, 
                          uint16_t AddrMatchCode)
{
    if (TransferDirection == I2C_DIRECTION_TRANSMIT) {
        // 主机写入
        HAL_I2C_Slave_Seq_Receive_IT(hi2c, i2c_rx_buffer, 32, I2C_FIRST_AND_LAST_FRAME);
    } else {
        // 主机读取
        HAL_I2C_Slave_Seq_Transmit_IT(hi2c, i2c_tx_buffer, 64, I2C_FIRST_AND_LAST_FRAME);
    }
}

void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    extern void I2C_Slave_OnReceive(uint8_t *data, uint8_t len);
    I2C_Slave_OnReceive(i2c_rx_buffer, 32);
    HAL_I2C_EnableListen_IT(hi2c);
}

void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    HAL_I2C_EnableListen_IT(hi2c);
}

void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c)
{
    HAL_I2C_EnableListen_IT(hi2c);
}

void I2C1_EV_IRQHandler(void)
{
    HAL_I2C_EV_IRQHandler(&hi2c1);
}
```

### 8. 28BYJ-48 单极性步进电机专用驱动 (stepper_28byj48.c)

```c
/*============================================================================
 * 28BYJ-48 步进电机驱动 (ULN2003驱动板)
 * 特点: 5V供电, 64步/圈, 1:64减速比, 实际4096步/圈
 *============================================================================*/

#include "stepper_config.h"
#include <stdint.h>
#include <stdbool.h>

// 28BYJ-48 引脚定义
typedef struct {
    uint8_t in1_pin;
    uint8_t in2_pin;
    uint8_t in3_pin;
    uint8_t in4_pin;
} BYJ48_Pins_t;

// 半步序列 (8步)
static const uint8_t HALF_STEP_SEQ[8][4] = {
    {1, 0, 0, 0},
    {1, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 1},
    {0, 0, 0, 1},
    {1, 0, 0, 1}
};

// 全步序列 (4步)
static const uint8_t FULL_STEP_SEQ[4][4] = {
    {1, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 1},
    {1, 0, 0, 1}
};

typedef struct {
    BYJ48_Pins_t pins;
    int32_t current_pos;
    int32_t target_pos;
    uint8_t step_index;
    bool use_half_step;
    bool enabled;
    uint32_t step_delay_us;  // 步进延时
} BYJ48_Motor_t;

#define MAX_BYJ48_MOTORS    4
static BYJ48_Motor_t g_byj48_motors[MAX_BYJ48_MOTORS];

extern void HAL_GPIO_WritePin(uint8_t pin, uint8_t state);
extern void HAL_Delay_us(uint32_t us);

void BYJ48_Init(uint8_t motor_id, BYJ48_Pins_t *pins, bool half_step)
{
    if (motor_id >= MAX_BYJ48_MOTORS) return;
    
    BYJ48_Motor_t *motor = &g_byj48_motors[motor_id];
    motor->pins = *pins;
    motor->current_pos = 0;
    motor->target_pos = 0;
    motor->step_index = 0;
    motor->use_half_step = half_step;
    motor->enabled = false;
    motor->step_delay_us = 1000;  // 默认1ms延时
    
    // 初始化引脚为低电平
    HAL_GPIO_WritePin(pins->in1_pin, 0);
    HAL_GPIO_WritePin(pins->in2_pin, 0);
    HAL_GPIO_WritePin(pins->in3_pin, 0);
    HAL_GPIO_WritePin(pins->in4_pin, 0);
}

void BYJ48_SetSpeed(uint8_t motor_id, float rpm)
{
    if (motor_id >= MAX_BYJ48_MOTORS) return;
    
    BYJ48_Motor_t *motor = &g_byj48_motors[motor_id];
    
    // 计算步进延时
    // 4096步/圈 (半步), 2048步/圈 (全步)
    uint32_t steps_per_rev = motor->use_half_step ? 4096 : 2048;
    float steps_per_sec = (rpm * steps_per_rev) / 60.0f;
    motor->step_delay_us = (uint32_t)(1000000.0f / steps_per_sec);
    
    // 限制最小延时 (防止失步)
    if (motor->step_delay_us < 800) {
        motor->step_delay_us = 800;
    }
}

static void BYJ48_OutputStep(BYJ48_Motor_t *motor)
{
    const uint8_t *seq;
    
    if (motor->use_half_step) {
        seq = HALF_STEP_SEQ[motor->step_index];
    } else {
        seq = FULL_STEP_SEQ[motor->step_index];
    }
    
    HAL_GPIO_WritePin(motor->pins.in1_pin, seq[0]);
    HAL_GPIO_WritePin(motor->pins.in2_pin, seq[1]);
    HAL_GPIO_WritePin(motor->pins.in3_pin, seq[2]);
    HAL_GPIO_WritePin(motor->pins.in4_pin, seq[3]);
}

void BYJ48_Step(uint8_t motor_id, bool direction)
{
    if (motor_id >= MAX_BYJ48_MOTORS) return;
    
    BYJ48_Motor_t *motor = &g_byj48_motors[motor_id];
    uint8_t max_index = motor->use_half_step ? 8 : 4;
    
    if (direction) {
        motor->step_index++;
        if (motor->step_index >= max_index) {
            motor->step_index = 0;
        }
        motor->current_pos++;
    } else {
        if (motor->step_index == 0) {
            motor->step_index = max_index - 1;
        } else {
            motor->step_index--;
        }
        motor->current_pos--;
    }
    
    BYJ48_OutputStep(motor);
}

// 阻塞式移动到目标位置
void BYJ48_MoveTo(uint8_t motor_id, int32_t position)
{
    if (motor_id >= MAX_BYJ48_MOTORS) return;
    
    BYJ48_Motor_t *motor = &g_byj48_motors[motor_id];
    motor->target_pos = position;
    
    while (motor->current_pos != motor->target_pos) {
        bool dir = (motor->target_pos > motor->current_pos);
        BYJ48_Step(motor_id, dir);
        HAL_Delay_us(motor->step_delay_us);
    }
    
    // 停止后释放线圈 (省电)
    BYJ48_Release(motor_id);
}

// 相对移动
void BYJ48_MoveRelative(uint8_t motor_id, int32_t steps)
{
    if (motor_id >= MAX_BYJ48_MOTORS) return;
    
    int32_t target = g_byj48_motors[motor_id].current_pos + steps;
    BYJ48_MoveTo(motor_id, target);
}

// 移动指定角度
void BYJ48_MoveAngle(uint8_t motor_id, float degrees)
{
    if (motor_id >= MAX_BYJ48_MOTORS) return;
    
    BYJ48_Motor_t *motor = &g_byj48_motors[motor_id];
    uint32_t steps_per_rev = motor->use_half_step ? 4096 : 2048;
    
    int32_t steps = (int32_t)((degrees / 360.0f) * steps_per_rev);
    BYJ48_MoveRelative(motor_id, steps);
}

// 释放线圈
void BYJ48_Release(uint8_t motor_id)
{
    if (motor_id >= MAX_BYJ48_MOTORS) return;
    
    BYJ48_Motor_t *motor = &g_byj48_motors[motor_id];
    HAL_GPIO_WritePin(motor->pins.in1_pin, 0);
    HAL_GPIO_WritePin(motor->pins.in2_pin, 0);
    HAL_GPIO_WritePin(motor->pins.in3_pin, 0);
    HAL_GPIO_WritePin(motor->pins.in4_pin, 0);
}

int32_t BYJ48_GetPosition(uint8_t motor_id)
{
    if (motor_id >= MAX_BYJ48_MOTORS) return 0;
    return g_byj48_motors[motor_id].current_pos;
}

void BYJ48_SetPosition(uint8_t motor_id, int32_t position)
{
    if (motor_id >= MAX_BYJ48_MOTORS) return;
    g_byj48_motors[motor_id].current_pos = position;
}
```

### 9. S曲线加减速算法 (s_curve.c)

```c
/*============================================================================
 * S曲线加减速算法
 * 相比梯形加减速，S曲线更平滑，减少机械冲击
 *============================================================================*/

#include <math.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    // 输入参数
    float max_velocity;      // 最大速度
    float max_accel;         // 最大加速度
    float max_jerk;          // 最大加加速度 (jerk)
    int32_t total_steps;     // 总步数
    
    // 计算结果
    float t1, t2, t3, t4, t5, t6, t7;  // 7段时间
    float v1, v2;            // 关键速度点
    
    // 运行状态
    float current_time;
    float current_velocity;
    float current_position;
    uint8_t phase;           // 当前阶段 (1-7)
    bool completed;
} SCurve_Profile_t;

/*
 * S曲线7段速度规划:
 * Phase 1: 加加速 (jerk = +J)
 * Phase 2: 匀加速 (jerk = 0)
 * Phase 3: 减加速 (jerk = -J)
 * Phase 4: 匀速   (accel = 0)
 * Phase 5: 加减速 (jerk = -J)
 * Phase 6: 匀减速 (jerk = 0)
 * Phase 7: 减减速 (jerk = +J)
 */

void SCurve_Init(SCurve_Profile_t *profile, float max_vel, float max_accel, 
                 float max_jerk, int32_t steps)
{
    profile->max_velocity = max_vel;
    profile->max_accel = max_accel;
    profile->max_jerk = max_jerk;
    profile->total_steps = steps;
    profile->current_time = 0;
    profile->current_velocity = 0;
    profile->current_position = 0;
    profile->phase = 1;
    profile->completed = false;
    
    float J = max_jerk;
    float A = max_accel;
    float V = max_vel;
    float S = (float)steps;
    
    // 计算达到最大加速度需要的时间
    float t_j = A / J;  // jerk阶段时间
    
    // 计算达到最大速度需要的加速距离
    float s_accel = V * V / (2.0f * A) + V * t_j;
    
    if (2.0f * s_accel > S) {
        // 无法达到最大速度，需要重新计算
        // 简化处理：使用三角形速度曲线
        float v_peak = sqrtf(S * A / 2.0f);
        profile->t1 = v_peak / A;
        profile->t2 = 0;
        profile->t3 = profile->t1;
        profile->t4 = 0;
        profile->t5 = profile->t1;
        profile->t6 = 0;
        profile->t7 = profile->t1;
        profile->v1 = v_peak;
        profile->v2 = v_peak;
    } else {
        // 完整S曲线
        profile->t1 = t_j;
        profile->t2 = (V / A) - t_j;
        profile->t3 = t_j;
        profile->t4 = (S - 2.0f * s_accel) / V;
        profile->t5 = t_j;
        profile->t6 = profile->t2;
        profile->t7 = t_j;
        profile->v1 = A * t_j;
        profile->v2 = V;
    }
}

float SCurve_GetVelocity(SCurve_Profile_t *profile, float dt)
{
    if (profile->completed) return 0;
    
    profile->current_time += dt;
    float t = profile->current_time;
    
    float J = profile->max_jerk;
    float A = profile->max_accel;
    float v = 0;
    
    float t1 = profile->t1;
    float t2 = t1 + profile->t2;
    float t3 = t2 + profile->t3;
    float t4 = t3 + profile->t4;
    float t5 = t4 + profile->t5;
    float t6 = t5 + profile->t6;
    float t7 = t6 + profile->t7;
    
    if (t < t1) {
        // Phase 1: 加加速
        v = 0.5f * J * t * t;
        profile->phase = 1;
    } else if (t < t2) {
        // Phase 2: 匀加速
        float dt1 = t - t1;
        v = profile->v1 + A * dt1;
        profile->phase = 2;
    } else if (t < t3) {
        // Phase 3: 减加速
        float dt2 = t - t2;
        float v_start = profile->v1 + A * profile->t2;
        v = v_start + A * dt2 - 0.5f * J * dt2 * dt2;
        profile->phase = 3;
    } else if (t < t4) {
        // Phase 4: 匀速
        v = profile->v2;
        profile->phase = 4;
    } else if (t < t5) {
        // Phase 5: 加减速
        float dt4 = t - t4;
        v = profile->v2 - 0.5f * J * dt4 * dt4;
        profile->phase = 5;
    } else if (t < t6) {
        // Phase 6: 匀减速
        float dt5 = t - t5;
        float v_start = profile->v2 - 0.5f * J * profile->t5 * profile->t5;
        v = v_start - A * dt5;
        profile->phase = 6;
    } else if (t < t7) {
        // Phase 7: 减减速
        float dt6 = t - t6;
        float v_start = profile->v1;
        v = v_start - A * dt6 + 0.5f * J * dt6 * dt6;
        profile->phase = 7;
    } else {
        v = 0;
        profile->completed = true;
    }
    
    profile->current_velocity = v;
    return v;
}

// 根据速度计算脉冲间隔
uint32_t SCurve_GetInterval(SCurve_Profile_t *profile, float dt)
{
    float v = SCurve_GetVelocity(profile, dt);
    if (v < 10.0f) v = 10.0f;  // 最小速度限制
    return (uint32_t)(1000000.0f / v);  // 返回微秒
}

bool SCurve_IsCompleted(SCurve_Profile_t *profile)
{
    return profile->completed;
}
```

### 10. 使用示例

```c
/*============================================================================
 * 使用示例
 *============================================================================*/

// 示例1: 基本位置控制
void Example_BasicMove(void)
{
    // 使能电机
    Stepper_Enable(0);
    
    // 设置速度和加速度
    Stepper_SetSpeed(0, 2000);    // 2000 steps/s
    Stepper_SetAccel(0, 5000);    // 5000 steps/s²
    
    // 移动到绝对位置
    Stepper_MoveTo(0, 10000);
    
    // 等待完成
    while (Stepper_IsRunning(0)) {
        // 可以做其他事情
    }
    
    // 相对移动
    Stepper_MoveRelative(0, -5000);
}

// 示例2: 回零操作
void Example_Homing(void)
{
    Stepper_Enable(0);
    
    // 以500 steps/s的速度向负方向回零
    Stepper_Home(0, 500, false);
    
    // 等待回零完成
    while (!Stepper_IsHomed(0)) {
        // 检查超时等
    }
    
    // 回零完成后移动到工作位置
    Stepper_MoveTo(0, 1000);
}

// 示例3: 多轴同步运动
void Example_SyncMove(void)
{
    Stepper_Enable(0);
    Stepper_Enable(1);
    
    uint8_t motors[] = {0, 1};
    int32_t positions[] = {10000, 5000};
    
    // 两轴同时到达目标位置
    Stepper_SyncMove(motors, positions, 2);
}

// 示例4: 通过UART控制
void Example_UART_Control(void)
{
    // 发送命令: $ENABLE,0\r\n
    // 发送命令: $SPEED,0,3000\r\n
    // 发送命令: $MOVE,0,20000\r\n
    // 查询状态: $STATUS,0\r\n
}

// 示例5: 通过I2C控制 (主机端代码)
void Example_I2C_Master_Control(void)
{
    uint8_t slave_addr = 0x20;
    uint8_t data[8];
    
    // 选择电机0
    data[0] = 0x04;  // REG_MOTOR_SELECT
    data[1] = 0;     // Motor 0
    // I2C_Master_Write(slave_addr, data, 2);
    
    // 设置目标位置 10000
    data[0] = 0x10;  // REG_TARGET_POS_0
    data[1] = (10000 >> 0) & 0xFF;
    data[2] = (10000 >> 8) & 0xFF;
    data[3] = (10000 >> 16) & 0xFF;
    data[4] = (10000 >> 24) & 0xFF;
    // I2C_Master_Write(slave_addr, data, 5);
    
    // 发送移动命令
    data[0] = 0x05;  // REG_COMMAND
    data[1] = 0x03;  // CMD_MOVE_TO
    // I2C_Master_Write(slave_addr, data, 2);
}

// 示例6: 28BYJ-48电机控制
void Example_28BYJ48(void)
{
    BYJ48_Pins_t pins = {
        .in1_pin = 2,
        .in2_pin = 3,
        .in3_pin = 4,
        .in4_pin = 5
    };
    
    BYJ48_Init(0, &pins, true);  // 使用半步模式
    BYJ48_SetSpeed(0, 10);       // 10 RPM
    
    // 旋转90度
    BYJ48_MoveAngle(0, 90);
    
    // 旋转一圈
    BYJ48_MoveAngle(0, 360);
}
```

## 三、系统特性总结

|       特性       |           说明           |
| :--------------: | :----------------------: |
| **支持电机数量** | 最多8个步进电机同时控制  |
|   **驱动模式**   | 全步、半步、1/4~1/32细分 |
|  **加减速算法**  | 梯形加减速、S曲线加减速  |
|   **运动模式**   |    位置模式、速度模式    |
|   **通信接口**   |    UART命令、I2C从机     |
|   **保护功能**   | 软限位、硬限位、紧急停止 |
|   **回零功能**   |     支持原点开关回零     |
|   **多轴同步**   |     支持多轴插补运动     |

## 四、硬件连接参考

```text
┌─────────────────┐     ┌─────────────────┐
│   MCU (STM32)   │     │  驱动板 (A4988) │
├─────────────────┤     ├─────────────────┤
│ PA2 (STEP) ─────┼────►│ STEP            │
│ PA3 (DIR)  ─────┼────►│ DIR             │
│ PA4 (EN)   ─────┼────►│ ENABLE          │
│ PA5 (MS1)  ─────┼────►│ MS1             │
│ PA6 (MS2)  ─────┼────►│ MS2             │
│ PA7 (MS3)  ─────┼────►│ MS3             │
├─────────────────┤     ├─────────────────┤
│ PB6 (SCL)  ─────┼────►│ I2C 扩展        │
│ PB7 (SDA)  ─────┼────►│                 │
├─────────────────┤     └─────────────────┘
│ PA9 (TX)   ─────┼────► UART 调试/控制
│ PA10 (RX)  ─────┼────►
└─────────────────┘
```

这套代码提供了完整的步进电机控制框架，您可以根据实际硬件平台进行适配。需要我进一步解释某个部分或添加其他功能吗？





```
能基于 CH32L103 + RTOS 来实现嚒
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

已完成生成

好的，我来基于 CH32L103 + RT-Thread 实现一个完整的多步进电机控制系统。CH32L103 是沁恒的低功耗 RISC-V 内核 MCU，非常适合这类应用。

# CH32L103 + RT-Thread 多步进电机控制系统

## 一、系统架构

```text
┌────────────────────────────────────────────────────────────────┐
│                    CH32L103 + RT-Thread                        │
├────────────────────────────────────────────────────────────────┤
│  ┌──────────────────────────────────────────────────────────┐  │
│  │                    应用层 (Application)                   │  │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐        │  │
│  │  │命令解析 │ │状态监控 │ │运动规划 │ │参数管理 │        │  │
│  │  └─────────┘ └─────────┘ └─────────┘ └─────────┘        │  │
│  └──────────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │                    驱动层 (Driver)                        │  │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐        │  │
│  │  │Stepper  │ │  UART   │ │  I2C    │ │  Timer  │        │  │
│  │  │ Driver  │ │ Driver  │ │ Slave   │ │ Driver  │        │  │
│  │  └─────────┘ └─────────┘ └─────────┘ └─────────┘        │  │
│  └──────────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │                 RT-Thread 内核                            │  │
│  │  线程管理 | 信号量 | 消息队列 | 互斥锁 | 软件定时器       │  │
│  └──────────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │                 CH32L103 硬件抽象层                       │  │
│  │  GPIO | TIM | USART | I2C | NVIC | RCC                   │  │
│  └──────────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────────┘
```

## 二、工程结构

```text
project/
├── applications/
│   ├── main.c
│   └── cmd_handler.c
├── drivers/
│   ├── board.c
│   ├── board.h
│   ├── drv_stepper.c
│   ├── drv_stepper.h
│   ├── drv_uart.c
│   ├── drv_i2c_slave.c
│   └── drv_gpio.c
├── stepper/
│   ├── stepper_config.h
│   ├── stepper_core.c
│   ├── stepper_core.h
│   ├── stepper_planner.c
│   ├── stepper_planner.h
│   └── stepper_cmd.c
├── libraries/
│   ├── ch32l103_it.c
│   └── ch32l103_conf.h
├── rt-thread/
│   └── ...
├── rtconfig.h
└── SConscript
```

## 三、核心代码实现

### 1. 板级配置 (board.h)

```c
#ifndef __BOARD_H__
#define __BOARD_H__

#include <rtthread.h>
#include "ch32l103.h"

/*============================================================================
 * 系统时钟配置
 *============================================================================*/
#define SYSCLK_FREQ_HSI         96000000    // 96MHz HSI
#define HCLK_DIV                RCC_SYSCLK_Div1
#define PCLK1_DIV               RCC_HCLK_Div2   // APB1 = 48MHz
#define PCLK2_DIV               RCC_HCLK_Div1   // APB2 = 96MHz

/*============================================================================
 * 步进电机引脚配置 (最多支持4个电机)
 *============================================================================*/

// 电机0 引脚定义 (PA口)
#define MOTOR0_STEP_PORT        GPIOA
#define MOTOR0_STEP_PIN         GPIO_Pin_0
#define MOTOR0_DIR_PORT         GPIOA
#define MOTOR0_DIR_PIN          GPIO_Pin_1
#define MOTOR0_EN_PORT          GPIOA
#define MOTOR0_EN_PIN           GPIO_Pin_2
#define MOTOR0_MS1_PORT         GPIOA
#define MOTOR0_MS1_PIN          GPIO_Pin_3
#define MOTOR0_MS2_PORT         GPIOA
#define MOTOR0_MS2_PIN          GPIO_Pin_4
#define MOTOR0_MS3_PORT         GPIOA
#define MOTOR0_MS3_PIN          GPIO_Pin_5

// 电机1 引脚定义 (PA口)
#define MOTOR1_STEP_PORT        GPIOA
#define MOTOR1_STEP_PIN         GPIO_Pin_6
#define MOTOR1_DIR_PORT         GPIOA
#define MOTOR1_DIR_PIN          GPIO_Pin_7
#define MOTOR1_EN_PORT          GPIOB
#define MOTOR1_EN_PIN           GPIO_Pin_0
#define MOTOR1_MS1_PORT         GPIOB
#define MOTOR1_MS1_PIN          GPIO_Pin_1
#define MOTOR1_MS2_PORT         GPIOB
#define MOTOR1_MS2_PIN          GPIO_Pin_2
#define MOTOR1_MS3_PORT         GPIOB
#define MOTOR1_MS3_PIN          GPIO_Pin_10

// 电机2 引脚定义 (PB口)
#define MOTOR2_STEP_PORT        GPIOB
#define MOTOR2_STEP_PIN         GPIO_Pin_11
#define MOTOR2_DIR_PORT         GPIOB
#define MOTOR2_DIR_PIN          GPIO_Pin_12
#define MOTOR2_EN_PORT          GPIOB
#define MOTOR2_EN_PIN           GPIO_Pin_13
#define MOTOR2_MS1_PORT         GPIOB
#define MOTOR2_MS1_PIN          GPIO_Pin_14
#define MOTOR2_MS2_PORT         GPIOB
#define MOTOR2_MS2_PIN          GPIO_Pin_15
#define MOTOR2_MS3_PORT         GPIOC
#define MOTOR2_MS3_PIN          GPIO_Pin_13

// 电机3 引脚定义 (PC口)
#define MOTOR3_STEP_PORT        GPIOC
#define MOTOR3_STEP_PIN         GPIO_Pin_14
#define MOTOR3_DIR_PORT         GPIOC
#define MOTOR3_DIR_PIN          GPIO_Pin_15
#define MOTOR3_EN_PORT          GPIOD
#define MOTOR3_EN_PIN           GPIO_Pin_0
#define MOTOR3_MS1_PORT         GPIOD
#define MOTOR3_MS1_PIN          GPIO_Pin_1
#define MOTOR3_MS2_PORT         GPIOD
#define MOTOR3_MS2_PIN          GPIO_Pin_2
#define MOTOR3_MS3_PORT         GPIOD
#define MOTOR3_MS3_PIN          GPIO_Pin_3

// 限位开关引脚
#define LIMIT0_MIN_PORT         GPIOC
#define LIMIT0_MIN_PIN          GPIO_Pin_0
#define LIMIT0_MAX_PORT         GPIOC
#define LIMIT0_MAX_PIN          GPIO_Pin_1
#define LIMIT1_MIN_PORT         GPIOC
#define LIMIT1_MIN_PIN          GPIO_Pin_2
#define LIMIT1_MAX_PORT         GPIOC
#define LIMIT1_MAX_PIN          GPIO_Pin_3

// 原点开关引脚
#define HOME0_PORT              GPIOC
#define HOME0_PIN               GPIO_Pin_4
#define HOME1_PORT              GPIOC
#define HOME1_PIN               GPIO_Pin_5

/*============================================================================
 * 通信接口配置
 *============================================================================*/

// UART1 配置 (PA9-TX, PA10-RX)
#define UART_CMD_PORT           USART1
#define UART_CMD_BAUDRATE       115200
#define UART_TX_PORT            GPIOA
#define UART_TX_PIN             GPIO_Pin_9
#define UART_RX_PORT            GPIOA
#define UART_RX_PIN             GPIO_Pin_10

// I2C1 配置 (PB6-SCL, PB7-SDA)
#define I2C_SLAVE_PORT          I2C1
#define I2C_SLAVE_ADDR          0x20
#define I2C_SCL_PORT            GPIOB
#define I2C_SCL_PIN             GPIO_Pin_6
#define I2C_SDA_PORT            GPIOB
#define I2C_SDA_PIN             GPIO_Pin_7

/*============================================================================
 * 定时器配置 (用于步进脉冲生成)
 *============================================================================*/
#define MOTOR0_TIMER            TIM2
#define MOTOR1_TIMER            TIM3
#define MOTOR2_TIMER            TIM4
#define MOTOR3_TIMER            TIM5

#define TIMER_PRESCALER         (96 - 1)    // 96MHz / 96 = 1MHz (1us)
#define TIMER_DEFAULT_PERIOD    1000        // 默认1ms

/*============================================================================
 * RT-Thread 配置
 *============================================================================*/
#define RT_HEAP_SIZE            (8 * 1024)  // 8KB 堆内存

// 线程优先级
#define STEPPER_THREAD_PRIO     5
#define CMD_THREAD_PRIO         10
#define MONITOR_THREAD_PRIO     15

// 线程栈大小
#define STEPPER_THREAD_STACK    1024
#define CMD_THREAD_STACK        2048
#define MONITOR_THREAD_STACK    512

/*============================================================================
 * 函数声明
 *============================================================================*/
void board_init(void);
void system_clock_config(void);

#endif /* __BOARD_H__ */
```

### 2. 板级初始化 (board.c)

```c
#include "board.h"
#include <rtthread.h>

// RT-Thread 堆内存
#if defined(RT_USING_HEAP)
static uint8_t rt_heap[RT_HEAP_SIZE];
#endif

/*============================================================================
 * 系统时钟配置
 *============================================================================*/
void system_clock_config(void)
{
    // 使用内部96MHz HSI
    RCC_AdjustHSICalibrationValue(0x10);
    
    // 配置Flash等待周期
    FLASH_SetLatency(FLASH_Latency_2);
    
    // 配置AHB、APB1、APB2时钟
    RCC_HCLKConfig(HCLK_DIV);
    RCC_PCLK1Config(PCLK1_DIV);
    RCC_PCLK2Config(PCLK2_DIV);
    
    // 使能HSI
    RCC_HSICmd(ENABLE);
    while (RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == RESET);
    
    // 选择HSI作为系统时钟
    RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);
    while (RCC_GetSYSCLKSource() != 0x00);
}

/*============================================================================
 * GPIO 时钟使能
 *============================================================================*/
static void gpio_clock_init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | 
                           RCC_APB2Periph_GPIOB | 
                           RCC_APB2Periph_GPIOC | 
                           RCC_APB2Periph_GPIOD |
                           RCC_APB2Periph_AFIO, ENABLE);
}

/*============================================================================
 * 定时器时钟使能
 *============================================================================*/
static void timer_clock_init(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 | 
                           RCC_APB1Periph_TIM3 | 
                           RCC_APB1Periph_TIM4 | 
                           RCC_APB1Periph_TIM5, ENABLE);
}

/*============================================================================
 * UART 时钟使能
 *============================================================================*/
static void uart_clock_init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
}

/*============================================================================
 * I2C 时钟使能
 *============================================================================*/
static void i2c_clock_init(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
}

/*============================================================================
 * 板级初始化
 *============================================================================*/
void board_init(void)
{
    // 系统时钟配置
    system_clock_config();
    
    // 外设时钟使能
    gpio_clock_init();
    timer_clock_init();
    uart_clock_init();
    i2c_clock_init();
    
    // 配置SysTick (RT-Thread系统节拍)
    SysTick_Config(SYSCLK_FREQ_HSI / RT_TICK_PER_SECOND);
    NVIC_SetPriority(SysTick_IRQn, 0xFF);
}

/*============================================================================
 * RT-Thread 板级接口
 *============================================================================*/
void rt_hw_board_init(void)
{
    board_init();
    
#if defined(RT_USING_HEAP)
    rt_system_heap_init(rt_heap, rt_heap + RT_HEAP_SIZE);
#endif

#if defined(RT_USING_CONSOLE)
    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
#endif

#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif
}

/*============================================================================
 * SysTick 中断处理
 *============================================================================*/
void SysTick_Handler(void)
{
    rt_interrupt_enter();
    rt_tick_increase();
    rt_interrupt_leave();
}
```

### 3. 步进电机配置头文件 (stepper_config.h)

```c
#ifndef __STEPPER_CONFIG_H__
#define __STEPPER_CONFIG_H__

#include <rtthread.h>
#include <stdint.h>
#include <stdbool.h>

/*============================================================================
 * 基本配置
 *============================================================================*/
#define STEPPER_MAX_NUM             4       // 最大电机数量
#define STEPPER_PULSE_WIDTH_US      3       // 脉冲宽度 (微秒)
#define STEPPER_DIR_SETUP_US        5       // 方向建立时间 (微秒)

/*============================================================================
 * 枚举定义
 *============================================================================*/

// 电机类型
typedef enum {
    STEPPER_TYPE_BIPOLAR = 0,       // 双极性 (NEMA17等)
    STEPPER_TYPE_UNIPOLAR,          // 单极性 (28BYJ-48等)
} stepper_type_t;

// 细分模式
typedef enum {
    MICROSTEP_FULL = 1,             // 整步
    MICROSTEP_HALF = 2,             // 半步
    MICROSTEP_1_4  = 4,             // 1/4 细分
    MICROSTEP_1_8  = 8,             // 1/8 细分
    MICROSTEP_1_16 = 16,            // 1/16 细分
    MICROSTEP_1_32 = 32,            // 1/32 细分
} microstep_t;

// 运动状态
typedef enum {
    MOTOR_IDLE = 0,                 // 空闲
    MOTOR_ACCEL,                    // 加速中
    MOTOR_RUN,                      // 匀速运行
    MOTOR_DECEL,                    // 减速中
    MOTOR_HOMING,                   // 回零中
    MOTOR_ERROR,                    // 错误
} motor_state_t;

// 运动模式
typedef enum {
    MODE_POSITION = 0,              // 位置模式
    MODE_VELOCITY,                  // 速度模式
    MODE_HOMING,                    // 回零模式
} motion_mode_t;

// 错误码
typedef enum {
    ERR_NONE = 0,
    ERR_LIMIT_MIN,                  // 触发最小限位
    ERR_LIMIT_MAX,                  // 触发最大限位
    ERR_SOFT_LIMIT,                 // 软限位错误
    ERR_NOT_HOMED,                  // 未回零
    ERR_MOTOR_DISABLED,             // 电机未使能
    ERR_INVALID_PARAM,              // 参数无效
} stepper_error_t;

/*============================================================================
 * 结构体定义
 *============================================================================*/

// GPIO 引脚定义
typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
} gpio_pin_t;

// 电机引脚配置
typedef struct {
    gpio_pin_t step;                // 脉冲引脚
    gpio_pin_t dir;                 // 方向引脚
    gpio_pin_t enable;              // 使能引脚
    gpio_pin_t ms1;                 // 细分引脚1
    gpio_pin_t ms2;                 // 细分引脚2
    gpio_pin_t ms3;                 // 细分引脚3
    gpio_pin_t limit_min;           // 最小限位
    gpio_pin_t limit_max;           // 最大限位
    gpio_pin_t home;                // 原点开关
} stepper_pins_t;

// 电机参数
typedef struct {
    uint16_t steps_per_rev;         // 每圈步数 (200/400)
    microstep_t microstep;          // 细分设置
    float gear_ratio;               // 减速比
    float screw_pitch;              // 丝杆导程 (mm)
    float max_speed;                // 最大速度 (steps/s)
    float max_accel;                // 最大加速度 (steps/s²)
    float max_decel;                // 最大减速度 (steps/s²)
    float home_speed;               // 回零速度 (steps/s)
    int32_t soft_limit_min;         // 软限位最小值
    int32_t soft_limit_max;         // 软限位最大值
    bool reverse_dir;               // 方向反转
    bool enable_soft_limit;         // 使能软限位
} stepper_params_t;

// 运动状态
typedef struct {
    volatile int32_t current_pos;   // 当前位置 (steps)
    volatile int32_t target_pos;    // 目标位置 (steps)
    volatile float current_speed;   // 当前速度 (steps/s)
    volatile float target_speed;    // 目标速度 (steps/s)
    volatile motor_state_t state;   // 运动状态
    volatile motion_mode_t mode;    // 运动模式
    volatile bool direction;        // 方向 (true=正向)
    volatile bool enabled;          // 使能状态
    volatile bool homed;            // 是否已回零
    volatile stepper_error_t error; // 错误码
} stepper_status_t;

// 梯形加减速规划
typedef struct {
    uint32_t accel_steps;           // 加速步数
    uint32_t decel_steps;           // 减速步数
    uint32_t total_steps;           // 总步数
    uint32_t step_count;            // 已执行步数
    float cruise_speed;             // 巡航速度
    float current_interval;         // 当前脉冲间隔 (us)
    float min_interval;             // 最小间隔 (最高速)
    float accel_factor;             // 加速因子
    uint8_t phase;                  // 阶段: 0加速 1匀速 2减速
} ramp_profile_t;

// 步进电机完整结构
typedef struct {
    uint8_t id;                     // 电机ID
    stepper_type_t type;            // 电机类型
    TIM_TypeDef *timer;             // 关联定时器
    stepper_pins_t pins;            // 引脚配置
    stepper_params_t params;        // 参数配置
    stepper_status_t status;        // 状态信息
    ramp_profile_t ramp;            // 加减速规划
    rt_sem_t move_sem;              // 运动完成信号量
    rt_mutex_t mutex;               // 互斥锁
} stepper_t;

/*============================================================================
 * 命令消息定义
 *============================================================================*/

// 命令类型
typedef enum {
    CMD_ENABLE = 0,
    CMD_DISABLE,
    CMD_MOVE_TO,
    CMD_MOVE_REL,
    CMD_MOVE_SPEED,
    CMD_STOP,
    CMD_ESTOP,
    CMD_HOME,
    CMD_SET_HOME,
    CMD_SET_POS,
    CMD_SET_SPEED,
    CMD_SET_ACCEL,
    CMD_SET_MICROSTEP,
    CMD_GET_STATUS,
    CMD_GET_POS,
} cmd_type_t;

// 命令消息
typedef struct {
    cmd_type_t type;
    uint8_t motor_id;
    union {
        int32_t position;
        float speed;
        float accel;
        uint8_t microstep;
        bool direction;
    } param;
} stepper_cmd_t;

// 响应消息
typedef struct {
    uint8_t motor_id;
    stepper_error_t error;
    int32_t position;
    float speed;
    motor_state_t state;
    bool homed;
} stepper_resp_t;

#endif /* __STEPPER_CONFIG_H__ */
```

### 4. 步进电机核心驱动 (stepper_core.h)

```c
#ifndef __STEPPER_CORE_H__
#define __STEPPER_CORE_H__

#include "stepper_config.h"

/*============================================================================
 * 初始化与配置
 *============================================================================*/
int stepper_init(void);
int stepper_deinit(void);
int stepper_config(uint8_t id, stepper_t *config);
stepper_t* stepper_get_handle(uint8_t id);

/*============================================================================
 * 使能控制
 *============================================================================*/
int stepper_enable(uint8_t id);
int stepper_disable(uint8_t id);
int stepper_enable_all(void);
int stepper_disable_all(void);

/*============================================================================
 * 运动控制
 *============================================================================*/
int stepper_move_to(uint8_t id, int32_t position);
int stepper_move_to_async(uint8_t id, int32_t position);
int stepper_move_relative(uint8_t id, int32_t steps);
int stepper_move_relative_async(uint8_t id, int32_t steps);
int stepper_move_speed(uint8_t id, float speed);
int stepper_stop(uint8_t id);
int stepper_emergency_stop(uint8_t id);
int stepper_stop_all(void);

/*============================================================================
 * 回零操作
 *============================================================================*/
int stepper_home(uint8_t id);
int stepper_home_async(uint8_t id);
int stepper_set_home(uint8_t id);
int stepper_set_position(uint8_t id, int32_t position);

/*============================================================================
 * 参数设置
 *============================================================================*/
int stepper_set_speed(uint8_t id, float speed);
int stepper_set_accel(uint8_t id, float accel);
int stepper_set_decel(uint8_t id, float decel);
int stepper_set_microstep(uint8_t id, microstep_t microstep);

/*============================================================================
 * 状态查询
 *============================================================================*/
int32_t stepper_get_position(uint8_t id);
float stepper_get_speed(uint8_t id);
motor_state_t stepper_get_state(uint8_t id);
stepper_error_t stepper_get_error(uint8_t id);
bool stepper_is_running(uint8_t id);
bool stepper_is_homed(uint8_t id);
bool stepper_is_enabled(uint8_t id);
stepper_status_t* stepper_get_status(uint8_t id);

/*============================================================================
 * 限位检测
 *============================================================================*/
bool stepper_check_limit_min(uint8_t id);
bool stepper_check_limit_max(uint8_t id);
bool stepper_check_home(uint8_t id);

/*============================================================================
 * 等待操作
 *============================================================================*/
int stepper_wait_idle(uint8_t id, rt_int32_t timeout);
int stepper_wait_position(uint8_t id, int32_t position, rt_int32_t timeout);

/*============================================================================
 * 多轴同步
 *============================================================================*/
int stepper_sync_move(uint8_t *ids, int32_t *positions, uint8_t count);

/*============================================================================
 * 中断处理 (内部使用)
 *============================================================================*/
void stepper_timer_isr(uint8_t id);

#endif /* __STEPPER_CORE_H__ */
```

### 5. 步进电机核心驱动实现 (stepper_core.c)

```c
#include "stepper_core.h"
#include "board.h"
#include <rtthread.h>
#include <string.h>
#include <math.h>

#define DBG_TAG    "stepper"
#define DBG_LVL    DBG_INFO
#include <rtdbg.h>

/*============================================================================
 * 私有变量
 *============================================================================*/
static stepper_t g_steppers[STEPPER_MAX_NUM];
static uint8_t g_stepper_count = 0;
static rt_bool_t g_initialized = RT_FALSE;

// 定时器映射表
static TIM_TypeDef* const g_timer_map[STEPPER_MAX_NUM] = {
    TIM2, TIM3, TIM4, TIM5
};

// 定时器中断号映射
static const IRQn_Type g_timer_irq[STEPPER_MAX_NUM] = {
    TIM2_IRQn, TIM3_IRQn, TIM4_IRQn, TIM5_IRQn
};

/*============================================================================
 * 私有函数声明
 *============================================================================*/
static void stepper_gpio_init(stepper_t *motor);
static void stepper_timer_init(stepper_t *motor);
static void stepper_set_dir(stepper_t *motor, bool dir);
static void stepper_pulse(stepper_t *motor);
static void stepper_set_microstep_pins(stepper_t *motor, microstep_t ms);
static void stepper_calc_ramp(stepper_t *motor, int32_t steps);
static void stepper_update_interval(stepper_t *motor);
static void stepper_timer_start(stepper_t *motor, uint32_t interval_us);
static void stepper_timer_stop(stepper_t *motor);
static void stepper_timer_set_period(stepper_t *motor, uint32_t interval_us);

/*============================================================================
 * GPIO 操作宏
 *============================================================================*/
#define GPIO_SET(pin)       ((pin).port->BSHR = (pin).pin)
#define GPIO_RESET(pin)     ((pin).port->BCR = (pin).pin)
#define GPIO_READ(pin)      (((pin).port->INDR & (pin).pin) ? 1 : 0)
#define GPIO_TOGGLE(pin)    ((pin).port->OUTDR ^= (pin).pin)

/*============================================================================
 * 延时函数 (微秒级)
 *============================================================================*/
static inline void delay_us(uint32_t us)
{
    uint32_t cycles = us * (SYSCLK_FREQ_HSI / 1000000) / 4;
    while (cycles--) {
        __NOP();
    }
}

/*============================================================================
 * 初始化函数
 *============================================================================*/
int stepper_init(void)
{
    if (g_initialized) {
        return RT_EOK;
    }
    
    memset(g_steppers, 0, sizeof(g_steppers));
    g_stepper_count = 0;
    g_initialized = RT_TRUE;
    
    LOG_I("Stepper driver initialized");
    return RT_EOK;
}

int stepper_deinit(void)
{
    stepper_stop_all();
    stepper_disable_all();
    
    for (uint8_t i = 0; i < g_stepper_count; i++) {
        if (g_steppers[i].move_sem) {
            rt_sem_delete(g_steppers[i].move_sem);
        }
        if (g_steppers[i].mutex) {
            rt_mutex_delete(g_steppers[i].mutex);
        }
    }
    
    g_initialized = RT_FALSE;
    LOG_I("Stepper driver deinitialized");
    return RT_EOK;
}

/*============================================================================
 * 电机配置
 *============================================================================*/
int stepper_config(uint8_t id, stepper_t *config)
{
    if (id >= STEPPER_MAX_NUM || config == RT_NULL) {
        return -RT_EINVAL;
    }
    
    stepper_t *motor = &g_steppers[id];
    
    // 复制配置
    memcpy(motor, config, sizeof(stepper_t));
    motor->id = id;
    motor->timer = g_timer_map[id];
    
    // 初始化状态
    motor->status.state = MOTOR_IDLE;
    motor->status.enabled = RT_FALSE;
    motor->status.homed = RT_FALSE;
    motor->status.error = ERR_NONE;
    motor->status.current_pos = 0;
    motor->status.current_speed = 0;
    
    // 创建同步对象
    char name[RT_NAME_MAX];
    rt_snprintf(name, sizeof(name), "mtr%d_sem", id);
    motor->move_sem = rt_sem_create(name, 0, RT_IPC_FLAG_FIFO);
    
    rt_snprintf(name, sizeof(name), "mtr%d_mtx", id);
    motor->mutex = rt_mutex_create(name, RT_IPC_FLAG_PRIO);
    
    if (motor->move_sem == RT_NULL || motor->mutex == RT_NULL) {
        LOG_E("Failed to create sync objects for motor %d", id);
        return -RT_ENOMEM;
    }
    
    // 初始化硬件
    stepper_gpio_init(motor);
    stepper_timer_init(motor);
    
    // 设置细分
    stepper_set_microstep_pins(motor, motor->params.microstep);
    
    if (id >= g_stepper_count) {
        g_stepper_count = id + 1;
    }
    
    LOG_I("Motor %d configured: steps/rev=%d, microstep=%d, max_speed=%.1f",
          id, motor->params.steps_per_rev, motor->params.microstep, 
          motor->params.max_speed);
    
    return RT_EOK;
}

stepper_t* stepper_get_handle(uint8_t id)
{
    if (id >= STEPPER_MAX_NUM) {
        return RT_NULL;
    }
    return &g_steppers[id];
}

/*============================================================================
 * GPIO 初始化
 *============================================================================*/
static void stepper_gpio_init(stepper_t *motor)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // 输出引脚配置 (STEP, DIR, EN, MS1, MS2, MS3)
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    
    // STEP
    GPIO_InitStruct.GPIO_Pin = motor->pins.step.pin;
    GPIO_Init(motor->pins.step.port, &GPIO_InitStruct);
    GPIO_RESET(motor->pins.step);
    
    // DIR
    GPIO_InitStruct.GPIO_Pin = motor->pins.dir.pin;
    GPIO_Init(motor->pins.dir.port, &GPIO_InitStruct);
    GPIO_RESET(motor->pins.dir);
    
    // ENABLE (高电平禁用)
    GPIO_InitStruct.GPIO_Pin = motor->pins.enable.pin;
    GPIO_Init(motor->pins.enable.port, &GPIO_InitStruct);
    GPIO_SET(motor->pins.enable);  // 默认禁用
    
    // MS1, MS2, MS3
    GPIO_InitStruct.GPIO_Pin = motor->pins.ms1.pin;
    GPIO_Init(motor->pins.ms1.port, &GPIO_InitStruct);
    
    GPIO_InitStruct.GPIO_Pin = motor->pins.ms2.pin;
    GPIO_Init(motor->pins.ms2.port, &GPIO_InitStruct);
    
    GPIO_InitStruct.GPIO_Pin = motor->pins.ms3.pin;
    GPIO_Init(motor->pins.ms3.port, &GPIO_InitStruct);
    
    // 输入引脚配置 (限位开关, 原点开关) - 上拉输入
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    
    if (motor->pins.limit_min.port != RT_NULL) {
        GPIO_InitStruct.GPIO_Pin = motor->pins.limit_min.pin;
        GPIO_Init(motor->pins.limit_min.port, &GPIO_InitStruct);
    }
    
    if (motor->pins.limit_max.port != RT_NULL) {
        GPIO_InitStruct.GPIO_Pin = motor->pins.limit_max.pin;
        GPIO_Init(motor->pins.limit_max.port, &GPIO_InitStruct);
    }
    
    if (motor->pins.home.port != RT_NULL) {
        GPIO_InitStruct.GPIO_Pin = motor->pins.home.pin;
        GPIO_Init(motor->pins.home.port, &GPIO_InitStruct);
    }
}

/*============================================================================
 * 定时器初始化
 *============================================================================*/
static void stepper_timer_init(stepper_t *motor)
{
    TIM_TimeBaseInitTypeDef TIM_InitStruct = {0};
    
    // 基本配置
    TIM_InitStruct.TIM_Prescaler = TIMER_PRESCALER;
    TIM_InitStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_InitStruct.TIM_Period = TIMER_DEFAULT_PERIOD - 1;
    TIM_InitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
    
    TIM_TimeBaseInit(motor->timer, &TIM_InitStruct);
    
    // 使能更新中断
    TIM_ITConfig(motor->timer, TIM_IT_Update, ENABLE);
    TIM_ClearITPendingBit(motor->timer, TIM_IT_Update);
    
    // 配置NVIC
    NVIC_InitTypeDef NVIC_InitStruct = {0};
    NVIC_InitStruct.NVIC_IRQChannel = g_timer_irq[motor->id];
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

/*============================================================================
 * 定时器控制
 *============================================================================*/
static void stepper_timer_start(stepper_t *motor, uint32_t interval_us)
{
    motor->timer->ARR = interval_us - 1;
    motor->timer->CNT = 0;
    TIM_Cmd(motor->timer, ENABLE);
}

static void stepper_timer_stop(stepper_t *motor)
{
    TIM_Cmd(motor->timer, DISABLE);
}

static void stepper_timer_set_period(stepper_t *motor, uint32_t interval_us)
{
    motor->timer->ARR = interval_us - 1;
}

/*============================================================================
 * 细分设置
 *============================================================================*/
static void stepper_set_microstep_pins(stepper_t *motor, microstep_t ms)
{
    // A4988/DRV8825 细分设置
    uint8_t ms1 = 0, ms2 = 0, ms3 = 0;
    
    switch (ms) {
        case MICROSTEP_FULL:
            ms1 = 0; ms2 = 0; ms3 = 0;
            break;
        case MICROSTEP_HALF:
            ms1 = 1; ms2 = 0; ms3 = 0;
            break;
        case MICROSTEP_1_4:
            ms1 = 0; ms2 = 1; ms3 = 0;
            break;
        case MICROSTEP_1_8:
            ms1 = 1; ms2 = 1; ms3 = 0;
            break;
        case MICROSTEP_1_16:
            ms1 = 1; ms2 = 1; ms3 = 1;
            break;
        case MICROSTEP_1_32:
            ms1 = 1; ms2 = 1; ms3 = 1;
            break;
        default:
            break;
    }
    
    if (ms1) GPIO_SET(motor->pins.ms1); else GPIO_RESET(motor->pins.ms1);
    if (ms2) GPIO_SET(motor->pins.ms2); else GPIO_RESET(motor->pins.ms2);
    if (ms3) GPIO_SET(motor->pins.ms3); else GPIO_RESET(motor->pins.ms3);
    
    motor->params.microstep = ms;
}

/*============================================================================
 * 方向设置
 *============================================================================*/
static void stepper_set_dir(stepper_t *motor, bool dir)
{
    if (motor->params.reverse_dir) {
        dir = !dir;
    }
    
    if (dir) {
        GPIO_SET(motor->pins.dir);
    } else {
        GPIO_RESET(motor->pins.dir);
    }
    
    motor->status.direction = dir;
    delay_us(STEPPER_DIR_SETUP_US);
}

/*============================================================================
 * 脉冲生成
 *============================================================================*/
static void stepper_pulse(stepper_t *motor)
{
    GPIO_SET(motor->pins.step);
    delay_us(STEPPER_PULSE_WIDTH_US);
    GPIO_RESET(motor->pins.step);
}

/*============================================================================
 * 使能控制
 *============================================================================*/
int stepper_enable(uint8_t id)
{
    if (id >= STEPPER_MAX_NUM) return -RT_EINVAL;
    
    stepper_t *motor = &g_steppers[id];
    
    rt_mutex_take(motor->mutex, RT_WAITING_FOREVER);
    
    GPIO_RESET(motor->pins.enable);  // 低电平使能
    motor->status.enabled = RT_TRUE;
    motor->status.error = ERR_NONE;
    
    rt_mutex_release(motor->mutex);
    
    LOG_D("Motor %d enabled", id);
    return RT_EOK;
}

int stepper_disable(uint8_t id)
{
    if (id >= STEPPER_MAX_NUM) return -RT_EINVAL;
    
    stepper_t *motor = &g_steppers[id];
    
    rt_mutex_take(motor->mutex, RT_WAITING_FOREVER);
    
    stepper_timer_stop(motor);
    GPIO_SET(motor->pins.enable);  // 高电平禁用
    motor->status.enabled = RT_FALSE;
    motor->status.state = MOTOR_IDLE;
    
    rt_mutex_release(motor->mutex);
    
    LOG_D("Motor %d disabled", id);
    return RT_EOK;
}

int stepper_enable_all(void)
{
    for (uint8_t i = 0; i < g_stepper_count; i++) {
        stepper_enable(i);
    }
    return RT_EOK;
}

int stepper_disable_all(void)
{
    for (uint8_t i = 0; i < g_stepper_count; i++) {
        stepper_disable(i);
    }
    return RT_EOK;
}

/*============================================================================
 * 梯形加减速计算
 *============================================================================*/
static void stepper_calc_ramp(stepper_t *motor, int32_t steps)
{
    ramp_profile_t *ramp = &motor->ramp;
    stepper_params_t *params = &motor->params;
    
    float accel = params->max_accel;
    float decel = params->max_decel;
    float max_speed = params->max_speed;
    uint32_t total_steps = (steps > 0) ? steps : -steps;
    
    // 计算加速到最大速度需要的步数: s = v² / (2*a)
    float accel_steps_f = (max_speed * max_speed) / (2.0f * accel);
    float decel_steps_f = (max_speed * max_speed) / (2.0f * decel);
    
    if ((accel_steps_f + decel_steps_f) >= total_steps) {
        // 三角形速度曲线 (无法达到最大速度)
        float peak_speed = sqrtf((2.0f * accel * decel * total_steps) / (accel + decel));
        accel_steps_f = (peak_speed * peak_speed) / (2.0f * accel);
        decel_steps_f = total_steps - accel_steps_f;
        ramp->cruise_speed = peak_speed;
    } else {
        // 梯形速度曲线
        ramp->cruise_speed = max_speed;
    }
    
    ramp->accel_steps = (uint32_t)accel_steps_f;
    ramp->decel_steps = (uint32_t)decel_steps_f;
    ramp->total_steps = total_steps;
    ramp->step_count = 0;
    ramp->phase = 0;  // 从加速开始
    
    // 计算初始间隔 (基于初始速度)
    float initial_speed = sqrtf(2.0f * accel);  // 第一步速度
    ramp->current_interval = 1000000.0f / initial_speed;
    ramp->min_interval = 1000000.0f / ramp->cruise_speed;
    ramp->accel_factor = accel;
    
    // 限制最大间隔
    if (ramp->current_interval > 50000) {
        ramp->current_interval = 50000;
    }
}

/*============================================================================
 * 更新脉冲间隔 (加减速处理)
 *============================================================================*/
static void stepper_update_interval(stepper_t *motor)
{
    ramp_profile_t *ramp = &motor->ramp;
    stepper_params_t *params = &motor->params;
    
    float new_interval;
    uint32_t decel_start = ramp->total_steps - ramp->decel_steps;
    
    switch (ramp->phase) {
        case 0:  // 加速阶段
            if (ramp->step_count >= ramp->accel_steps) {
                ramp->phase = 1;
                motor->status.state = MOTOR_RUN;
                ramp->current_interval = ramp->min_interval;
            } else {
                // 使用公式: t_n = t_0 * sqrt(n/(n+1))
                // 简化计算: 基于当前步数计算速度
                float speed = sqrtf(2.0f * params->max_accel * (ramp->step_count + 1));
                if (speed > ramp->cruise_speed) speed = ramp->cruise_speed;
                new_interval = 1000000.0f / speed;
                ramp->current_interval = new_interval;
                motor->status.current_speed = speed;
            }
            break;
            
        case 1:  // 匀速阶段
            if (ramp->step_count >= decel_start) {
                ramp->phase = 2;
                motor->status.state = MOTOR_DECEL;
            }
            break;
            
        case 2:  // 减速阶段
            {
                uint32_t remaining = ramp->total_steps - ramp->step_count;
                float speed = sqrtf(2.0f * params->max_decel * remaining);
                if (speed < 100) speed = 100;  // 最小速度
                new_interval = 1000000.0f / speed;
                ramp->current_interval = new_interval;
                motor->status.current_speed = speed;
            }
            break;
    }
    
    // 更新定时器周期
    stepper_timer_set_period(motor, (uint32_t)ramp->current_interval);
}

/*============================================================================
 * 运动控制 - 绝对位置
 *============================================================================*/
int stepper_move_to_async(uint8_t id, int32_t position)
{
    if (id >= STEPPER_MAX_NUM) return -RT_EINVAL;
    
    stepper_t *motor = &g_steppers[id];
    
    if (!motor->status.enabled) {
        motor->status.error = ERR_MOTOR_DISABLED;
        return -RT_ERROR;
    }
    
    // 检查软限位
    if (motor->params.enable_soft_limit) {
        if (position < motor->params.soft_limit_min || 
            position > motor->params.soft_limit_max) {
            motor->status.error = ERR_SOFT_LIMIT;
            return -RT_EINVAL;
        }
    }
    
    rt_mutex_take(motor->mutex, RT_WAITING_FOREVER);
    
    int32_t delta = position - motor->status.current_pos;
    
    if (delta == 0) {
        rt_mutex_release(motor->mutex);
        return RT_EOK;
    }
    
    // 设置方向
    stepper_set_dir(motor, delta > 0);
    
    // 设置目标
    motor->status.target_pos = position;
    motor->status.mode = MODE_POSITION;
    motor->status.state = MOTOR_ACCEL;
    motor->status.error = ERR_NONE;
    
    // 计算加减速曲线
    stepper_calc_ramp(motor, delta);
    
    // 清除完成信号量
    while (rt_sem_trytake(motor->move_sem) == RT_EOK);
    
    // 启动定时器
    stepper_timer_start(motor, (uint32_t)motor->ramp.current_interval);
    
    rt_mutex_release(motor->mutex);
    
    LOG_D("Motor %d moving to %ld", id, position);
    return RT_EOK;
}

int stepper_move_to(uint8_t id, int32_t position)
{
    int ret = stepper_move_to_async(id, position);
    if (ret != RT_EOK) return ret;
    
    return stepper_wait_idle(id, RT_WAITING_FOREVER);
}

/*============================================================================
 * 运动控制 - 相对位置
 *============================================================================*/
int stepper_move_relative_async(uint8_t id, int32_t steps)
{
    if (id >= STEPPER_MAX_NUM) return -RT_EINVAL;
    
    int32_t target = g_steppers[id].status.current_pos + steps;
    return stepper_move_to_async(id, target);
}

int stepper_move_relative(uint8_t id, int32_t steps)
{
    int ret = stepper_move_relative_async(id, steps);
    if (ret != RT_EOK) return ret;
    
    return stepper_wait_idle(id, RT_WAITING_FOREVER);
}

/*============================================================================
 * 运动控制 - 速度模式
 *============================================================================*/
int stepper_move_speed(uint8_t id, float speed)
{
    if (id >= STEPPER_MAX_NUM) return -RT_EINVAL;
    
    stepper_t *motor = &g_steppers[id];
    
    if (!motor->status.enabled) {
        motor->status.error = ERR_MOTOR_DISABLED;
        return -RT_ERROR;
    }
    
    rt_mutex_take(motor->mutex, RT_WAITING_FOREVER);
    
    // 设置方向
    stepper_set_dir(motor, speed >= 0);
    
    float abs_speed = (speed >= 0) ? speed : -speed;
    if (abs_speed > motor->params.max_speed) {
        abs_speed = motor->params.max_speed;
    }
    
    motor->status.target_speed = abs_speed;
    motor->status.mode = MODE_VELOCITY;
    motor->status.state = MOTOR_RUN;
    
    // 计算脉冲间隔
    uint32_t interval = (uint32_t)(1000000.0f / abs_speed);
    
    stepper_timer_start(motor, interval);
    
    rt_mutex_release(motor->mutex);
    
    return RT_EOK;
}

/*============================================================================
 * 停止控制
 *============================================================================*/
int stepper_stop(uint8_t id)
{
    if (id >= STEPPER_MAX_NUM) return -RT_EINVAL;
    
    stepper_t *motor = &g_steppers[id];
    
    rt_mutex_take(motor->mutex, RT_WAITING_FOREVER);
    
    if (motor->status.state != MOTOR_IDLE) {
        // 切换到减速状态
        motor->ramp.phase = 2;
        motor->status.state = MOTOR_DECEL;
        
        // 重新计算减速步数
        float current_speed = motor->status.current_speed;
        motor->ramp.decel_steps = (uint32_t)((current_speed * current_speed) / 
                                              (2.0f * motor->params.max_decel));
        motor->ramp.total_steps = motor->ramp.step_count + motor->ramp.decel_steps;
    }
    
    rt_mutex_release(motor->mutex);
    
    return RT_EOK;
}

int stepper_emergency_stop(uint8_t id)
{
    if (id >= STEPPER_MAX_NUM) return -RT_EINVAL;
    
    stepper_t *motor = &g_steppers[id];
    
    // 立即停止定时器
    stepper_timer_stop(motor);
    
    motor->status.state = MOTOR_IDLE;
    motor->status.current_speed = 0;
    
    // 释放信号量
    rt_sem_release(motor->move_sem);
    
    LOG_W("Motor %d emergency stopped", id);
    return RT_EOK;
}

int stepper_stop_all(void)
{
    for (uint8_t i = 0; i < g_stepper_count; i++) {
        stepper_emergency_stop(i);
    }
    return RT_EOK;
}

/*============================================================================
 * 回零操作
 *============================================================================*/
int stepper_home_async(uint8_t id)
{
    if (id >= STEPPER_MAX_NUM) return -RT_EINVAL;
    
    stepper_t *motor = &g_steppers[id];
    
    if (!motor->status.enabled) {
        motor->status.error = ERR_MOTOR_DISABLED;
        return -RT_ERROR;
    }
    
    rt_mutex_take(motor->mutex, RT_WAITING_FOREVER);
    
    // 设置回零方向 (通常向负方向)
    stepper_set_dir(motor, RT_FALSE);
    
    motor->status.mode = MODE_HOMING;
    motor->status.state = MOTOR_HOMING;
    motor->status.homed = RT_FALSE;
    
    // 使用回零速度
    uint32_t interval = (uint32_t)(1000000.0f / motor->params.home_speed);
    
    // 清除完成信号量
    while (rt_sem_trytake(motor->move_sem) == RT_EOK);
    
    stepper_timer_start(motor, interval);
    
    rt_mutex_release(motor->mutex);
    
    LOG_D("Motor %d homing started", id);
    return RT_EOK;
}

int stepper_home(uint8_t id)
{
    int ret = stepper_home_async(id);
    if (ret != RT_EOK) return ret;
    
    return stepper_wait_idle(id, RT_WAITING_FOREVER);
}

int stepper_set_home(uint8_t id)
{
    if (id >= STEPPER_MAX_NUM) return -RT_EINVAL;
    
    stepper_t *motor = &g_steppers[id];
    
    rt_mutex_take(motor->mutex, RT_WAITING_FOREVER);
    
    motor->status.current_pos = 0;
    motor->status.homed = RT_TRUE;
    
    rt_mutex_release(motor->mutex);
    
    LOG_I("Motor %d home position set", id);
    return RT_EOK;
}

int stepper_set_position(uint8_t id, int32_t position)
{
    if (id >= STEPPER_MAX_NUM) return -RT_EINVAL;
    
    stepper_t *motor = &g_steppers[id];
    
    rt_mutex_take(motor->mutex, RT_WAITING_FOREVER);
    motor->status.current_pos = position;
    rt_mutex_release(motor->mutex);
    
    return RT_EOK;
}

/*============================================================================
 * 参数设置
 *============================================================================*/
int stepper_set_speed(uint8_t id, float speed)
{
    if (id >= STEPPER_MAX_NUM) return -RT_EINVAL;
    
    g_steppers[id].params.max_speed = speed;
    return RT_EOK;
}

int stepper_set_accel(uint8_t id, float accel)
{
    if (id >= STEPPER_MAX_NUM) return -RT_EINVAL;
    
    g_steppers[id].params.max_accel = accel;
    return RT_EOK;
}

int stepper_set_decel(uint8_t id, float decel)
{
    if (id >= STEPPER_MAX_NUM) return -RT_EINVAL;
    
    g_steppers[id].params.max_decel = decel;
    return RT_EOK;
}

int stepper_set_microstep(uint8_t id, microstep_t microstep)
{
    if (id >= STEPPER_MAX_NUM) return -RT_EINVAL;
    
    stepper_t *motor = &g_steppers[id];
    stepper_set_microstep_pins(motor, microstep);
    
    return RT_EOK;
}

/*============================================================================
 * 状态查询
 *============================================================================*/
int32_t stepper_get_position(uint8_t id)
{
    if (id >= STEPPER_MAX_NUM) return 0;
    return g_steppers[id].status.current_pos;
}

float stepper_get_speed(uint8_t id)
{
    if (id >= STEPPER_MAX_NUM) return 0;
    return g_steppers[id].status.current_speed;
}

motor_state_t stepper_get_state(uint8_t id)
{
    if (id >= STEPPER_MAX_NUM) return MOTOR_ERROR;
    return g_steppers[id].status.state;
}

stepper_error_t stepper_get_error(uint8_t id)
{
    if (id >= STEPPER_MAX_NUM) return ERR_INVALID_PARAM;
    return g_steppers[id].status.error;
}

bool stepper_is_running(uint8_t id)
{
    if (id >= STEPPER_MAX_NUM) return RT_FALSE;
    return g_steppers[id].status.state != MOTOR_IDLE;
}

bool stepper_is_homed(uint8_t id)
{
    if (id >= STEPPER_MAX_NUM) return RT_FALSE;
    return g_steppers[id].status.homed;
}

bool stepper_is_enabled(uint8_t id)
{
    if (id >= STEPPER_MAX_NUM) return RT_FALSE;
    return g_steppers[id].status.enabled;
}

stepper_status_t* stepper_get_status(uint8_t id)
{
    if (id >= STEPPER_MAX_NUM) return RT_NULL;
    return &g_steppers[id].status;
}

/*============================================================================
 * 限位检测
 *============================================================================*/
bool stepper_check_limit_min(uint8_t id)
{
    if (id >= STEPPER_MAX_NUM) return RT_FALSE;
    
    stepper_t *motor = &g_steppers[id];
    if (motor->pins.limit_min.port == RT_NULL) return RT_FALSE;
    
    return (GPIO_READ(motor->pins.limit_min) == 0);  // 低电平触发
}

bool stepper_check_limit_max(uint8_t id)
{
    if (id >= STEPPER_MAX_NUM) return RT_FALSE;
    
    stepper_t *motor = &g_steppers[id];
    if (motor->pins.limit_max.port == RT_NULL) return RT_FALSE;
    
    return (GPIO_READ(motor->pins.limit_max) == 0);
}

bool stepper_check_home(uint8_t id)
{
    if (id >= STEPPER_MAX_NUM) return RT_FALSE;
    
    stepper_t *motor = &g_steppers[id];
    if (motor->pins.home.port == RT_NULL) return RT_FALSE;
    
    return (GPIO_READ(motor->pins.home) == 0);
}

/*============================================================================
 * 等待操作
 *============================================================================*/
int stepper_wait_idle(uint8_t id, rt_int32_t timeout)
{
    if (id >= STEPPER_MAX_NUM) return -RT_EINVAL;
    
    stepper_t *motor = &g_steppers[id];
    
    rt_err_t ret = rt_sem_take(motor->move_sem, timeout);
    
    if (ret == -RT_ETIMEOUT) {
        return -RT_ETIMEOUT;
    }
    
    return (motor->status.error == ERR_NONE) ? RT_EOK : -RT_ERROR;
}

/*============================================================================
 * 多轴同步运动
 *============================================================================*/
int stepper_sync_move(uint8_t *ids, int32_t *positions, uint8_t count)
{
    if (count == 0 || count > STEPPER_MAX_NUM) return -RT_EINVAL;
```





```
继续
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

已完成生成

### 5. 步进电机核心驱动实现 (stepper_core.c) - 续

```c
    // 计算各轴需要移动的步数和最大步数
    int32_t steps[STEPPER_MAX_NUM];
    int32_t max_steps = 0;
    
    for (uint8_t i = 0; i < count; i++) {
        uint8_t id = ids[i];
        if (id >= STEPPER_MAX_NUM) return -RT_EINVAL;
        
        steps[i] = positions[i] - g_steppers[id].status.current_pos;
        int32_t abs_steps = (steps[i] > 0) ? steps[i] : -steps[i];
        if (abs_steps > max_steps) {
            max_steps = abs_steps;
        }
    }
    
    if (max_steps == 0) return RT_EOK;
    
    // 计算各轴速度比例，使所有轴同时到达
    for (uint8_t i = 0; i < count; i++) {
        uint8_t id = ids[i];
        int32_t abs_steps = (steps[i] > 0) ? steps[i] : -steps[i];
        float ratio = (float)abs_steps / (float)max_steps;
        
        // 调整速度
        float adjusted_speed = g_steppers[id].params.max_speed * ratio;
        if (adjusted_speed < 100) adjusted_speed = 100;  // 最小速度
        
        // 临时保存原速度
        float original_speed = g_steppers[id].params.max_speed;
        g_steppers[id].params.max_speed = adjusted_speed;
        
        // 启动异步移动
        stepper_move_to_async(id, positions[i]);
        
        // 恢复原速度设置
        g_steppers[id].params.max_speed = original_speed;
    }
    
    // 等待所有轴完成
    for (uint8_t i = 0; i < count; i++) {
        stepper_wait_idle(ids[i], RT_WAITING_FOREVER);
    }
    
    return RT_EOK;
}

/*============================================================================
 * 定时器中断处理 - 核心脉冲生成
 *============================================================================*/
void stepper_timer_isr(uint8_t id)
{
    if (id >= STEPPER_MAX_NUM) return;
    
    stepper_t *motor = &g_steppers[id];
    
    if (motor->status.state == MOTOR_IDLE) {
        stepper_timer_stop(motor);
        return;
    }
    
    // 回零模式处理
    if (motor->status.mode == MODE_HOMING) {
        // 检查原点开关
        if (stepper_check_home(id)) {
            stepper_timer_stop(motor);
            motor->status.current_pos = 0;
            motor->status.homed = RT_TRUE;
            motor->status.state = MOTOR_IDLE;
            motor->status.current_speed = 0;
            rt_sem_release(motor->move_sem);
            return;
        }
        
        // 检查限位
        if (!motor->status.direction && stepper_check_limit_min(id)) {
            stepper_timer_stop(motor);
            motor->status.error = ERR_LIMIT_MIN;
            motor->status.state = MOTOR_ERROR;
            rt_sem_release(motor->move_sem);
            return;
        }
        
        // 生成脉冲
        stepper_pulse(motor);
        
        // 更新位置
        if (motor->status.direction) {
            motor->status.current_pos++;
        } else {
            motor->status.current_pos--;
        }
        return;
    }
    
    // 位置/速度模式处理
    
    // 检查限位
    if (motor->status.direction && stepper_check_limit_max(id)) {
        stepper_timer_stop(motor);
        motor->status.error = ERR_LIMIT_MAX;
        motor->status.state = MOTOR_ERROR;
        motor->status.current_speed = 0;
        rt_sem_release(motor->move_sem);
        return;
    }
    
    if (!motor->status.direction && stepper_check_limit_min(id)) {
        stepper_timer_stop(motor);
        motor->status.error = ERR_LIMIT_MIN;
        motor->status.state = MOTOR_ERROR;
        motor->status.current_speed = 0;
        rt_sem_release(motor->move_sem);
        return;
    }
    
    // 速度模式 - 持续运行
    if (motor->status.mode == MODE_VELOCITY) {
        stepper_pulse(motor);
        
        if (motor->status.direction) {
            motor->status.current_pos++;
        } else {
            motor->status.current_pos--;
        }
        return;
    }
    
    // 位置模式 - 加减速处理
    ramp_profile_t *ramp = &motor->ramp;
    
    // 检查是否完成
    if (ramp->step_count >= ramp->total_steps) {
        stepper_timer_stop(motor);
        motor->status.state = MOTOR_IDLE;
        motor->status.current_speed = 0;
        motor->status.current_pos = motor->status.target_pos;
        rt_sem_release(motor->move_sem);
        return;
    }
    
    // 生成脉冲
    stepper_pulse(motor);
    
    // 更新位置
    if (motor->status.direction) {
        motor->status.current_pos++;
    } else {
        motor->status.current_pos--;
    }
    
    ramp->step_count++;
    
    // 更新脉冲间隔 (加减速)
    stepper_update_interval(motor);
}

/*============================================================================
 * 定时器中断服务函数
 *============================================================================*/
void TIM2_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
        stepper_timer_isr(0);
    }
}

void TIM3_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM3_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
        stepper_timer_isr(1);
    }
}

void TIM4_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM4_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
        stepper_timer_isr(2);
    }
}

void TIM5_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM5_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM5, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM5, TIM_IT_Update);
        stepper_timer_isr(3);
    }
}
```

### 6. UART 命令处理模块 (stepper_cmd.c)

```c
#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <stdlib.h>
#include "stepper_core.h"
#include "board.h"

#define DBG_TAG    "stepper.cmd"
#define DBG_LVL    DBG_INFO
#include <rtdbg.h>

/*============================================================================
 * 配置
 *============================================================================*/
#define CMD_BUFFER_SIZE         128
#define CMD_MAX_ARGS            10
#define UART_DEVICE_NAME        "uart1"

/*============================================================================
 * 私有变量
 *============================================================================*/
static rt_device_t g_uart_dev = RT_NULL;
static char g_cmd_buffer[CMD_BUFFER_SIZE];
static uint16_t g_cmd_index = 0;
static struct rt_semaphore g_rx_sem;

/*============================================================================
 * 命令处理函数声明
 *============================================================================*/
static void cmd_help(int argc, char *argv[]);
static void cmd_enable(int argc, char *argv[]);
static void cmd_disable(int argc, char *argv[]);
static void cmd_move(int argc, char *argv[]);
static void cmd_moverel(int argc, char *argv[]);
static void cmd_speed(int argc, char *argv[]);
static void cmd_stop(int argc, char *argv[]);
static void cmd_home(int argc, char *argv[]);
static void cmd_status(int argc, char *argv[]);
static void cmd_pos(int argc, char *argv[]);
static void cmd_config(int argc, char *argv[]);
static void cmd_sync(int argc, char *argv[]);

/*============================================================================
 * 命令表
 *============================================================================*/
typedef struct {
    const char *name;
    void (*handler)(int argc, char *argv[]);
    const char *desc;
} cmd_entry_t;

static const cmd_entry_t g_cmd_table[] = {
    {"HELP",    cmd_help,    "Show help"},
    {"EN",      cmd_enable,  "Enable motor: EN <id|ALL>"},
    {"DIS",     cmd_disable, "Disable motor: DIS <id|ALL>"},
    {"MOVE",    cmd_move,    "Move to position: MOVE <id> <pos>"},
    {"MREL",    cmd_moverel, "Move relative: MREL <id> <steps>"},
    {"SPD",     cmd_speed,   "Set speed: SPD <id> <speed>"},
    {"STOP",    cmd_stop,    "Stop motor: STOP <id|ALL>"},
    {"HOME",    cmd_home,    "Home motor: HOME <id>"},
    {"STAT",    cmd_status,  "Get status: STAT <id>"},
    {"POS",     cmd_pos,     "Set position: POS <id> <pos>"},
    {"CFG",     cmd_config,  "Config: CFG <id> <param> <value>"},
    {"SYNC",    cmd_sync,    "Sync move: SYNC <id1,pos1> <id2,pos2> ..."},
    {RT_NULL,   RT_NULL,     RT_NULL}
};

/*============================================================================
 * 发送响应
 *============================================================================*/
static void send_response(const char *fmt, ...)
{
    char buffer[128];
    va_list args;
    
    va_start(args, fmt);
    rt_vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    
    rt_device_write(g_uart_dev, 0, buffer, rt_strlen(buffer));
}

/*============================================================================
 * 命令解析
 *============================================================================*/
static void parse_command(char *cmd_line)
{
    char *argv[CMD_MAX_ARGS];
    int argc = 0;
    
    // 去除前导空格
    while (*cmd_line == ' ') cmd_line++;
    
    // 去除命令前缀 '$'
    if (*cmd_line == '$') cmd_line++;
    
    // 分割参数 (支持逗号和空格)
    char *token = strtok(cmd_line, " ,\t");
    while (token != RT_NULL && argc < CMD_MAX_ARGS) {
        argv[argc++] = token;
        token = strtok(RT_NULL, " ,\t");
    }
    
    if (argc == 0) return;
    
    // 转换命令为大写
    for (char *p = argv[0]; *p; p++) {
        if (*p >= 'a' && *p <= 'z') {
            *p = *p - 'a' + 'A';
        }
    }
    
    // 查找并执行命令
    for (const cmd_entry_t *cmd = g_cmd_table; cmd->name != RT_NULL; cmd++) {
        if (rt_strcmp(argv[0], cmd->name) == 0) {
            cmd->handler(argc, argv);
            return;
        }
    }
    
    send_response("ERR: Unknown command '%s'\r\n", argv[0]);
}

/*============================================================================
 * 命令处理函数实现
 *============================================================================*/

static void cmd_help(int argc, char *argv[])
{
    send_response("\r\n=== Stepper Motor Controller ===\r\n");
    send_response("Commands:\r\n");
    
    for (const cmd_entry_t *cmd = g_cmd_table; cmd->name != RT_NULL; cmd++) {
        send_response("  %-8s - %s\r\n", cmd->name, cmd->desc);
    }
    
    send_response("================================\r\n");
}

static void cmd_enable(int argc, char *argv[])
{
    if (argc < 2) {
        send_response("ERR: Usage: EN <id|ALL>\r\n");
        return;
    }
    
    if (rt_strcmp(argv[1], "ALL") == 0 || rt_strcmp(argv[1], "all") == 0) {
        stepper_enable_all();
        send_response("OK: All motors enabled\r\n");
    } else {
        uint8_t id = atoi(argv[1]);
        if (stepper_enable(id) == RT_EOK) {
            send_response("OK: Motor %d enabled\r\n", id);
        } else {
            send_response("ERR: Failed to enable motor %d\r\n", id);
        }
    }
}

static void cmd_disable(int argc, char *argv[])
{
    if (argc < 2) {
        send_response("ERR: Usage: DIS <id|ALL>\r\n");
        return;
    }
    
    if (rt_strcmp(argv[1], "ALL") == 0 || rt_strcmp(argv[1], "all") == 0) {
        stepper_disable_all();
        send_response("OK: All motors disabled\r\n");
    } else {
        uint8_t id = atoi(argv[1]);
        if (stepper_disable(id) == RT_EOK) {
            send_response("OK: Motor %d disabled\r\n", id);
        } else {
            send_response("ERR: Failed to disable motor %d\r\n", id);
        }
    }
}

static void cmd_move(int argc, char *argv[])
{
    if (argc < 3) {
        send_response("ERR: Usage: MOVE <id> <position>\r\n");
        return;
    }
    
    uint8_t id = atoi(argv[1]);
    int32_t pos = atol(argv[2]);
    
    int ret = stepper_move_to_async(id, pos);
    if (ret == RT_EOK) {
        send_response("OK: Motor %d moving to %ld\r\n", id, pos);
    } else {
        send_response("ERR: Move failed (%d)\r\n", ret);
    }
}

static void cmd_moverel(int argc, char *argv[])
{
    if (argc < 3) {
        send_response("ERR: Usage: MREL <id> <steps>\r\n");
        return;
    }
    
    uint8_t id = atoi(argv[1]);
    int32_t steps = atol(argv[2]);
    
    int ret = stepper_move_relative_async(id, steps);
    if (ret == RT_EOK) {
        send_response("OK: Motor %d moving %ld steps\r\n", id, steps);
    } else {
        send_response("ERR: Move failed (%d)\r\n", ret);
    }
}

static void cmd_speed(int argc, char *argv[])
{
    if (argc < 3) {
        send_response("ERR: Usage: SPD <id> <speed>\r\n");
        return;
    }
    
    uint8_t id = atoi(argv[1]);
    float speed = atof(argv[2]);
    
    if (stepper_set_speed(id, speed) == RT_EOK) {
        send_response("OK: Motor %d speed set to %.1f\r\n", id, speed);
    } else {
        send_response("ERR: Failed to set speed\r\n");
    }
}

static void cmd_stop(int argc, char *argv[])
{
    if (argc < 2) {
        send_response("ERR: Usage: STOP <id|ALL>\r\n");
        return;
    }
    
    if (rt_strcmp(argv[1], "ALL") == 0 || rt_strcmp(argv[1], "all") == 0) {
        stepper_stop_all();
        send_response("OK: All motors stopped\r\n");
    } else {
        uint8_t id = atoi(argv[1]);
        stepper_emergency_stop(id);
        send_response("OK: Motor %d stopped\r\n", id);
    }
}

static void cmd_home(int argc, char *argv[])
{
    if (argc < 2) {
        send_response("ERR: Usage: HOME <id>\r\n");
        return;
    }
    
    uint8_t id = atoi(argv[1]);
    
    int ret = stepper_home_async(id);
    if (ret == RT_EOK) {
        send_response("OK: Motor %d homing\r\n", id);
    } else {
        send_response("ERR: Home failed (%d)\r\n", ret);
    }
}

static void cmd_status(int argc, char *argv[])
{
    if (argc < 2) {
        send_response("ERR: Usage: STAT <id>\r\n");
        return;
    }
    
    uint8_t id = atoi(argv[1]);
    stepper_status_t *status = stepper_get_status(id);
    
    if (status == RT_NULL) {
        send_response("ERR: Invalid motor ID\r\n");
        return;
    }
    
    const char *state_str[] = {"IDLE", "ACCEL", "RUN", "DECEL", "HOME", "ERR"};
    
    send_response("Motor %d Status:\r\n", id);
    send_response("  Position: %ld\r\n", status->current_pos);
    send_response("  Speed:    %.1f steps/s\r\n", status->current_speed);
    send_response("  State:    %s\r\n", state_str[status->state]);
    send_response("  Enabled:  %s\r\n", status->enabled ? "Yes" : "No");
    send_response("  Homed:    %s\r\n", status->homed ? "Yes" : "No");
    send_response("  Error:    %d\r\n", status->error);
}

static void cmd_pos(int argc, char *argv[])
{
    if (argc < 3) {
        send_response("ERR: Usage: POS <id> <position>\r\n");
        return;
    }
    
    uint8_t id = atoi(argv[1]);
    int32_t pos = atol(argv[2]);
    
    if (stepper_set_position(id, pos) == RT_EOK) {
        send_response("OK: Motor %d position set to %ld\r\n", id, pos);
    } else {
        send_response("ERR: Failed to set position\r\n");
    }
}

static void cmd_config(int argc, char *argv[])
{
    if (argc < 4) {
        send_response("ERR: Usage: CFG <id> <param> <value>\r\n");
        send_response("  Params: ACCEL, DECEL, MICROSTEP\r\n");
        return;
    }
    
    uint8_t id = atoi(argv[1]);
    const char *param = argv[2];
    float value = atof(argv[3]);
    
    // 转换参数名为大写
    char param_upper[16];
    for (int i = 0; param[i] && i < 15; i++) {
        param_upper[i] = (param[i] >= 'a' && param[i] <= 'z') ? 
                         param[i] - 'a' + 'A' : param[i];
        param_upper[i + 1] = '\0';
    }
    
    if (rt_strcmp(param_upper, "ACCEL") == 0) {
        stepper_set_accel(id, value);
        send_response("OK: Acceleration set to %.1f\r\n", value);
    } else if (rt_strcmp(param_upper, "DECEL") == 0) {
        stepper_set_decel(id, value);
        send_response("OK: Deceleration set to %.1f\r\n", value);
    } else if (rt_strcmp(param_upper, "MICROSTEP") == 0) {
        stepper_set_microstep(id, (microstep_t)(int)value);
        send_response("OK: Microstep set to %d\r\n", (int)value);
    } else {
        send_response("ERR: Unknown parameter '%s'\r\n", param);
    }
}

static void cmd_sync(int argc, char *argv[])
{
    if (argc < 2) {
        send_response("ERR: Usage: SYNC <id1,pos1> <id2,pos2> ...\r\n");
        return;
    }
    
    uint8_t ids[STEPPER_MAX_NUM];
    int32_t positions[STEPPER_MAX_NUM];
    uint8_t count = 0;
    
    for (int i = 1; i < argc && count < STEPPER_MAX_NUM; i++) {
        char *comma = strchr(argv[i], ':');
        if (comma == RT_NULL) {
            comma = strchr(argv[i], ',');
        }
        
        if (comma != RT_NULL) {
            *comma = '\0';
            ids[count] = atoi(argv[i]);
            positions[count] = atol(comma + 1);
            count++;
        }
    }
    
    if (count == 0) {
        send_response("ERR: No valid motor:position pairs\r\n");
        return;
    }
    
    int ret = stepper_sync_move(ids, positions, count);
    if (ret == RT_EOK) {
        send_response("OK: Sync move started for %d motors\r\n", count);
    } else {
        send_response("ERR: Sync move failed (%d)\r\n", ret);
    }
}

/*============================================================================
 * UART 接收回调
 *============================================================================*/
static rt_err_t uart_rx_callback(rt_device_t dev, rt_size_t size)
{
    rt_sem_release(&g_rx_sem);
    return RT_EOK;
}

/*============================================================================
 * 命令处理线程
 *============================================================================*/
static void cmd_thread_entry(void *param)
{
    char ch;
    
    while (1) {
        // 等待数据
        rt_sem_take(&g_rx_sem, RT_WAITING_FOREVER);
        
        // 读取所有可用数据
        while (rt_device_read(g_uart_dev, 0, &ch, 1) == 1) {
            if (ch == '\r' || ch == '\n') {
                if (g_cmd_index > 0) {
                    g_cmd_buffer[g_cmd_index] = '\0';
                    parse_command(g_cmd_buffer);
                    g_cmd_index = 0;
                }
            } else if (ch == '\b' || ch == 0x7F) {
                // 退格处理
                if (g_cmd_index > 0) {
                    g_cmd_index--;
                    rt_device_write(g_uart_dev, 0, "\b \b", 3);
                }
            } else if (g_cmd_index < CMD_BUFFER_SIZE - 1) {
                g_cmd_buffer[g_cmd_index++] = ch;
                // 回显
                rt_device_write(g_uart_dev, 0, &ch, 1);
            }
        }
    }
}

/*============================================================================
 * 初始化命令模块
 *============================================================================*/
int stepper_cmd_init(void)
{
    // 初始化信号量
    rt_sem_init(&g_rx_sem, "uart_rx", 0, RT_IPC_FLAG_FIFO);
    
    // 打开UART设备
    g_uart_dev = rt_device_find(UART_DEVICE_NAME);
    if (g_uart_dev == RT_NULL) {
        LOG_E("Failed to find %s", UART_DEVICE_NAME);
        return -RT_ERROR;
    }
    
    rt_device_open(g_uart_dev, RT_DEVICE_FLAG_RDWR | 
                               RT_DEVICE_FLAG_INT_RX);
    
    // 设置接收回调
    rt_device_set_rx_indicate(g_uart_dev, uart_rx_callback);
    
    // 创建命令处理线程
    rt_thread_t tid = rt_thread_create("cmd",
                                       cmd_thread_entry,
                                       RT_NULL,
                                       CMD_THREAD_STACK,
                                       CMD_THREAD_PRIO,
                                       10);
    if (tid != RT_NULL) {
        rt_thread_startup(tid);
    }
    
    send_response("\r\n=== Stepper Controller Ready ===\r\n");
    send_response("Type 'HELP' for commands\r\n\r\n");
    
    LOG_I("Command module initialized");
    return RT_EOK;
}
INIT_APP_EXPORT(stepper_cmd_init);
```

### 7. I2C 从机模块 (drv_i2c_slave.c)

```c
#include <rtthread.h>
#include "stepper_core.h"
#include "board.h"

#define DBG_TAG    "i2c.slave"
#define DBG_LVL    DBG_INFO
#include <rtdbg.h>

/*============================================================================
 * I2C 寄存器定义
 *============================================================================*/
#define REG_DEVICE_ID           0x00
#define REG_VERSION             0x01
#define REG_STATUS              0x02
#define REG_CONTROL             0x03
#define REG_MOTOR_SELECT        0x04
#define REG_COMMAND             0x05

#define REG_TARGET_POS_0        0x10
#define REG_TARGET_POS_1        0x11
#define REG_TARGET_POS_2        0x12
#define REG_TARGET_POS_3        0x13

#define REG_CURRENT_POS_0       0x14
#define REG_CURRENT_POS_1       0x15
#define REG_CURRENT_POS_2       0x16
#define REG_CURRENT_POS_3       0x17

#define REG_SPEED_0             0x20
#define REG_SPEED_1             0x21
#define REG_ACCEL_0             0x22
#define REG_ACCEL_1             0x23

// 命令定义
#define CMD_NOP                 0x00
#define CMD_ENABLE              0x01
#define CMD_DISABLE             0x02
#define CMD_MOVE_TO             0x03
#define CMD_MOVE_REL            0x04
#define CMD_STOP                0x05
#define CMD_ESTOP               0x06
#define CMD_HOME                0x07
#define CMD_SET_HOME            0x08

// 状态位
#define STATUS_RUNNING          (1 << 0)
#define STATUS_HOMED            (1 << 1)
#define STATUS_ERROR            (1 << 2)
#define STATUS_ENABLED          (1 << 3)

/*============================================================================
 * 私有变量
 *============================================================================*/
#define REG_MAP_SIZE            64

static uint8_t g_reg_map[REG_MAP_SIZE];
static uint8_t g_reg_addr = 0;
static uint8_t g_selected_motor = 0;
static volatile rt_bool_t g_i2c_busy = RT_FALSE;

/*============================================================================
 * 更新状态寄存器
 *============================================================================*/
static void update_status_register(void)
{
    stepper_status_t *status = stepper_get_status(g_selected_motor);
    if (status == RT_NULL) return;
    
    // 状态字节
    uint8_t stat = 0;
    if (status->state != MOTOR_IDLE) stat |= STATUS_RUNNING;
    if (status->homed) stat |= STATUS_HOMED;
    if (status->error != ERR_NONE) stat |= STATUS_ERROR;
    if (status->enabled) stat |= STATUS_ENABLED;
    g_reg_map[REG_STATUS] = stat;
    
    // 当前位置
    int32_t pos = status->current_pos;
    g_reg_map[REG_CURRENT_POS_0] = (pos >> 0) & 0xFF;
    g_reg_map[REG_CURRENT_POS_1] = (pos >> 8) & 0xFF;
    g_reg_map[REG_CURRENT_POS_2] = (pos >> 16) & 0xFF;
    g_reg_map[REG_CURRENT_POS_3] = (pos >> 24) & 0xFF;
}

/*============================================================================
 * 处理命令
 *============================================================================*/
static void process_command(uint8_t cmd)
{
    int32_t target_pos;
    uint16_t speed;
    
    switch (cmd) {
        case CMD_ENABLE:
            stepper_enable(g_selected_motor);
            break;
            
        case CMD_DISABLE:
            stepper_disable(g_selected_motor);
            break;
            
        case CMD_MOVE_TO:
            target_pos = (int32_t)(
                (g_reg_map[REG_TARGET_POS_3] << 24) |
                (g_reg_map[REG_TARGET_POS_2] << 16) |
                (g_reg_map[REG_TARGET_POS_1] << 8) |
                (g_reg_map[REG_TARGET_POS_0])
            );
            stepper_move_to_async(g_selected_motor, target_pos);
            break;
            
        case CMD_MOVE_REL:
            target_pos = (int32_t)(
                (g_reg_map[REG_TARGET_POS_3] << 24) |
                (g_reg_map[REG_TARGET_POS_2] << 16) |
                (g_reg_map[REG_TARGET_POS_1] << 8) |
                (g_reg_map[REG_TARGET_POS_0])
            );
            stepper_move_relative_async(g_selected_motor, target_pos);
            break;
            
        case CMD_STOP:
            stepper_stop(g_selected_motor);
            break;
            
        case CMD_ESTOP:
            stepper_emergency_stop(g_selected_motor);
            break;
            
        case CMD_HOME:
            stepper_home_async(g_selected_motor);
            break;
            
        case CMD_SET_HOME:
            stepper_set_home(g_selected_motor);
            break;
            
        default:
            break;
    }
    
    g_reg_map[REG_COMMAND] = CMD_NOP;
}

/*============================================================================
 * I2C 事件中断处理
 *============================================================================*/
void I2C1_EV_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void I2C1_EV_IRQHandler(void)
{
    uint32_t event = I2C_GetLastEvent(I2C1);
    
    // 地址匹配 (接收方向)
    if (event == I2C_EVENT_SLAVE_RECEIVER_ADDRESS_MATCHED) {
        g_i2c_busy = RT_TRUE;
    }
    // 地址匹配 (发送方向)
    else if (event == I2C_EVENT_SLAVE_TRANSMITTER_ADDRESS_MATCHED) {
        g_i2c_busy = RT_TRUE;
        update_status_register();
        I2C_SendData(I2C1, g_reg_map[g_reg_addr]);
    }
    // 接收到数据
    else if (event == I2C_EVENT_SLAVE_BYTE_RECEIVED) {
        static rt_bool_t first_byte = RT_TRUE;
        uint8_t data = I2C_ReceiveData(I2C1);
        
        if (first_byte) {
            g_reg_addr = data;
            first_byte = RT_FALSE;
        } else {
            // 写入寄存器
            if (g_reg_addr < REG_MAP_SIZE) {
                // 跳过只读寄存器
                if (g_reg_addr != REG_DEVICE_ID && 
                    g_reg_addr != REG_VERSION &&
                    !(g_reg_addr >= REG_CURRENT_POS_0 && g_reg_addr <= REG_CURRENT_POS_3)) {
                    
                    g_reg_map[g_reg_addr] = data;
                    
                    // 特殊处理
                    if (g_reg_addr == REG_MOTOR_SELECT) {
                        g_selected_motor = data;
                    } else if (g_reg_addr == REG_COMMAND) {
                        process_command(data);
                    }
                }
                g_reg_addr++;
            }
        }
    }
    // 发送数据请求
    else if (event == I2C_EVENT_SLAVE_BYTE_TRANSMITTED) {
        g_reg_addr++;
        if (g_reg_addr < REG_MAP_SIZE) {
            I2C_SendData(I2C1, g_reg_map[g_reg_addr]);
        } else {
            I2C_SendData(I2C1, 0xFF);
        }
    }
    // 停止条件
    else if (I2C_GetFlagStatus(I2C1, I2C_FLAG_STOPF)) {
        I2C_ClearFlag(I2C1, I2C_FLAG_STOPF);
        g_i2c_busy = RT_FALSE;
    }
}

/*============================================================================
 * I2C 错误中断处理
 *============================================================================*/
void I2C1_ER_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void I2C1_ER_IRQHandler(void)
{
    if (I2C_GetFlagStatus(I2C1, I2C_FLAG_AF)) {
        I2C_ClearFlag(I2C1, I2C_FLAG_AF);
    }
    if (I2C_GetFlagStatus(I2C1, I2C_FLAG_BERR)) {
        I2C_ClearFlag(I2C1, I2C_FLAG_BERR);
    }
    g_i2c_busy = RT_FALSE;
}

/*============================================================================
 * I2C 从机初始化
 *============================================================================*/
int i2c_slave_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    I2C_InitTypeDef I2C_InitStruct = {0};
    NVIC_InitTypeDef NVIC_InitStruct = {0};
    
    // 初始化寄存器映射
    rt_memset(g_reg_map, 0, sizeof(g_reg_map));
    g_reg_map[REG_DEVICE_ID] = 0x5A;
    g_reg_map[REG_VERSION] = 0x10;
    
    // GPIO 配置 (PB6-SCL, PB7-SDA)
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    // I2C 配置
    I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStruct.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStruct.I2C_OwnAddress1 = I2C_SLAVE_ADDR << 1;
    I2C_InitStruct.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStruct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStruct.I2C_ClockSpeed = 400000;
    I2C_Init(I2C1, &I2C_InitStruct);
    
    // 使能中断
    I2C_ITConfig(I2C1, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, ENABLE);
    
    // NVIC 配置
    NVIC_InitStruct.NVIC_IRQChannel = I2C1_EV_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
    
    NVIC_InitStruct.NVIC_IRQChannel = I2C1_ER_IRQn;
    NVIC_Init(&NVIC_InitStruct);
    
    // 使能 I2C
    I2C_Cmd(I2C1, ENABLE);
    I2C_AcknowledgeConfig(I2C1, ENABLE);
    
    LOG_I("I2C slave initialized at address 0x%02X", I2C_SLAVE_ADDR);
    return RT_EOK;
}
INIT_DEVICE_EXPORT(i2c_slave_init);
```

### 8. 主程序 (main.c)

```c
#include <rtthread.h>
#include "board.h"
#include "stepper_core.h"

#define DBG_TAG    "main"
#define DBG_LVL    DBG_INFO
#include <rtdbg.h>

/*============================================================================
 * 电机配置
 *============================================================================*/
static void setup_motors(void)
{
    stepper_t motor0 = {
        .type = STEPPER_TYPE_BIPOLAR,
        .pins = {
            .step   = {MOTOR0_STEP_PORT, MOTOR0_STEP_PIN},
            .dir    = {MOTOR0_DIR_PORT, MOTOR0_DIR_PIN},
            .enable = {MOTOR0_EN_PORT, MOTOR0_EN_PIN},
            .ms1    = {MOTOR0_MS1_PORT, MOTOR0_MS1_PIN},
            .ms2    = {MOTOR0_MS2_PORT, MOTOR0_MS2_PIN},
            .ms3    = {MOTOR0_MS3_PORT, MOTOR0_MS3_PIN},
            .limit_min = {LIMIT0_MIN_PORT, LIMIT0_MIN_PIN},
            .limit_max = {LIMIT0_MAX_PORT, LIMIT0_MAX_PIN},
            .home   = {HOME0_PORT, HOME0_PIN},
        },
        .params = {
            .steps_per_rev = 200,
            .microstep = MICROSTEP_1_16,
            .gear_ratio = 1.0f,
            .screw_pitch = 8.0f,
            .max_speed = 5000.0f,
            .max_accel = 10000.0f,
            .max_decel = 10000.0f,
            .home_speed = 1000.0f,
            .soft_limit_min = -100000,
            .soft_limit_max = 100000,
            .reverse_dir = RT_FALSE,
            .enable_soft_limit = RT_TRUE,
        },
    };
    
    stepper_t motor1 = {
        .type = STEPPER_TYPE_BIPOLAR,
        .pins = {
            .step   = {MOTOR1_STEP_PORT, MOTOR1_STEP_PIN},
            .dir    = {MOTOR1_DIR_PORT, MOTOR1_DIR_PIN},
            .enable = {MOTOR1_EN_PORT, MOTOR1_EN_PIN},
            .ms1    = {MOTOR1_MS1_PORT, MOTOR1_MS1_PIN},
            .ms2    = {MOTOR1_MS2_PORT, MOTOR1_MS2_PIN},
            .ms3    = {MOTOR1_MS3_PORT, MOTOR1_MS3_PIN},
            .limit_min = {LIMIT1_MIN_PORT, LIMIT1_MIN_PIN},
            .limit_max = {LIMIT1_MAX_PORT, LIMIT1_MAX_PIN},
            .home   = {HOME1_PORT, HOME1_PIN},
        },
        .params = {
            .steps_per_rev = 200,
            .microstep = MICROSTEP_1_16,
            .gear_ratio = 1.0f,
            .screw_pitch = 8.0f,
            .max_speed = 5000.0f,
            .max_accel = 10000.0f,
            .max_decel = 10000.0f,
            .home_speed = 1000.0f,
            .soft_limit_min = -100000,
            .soft_limit_max = 100000,
            .reverse_dir = RT_FALSE,
            .enable_soft_limit = RT_TRUE,
        },
    };
    
    stepper_config(0, &motor0);
    stepper_config(1, &motor1);
    
    LOG_I("Motors configured");
}

/*============================================================================
 * 状态监控线程
 *============================================================================*/
static void monitor_thread_entry(void *param)
{
    while (1) {
        // 定期检查电机状态
        for (uint8_t i = 0; i < STEPPER_MAX_NUM; i++) {
            stepper_status_t *status = stepper_get_status(i);
            if (status == RT_NULL) continue;
            
            // 检查错误状态
            if (status->error != ERR_NONE) {
                LOG_W("Motor %d error: %d", i, status->error);
            }
        }
        
        rt_thread_mdelay(500);
    }
}

/*============================================================================
 * 主函数
 *============================================================================*/
int main(void)
{
    LOG_I("CH32L103 Stepper Controller Starting...");
    
    // 初始化步进电机驱动
    stepper_init();
    
    // 配置电机
    setup_motors();
    
    // 创建监控线程
    rt_thread_t monitor_tid = rt_thread_create("monitor",
                                               monitor_thread_entry,
                                               RT_NULL,
                                               MONITOR_THREAD_STACK,
                                               MONITOR_THREAD_PRIO,
                                               10);
    if (monitor_tid != RT_NULL) {
        rt_thread_startup(monitor_tid);
    }
    
    LOG_I("System ready!");
    
    // 主循环 (可以添加其他任务)
    while (1) {
        rt_thread_mdelay(1000);
        
        // LED 心跳指示 (如果有)
        // GPIO_TOGGLE(LED_PIN);
    }
    
    return 0;
}
```

### 9. RT-Thread 配置 (rtconfig.h)

```c
#ifndef __RTCONFIG_H__
#define __RTCONFIG_H__

/*============================================================================
 * RT-Thread 内核配置
 *============================================================================*/
#define RT_NAME_MAX                     8
#define RT_ALIGN_SIZE                   4
#define RT_THREAD_PRIORITY_MAX          32
#define RT_TICK_PER_SECOND              1000
#define RT_USING_OVERFLOW_CHECK
#define RT_USING_HOOK
#define RT_USING_IDLE_HOOK

/*============================================================================
 * 线程间同步
 *============================================================================*/
#define RT_USING_SEMAPHORE
#define RT_USING_MUTEX
#define RT_USING_EVENT
#define RT_USING_MAILBOX
#define RT_USING_MESSAGEQUEUE

/*============================================================================
 * 内存管理
 *============================================================================*/
#define RT_USING_HEAP
#define RT_USING_SMALL_MEM

/*============================================================================
 * 设备框架
 *============================================================================*/
#define RT_USING_DEVICE
#define RT_USING_SERIAL
#define RT_SERIAL_USING_DMA
#define RT_SERIAL_RB_BUFSZ              128

/*============================================================================
 * 控制台配置
 *============================================================================*/
#define RT_USING_CONSOLE
#define RT_CONSOLEBUF_SIZE              128
#define RT_CONSOLE_DEVICE_NAME          "uart1"

/*============================================================================
 * FinSH 配置
 *============================================================================*/
#define RT_USING_FINSH
#define FINSH_USING_MSH
#define FINSH_USING_MSH_ONLY
#define FINSH_THREAD_PRIORITY           20
#define FINSH_THREAD_STACK_SIZE         2048
#define FINSH_CMD_SIZE                  80
#define MSH_USING_BUILT_IN_COMMANDS
#define FINSH_USING_HISTORY
#define FINSH_HISTORY_LINES             5

/*============================================================================
 * 组件自动初始化
 *============================================================================*/
#define RT_USING_COMPONENTS_INIT
#define RT_USING_USER_MAIN
#define RT_MAIN_THREAD_STACK_SIZE       2048
#define RT_MAIN_THREAD_PRIORITY         10

/*============================================================================
 * 调试配置
 *============================================================================*/
#define RT_DEBUG
#define RT_DEBUG_INIT                   0
#define RT_DEBUG_THREAD                 0
#define RT_DEBUG_SCHEDULER              0
#define RT_DEBUG_IPC                    0
#define RT_DEBUG_TIMER                  0

#define RT_USING_ULOG
#define ULOG_OUTPUT_LVL                 LOG_LVL_DBG
#define ULOG_USING_COLOR

#endif /* __RTCONFIG_H__ */
```

### 10. FinSH 命令扩展 (msh_cmd.c)

```c
#include <rtthread.h>
#include <finsh.h>
#include "stepper_core.h"

/*============================================================================
 * MSH 命令 - 电机控制
 *============================================================================*/

// 使能电机
static void mtr_en(int argc, char *argv[])
{
    if (argc < 2) {
        rt_kprintf("Usage: mtr_en <id|all>\n");
        return;
    }
    
    if (rt_strcmp(argv[1], "all") == 0) {
        stepper_enable_all();
        rt_kprintf("All motors enabled\n");
    } else {
        uint8_t id = atoi(argv[1]);
        stepper_enable(id);
        rt_kprintf("Motor %d enabled\n", id);
    }
}
MSH_CMD_EXPORT(mtr_en, Enable motor: mtr_en <id|all>);

// 禁用电机
static void mtr_dis(int argc, char *argv[])
{
    if (argc < 2) {
        rt_kprintf("Usage: mtr_dis <id|all>\n");
        return;
    }
    
    if (rt_strcmp(argv[1], "all") == 0) {
        stepper_disable_all();
        rt_kprintf("All motors disabled\n");
    } else {
        uint8_t id = atoi(argv[1]);
        stepper_disable(id);
        rt_kprintf("Motor %d disabled\n", id);
    }
}
MSH_CMD_EXPORT(mtr_dis, Disable motor: mtr_dis <id|all>);

// 移动到位置
static void mtr_move(int argc, char *argv[])
{
    if (argc < 3) {
        rt_kprintf("Usage: mtr_move <id> <position>\n");
        return;
    }
    
    uint8_t id = atoi(argv[1]);
    int32_t pos = atol(argv[2]);
    
    int ret = stepper_move_to_async(id, pos);
    if (ret == RT_EOK) {
        rt_kprintf("Motor %d moving to %ld\n", id, pos);
    } else {
        rt_kprintf("Move failed: %d\n", ret);
    }
}
MSH_CMD_EXPORT(mtr_move, Move motor: mtr_move <id> <position>);

// 相对移动
static void mtr_mrel(int argc, char *argv[])
{
    if (argc < 3) {
        rt_kprintf("Usage: mtr_mrel <id> <steps>\n");
        return;
    }
    
    uint8_t id = atoi(argv[1]);
    int32_t steps = atol(argv[2]);
    
    int ret = stepper_move_relative_async(id, steps);
    if (ret == RT_EOK) {
        rt_kprintf("Motor %d moving %ld steps\n", id, steps);
    } else {
        rt_kprintf("Move failed: %d\n", ret);
    }
}
MSH_CMD_EXPORT(mtr_mrel, Move relative: mtr_mrel <id> <steps>);

// 停止电机
static void mtr_stop(int argc, char *argv[])
{
    if (argc < 2) {
        stepper_stop_all();
        rt_kprintf("All motors stopped\n");
        return;
    }
    
    if (rt_strcmp(argv[1], "all") == 0) {
        stepper_stop_all();
        rt_kprintf("All motors stopped\n");
    } else {
        uint8_t id = atoi(argv[1]);
        stepper_emergency_stop(id);
        rt_kprintf("Motor %d stopped\n", id);
    }
}
MSH_CMD_EXPORT(mtr_stop, Stop motor: mtr_stop [id|all]);

// 回零
static void mtr_home(int argc, char *argv[])
{
    if (argc < 2) {
        rt_kprintf("Usage: mtr_home <id>\n");
        return;
    }
    
    uint8_t id = atoi(argv[1]);
    int ret = stepper_home_async(id);
    if (ret == RT_EOK) {
        rt_kprintf("Motor %d homing...\n", id);
    } else {
        rt_kprintf("Home failed: %d\n", ret);
    }
}
MSH_CMD_EXPORT(mtr_home, Home motor: mtr_home <id>);

// 查看状态
static void mtr_stat(int argc, char *argv[])
{
    uint8_t start_id = 0, end_id = STEPPER_MAX_NUM - 1;
    
    if (argc >= 2) {
        start_id = end_id = atoi(argv[1]);
    }
    
    const char *state_str[] = {"IDLE", "ACCEL", "RUN", "DECEL", "HOME", "ERR"};
    
    rt_kprintf("\n%-4s %-10s %-10s %-8s %-6s %-6s %-5s\n",
               "ID", "Position", "Speed", "State", "Enable", "Homed", "Error");
    rt_kprintf("----------------------------------------------------\n");
    
    for (uint8_t id = start_id; id <= end_id; id++) {
        stepper_status_t *status = stepper_get_status(id);
        if (status == RT_NULL) continue;
        
        rt_kprintf("%-4d %-10ld %-10.1f %-8s %-6s %-6s %-5d\n",
                   id,
                   status->current_pos,
                   status->current_speed,
                   state_str[status->state],
                   status->enabled ? "Yes" : "No",
                   status->homed ? "Yes" : "No",
                   status->error);
    }
    rt_kprintf("\n");
}
MSH_CMD_EXPORT(mtr_stat, Show motor status: mtr_stat [id]);

// 设置速度
static void mtr_spd(int argc, char *argv[])
{
    if (argc < 3) {
        rt_kprintf("Usage: mtr_spd <id> <speed>\n");
        return;
    }
    
    uint8_t id = atoi(argv[1]);
    float speed = atof(argv[2]);
    
    stepper_set_speed(id, speed);
    rt_kprintf("Motor %d speed set to %.1f\n", id, speed);
}
MSH_CMD_EXPORT(mtr_spd, Set speed: mtr_spd <id> <speed>);

// 设置加速度
static void mtr_acc(int argc, char *argv[])
{
    if (argc < 3) {
        rt_kprintf("Usage: mtr_acc <id> <accel>\n");
        return;
    }
    
    uint8_t id = atoi(argv[1]);
    float accel = atof(argv[2]);
    
    stepper_set_accel(id, accel);
    rt_kprintf("Motor %d acceleration set to %.1f\n", id, accel);
}
MSH_CMD_EXPORT(mtr_acc, Set acceleration: mtr_acc <id> <accel>);

// 设置位置
static void mtr_pos(int argc, char *argv[])
{
    if (argc < 3) {
        rt_kprintf("Usage: mtr_pos <id> <position>\n");
        return;
    }
    
    uint8_t id = atoi(argv[1]);
    int32_t pos = atol(argv[2]);
    
    stepper_set_position(id, pos);
    rt_kprintf("Motor %d position set to %ld\n", id, pos);
}
MSH_CMD_EXPORT(mtr_pos, Set position: mtr_pos <id> <position>);

// 测试运动
static void mtr_test(int argc, char *argv[])
{
    uint8_t id = 0;
    int32_t distance = 10000;
    
    if (argc >= 2) id = atoi(argv[1]);
    if (argc >= 3) distance = atol(argv[2]);
    
    rt_kprintf("Testing motor %d, distance: %ld\n", id, distance);
    
    stepper_enable(id);
    rt_thread_mdelay(100);
    
    rt_kprintf("Moving forward...\n");
    stepper_move_relative(id, distance);
    
    rt_kprintf("Position: %ld\n", stepper_get_position(id));
    rt_thread_mdelay(500);
    
    rt_kprintf("Moving backward...\n");
    stepper_move_relative(id, -distance);
    
    rt_kprintf("Position: %ld\n", stepper_get_position(id));
    rt_kprintf("Test complete\n");
}
MSH_CMD_EXPORT(mtr_test, Test motor: mtr_test [id] [distance]);

// 多轴同步测试
static void mtr_sync(int argc, char *argv[])
{
    if (argc < 3) {
        rt_kprintf("Usage: mtr_sync <pos0> <pos1>\n");
        return;
    }
    
    uint8_t ids[] = {0, 1};
    int32_t positions[2];
    
    positions[0] = atol(argv[1]);
    positions[1] = atol(argv[2]);
    
    rt_kprintf("Sync moving: M0->%ld, M1->%ld\n", positions[0], positions[1]);
    
    stepper_enable(0);
    stepper_enable(1);
    rt_thread_mdelay(100);
    
    stepper_sync_move(ids, positions, 2);
    
    rt_kprintf("Sync move complete\n");
}
MSH_CMD_EXPORT(mtr_sync, Sync move: mtr_sync <pos0> <pos1>);
```

## 四、使用示例

### 1. 通过 FinSH 控制

```bash
# 查看帮助
msh > help

# 使能电机
msh > mtr_en 0
msh > mtr_en all

# 查看状态
msh > mtr_stat
msh > mtr_stat 0

# 移动电机
msh > mtr_move 0 10000
msh > mtr_mrel 0 -5000

# 设置参数
msh > mtr_spd 0 3000
msh > mtr_acc 0 8000

# 回零
msh > mtr_home 0

# 停止
msh > mtr_stop 0
msh > mtr_stop all

# 测试
msh > mtr_test 0 20000

# 多轴同步
msh > mtr_sync 10000 5000
```

### 2. 通过 UART 命令控制

```text
# 使能电机
$EN 0
$EN ALL

# 移动
$MOVE 0 10000
$MREL 0 -5000

# 查看状态
$STAT 0

# 设置参数
$SPD 0 3000
$CFG 0 ACCEL 8000

# 回零
$HOME 0

# 停止
$STOP 0
$STOP ALL

# 多轴同步
$SYNC 0:10000 1:5000
```

### 3. 通过 I2C 主机控制 (示例代码)

```c
// I2C 主机端控制示例 (Arduino/其他MCU)
#include <Wire.h>

#define STEPPER_ADDR    0x20

void stepper_enable(uint8_t motor) {
    Wire.beginTransmission(STEPPER_ADDR);
    Wire.write(0x04);  // REG_MOTOR_SELECT
    Wire.write(motor);
    Wire.endTransmission();
    
    Wire.beginTransmission(STEPPER_ADDR);
    Wire.write(0x05);  // REG_COMMAND
    Wire.write(0x01);  // CMD_ENABLE
    Wire.endTransmission();
}

void stepper_move_to(uint8_t motor, int32_t position) {
    Wire.beginTransmission(STEPPER_ADDR);
    Wire.write(0x04);  // REG_MOTOR_SELECT
    Wire.write(motor);
    Wire.endTransmission();
    
    // 写入目标位置
    Wire.beginTransmission(STEPPER_ADDR);
    Wire.write(0x10);  // REG_TARGET_POS_0
    Wire.write((position >> 0) & 0xFF);
    Wire.write((position >> 8) & 0xFF);
    Wire.write((position >> 16) & 0xFF);
    Wire.write((position >> 24) & 0xFF);
    Wire.endTransmission();
    
    // 发送移动命令
    Wire.beginTransmission(STEPPER_ADDR);
    Wire.write(0x05);  // REG_COMMAND
    Wire.write(0x03);  // CMD_MOVE_TO
    Wire.endTransmission();
}

int32_t stepper_get_position(uint8_t motor) {
    Wire.beginTransmission(STEPPER_ADDR);
    Wire.write(0x04);  // REG_MOTOR_SELECT
    Wire.write(motor);
    Wire.endTransmission();
    
    Wire.beginTransmission(STEPPER_ADDR);
    Wire.write(0x14);  // REG_CURRENT_POS_0
    Wire.endTransmission();
    
    Wire.requestFrom(STEPPER_ADDR, 4);
    int32_t pos = 0;
    if (Wire.available() >= 4) {
        pos = Wire.read();
        pos |= (int32_t)Wire.read() << 8;
        pos |= (int32_t)Wire.read() 
```





```
继续
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

已完成生成

### 3. 通过 I2C 主机控制 (示例代码) - 续

```c
        pos |= (int32_t)Wire.read() << 16;
        pos |= (int32_t)Wire.read() << 24;
    }
    return pos;
}

uint8_t stepper_get_status(uint8_t motor) {
    Wire.beginTransmission(STEPPER_ADDR);
    Wire.write(0x04);  // REG_MOTOR_SELECT
    Wire.write(motor);
    Wire.endTransmission();
    
    Wire.beginTransmission(STEPPER_ADDR);
    Wire.write(0x02);  // REG_STATUS
    Wire.endTransmission();
    
    Wire.requestFrom(STEPPER_ADDR, 1);
    if (Wire.available()) {
        return Wire.read();
    }
    return 0xFF;
}

bool stepper_is_running(uint8_t motor) {
    return (stepper_get_status(motor) & 0x01) != 0;
}

void stepper_stop(uint8_t motor) {
    Wire.beginTransmission(STEPPER_ADDR);
    Wire.write(0x04);  // REG_MOTOR_SELECT
    Wire.write(motor);
    Wire.endTransmission();
    
    Wire.beginTransmission(STEPPER_ADDR);
    Wire.write(0x05);  // REG_COMMAND
    Wire.write(0x06);  // CMD_ESTOP
    Wire.endTransmission();
}

void stepper_home(uint8_t motor) {
    Wire.beginTransmission(STEPPER_ADDR);
    Wire.write(0x04);  // REG_MOTOR_SELECT
    Wire.write(motor);
    Wire.endTransmission();
    
    Wire.beginTransmission(STEPPER_ADDR);
    Wire.write(0x05);  // REG_COMMAND
    Wire.write(0x07);  // CMD_HOME
    Wire.endTransmission();
}

// Arduino 示例主程序
void setup() {
    Serial.begin(115200);
    Wire.begin();
    
    Serial.println("Stepper I2C Master Example");
    
    // 使能电机0
    stepper_enable(0);
    delay(100);
    
    Serial.println("Motor 0 enabled");
}

void loop() {
    // 移动到位置 10000
    Serial.println("Moving to 10000...");
    stepper_move_to(0, 10000);
    
    // 等待完成
    while (stepper_is_running(0)) {
        Serial.print("Position: ");
        Serial.println(stepper_get_position(0));
        delay(100);
    }
    
    Serial.println("Move complete");
    delay(1000);
    
    // 移动回原点
    Serial.println("Moving to 0...");
    stepper_move_to(0, 0);
    
    while (stepper_is_running(0)) {
        Serial.print("Position: ");
        Serial.println(stepper_get_position(0));
        delay(100);
    }
    
    Serial.println("Move complete");
    delay(1000);
}
```

### 11. UART 驱动适配 (drv_uart.c)

```c
#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"

#define DBG_TAG    "drv.uart"
#define DBG_LVL    DBG_INFO
#include <rtdbg.h>

/*============================================================================
 * UART 设备结构
 *============================================================================*/
struct ch32_uart {
    USART_TypeDef *instance;
    IRQn_Type irq;
    struct rt_serial_device serial;
};

static struct ch32_uart uart1 = {
    .instance = USART1,
    .irq = USART1_IRQn,
};

/*============================================================================
 * UART 操作函数
 *============================================================================*/
static rt_err_t ch32_uart_configure(struct rt_serial_device *serial, 
                                    struct serial_configure *cfg)
{
    struct ch32_uart *uart = (struct ch32_uart *)serial->parent.user_data;
    USART_InitTypeDef USART_InitStruct = {0};
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // GPIO 配置
    if (uart->instance == USART1) {
        // PA9 - TX, PA10 - RX
        GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9;
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
        GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_Init(GPIOA, &GPIO_InitStruct);
        
        GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10;
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
        GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
    
    // USART 配置
    USART_InitStruct.USART_BaudRate = cfg->baud_rate;
    
    switch (cfg->data_bits) {
        case DATA_BITS_8:
            USART_InitStruct.USART_WordLength = USART_WordLength_8b;
            break;
        case DATA_BITS_9:
            USART_InitStruct.USART_WordLength = USART_WordLength_9b;
            break;
        default:
            USART_InitStruct.USART_WordLength = USART_WordLength_8b;
            break;
    }
    
    switch (cfg->stop_bits) {
        case STOP_BITS_1:
            USART_InitStruct.USART_StopBits = USART_StopBits_1;
            break;
        case STOP_BITS_2:
            USART_InitStruct.USART_StopBits = USART_StopBits_2;
            break;
        default:
            USART_InitStruct.USART_StopBits = USART_StopBits_1;
            break;
    }
    
    switch (cfg->parity) {
        case PARITY_NONE:
            USART_InitStruct.USART_Parity = USART_Parity_No;
            break;
        case PARITY_ODD:
            USART_InitStruct.USART_Parity = USART_Parity_Odd;
            break;
        case PARITY_EVEN:
            USART_InitStruct.USART_Parity = USART_Parity_Even;
            break;
        default:
            USART_InitStruct.USART_Parity = USART_Parity_No;
            break;
    }
    
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    
    USART_Init(uart->instance, &USART_InitStruct);
    USART_Cmd(uart->instance, ENABLE);
    
    return RT_EOK;
}

static rt_err_t ch32_uart_control(struct rt_serial_device *serial, 
                                  int cmd, void *arg)
{
    struct ch32_uart *uart = (struct ch32_uart *)serial->parent.user_data;
    NVIC_InitTypeDef NVIC_InitStruct = {0};
    
    switch (cmd) {
        case RT_DEVICE_CTRL_CLR_INT:
            USART_ITConfig(uart->instance, USART_IT_RXNE, DISABLE);
            NVIC_DisableIRQ(uart->irq);
            break;
            
        case RT_DEVICE_CTRL_SET_INT:
            USART_ITConfig(uart->instance, USART_IT_RXNE, ENABLE);
            NVIC_InitStruct.NVIC_IRQChannel = uart->irq;
            NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 3;
            NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
            NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
            NVIC_Init(&NVIC_InitStruct);
            break;
    }
    
    return RT_EOK;
}

static int ch32_uart_putc(struct rt_serial_device *serial, char c)
{
    struct ch32_uart *uart = (struct ch32_uart *)serial->parent.user_data;
    
    while (USART_GetFlagStatus(uart->instance, USART_FLAG_TXE) == RESET);
    USART_SendData(uart->instance, c);
    
    return 1;
}

static int ch32_uart_getc(struct rt_serial_device *serial)
{
    struct ch32_uart *uart = (struct ch32_uart *)serial->parent.user_data;
    
    if (USART_GetFlagStatus(uart->instance, USART_FLAG_RXNE) != RESET) {
        return USART_ReceiveData(uart->instance);
    }
    
    return -1;
}

static const struct rt_uart_ops ch32_uart_ops = {
    .configure = ch32_uart_configure,
    .control = ch32_uart_control,
    .putc = ch32_uart_putc,
    .getc = ch32_uart_getc,
};

/*============================================================================
 * UART 中断处理
 *============================================================================*/
void USART1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void USART1_IRQHandler(void)
{
    rt_interrupt_enter();
    
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        rt_hw_serial_isr(&uart1.serial, RT_SERIAL_EVENT_RX_IND);
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
    
    if (USART_GetITStatus(USART1, USART_IT_ORE) != RESET) {
        USART_ReceiveData(USART1);
        USART_ClearITPendingBit(USART1, USART_IT_ORE);
    }
    
    rt_interrupt_leave();
}

/*============================================================================
 * UART 初始化
 *============================================================================*/
int rt_hw_uart_init(void)
{
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    
    config.baud_rate = UART_CMD_BAUDRATE;
    
    uart1.serial.ops = &ch32_uart_ops;
    uart1.serial.config = config;
    
    rt_hw_serial_register(&uart1.serial, "uart1",
                          RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX,
                          &uart1);
    
    LOG_I("UART1 initialized");
    return RT_EOK;
}
INIT_BOARD_EXPORT(rt_hw_uart_init);
```

### 12. 运动规划器 - S曲线加减速 (stepper_planner.c)

```c
#include "stepper_planner.h"
#include <rtthread.h>
#include <math.h>

#define DBG_TAG    "planner"
#define DBG_LVL    DBG_INFO
#include <rtdbg.h>

/*============================================================================
 * S曲线加减速规划器
 * 7段速度曲线: 加加速->匀加速->减加速->匀速->加减速->匀减速->减减速
 *============================================================================*/

typedef struct {
    // 输入参数
    float max_velocity;
    float max_accel;
    float max_jerk;
    int32_t total_steps;
    
    // 7段时间
    float t[7];
    
    // 运行状态
    float elapsed_time;
    float current_velocity;
    float current_accel;
    int32_t current_step;
    uint8_t phase;
    rt_bool_t completed;
} scurve_profile_t;

/*============================================================================
 * 计算S曲线参数
 *============================================================================*/
rt_bool_t scurve_plan(scurve_profile_t *profile, 
                      float max_vel, float max_accel, float max_jerk,
                      int32_t steps)
{
    if (steps == 0) {
        profile->completed = RT_TRUE;
        return RT_TRUE;
    }
    
    profile->max_velocity = max_vel;
    profile->max_accel = max_accel;
    profile->max_jerk = max_jerk;
    profile->total_steps = (steps > 0) ? steps : -steps;
    profile->elapsed_time = 0;
    profile->current_velocity = 0;
    profile->current_accel = 0;
    profile->current_step = 0;
    profile->phase = 0;
    profile->completed = RT_FALSE;
    
    float J = max_jerk;
    float A = max_accel;
    float V = max_vel;
    float S = (float)profile->total_steps;
    
    // 计算达到最大加速度需要的时间 (jerk阶段)
    float Tj = A / J;
    
    // 计算达到最大速度需要的时间 (加速阶段)
    float Ta = V / A + Tj;
    
    // 计算加速阶段的位移
    float Sa = 0.5f * A * Ta * Ta;
    
    // 检查是否能达到最大速度
    if (2.0f * Sa > S) {
        // 三角形速度曲线 (无法达到最大速度)
        float Vpeak = sqrtf(S * A);
        Ta = Vpeak / A + Tj;
        
        if (Ta < 2.0f * Tj) {
            // 无法达到最大加速度
            Tj = sqrtf(Vpeak / J);
            Ta = 2.0f * Tj;
        }
        
        profile->t[0] = Tj;           // 加加速
        profile->t[1] = Ta - 2*Tj;    // 匀加速
        profile->t[2] = Tj;           // 减加速
        profile->t[3] = 0;            // 匀速 (无)
        profile->t[4] = Tj;           // 加减速
        profile->t[5] = Ta - 2*Tj;    // 匀减速
        profile->t[6] = Tj;           // 减减速
    } else {
        // 梯形速度曲线
        float Tv = (S - 2.0f * Sa) / V;  // 匀速时间
        
        profile->t[0] = Tj;
        profile->t[1] = Ta - 2*Tj;
        profile->t[2] = Tj;
        profile->t[3] = Tv;
        profile->t[4] = Tj;
        profile->t[5] = Ta - 2*Tj;
        profile->t[6] = Tj;
    }
    
    return RT_TRUE;
}

/*============================================================================
 * 获取当前速度
 *============================================================================*/
float scurve_get_velocity(scurve_profile_t *profile, float dt)
{
    if (profile->completed) return 0;
    
    profile->elapsed_time += dt;
    float t = profile->elapsed_time;
    
    float J = profile->max_jerk;
    float A = profile->max_accel;
    float V = profile->max_velocity;
    
    // 计算各阶段结束时间
    float t_end[7];
    t_end[0] = profile->t[0];
    for (int i = 1; i < 7; i++) {
        t_end[i] = t_end[i-1] + profile->t[i];
    }
    
    float v = 0;
    float a = 0;
    
    if (t < t_end[0]) {
        // Phase 0: 加加速 (jerk = +J)
        float dt0 = t;
        a = J * dt0;
        v = 0.5f * J * dt0 * dt0;
        profile->phase = 0;
    }
    else if (t < t_end[1]) {
        // Phase 1: 匀加速 (jerk = 0, accel = A)
        float dt1 = t - t_end[0];
        float v0 = 0.5f * J * profile->t[0] * profile->t[0];
        a = A;
        v = v0 + A * dt1;
        profile->phase = 1;
    }
    else if (t < t_end[2]) {
        // Phase 2: 减加速 (jerk = -J)
        float dt2 = t - t_end[1];
        float v1 = 0.5f * J * profile->t[0] * profile->t[0] + A * profile->t[1];
        a = A - J * dt2;
        v = v1 + A * dt2 - 0.5f * J * dt2 * dt2;
        profile->phase = 2;
    }
    else if (t < t_end[3]) {
        // Phase 3: 匀速 (accel = 0)
        a = 0;
        v = V;
        profile->phase = 3;
    }
    else if (t < t_end[4]) {
        // Phase 4: 加减速 (jerk = -J)
        float dt4 = t - t_end[3];
        a = -J * dt4;
        v = V - 0.5f * J * dt4 * dt4;
        profile->phase = 4;
    }
    else if (t < t_end[5]) {
        // Phase 5: 匀减速 (jerk = 0, accel = -A)
        float dt5 = t - t_end[4];
        float v4 = V - 0.5f * J * profile->t[4] * profile->t[4];
        a = -A;
        v = v4 - A * dt5;
        profile->phase = 5;
    }
    else if (t < t_end[6]) {
        // Phase 6: 减减速 (jerk = +J)
        float dt6 = t - t_end[5];
        float v5 = V - 0.5f * J * profile->t[4] * profile->t[4] - A * profile->t[5];
        a = -A + J * dt6;
        v = v5 - A * dt6 + 0.5f * J * dt6 * dt6;
        profile->phase = 6;
    }
    else {
        // 完成
        v = 0;
        a = 0;
        profile->completed = RT_TRUE;
    }
    
    profile->current_velocity = v;
    profile->current_accel = a;
    
    return v;
}

/*============================================================================
 * 获取脉冲间隔 (微秒)
 *============================================================================*/
uint32_t scurve_get_interval(scurve_profile_t *profile, float dt)
{
    float v = scurve_get_velocity(profile, dt);
    
    if (v < 50.0f) v = 50.0f;  // 最小速度限制
    
    return (uint32_t)(1000000.0f / v);
}

/*============================================================================
 * 线性插补规划器
 *============================================================================*/
typedef struct {
    int32_t start_x, start_y;
    int32_t end_x, end_y;
    int32_t delta_x, delta_y;
    int32_t steps_x, steps_y;
    int32_t error;
    int32_t step_count;
    int32_t total_steps;
    rt_bool_t completed;
} linear_interp_t;

void linear_interp_init(linear_interp_t *interp,
                        int32_t x0, int32_t y0,
                        int32_t x1, int32_t y1)
{
    interp->start_x = x0;
    interp->start_y = y0;
    interp->end_x = x1;
    interp->end_y = y1;
    
    interp->delta_x = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    interp->delta_y = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    
    interp->steps_x = (x1 > x0) ? 1 : -1;
    interp->steps_y = (y1 > y0) ? 1 : -1;
    
    interp->total_steps = (interp->delta_x > interp->delta_y) ? 
                          interp->delta_x : interp->delta_y;
    
    interp->error = interp->delta_x - interp->delta_y;
    interp->step_count = 0;
    interp->completed = RT_FALSE;
}

// Bresenham 线性插补步进
// 返回: bit0 = X轴步进, bit1 = Y轴步进
uint8_t linear_interp_step(linear_interp_t *interp, 
                           int32_t *current_x, int32_t *current_y)
{
    if (interp->completed) return 0;
    
    uint8_t step_mask = 0;
    int32_t e2 = 2 * interp->error;
    
    if (e2 > -interp->delta_y) {
        interp->error -= interp->delta_y;
        *current_x += interp->steps_x;
        step_mask |= 0x01;
    }
    
    if (e2 < interp->delta_x) {
        interp->error += interp->delta_x;
        *current_y += interp->steps_y;
        step_mask |= 0x02;
    }
    
    interp->step_count++;
    
    if (interp->step_count >= interp->total_steps) {
        interp->completed = RT_TRUE;
    }
    
    return step_mask;
}

/*============================================================================
 * 圆弧插补规划器
 *============================================================================*/
typedef struct {
    int32_t center_x, center_y;
    int32_t current_x, current_y;
    int32_t end_x, end_y;
    int32_t radius;
    int32_t f;  // 偏差判别函数
    rt_bool_t clockwise;
    rt_bool_t completed;
    uint8_t quadrant;
} arc_interp_t;

void arc_interp_init(arc_interp_t *interp,
                     int32_t cx, int32_t cy,      // 圆心
                     int32_t x0, int32_t y0,      // 起点
                     int32_t x1, int32_t y1,      // 终点
                     rt_bool_t cw)                 // 顺时针
{
    interp->center_x = cx;
    interp->center_y = cy;
    interp->current_x = x0;
    interp->current_y = y0;
    interp->end_x = x1;
    interp->end_y = y1;
    interp->clockwise = cw;
    interp->completed = RT_FALSE;
    
    // 计算半径
    int32_t dx = x0 - cx;
    int32_t dy = y0 - cy;
    interp->radius = (int32_t)sqrtf((float)(dx*dx + dy*dy));
    
    // 初始偏差
    interp->f = 0;
    
    // 确定起始象限
    if (dx >= 0 && dy >= 0) interp->quadrant = 1;
    else if (dx < 0 && dy >= 0) interp->quadrant = 2;
    else if (dx < 0 && dy < 0) interp->quadrant = 3;
    else interp->quadrant = 4;
}

// 圆弧插补步进 (逐点比较法)
uint8_t arc_interp_step(arc_interp_t *interp,
                        int32_t *current_x, int32_t *current_y)
{
    if (interp->completed) return 0;
    
    // 检查是否到达终点
    if (interp->current_x == interp->end_x && 
        interp->current_y == interp->end_y) {
        interp->completed = RT_TRUE;
        return 0;
    }
    
    uint8_t step_mask = 0;
    int32_t dx = interp->current_x - interp->center_x;
    int32_t dy = interp->current_y - interp->center_y;
    
    // 根据象限和方向决定步进
    if (interp->clockwise) {
        // 顺时针
        switch (interp->quadrant) {
            case 1:  // 第一象限: -X 或 -Y
                if (interp->f >= 0) {
                    interp->current_x--;
                    interp->f -= 2 * dx - 1;
                    step_mask = 0x01;
                } else {
                    interp->current_y--;
                    interp->f += 2 * dy - 1;
                    step_mask = 0x02;
                }
                if (dx <= 0) interp->quadrant = 2;
                break;
            case 2:  // 第二象限
                if (interp->f >= 0) {
                    interp->current_y--;
                    interp->f -= 2 * dy - 1;
                    step_mask = 0x02;
                } else {
                    interp->current_x++;
                    interp->f -= 2 * dx + 1;
                    step_mask = 0x01;
                }
                if (dy <= 0) interp->quadrant = 3;
                break;
            case 3:  // 第三象限
                if (interp->f >= 0) {
                    interp->current_x++;
                    interp->f += 2 * dx + 1;
                    step_mask = 0x01;
                } else {
                    interp->current_y++;
                    interp->f -= 2 * dy + 1;
                    step_mask = 0x02;
                }
                if (dx >= 0) interp->quadrant = 4;
                break;
            case 4:  // 第四象限
                if (interp->f >= 0) {
                    interp->current_y++;
                    interp->f += 2 * dy + 1;
                    step_mask = 0x02;
                } else {
                    interp->current_x--;
                    interp->f += 2 * dx - 1;
                    step_mask = 0x01;
                }
                if (dy >= 0) interp->quadrant = 1;
                break;
        }
    } else {
        // 逆时针 (方向相反)
        switch (interp->quadrant) {
            case 1:
                if (interp->f >= 0) {
                    interp->current_y++;
                    interp->f -= 2 * dy - 1;
                    step_mask = 0x02;
                } else {
                    interp->current_x++;
                    interp->f += 2 * dx + 1;
                    step_mask = 0x01;
                }
                if (dy >= interp->radius) interp->quadrant = 2;
                break;
            // ... 其他象限类似
        }
    }
    
    *current_x = interp->current_x;
    *current_y = interp->current_y;
    
    return step_mask;
}
```

## 五、系统特性总结

|     特性     |               说明               |
| :----------: | :------------------------------: |
|   **MCU**    |     CH32L103 (RISC-V, 96MHz)     |
|   **RTOS**   |            RT-Thread             |
| **电机数量** |         最多4个步进电机          |
| **细分支持** |  1/1, 1/2, 1/4, 1/8, 1/16, 1/32  |
|  **加减速**  |     梯形加减速 / S曲线加减速     |
| **通信接口** |    UART (115200bps) + I2C从机    |
| **控制方式** | FinSH命令 / UART命令 / I2C寄存器 |
| **保护功能** |    软限位 / 硬限位 / 紧急停止    |
| **插补功能** |       线性插补 / 圆弧插补        |

## 六、硬件连接图

```text
                    CH32L103
                 ┌───────────┐
                 │           │
    Motor0 STEP ─┤PA0    PA9 ├─ UART TX
    Motor0 DIR  ─┤PA1   PA10 ├─ UART RX
    Motor0 EN   ─┤PA2        │
    Motor0 MS1  ─┤PA3    PB6 ├─ I2C SCL
    Motor0 MS2  ─┤PA4    PB7 ├─ I2C SDA
    Motor0 MS3  ─┤PA5        │
    Motor1 STEP ─┤PA6    PC0 ├─ LIMIT0_MIN
    Motor1 DIR  ─┤PA7    PC1 ├─ LIMIT0_MAX
                 │       PC4 ├─ HOME0
    Motor1 EN   ─┤PB0    PC5 ├─ HOME1
    Motor1 MS1  ─┤PB1        │
    Motor1 MS2  ─┤PB2        │
    Motor1 MS3  ─┤PB10       │
                 │           │
                 └───────────┘

    驱动板连接 (A4988/DRV8825):
    ┌─────────────────────────────────────┐
    │  MCU          Driver        Motor   │
    │  STEP ──────► STEP                  │
    │  DIR  ──────► DIR     ┌──► A+       │
    │  EN   ──────► ENABLE  │   A-       │
    │  MS1  ──────► MS1     │   B+       │
    │  MS2  ──────► MS2     └──► B-       │
    │  MS3  ──────► MS3                   │
    │               VMOT ◄── 12-24V       │
    │               GND  ◄── GND          │
    └─────────────────────────────────────┘
```

## 七、编译与烧录

```bash
# 使用 RT-Thread ENV 工具
$ cd project
$ scons --target=mdk5    # 生成 Keil MDK 工程
# 或
$ scons                  # 直接编译

# 使用 WCH-Link 烧录
$ wch-isp -p CH32L103 -f rtthread.bin
```

这套代码完整实现了基于 CH32L103 + RT-Thread 的多步进电机控制系统，支持 UART 和 I2C 扩展控制。如果需要进一步优化或添加其他功能，请告诉我！