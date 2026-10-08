/**
 * @file xy_device_pm.c
 * @brief Device Power Management Implementation
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 设备电源管理实现 - 支持低功耗模式
 */

#include "xy_device_pm.h"
#include "xy_device_core.h"
#include <string.h>

/* ==================== Private Types ==================== */

/**
 * @brief 设备电源管理私有数据
 */
typedef struct {
    xy_device_t *owner;                      /**< Owning device; keep dev->data for the driver */
    xy_device_pm_state_t current_state;    /**< 当前电源状态 */
    xy_device_pm_state_t last_state;       /**< 上一个状态 (用于唤醒) */
    xy_device_pm_policy_t policy;          /**< 电源管理策略 */
    const xy_device_pm_ops_t *ops;         /**< 电源管理操作集 */
    uint32_t idle_timeout_ms;              /**< 空闲超时 (毫秒) */
    uint32_t last_activity_time;           /**< 最后活动时间 */
    bool wakeup_enabled;                   /**< 唤醒功能使能 */
} xy_device_pm_data_t;

/* ==================== Private Variables ==================== */

/* 每个设备最多支持 16 个电源管理实例 */
static xy_device_pm_data_t pm_data[16];
static bool pm_initialized = false;

/* ==================== Private Functions ==================== */

/**
 * @brief 查找设备的电源管理数据
 */
static xy_device_pm_data_t *pm_find_data(xy_device_t *dev)
{
    if (!pm_initialized || !dev) {
        return NULL;
    }

    for (size_t i = 0; i < sizeof(pm_data) / sizeof(pm_data[0]); ++i) {
        if (pm_data[i].owner == dev && pm_data[i].ops != NULL) {
            return &pm_data[i];
        }
    }

    return NULL;
}

/**
 * @brief 获取当前时间戳 (毫秒)
 */
static uint32_t pm_get_tick_ms(void)
{
    return xy_device_get_tick();
}

static bool pm_state_is_valid(xy_device_pm_state_t state)
{
    return state >= XY_DEVICE_PM_STATE_ACTIVE && state <= XY_DEVICE_PM_STATE_OFF;
}

static bool pm_policy_is_valid(xy_device_pm_policy_t policy)
{
    return policy >= XY_DEVICE_PM_POLICY_ALWAYS_ON && policy <= XY_DEVICE_PM_POLICY_MANUAL;
}

/* ==================== Public Implementation ==================== */

int xy_device_pm_init(xy_device_t *dev, const xy_device_pm_ops_t *pm_ops)
{
    if (!dev || !pm_ops) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    /* Repeated initialization of the same live owner is idempotent. */
    xy_device_pm_data_t *data = pm_find_data(dev);
    if (data) {
        return data->ops == pm_ops ? XY_DEVICE_OK : XY_DEVICE_ALREADY_INIT;
    }

    /* Allocate the first free power-management slot. */
    for (size_t i = 0; i < sizeof(pm_data) / sizeof(pm_data[0]); ++i) {
        if (pm_data[i].ops == NULL) {
            data = &pm_data[i];
            memset(data, 0, sizeof(*data));
            data->owner = dev;
            break;
        }
    }

    if (!data) {
        return XY_DEVICE_NO_MEM;
    }
    
    /* 初始化电源管理数据 */
    data->current_state = XY_DEVICE_PM_STATE_ACTIVE;
    data->last_state = XY_DEVICE_PM_STATE_ACTIVE;
    data->policy = XY_DEVICE_PM_POLICY_AUTO;
    data->ops = pm_ops;
    data->idle_timeout_ms = 0; /* 默认禁用空闲检测 */
    data->last_activity_time = pm_get_tick_ms();
    data->wakeup_enabled = false;
    
    pm_initialized = true;
    
    return XY_DEVICE_OK;
}

