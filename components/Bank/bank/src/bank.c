/**
 * @file bank.c
 * @author Xusheng Chen
 * @brief
 * @version 0.1
 * @date 2025-01-23
 *
 * @copyright Copyright (c) 2025
 *
 */

/**
 * @brief Bank App APIs
 * Bank Golbal APIs
 * - bank_get_xx
 * - bank_control_xx
 *
 * Bank Private APIs:
 * - bank_chrg_xx
 * - bank_batt_xx
 * - bank_event_xx_
 * - bank_msg_send
 * - bank_msg_rcv
 * - _xxxx
 *
 * Chrg Global APIs
 * - bank_chrg_xx
 *
 * Chrg Private APIs
 * - chrg_xx
 * - _xxxx
 *
 * Battery Global APIs
 * - bank_batt_xx
 *
 * Battery Private APIs
 * - batt_xx
 * - _xxxx
 */

#include "charge-log.h"
#include "event-log.h"
#include "bank.h"
#include "bank-internal.h"
#include "queue.h"
#include "target.h"

/* Private constants ---------------------------------------------------------*/
#define TAG "[Bank]"
#define TAG_EVT "[Bank][EVT]"
#define TAG_MSG "[Bank][MSG]"

#define USB_LOG_TYPE_ABSORTED 0x00
#define USB_LOG_TYPE_INSERTION 0x01
#define USB_LOG_TYPE_EXTRACTION 0x02

/**
 * @addtogroup Bank
 * @{
 */

/* Private variables ---------------------------------------------------------*/
static TimerHandle_t _bank_periodic_process_timer;

static QueueHandle_t bank_msq          = NULL;
const char *bank_msg_str[bank_msg_max] = {
    "init",     "periodic_update", "chrg_init",  "chrg_start", "chrg_resume",
    "chrg_top", "chrg_suspend",    "chrg_ghost", "chrg_idle",  "force_update"
};

const char *bank_chrg_stop_reason_str[] = { "completed", "user",
                                            "overheat",  "ovp",
                                            "hardfualt", "timeout",
                                            "chipfault", "cold" };


const char *bank_chrg_start_reation_str[] = { "normal", "command", "heating",
                                              "resume" };

/**
 * @brief
 *
 */
static bank_mode_t _band_mode = bank_mode_normal;

data_mgmt_charge_t _chrg_log_t = { 0 };

/* Private function prototypes  ----------------------------------------------*/

/**
 * @addtogroup Bank_Coontrol_Handler
 * @{
 */
static bank_ctrl_stat bank_control_enter_critical(void *arg);
static bank_ctrl_stat
bank_control_start_charging(bank_chrg_start_reason_t reason);
static bank_ctrl_stat
bank_control_stop_charging(bank_chrg_stop_reason_t reason);
static bank_ctrl_stat bank_control_start_heating(void);
static bank_ctrl_stat bank_control_stop_heating(void);
static bank_ctrl_stat bank_control_periodic_process_on(void);
static bank_ctrl_stat bank_control_periodic_process_off(void);

/** @} end of group Bank_Coontrol_Handler */

/**
 * @addtogroup Bank_Notify
 * @{
 */
/**
 * @defgroup Bank_Event_Handler Bank Event Handler
 * @{
 */
static void bank_event_callback(bank_event_t event, const void *data);
static void bank_event_chrg_wake_up(void *data);
static void bank_event_chrg_log_record(uint8_t start_log);
static void bank_event_chrg_has_fault(void *data);
static void bank_event_chrg_bus_status_changed(bank_event_t event, void *data);
static void bank_event_chrg_status_changed(bank_event_t event, void *data);
static void bank_event_chrg_host_communication(void *data);
static void bank_event_batt_level_changed(void *data);

/** @} end of group Bank_Event_Handler */

/** @} end of group Bank_Notify */

static uint32_t bank_msg_send(bank_msg_type_t ops, uint32_t p1, uint32_t p2);
static int32_t bank_msg_rcv(bank_msg_t *mail);
static void bank_periodic_process(void *arg);

static uint8_t bank_mode_update(void);


/* Exported functions -------------------------------------------------------*/

/**
 * @addtogroup Bank_Exported_Functions Bank Exported Functions
 * @{
 */

/**
 * @brief
 *
 * @param arg
 */


