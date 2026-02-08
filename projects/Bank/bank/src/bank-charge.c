#include "bank-internal.h"
#define TAG "[Bank][Chrg]"

/**
 * @addtogroup Bank_Private_Modules
 * @{
 */

/**
 * @addtogroup Bank_Charge
 * @{
 */

typedef struct {
    uint8_t vbus_in_cnt;
    uint8_t vbus_out_cnt;
    bank_chrg_cable_status_t vbus_cable_status;
    uint16_t chrg_volt_lowvz;
    uint16_t chrg_duration;
    uint16_t chrg_target_volt;
    uint16_t chrg_target_current;
    int16_t chrg_current;
    uint16_t chrg_vol;
    uint64_t chrg_energy; // mW
    uint32_t chrg_last_full_tick;
} chrg_ctx_t;


typedef enum {
    chrg_time_start = 0,
    chrg_time_precharging,
    chrg_time_charging,
    chrg_time_ghost_charging,
    chrg_time_full,
    chrg_time_stop
} chrg_time_t;

typedef enum {
    chrg_statistics_start = 0,
    chrg_statistics_precharging,
    chrg_statistics_charging,
    chrg_statistics_ghost_charging,
    chrg_statistics_full,
    chrg_statistics_stop
} chrg_statistics_t;

/** When chrg disable, PG good , won't triiger this. */
#define CHRG_EVENT_BIT_STATUS_CHANGED 0x0001
/**
 * @brief Power good Pin Trigger
 * When power not good, won't trigger this. */
#define CHRG_EVENT_BIT_PG_CHANGED 0x0002

#define CHRG_POWER_ON 0
#define CHRG_POWER_OFF 1
#define CHRG_FULL_VOLT_DELTA 100

/* Private types -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static TimerHandle_t _vbus_detect_timer;
static TimerHandle_t _chrg_energy_timer;

static uint8_t _vbus_in_cnt                        = 0;
static uint8_t _vbus_out_cnt                       = 0;
static bank_chrg_cable_status_t _vbus_cable_status = bank_chrg_cable_out;

static chrg_ctx_t _chrg_ctx;

// map to driver_charger_source_type_t;
const char *source_type_name[driver_charger_source_type_none + 1] = {
    "DCP2050", "SDP500",   "SDP100",   "CDP2050", "CDP500",
    "CDP1500", "CDPH2050", "DOCP2050", "NSA1000", "NSA2000",
    "NSA2100", "NSA2400",  "UNKNOWN",  "ERROR"
};

const char *chrg_state_str[bank_chrg_state_max] = {
    "reseting",      "reset",       "configuring", "intializing",
    "initialized",   "idle",        "detecting",   "detecting_nsa",
    "enabling",      "precharging", "charging",    "ghost_charge",
    "topoff_charge", "charged",     "fault",       "toor"
};

const char *start_reason_str[bank_chrg_start_reason_unknown] = {
    "insert", "user", "cmd", "heat_off", "unknown"
};

const char *stop_reason_str[bank_chrg_stop_reason_unknown] = {
    "completed", "user", "overheat", "vor", "hfault",  "timeout",
    "cfault",    "cold", "heat",     "cmd", "extract", "unknown"
};


/* Private function prototypes ----------------------------------------------*/
static cntlr_chrg_faults_t chrg_fault_detect(void);
static int32_t chrg_init_process(const bank_msg_t *mail);
static int32_t chrg_idle_process(const bank_msg_t *mail);
static int32_t chrg_detect_process(const bank_msg_t *mail);
static int32_t chrg_precharging_process(const bank_msg_t *mail);
static int32_t chrg_charging_process(const bank_msg_t *mail);
static int32_t chrg_fault_process(const bank_msg_t *mail);
static int32_t chrg_toor_process(const bank_msg_t *mail);
static int32_t chrg_stop_charging_process(const bank_msg_t *mail);

static int32_t chrg_start_charing(const bank_chrg_state_t from_state);
static int32_t chrg_stop_charging(const bank_chrg_state_t from_state);
static int32_t chrg_statistics(chrg_statistics_t st_type);

static uint8_t _not_allowed_to_charge(const bank_msg_t *mail);
static uint16_t _input_max_current_wrapper(uint8_t src_type);
static int16_t chrg_get_mcu_temp(void);

static uint8_t chrg_full_checked(void);


static void chrg_vbus_monitor(void *arg);
static void chrg_vbus_monitor_start(void);
static void chrg_vbus_monitor_stop(void);