int xy_device_pm_deinit(xy_device_t *dev)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }

    xy_device_pm_data_t *data = pm_find_data(dev);
    if (!data) {
        return XY_DEVICE_NOT_INIT;
    }

    memset(data, 0, sizeof(*data));
    return XY_DEVICE_OK;
}

int xy_device_pm_set_state(xy_device_t *dev, xy_device_pm_state_t state)
{
    if (!dev || !pm_state_is_valid(state)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    xy_device_pm_data_t *data = pm_find_data(dev);
    
    if (!data || !data->ops) {
        return XY_DEVICE_NOT_INIT;
    }
    
    /* 状态未改变 */
    if (data->current_state == state) {
        return XY_DEVICE_OK;
    }
    
    /* 调用底层操作 */
    if (data->ops->set_state) {
        int ret = data->ops->set_state(dev, state);
        if (ret != XY_DEVICE_OK) {
            return ret;
        }
    }
    
    /* 更新状态 */
    data->last_state = data->current_state;
    data->current_state = state;
    data->last_activity_time = pm_get_tick_ms();
    
    return XY_DEVICE_OK;
}

int xy_device_pm_get_state(xy_device_t *dev, xy_device_pm_state_t *state)
{
    if (!dev || !state) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    xy_device_pm_data_t *data = pm_find_data(dev);
    
    if (!data) {
        return XY_DEVICE_NOT_INIT;
    }
    
    /* Commit driver-reported state only after a successful callback. */
    if (data->ops && data->ops->get_state) {
        xy_device_pm_state_t reported_state;
        xy_device_pm_state_t previous_state = data->current_state;
        int ret = data->ops->get_state(dev, &reported_state);
        if (ret != XY_DEVICE_OK) {
            return ret;
        }
        if (!pm_state_is_valid(reported_state)) {
            return XY_DEVICE_INVALID_PARAM;
        }

        if (reported_state != previous_state) {
            data->last_state = previous_state;
            data->current_state = reported_state;
            data->last_activity_time = pm_get_tick_ms();
        }
    }

    *state = data->current_state;
    return XY_DEVICE_OK;
}

int xy_device_pm_set_wakeup(xy_device_t *dev, bool enable)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    xy_device_pm_data_t *data = pm_find_data(dev);
    
    if (!data || !data->ops) {
        return XY_DEVICE_NOT_INIT;
    }
    
    /* 调用底层操作 */
    if (data->ops->set_wakeup) {
        int ret = data->ops->set_wakeup(dev, enable);
        if (ret != XY_DEVICE_OK) {
            return ret;
        }
    }

    data->wakeup_enabled = enable;
    return XY_DEVICE_OK;
}

int xy_device_pm_get_consumption(xy_device_t *dev, uint32_t *uw)
{
    if (!dev || !uw) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    xy_device_pm_data_t *data = pm_find_data(dev);
    
    if (!data || !data->ops) {
        return XY_DEVICE_NOT_INIT;
    }
    
    /* Commit callback output only after the driver reports success. */
    if (data->ops->get_power_consumption) {
        uint32_t measured_uw;
        int ret = data->ops->get_power_consumption(dev, &measured_uw);
        if (ret != XY_DEVICE_OK) {
            return ret;
        }

        *uw = measured_uw;
        return XY_DEVICE_OK;
    }
    
    /* 默认实现：根据状态估算 */
    switch (data->current_state) {
        case XY_DEVICE_PM_STATE_ACTIVE:
            *uw = 10000; /* 10mW */
            break;
        case XY_DEVICE_PM_STATE_SLEEP:
            *uw = 100; /* 100uW */
            break;
        case XY_DEVICE_PM_STATE_DEEP_SLEEP:
            *uw = 10; /* 10uW */
            break;
        case XY_DEVICE_PM_STATE_OFF:
            *uw = 1; /* 1uW (漏电) */
            break;
        default:
            *uw = 0;
    }
    
    return XY_DEVICE_OK;
}