int32_t bank_init(void)
{
    if (bank_error_none == bank_chrg_init()) {
        LOGD(TAG, "bank_chrg_init successful!!!\r\n");
    } else {
        LOGE(TAG, "Charged init failed !!!\r\n");
    }

    // Must wait for chrg disable and idle battery voltage stably.
    vTaskDelay(50);

    if (bank_error_none == bank_batt_init()) {
        LOGD(TAG, "bank_batt_init successful!!!\r\n");
    } else {
        LOGE(TAG, "Charged init failed !!!\r\n");
    }

    if (bank_msq == NULL) {
        bank_msq = xQueueCreate(BANK_CFG_MSQ_MAX_NUM, sizeof(bank_msg_t));
        if (bank_msq == NULL) {
            LOGE(TAG, "Fail to create bank_msq\n");
        }
    }

    bank_event_hdlr_register(bank_event_callback);

    if (_bank_periodic_process_timer == NULL) {
        _bank_periodic_process_timer = xTimerCreate(
            "Bank", BANK_CFG_PROCESS_PERIOD, pdTRUE, (void *)NULL,
            bank_periodic_process);

        if (_bank_periodic_process_timer == NULL) {
            LOGE(TAG, "Fail to create _bank_periodic_process_timer!!!\r\n");
        }
    }

    return 0;
}

void bank_process(void)
{
    bank_msg_t mail = { bank_msg_none, 0, NULL };
    bank_msg_rcv(&mail);
    // TODO: need to lock process
    bank_chrg_process(&mail);
    bank_batt_process(&mail);
    // TODO: need to lock process
}

/**
 * @brief
 *
 * @param cmd
 * @param tick_wait
 * @return
 */
bank_ctrl_stat bank_control(bank_ctrl_cmd_t cmd, uint16_t tick_wait)
{
    switch (cmd) {
    case bank_ctrl_cmd_period_update_on:
        bank_control_periodic_process_on();
        break;
    case bank_ctrl_cmd_period_update_off:
        bank_control_periodic_process_off();
        break;
    default:
        break;
    }
    return bank_ctrl_nonsupport;
}

uint16_t bank_get_battery_voltage(void)
{
    return bank_batt_volt();
}

uint16_t bank_get_battery_voltage_rt(void)
{
    // return bank_adc_to_battery();
    return bank_batt_volt_adc();
}

uint16_t bang_get_battery_voltage_adc(void)
{
    return bank_batt_volt_adc();
}

uint8_t bank_get_battery_percent(void)
{
    return bank_batt_soc();
}

// TODO
uint8_t bank_get_battery_level(void)
{
    return bank_batt_level();
}

// TODO, need wrapped for ui
uint8_t bank_get_battery_ui_level(void)
{
    return bank_batt_level();
}

uint8_t bank_get_charge_state(void)
{
    // TODO: wrapper for other task
    return bank_chrg_state();
}

uint8_t bank_get_charge_bus_state(void)
{
    return bank_chrg_cable_status();
}

/** @} end of group Bank_Exported_Functions */


/* Private functions ---------------------------------------------------------*/
/**
 * @addtogroup Bank_Private_Functions Bank Private Functions
 * @{
 */

/** Bank contorl. */
/**
 * @brief
 *
 * @param arg
 * @return int32_t 0 success, !0 otherwise
 */
static bank_ctrl_stat bank_control_enter_critical(void *arg)
{
    UNUSED(arg);
    return 0;
}

static bank_ctrl_stat
bank_control_start_charging(bank_chrg_start_reason_t reason)
{
    bank_chrg_set_start_reason(reason);
    bank_msg_send(bank_msg_chrg_start, reason, 0);
    return bank_ctrl_ok;
}

static bank_ctrl_stat bank_control_stop_charging(bank_chrg_stop_reason_t reason)
{
    bank_chrg_set_stop_reason(reason);
    bank_msg_send(bank_msg_chrg_top, reason, 0);
    return bank_ctrl_ok;
}

static bank_ctrl_stat bank_control_start_heating(void)
{
    return bank_ctrl_ok;
}

static bank_ctrl_stat bank_control_stop_heating(void)
{
    return bank_ctrl_ok;
}