static void chrg_state_update_callback(uint32_t id, uint32_t event);
static void chrg_detect_callback(uint32_t id, uint32_t event);
static void chrg_ext_pwr_detect_callback(uint32_t id, uint32_t event);
cntlr_chrg_irq_cb_t bank_cb = { chrg_state_update_callback,
                                chrg_detect_callback,
                                chrg_ext_pwr_detect_callback };

/* Exported functions -------------------------------------------------------*/

int32_t bank_chrg_init(void)
{
    int ret = bank_error_none;

    if (_vbus_detect_timer == NULL) {
        _vbus_detect_timer = xTimerCreate(
            "Vbus", BANK_CFG_VBUS_DETECT_PERIOD, pdTRUE, (void *)NULL,
            chrg_vbus_monitor);
    }

    cntlr_charge_register_cb(&bank_cb);

    if (cntlr_charge_init()) {
        bank_chrg_set_state(bank_chrg_state_idle);
    } else {
        LOGE(TAG, "cntlr charge init failed\r\n");
        ret = bank_error_chrg_init;
    }

    // TODO get current charger chip to update the
    // cntlr_charge_self_test()
    chrg_vbus_monitor_start();

    return ret;
}

int32_t bank_chrg_process(const bank_msg_t *mail)
{
    int32_t ret = 0;
    // uint16_t batt_volt      = 0;
    // uint16_t target_current = 0;
    // uint16_t target_volt    = 0;
    // uint16_t input_current  = 0;
    // int16_t batt_current    = 0;
    static uint8_t loop_cnt = 0;
    if (loop_cnt > BANK_CFG_PROCESS_15S_CNT) {
        loop_cnt = 0;
        // LOGD(
        //     TAG, "state:%d get charge status:%d\n", bank_chrg_state(),
        //     cntlr_charge_state());
        // cntlr_charge_input_current(&input_current);
        // cntlr_charge_target_voltage(&target_volt);
        // cntlr_charge_target_current(&target_current);
        // cntlr_charge_battery_voltage(&batt_volt);
        // cntlr_charge_battery_current(&batt_current);

        // // LOGD(TAG, "set input volt:%d\n", 4600);
        // LOGD(TAG, "input current:%d\n", input_current);
        // LOGD(TAG, "target current:%d\n", target_current);
        // LOGD(TAG, "target voltage:%d\n", target_volt);
        // LOGD(TAG, "batt voltage:%d\n", batt_volt);
        // LOGD(TAG, "batt current:%d\n", batt_current);
    } else {
        loop_cnt++;
    }

    chrg_fault_detect();

    switch (bank_chrg_state()) {
    case bank_chrg_state_reseting:
    case bank_chrg_state_reset:
    case bank_chrg_state_configuring:
    case bank_chrg_state_initializing:
    case bank_chrg_state_initialized: {
        ret = chrg_init_process(mail);
    } break;
    case bank_chrg_state_idle: {
        ret = chrg_idle_process(mail);
    } break;

    case bank_chrg_state_detecting:
    case bank_chrg_state_detecting_non_standard:
    case bank_chrg_state_enabling: {
        ret = chrg_detect_process(mail);
    } break;
    case bank_chrg_state_precharging: {
        ret = chrg_precharging_process(mail);
    } break;
    case bank_chrg_state_charging:
    case bank_chrg_state_ghost_charge:
    case bank_chrg_state_topoff_charge:
    case bank_chrg_state_charged: {
        ret = chrg_charging_process(mail);
    } break;
    case bank_chrg_state_fault: {
        ret = chrg_fault_process(mail);
    } break;
    case bank_chrg_state_toor: {
        ret = chrg_toor_process(mail);
    } break;
    default:
        break;
    }

    return ret;
}


int32_t chrg_cable_connected(void)
{
    uint8_t power_pin;
    cntlr_charge_pin_level(chrg_det_pin_idx, &power_pin);
    return power_pin;
}


uint8_t bank_chrg_cable_status(void)
{
    // TODO, need change to vbus comp detect for cable status check.
    uint8_t power_pin_level;
    if (cntlr_charge_pin_level(chrg_det_pin_idx, &power_pin_level)) {
        // 0 , power on , 1 power off
        if (power_pin_level == 0) {
            return bank_chrg_cable_in;
        } else {
            return bank_chrg_cable_out;
        }
    }
    return bank_chrg_cable_out;
}


/* Private functions --------------------------------------------------------*/


