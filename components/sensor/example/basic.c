#include "sensor_core.h"
#include "sensor_mpu6050.h"
#include <stdio.h>

/*
 * 框架要求你提供的 HAL 入口。框架只透传总线句柄、从不解引用它，所以这里
 * 用不到的参数一律显式忽略，而不是让编译器在每次构建时提醒你一遍。
 */

/* 总线句柄由应用层提供：框架把 `void *` 原样交给驱动，只有驱动知道它是什么。 */
static int g_i2c_bus_handle;

void delay_ms(uint32_t ms)
{
    (void)ms; /* 实现延时 */
}

uint32_t get_tick_ms(void)
{
    return 0; /* 返回系统时间戳(ms) */
}

int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data,
                     uint16_t len)
{
    (void)bus;
    (void)addr;
    (void)reg;
    (void)data;
    (void)len;
    return 0; /* 实现I2C读取 */
}

int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data,
                      uint16_t len)
{
    (void)bus;
    (void)addr;
    (void)reg;
    (void)data;
    (void)len;
    return 0; /* 实现I2C写入 */
}

/**
 * @brief 基础使用示例
 *
 * @return 0 表示创建/注册/初始化/读取全部成功；任何一步失败即返回非 0，
 *         所以这段样张被当作构建守卫运行时，失败不会被当成通过。
 */
int example_basic(void)
{
    /* 句柄不能是 NULL：`mpu6050_create_accel()` 明确拒绝 NULL 并要求非空总线，
     * 传 NULL 会让流程在第一行就结束，而下面每一步都还没跑过。 */
    void *i2c_bus = &g_i2c_bus_handle;

    printf("\n=== Basic Example ===\n");

    /* 创建传感器设备 */
    sensor_device_t *accel = mpu6050_create_accel("accel0", i2c_bus);
    if (accel == NULL) {
        printf("Failed to create accelerometer\n");
        return 1;
    }

    /* 注册设备 */
    if (sensor_register(accel) != SENSOR_EOK) {
        printf("Failed to register sensor\n");
        return 1;
    }

    /* 初始化设备 */
    if (sensor_init(accel) != SENSOR_EOK) {
        printf("Failed to initialize sensor\n");
        return 1;
    }

    /* 读取数据 */
    for (int i = 0; i < 10; i++) {
        sensor_data_t data;

        if (sensor_read(accel, &data) == SENSOR_EOK) {
            printf("[%u] Accel: X=%d, Y=%d, Z=%d mg\n", data.timestamp,
                   data.value.val_3axis.x, data.value.val_3axis.y,
                   data.value.val_3axis.z);
        }

        delay_ms(100);
    }

    /* 反初始化 */
    sensor_deinit(accel);
    sensor_unregister(accel);
    return 0;
}

int main(void)
{
    return example_basic();
}
