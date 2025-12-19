
#include <stdint.h>
#include "rtc.h" // RTC驱动头文件

// 闹钟重复模式枚举
typedef enum {
    ALARM_ONCE,     // 仅一次
    ALARM_PERIODIC, // 周期重复
    ALARM_DAILY,    // 每天重复
    ALARM_WEEKLY    // 每周重复
} AlarmMode;

// 闹钟结构体
typedef struct {
    uint8_t enabled;   // 闹钟是否启用
    AlarmMode mode;    // 闹钟重复模式
    uint8_t hour;      // 下一次触发时间(小时)
    uint8_t minute;    // 下一次触发时间(分钟)
    uint8_t second;    // 下一次触发时间(秒)
    uint8_t period;    // 重复周期(仅在ALARM_PERIODIC模式下使用)
    uint8_t dayOfWeek; // 仅在周重复模式下使用
} Alarm;

#define MAX_ALARMS 5 // 最大闹钟数量

volatile Alarm alarms[MAX_ALARMS]; // 闹钟数组
volatile uint8_t alarmCount = 0;   // 当前闹钟数量

// RTC中断服务函数
void RTC_IRQHandler(void) {
    RTC_ClearITPendingBit(RTC_IT_SEC); // 清除RTC秒中断标志

    RTC_TimeTypeDef currentTime;
    RTC_GetTime(RTC_Format_BINorBCD, &currentTime); // 获取当前时间

    for (uint8_t i = 0; i < alarmCount; i++) {
        if (alarms[i].enabled &&
            alarms[i].hour == currentTime.RTC_Hours &&
            alarms[i].minute == currentTime.RTC_Minutes &&
            alarms[i].second == currentTime.RTC_Seconds) {
            handleAlarm(i, &currentTime); // 处理闹钟事件
        }
    }
}

void setAlarm(uint8_t index, uint8_t hour, uint8_t minute, uint8_t second, AlarmMode mode, uint8_t period, uint8_t dayOfWeek) {
    if (index < MAX_ALARMS) {
        alarms[index].enabled = 1;
        alarms[index].mode = mode;
        alarms[index].hour = hour;
        alarms[index].minute = minute;
        alarms[index].second = second;
        alarms[index].period = period;
        alarms[index].dayOfWeek = dayOfWeek;
        alarmCount++;
    }
}

void handleAlarm(uint8_t index, RTC_TimeTypeDef* currentTime) {
    // 闹钟触发后的处理逻辑

    // 根据重复模式设置下一次闹钟时间
    switch (alarms[index].mode) {
        case ALARM_ONCE:
            alarms[index].enabled = 0; // 关闭当前闹钟
            break;
        case ALARM_PERIODIC:
            alarms[index].hour = (alarms[index].hour + alarms[index].period) % 24;
            break;
        case ALARM_DAILY:
            alarms[index].hour = (alarms[index].hour + 24) % 24;
            break;
        case ALARM_WEEKLY:
            // 计算下一个周几的时间
            // ...
            break;
    }

    // ...其他代码
}

int main(void) {
    RTC_Init(); // 初始化RTC
    RTC_ITConfig(RTC_IT_SEC, ENABLE); // 使能RTC秒中断

    setAlarm(0, 12, 30, 0, ALARM_PERIODIC, 4, 0);  // 每4小时触发一次,从12:30开始
    setAlarm(1, 9, 0, 0, ALARM_PERIODIC, 6, 0);    // 每6小时触发一次,从9:00开始
    setAlarm(2, 18, 0, 0, ALARM_WEEKLY, 0, 3);     // 每周三18:00触发
    setAlarm(3, 9, 0, 0, ALARM_ONCE, 0, 0);        // 仅一次9:00触发

    while (1) {
        // 进入低功耗模式等待RTC中断
        PWR_EnterSleepMode();
    }
}