static cntlr_chrg_faults_t chrg_fault_detect(void)
{
    static uint8_t chrg_fault_cnt = 0;
    uint32_t chrg_fault           = cntlr_chrg_faults_none;

    if (cntlr_charge_faults(&chrg_fault)) {
        if (chrg_fault) {
            chrg_fault_cnt++;
        } else {
            chrg_fault_cnt = 0;
        }
    }

    if (chrg_fault_cnt > 10) {
        bank_event_send(bank_event_chrg_fault, &chrg_fault);
        chrg_fault_cnt = 0;
    }

    // if battery unplug status will flash 1Hz , or temperature drop too much
    return chrg_fault;
}

static int32_t chrg_init_process(const bank_msg_t *mail)
{
    int32_t ret = bank_error_none;

    return ret;
}

static int32_t chrg_idle_process(const bank_msg_t *mail)
{
    int32_t ret = bank_error_none;
    switch (mail->msg) {
    case bank_msg_chrg_start:
        // check last full charge tick, if less than 20S, set charge full again
        // TODO: check the last full charge tick and band not changed.
        LOGD(TAG, "start usb_detect....\n");
        bank_chrg_set_state(bank_chrg_state_detecting);
        cntlr_charge_source_type_detection_enable();
        break;
    case bank_msg_none:
        break;
    default:
        // LOGD(
        //     TAG, "State:%d: Msg:%d  Unsupported!\n", bank_chrg_state(),
        //     mail->msg);
        break;
    }

exit:
    return ret;
}

static int32_t chrg_detect_process(const bank_msg_t *mail)
{
    int32_t ret                     = bank_error_none;
    static uint8_t detected_timeout = 0;
    static uint8_t retry_cnt        = BANK_CFG_CHRG_SOURCE_DETECT_RETRY_MAX;
    static uint8_t detection_done   = 0;
    uint8_t detected_type           = driver_charger_source_type_none;
    // uint16_t input_current        = 0;

    LOGD(TAG, "charge source detecting....\n");
    if (_not_allowed_to_charge(mail)) {
        detected_timeout = 0;
        retry_cnt        = BANK_CFG_CHRG_SOURCE_DETECT_RETRY_MAX;
        bank_chrg_set_state(bank_chrg_state_idle);
        goto exit;
    }

    cntlr_charge_get_source_type_detection_status(&detection_done);
    if ((detection_done == 0) && cntlr_charge_get_source_type(&detected_type)) {
        detected_timeout = 0;
        // cntlr_charge_input_current(&input_current);
        LOGD(TAG, "Detected type: %s\n", source_type_name[detected_type]);
        // LOGD(TAG, "cntlr_charge_input_current: %d\n", input_current);
    }

    detected_timeout++;

    if ((((detected_type == driver_charger_source_type_unknown_500ma)
          || (detected_type == driver_charger_source_type_none))
         && (detection_done == 0))
        || (detected_timeout > BANK_CFG_CHRG_SOURCE_DETECT_TIMEOUT)) {
        detected_timeout = 0;

        if (retry_cnt) {
            retry_cnt--;
            LOGD(TAG, "source detect retry :%d....\n", retry_cnt);
            cntlr_charge_source_type_detection_enable();
        }
    }


    if ((((detected_type == driver_charger_source_type_unknown_500ma)
          || (detected_type == driver_charger_source_type_none))
         && (retry_cnt == 0))
        || ((detected_type != driver_charger_source_type_unknown_500ma)
            && (detected_type != driver_charger_source_type_none))) {
        if (detected_type == driver_charger_source_type_none) {
            // TODO no charger found.
        } else {
            // TODO: stop detection
            LOGD(TAG, "stage changed , go to pre-charing...\n");
            // TODO need update by battery
#if BANK_CFG_CHRG_NO_CURRENT_LIMIT
            cntlr_charge_set_input_current(CHARGE_DRV_CHIP_INPUT_MAXCURR);
            cntlr_charge_set_target_current(CHARGE_DRV_CHIP_MAXCURR);
            cntlr_charge_set_target_voltage(bank_chrg_target_volt());
#else
            // by source type defined
            cntlr_charge_set_input_current(
                _input_max_current_wrapper(detected_type));
            // TODO need battery update target value
            cntlr_charge_set_target_current(BANK_CFG_CHRG_CURRENT_DEFAULT);
            cntlr_charge_set_target_voltage(bank_chrg_target_volt());
#endif
            detected_timeout = 0;
            retry_cnt        = BANK_CFG_CHRG_SOURCE_DETECT_RETRY_MAX;
            chrg_start_charing(bank_chrg_state());
            bank_chrg_set_target_duration(BANK_CFG_PRE_CHRG_TIMEOUT);
            bank_chrg_set_state(bank_chrg_state_precharging);
            // clear charing statistic
            chrg_statistics(chrg_statistics_start);
        }
    }
exit:
    return ret;
}