static bank_ctrl_stat bank_control_periodic_process_off(void)
{
    UBaseType_t uxQueueSize       = uxQueueMessagesWaiting(bank_msq);
    uint8_t bank_process_off_time = 0;
    LOGD(TAG, "bank_process_off\n");
    while (uxQueueSize) {
        vTaskDelay(BANK_CFG_PROCESS_PERIOD);
        bank_process_off_time++;
        uxQueueSize = uxQueueMessagesWaiting(bank_msq);
        if (bank_process_off_time > BANK_CFG_MSQ_MAX_NUM) {
            LOGW(TAG, "bank_process_off takes too long\n");
        }
    }

    return 0;
}

/**
 * @brief
 *  call by power on or wakeup
 *
 */
static bank_ctrl_stat bank_control_periodic_process_on(void)
{
    bank_mode_update();
    if (_bank_periodic_process_timer) {
        xTimerReset(_bank_periodic_process_timer, 0);
        xTimerStart(_bank_periodic_process_timer, 0);
    }
    LOGD(
        TAG,
        "bank contro"
        "l process on\n");
    return 0;
}


/**
 * @brief
 *
 * @param event bank_event_t
 * @param data
 */
static void bank_event_callback(bank_event_t event, const void *data)
{
    switch (event) {
    case bank_event_chrg_start:
    case bank_event_chrg_suspend:
    case bank_event_chrg_resume:
    case bank_event_chrg_stop:
        LOGD(TAG_EVT, "charge sate event:%d\n", event);
        bank_event_chrg_status_changed(event, data);
        break;
    case bank_event_chrg_bus_in:
    case bank_event_chrg_bus_out:
        bank_event_chrg_bus_status_changed(event, data);
        break;
    case bank_event_chrg_fault:
        bank_event_chrg_has_fault(data);
        break;
    case bank_event_chrg_int:
        bank_event_chrg_wake_up(data);
        break;

    default:
        LOGD(TAG_EVT, "bank unknonw event: %d\r\n", event);
        break;
    }
}

/**
 * @brief
 *
 * @param data
 * @note gpio intteryption operation, no blocking control is allowed!
 */
static void bank_event_chrg_power_status_changed(void *data)
{
    uint8_t system_status_sleep = 0;
    if (system_status_sleep) {
        // notify for device
        system_status_sleep = 0;
    }
}

static void bank_event_chrg_log_record(uint8_t start_log)
{
    if (start_log) {
        LOGD(TAG, "start charging log\r\n");
        _chrg_log_t.start_time      = app_rtc_get_utc_time();
        _chrg_log_t.batt_temp_start = bank_batt_temp();
        _chrg_log_t.mcu_temp_start  = bank_batt_ambient_temp();
        _chrg_log_t.voltage_start   = bank_batt_volt() * 32 / 1000;
        _chrg_log_t.gauge_start     = bank_batt_level();
        _chrg_log_t.pwr_src         = bank_chrg_source();

    } else {
        LOGD(TAG, "stop charging log\r\n");
        _chrg_log_t.duration      = bank_chrg_duration();
        _chrg_log_t.batt_temp_end = bank_batt_temp();
        _chrg_log_t.mcu_temp_end  = bank_batt_ambient_temp();
        _chrg_log_t.voltage_end   = bank_batt_volt() * 32 / 1000;
        _chrg_log_t.gauge_end     = bank_batt_level();
        _chrg_log_t.int_resistor =
            (bank_chrg_input_volt() - bank_batt_volt()) * 1000 / 64;
        _chrg_log_t.stop_reason = bank_chrg_stop_reason();
        _chrg_log_t.energy      = bank_chrg_energy();
    }
}

static void bank_event_chrg_has_fault(void *data)
{
    uint32_t fault = 0;
    if (data) {
        fault = *(uint32_t *)data;
    }
    LOGE(TAG, "charge fault %2x\r\n", fault);
    bank_control_stop_charging(bank_chrg_stop_reason_chip_fault);

    // set stop reason
    // send chrg control msg
}

static void bank_event_chrg_bus_status_changed(bank_event_t event, void *data)
{
    uint32_t status = 0;

    if (data) {
        status = *(uint32_t *)data;
    }

    if (status == bank_chrg_cable_out) {
        LOGD(TAG_EVT, "charger cable out\n");
        // cntlr_usb_deinitialize();
        // FIXME reason set show set by chrger
        bank_chrg_set_stop_reason(bank_chrg_stop_reason_user_action);
        bank_msg_send(bank_msg_chrg_top, bank_chrg_stop_reason_user_action, 0);

        // TODO record_usb_insertion_event_log(USB_LOG_TYPE_EXTRACTION, 0);
    } else {
        LOGD(TAG_EVT, "charger cable in, start detecting\n");
        // FIXME reason set show set by chrger
        bank_chrg_set_start_reason(bank_chrg_start_reason_insertion);
        bank_msg_send(bank_msg_chrg_start, bank_chrg_start_reason_insertion, 0);
        // cntlr_usb_deinitialize();
        // TODO record_usb_insertion_event_log(USB_LOG_TYPE_INSERTION, 0);
    }
}