int xy_device_pm_set_policy(xy_device_t *dev, xy_device_pm_policy_t policy)
{
    if (!dev || !pm_policy_is_valid(policy)) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    xy_device_pm_data_t *data = pm_find_data(dev);
    
    if (!data) {
        return XY_DEVICE_NOT_INIT;
    }
    
    data->policy = policy;
    return XY_DEVICE_OK;
}

xy_device_pm_policy_t xy_device_pm_get_policy(xy_device_t *dev)
{
    if (!dev) {
        return XY_DEVICE_PM_POLICY_ALWAYS_ON;
    }
    
    xy_device_pm_data_t *data = pm_find_data(dev);
    
    if (!data) {
        return XY_DEVICE_PM_POLICY_ALWAYS_ON;
    }
    
    return data->policy;
}

int xy_device_pm_sleep(xy_device_t *dev)
{
    return xy_device_pm_set_state(dev, XY_DEVICE_PM_STATE_SLEEP);
}

int xy_device_pm_wakeup(xy_device_t *dev)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    xy_device_pm_data_t *data = pm_find_data(dev);
    
    if (!data) {
        return XY_DEVICE_NOT_INIT;
    }
    
    /* 恢复到上一个状态或 ACTIVE */
    xy_device_pm_state_t target_state = data->last_state;
    if (target_state == XY_DEVICE_PM_STATE_OFF || 
        target_state == XY_DEVICE_PM_STATE_DEEP_SLEEP) {
        target_state = XY_DEVICE_PM_STATE_ACTIVE;
    }
    
    return xy_device_pm_set_state(dev, target_state);
}

int xy_device_pm_off(xy_device_t *dev)
{
    return xy_device_pm_set_state(dev, XY_DEVICE_PM_STATE_OFF);
}

int xy_device_pm_on(xy_device_t *dev)
{
    return xy_device_pm_set_state(dev, XY_DEVICE_PM_STATE_ACTIVE);
}

/* ==================== Auto Power Management ==================== */

/**
 * @brief 检查设备是否空闲超时
 */
void xy_device_pm_check_idle(xy_device_t *dev)
{
    if (!dev) {
        return;
    }
    
    xy_device_pm_data_t *data = pm_find_data(dev);
    
    if (!data || data->policy != XY_DEVICE_PM_POLICY_AUTO) {
        return;
    }
    
    if (data->idle_timeout_ms == 0) {
        return; /* 空闲检测禁用 */
    }
    
    uint32_t now = pm_get_tick_ms();
    uint32_t idle_time = now - data->last_activity_time;
    
    if (idle_time >= data->idle_timeout_ms) {
        /* 空闲超时，进入睡眠 */
        if (data->current_state == XY_DEVICE_PM_STATE_ACTIVE) {
            xy_device_pm_sleep(dev);
        }
    }
}

/**
 * @brief 设置空闲超时
 */
int xy_device_pm_set_idle_timeout(xy_device_t *dev, uint32_t timeout_ms)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    xy_device_pm_data_t *data = pm_find_data(dev);
    
    if (!data) {
        return XY_DEVICE_NOT_INIT;
    }
    
    data->idle_timeout_ms = timeout_ms;
    return XY_DEVICE_OK;
}

/**
 * @brief 记录设备活动
 */
void xy_device_pm_record_activity(xy_device_t *dev)
{
    if (!dev) {
        return;
    }
    
    xy_device_pm_data_t *data = pm_find_data(dev);
    
    if (!data) {
        return;
    }
    
    data->last_activity_time = pm_get_tick_ms();
    
    /* 如果是自动策略且当前在睡眠，唤醒设备 */
    if (data->policy == XY_DEVICE_PM_POLICY_AUTO &&
        data->current_state != XY_DEVICE_PM_STATE_ACTIVE) {
        xy_device_pm_wakeup(dev);
    }
}

/* ==================== End of File ==================== */