static int32_t chrg_precharging_process(const bank_msg_t *mail)
{
    int32_t ret                   = bank_error_none;
    static uint8_t chrg_step_cont = 0;
    uint16_t target_current_set;

    if (_not_allowed_to_charge(mail)) {
        chrg_stop_charging(bank_chrg_state());
        bank_chrg_set_state(bank_chrg_state_idle);
        chrg_step_cont = 0;
        goto exit;
    }

    if (bank_batt_volt() < bank_chrg_volt_lowvz()) {
        cntlr_charge_set_target_current(bank_chrg_precharge_current());
        chrg_step_cont = 0;
    } else {
        if (chrg_step_cont == 0) {
            // FIXME : cal chrg timeout type. source type and chrg current.
            if (bank_chrg_high_current_source()) {
                bank_chrg_set_target_duration(BANK_CFG_HI_CHRG_TIMEOUT);
            } else {
                bank_chrg_set_target_duration(BANK_CFG_LO_CHRG_TIMEOUT);
            }
        }

        chrg_step_cont++;

        target_current_set = chrg_step_cont * BANK_CFG_CHRG_CURRENT_STEP_SIZE
            + BANK_CFG_CHRG_CURRENT_DEFAULT;

#if BANK_CFG_CHRG_NO_CURRENT_LIMIT
        if (target_current_set <= BANK_CFG_CHRG_CURRENT_EXTR) {
            cntlr_charge_set_target_current(target_current_set);
        }
#else
        if (target_current_set <= bank_chrg_target_current()
            && target_current_set <= BANK_CFG_CHRG_CURRENT_MAX
            && chrg_step_cont
                <= (BANK_CFG_CHRG_CURRENT_MAX
                    / BANK_CFG_CHRG_CURRENT_STEP_SIZE)) {
            cntlr_charge_set_target_current(target_current_set);
        }
#endif
        else {
            chrg_step_cont = 0;
            bank_chrg_set_state(bank_chrg_state_charging);
        }
    }

    // TODO: target_update();

    // precharging statistics
    chrg_statistics(chrg_statistics_precharging);


exit:
    return ret;
}

static int32_t chrg_charging_process(const bank_msg_t *mail)
{
    int32_t ret = bank_error_none;

    // if need to stop & mail == stop, pause..
    if (_not_allowed_to_charge(mail)) {
        chrg_stop_charging(bank_chrg_state());
        bank_chrg_set_state(bank_chrg_state_idle);
        goto exit;
    }

    switch (mail->msg) {
    case bank_msg_chrg_start:
        /* code chrg_start */
        break;
    case bank_msg_none:
        break;
    default:
        /* code idle process */
        break;
    }

    if (chrg_full_checked()) {
        bank_chrg_set_state(bank_chrg_state_charged);
        bank_chrg_set_stop_reason(bank_chrg_stop_reason_completed);
        chrg_stop_charging(bank_chrg_state());
    } else {
        chrg_statistics(chrg_statistics_charging);
    }
    // TODO: target_update();
    // TODO: timeout check(precharge).

exit:
    return ret;
}

static int32_t chrg_full_process(const bank_msg_t *mail)
{
    int32_t ret = bank_error_chrg_fault;

    switch (mail->msg) {
    case bank_msg_chrg_start:
        /* by scp*/
        break;
    case bank_msg_chrg_top:
        /* from usb out. or scp cmd. */
        bank_chrg_set_state(bank_chrg_state_idle);
        break;
    case bank_msg_none:
        break;
    default:
        /* code idle process */
        break;
    }


exit:
    return ret;
}

static int32_t chrg_fault_process(const bank_msg_t *mail)
{
    int32_t ret = bank_error_chrg_fault;

    switch (mail->msg) {
    case bank_msg_chrg_start:
        /* code chrg_start */
        break;
    case bank_msg_none:
        break;
    default:
        /* code idle process */
        break;
    }

    // TODO check if usb out or no fault and last chrg state.

exit:
    return ret;
}