/**
 * @brief
 *
 * @param event
 * @param data change reason
 */
static void bank_event_chrg_status_changed(bank_event_t event, void *data)
{
    switch (event) {
    case bank_event_chrg_start:
        LOGD(TAG_EVT, "charging start\n");
        // TODO update start chrg log time. reason ...
        cntlr_usb_init();
        break;
    case bank_event_chrg_suspend:
        break;
    case bank_event_chrg_resume:
        break;
    case bank_event_chrg_stop:
        // TODO update stop charge log..
        // Get stop reason.
        LOGD(TAG_EVT, "charging stop\n");
        break;
    default:
        break;
    }
}

/**
 * @brief
 *
 * @param data
 * @warning Interruption call , do not allow logging for deployment
 */
static void bank_event_chrg_wake_up(void *data)
{
    // check system status to determine should send msg
    // send ui msg to wake up
    // TODO: notify to ui task.
    return 0;
}

static void bank_event_chrg_host_communication(void *data)
{
    uint32_t host_communication_on = 0;
    if (data) {
        host_communication_on = *(uint32_t *)data;
    }
    LOGD(TAG_EVT, "charger communication handle %d\r\n", host_communication_on);
}

static void bank_event_batt_level_changed(void *data)
{
}


static uint32_t bank_msg_send(bank_msg_type_t ops, uint32_t p1, uint32_t p2)
{
    uint32_t ret    = bank_error_none;
    bank_msg_t mail = { 0 };
    if (bank_msq == NULL) {
        ret = bank_error_invalid_hdlr;
    }
    mail.msg = ops;
    mail.p1  = p1;
    mail.p2  = p2;
    if (__get_IPSR() != 0) {
        if (xQueueSendFromISR(bank_msq, &mail, 0) == errQUEUE_FULL) {
            ret = bank_error_msq_full;
            LOGE(TAG_MSG, "MSQ Fulled!\n");
        }
    } else {
        if (xQueueSend(bank_msq, &mail, 0) == errQUEUE_FULL) {
            ret = bank_error_msq_full;
            LOGE(TAG_MSG, "MSQ Fulled!\n");
        }
    }
    return ret;
}

static int32_t bank_msg_rcv(bank_msg_t *mail)
{
    int32_t ret = bank_error_none;
    if (xQueueReceive(bank_msq, mail, portMAX_DELAY)) {
        if (mail->msg != bank_msg_periodic_update) {
            LOGD(TAG, "Receive Message:%s\n", bank_msg_str[mail->msg]);
        }
    } else {
        ret = bank_error_msq_empty;
    }
    return ret;
}

/**
 * @brief bank_periodic_process timer handler
 *
 */
static void bank_periodic_process(void *arg)
{
    static uint32_t tick_cnt = 0;
    bank_msg_t mail          = { bank_msg_periodic_update, 0, 0 };

    if (_band_mode == bank_mode_normal) {
        bank_msg_send(bank_msg_periodic_update, 0, 0);
    }

    if (tick_cnt > BANK_CFG_PROCESS_5S_CNT) {
        LOGD(TAG, "bank battery get :%d mV!!!\r\n", bank_batt_volt());
        LOGD(TAG, "bank battery ambient temp:%d\r\n", bank_batt_ambient_temp());
        LOGD(TAG, "bank battery temp:%d\r\n", bank_batt_temp());
        tick_cnt = 0;
    } else {
        tick_cnt++;
    }
}


static uint8_t bank_mode_update(void)
{
    _band_mode = bank_mode_normal;
}

uint8_t bank_allow_sleeping(void)
{
    return (
        bank_chrg_cable_status() == bank_chrg_cable_out
        && bank_chrg_state() == bank_chrg_state_idle);
}

uint8_t bank_allow_heating(void)
{
    // need update soc immediately
    return (batt_update_soc() > bank_batt_level_critical);
}
/** @}  end of group Bank_Private_Functions */

/** @} end of group Bank */