static int32_t chrg_toor_process(const bank_msg_t *mail)
{
    int32_t ret = bank_error_chrg_fault;

    // TODO check temp check last chrg_state, if charging, charging resume.
    // and fixs duration.
    switch (mail->msg) {
    case bank_msg_chrg_start:
        /* code chrg_start */
        break;
    case bank_msg_chrg_top:
    case bank_msg_none:
        break;
    default:
        /* code idle process */
        break;
    }

exit:
    return ret;
}

static int32_t chrg_start_charing(const bank_chrg_state_t from_state)
{
    int32_t ret = bank_error_none;
    // TODO, set precharing current, target current...
    if (cntlr_charge_enable()) {
        LOGD(TAG, "start charging from state:%s", chrg_state_str[from_state]);
        bank_event_send(bank_event_chrg_start, &from_state);
    }
    return ret;
}

static int32_t chrg_stop_charging(const bank_chrg_state_t from_state)
{
    int32_t ret = bank_error_none;
    // TODO, set precharing current, target current...
    if (cntlr_charge_disable()) {
        LOGD(TAG, "stop charging from state:%s", chrg_state_str[from_state]);
        bank_event_send(bank_event_chrg_stop, &from_state);
    }
    return ret;
}

/**
 * @brief Charge statistics
 * - Energy statistics
 * - Timeout statistics
 * TODO: chrg_statistics_t st_type show equal to chrg_state.
 */
static int32_t chrg_statistics(chrg_statistics_t st_type)
{
    int32_t ret                   = 0;
    uint32_t rtc_tick             = cntlr_rtc_gettime();
    uint32_t interval             = 0;
    static uint32_t last_rtc_tick = 0;

    chrg_statistics_t last_st_type = chrg_statistics_start;

    if (rtc_tick > last_rtc_tick) {
        interval = rtc_tick - last_rtc_tick;
    }

    if (interval > 0 && interval < 5) {
        _chrg_ctx.chrg_duration += interval;
        bank_chrg_set_duration(_chrg_ctx.chrg_duration);

        if (cntlr_charge_battery_current(&_chrg_ctx.chrg_current)
            && cntlr_charge_battery_voltage(&_chrg_ctx.chrg_vol)) {
            _chrg_ctx.chrg_energy +=
                interval * (_chrg_ctx.chrg_current) * (_chrg_ctx.chrg_vol);
            bank_chrg_set_energy(_chrg_ctx.chrg_energy / 1000000);
        }
    }

    switch (st_type) {
    case chrg_statistics_start:
        bank_chrg_set_energy(0);
        break;
    case chrg_statistics_precharging:
    case chrg_statistics_charging:
        if (_chrg_ctx.chrg_duration > bank_chrg_target_duration()) {
            // TODO: chrg time out
            // ret = chrg time out
        }
    case chrg_statistics_ghost_charging:
        break;
    case chrg_statistics_stop:
        bank_chrg_set_total_energy(
            bank_chrg_total_energy() + bank_chrg_energy());
        if (st_type == chrg_statistics_full) {
            _chrg_ctx.chrg_last_full_tick = rtc_tick;
        }
        LOGD(TAG, "chrg energy:%d\n", bank_chrg_energy());
        LOGD(TAG, "chrg total energy:%d\n", bank_chrg_total_energy());
        break;
    case chrg_statistics_full:
        break;
    default:
        break;
    }

    last_rtc_tick = rtc_tick;

    if (last_st_type != st_type) {
        _chrg_ctx.chrg_duration = 0;
        last_st_type            = st_type;
    }
    return ret;
}

static uint8_t _not_allowed_to_charge(const bank_msg_t *mail)
{
    if ((mail->msg == bank_msg_chrg_top)
        || (mail->msg == bank_msg_chrg_suspend)) {
        LOGD(
            TAG,
            "not_allowed_to_charge by reason: "
            "%s\n",
            stop_reason_str[bank_chrg_stop_reason()]);
        return true;
    }
    return false;
}

/**
 * @brief
 *
 * @note //TODO need wrapper by config file
 */
static uint16_t _input_max_current_wrapper(uint8_t src_type)
{
    switch (src_type) {
    case driver_charger_source_type_sdp_100ma:
        return BANK_CFG_CHRG_INPUT_LO_CURRENT;
    case driver_charger_source_type_unknown_500ma:
    case driver_charger_source_type_cdp_audio_500ma:
    case driver_charger_source_type_sdp_500ma:
        return BANK_CFG_CHRG_INPUT_DEFAULT_CURRENT;
    case driver_charger_source_type_non_standard_1000ma:
        return BANK_CFG_CHRG_INPUT_MEDIUM_CURRENT;
    case driver_charger_source_type_cdp_medium_1500ma:
        return BANK_CFG_CHRG_INPUT_HIGH_CURRENT;
    case driver_charger_source_type_dcp_2050ma:
    case driver_charger_source_type_cdp_default_2050ma:
    case driver_charger_source_type_cdp_high_2050ma:
    case driver_charger_source_type_sdp_scp_dock_2050ma:
    case driver_charger_source_type_non_standard_2000ma:
    case driver_charger_source_type_non_standard_2100ma:
    case driver_charger_source_type_non_standard_2400ma:
        return BANK_CFG_CHRG_INPUT_MAX_CURRENT;
    default:
        return 0;
    }
}

static int16_t chrg_get_mcu_temp(void)
{
    return 25;
}

static uint8_t chrg_full_checked(void)
{
    uint8_t pwr_pin_level = CHRG_POWER_OFF;

    cntlr_charge_pin_level(chrg_det_pin_idx, &pwr_pin_level);
    // TODO: add more conditions if needed.
    if ((cntlr_charge_state() == cntlr_chrg_state_charged)
        && pwr_pin_level == CHRG_POWER_ON
        && bank_batt_volt()
            > (bank_chrg_target_volt() - CHRG_FULL_VOLT_DELTA)) {
        return true;
    } else {
        return false;
    }
}

static void chrg_vbus_monitor(void *arg)
{
    uint8_t vbus_pin_level = 0;

    if (cntlr_charge_pin_level(chrg_det_pin_idx, &vbus_pin_level)) {
        if (vbus_pin_level) {
            _vbus_out_cnt++;
            if (_vbus_in_cnt) {
                _vbus_in_cnt--;
            }
        } else {
            _vbus_in_cnt++;
            if (_vbus_out_cnt) {
                _vbus_out_cnt--;
            }
        }
    }

    if (_vbus_in_cnt >= BANK_CFG_VBUS_DETECT_IN_COUNT) {
        if (bank_chrg_cable_out == _vbus_cable_status) {
            _vbus_cable_status = bank_chrg_cable_in;
            bank_event_send(bank_event_chrg_bus_in, &_vbus_cable_status);
        }
        _vbus_in_cnt  = 0;
        _vbus_out_cnt = 0;
    } else if (_vbus_out_cnt >= BANK_CFG_VBUS_DETECT_OUT_COUNT) {
        if (bank_chrg_cable_in == _vbus_cable_status) {
            _vbus_cable_status = bank_chrg_cable_out;
            bank_event_send(bank_event_chrg_bus_out, &_vbus_cable_status);
        }
        _vbus_in_cnt  = 0;
        _vbus_out_cnt = 0;
    }
}


static void chrg_vbus_monitor_start(void)
{
    if (_vbus_detect_timer) {
        xTimerReset(_vbus_detect_timer, 0);
        xTimerStart(_vbus_detect_timer, 0);
    }
    _vbus_in_cnt  = 0;
    _vbus_out_cnt = 0;
}

static void chrg_vbus_monitor_stop(void)
{
    if (_vbus_detect_timer) {
        xTimerStop(_vbus_detect_timer, 0);
    }
    _vbus_in_cnt  = 0;
    _vbus_out_cnt = 0;
}

static void chrg_state_update_callback(uint32_t id, uint32_t event)
{
    UNUSED(id);
    UNUSED(event);
    uint32_t int_id = CHRG_EVENT_BIT_STATUS_CHANGED;
    bank_event_send(bank_event_chrg_int, &int_id);
}

/**
 * @brief charger power detect pin status changed
 *
 * @param id
 * @param event
 */
static void chrg_detect_callback(uint32_t id, uint32_t event)
{
    UNUSED(id);
    UNUSED(event);
    uint32_t int_id = CHRG_EVENT_BIT_PG_CHANGED;
    /** charger power detect nitify. */
    bank_event_send(bank_event_chrg_int, &int_id);
}

static void chrg_ext_pwr_detect_callback(uint32_t id, uint32_t event)
{
    UNUSED(id);
    UNUSED(event);
    bank_event_send(bank_event_chrg_int, NULL);
}

/** @} end of group Bank_Charge */
/** @} end of group Bank_Private_Modules */
