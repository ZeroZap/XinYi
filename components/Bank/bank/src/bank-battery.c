#include "controller-adc.h"
#include "bank-internal.h"

#if (BANK_CFG_BATT_MFG_ATL_ENABLE)
#include "bank-battery-atl.h"
#endif

#define TAG "[Bank][Batt]"

/* Private types -------------------------------------------------------------*/
/** Different direction of changes allowed to update SOC on blackboard. */
typedef enum {
    /** Only increasing SOC is allowed. */
    soc_direction_up = 0x00,

    /** Only decreasing SOC is allowed. */
    soc_direction_down
} soc_direction_t;

typedef enum {
    soc_update_normal = 0,
    soc_update_at_once,
} soc_update_mode_t;

typedef struct {
    bank_chrg_state_t last_state;
    const bank_batt_soc_element_t *soc_idle;
    const bank_batt_soc_element_t *soc_charging;
    bank_batt_mfg_t mfg;
    soc_direction_t soc_direction;
    const bank_batt_target_profile_t *target_profile; /* from profile table. */
    const batt_ntc_temprature *ntc_temp_table;
    uint16_t idle_protect_mv;                   /* from config. */
    uint16_t idle_flat_mv;                      /* from config. */
    uint16_t idle_critical_mv;                  /* from config. */
    uint16_t low_current_charging_critical_mv;  /* from config. */
    uint16_t high_current_charging_critical_mv; /* from config. */
    uint16_t v100_idle;                         /* from config. */
    uint16_t v100_charging;                     /* from config. */
} bank_batt_ctx_t;

/* Private constants ---------------------------------------------------------*/
#define BATT_ADC_UPDATE_MAX_CNT 5

/* Private macros ------------------------------------------------------------*/
#define ARRAY_LENGTH(arr) (sizeof(arr) / sizeof((arr)[0]))

/* Private variables ---------------------------------------------------------*/
static bank_chrg_state_t _chrg_last_state = bank_chrg_state_reset;

static bank_batt_ctx_t _bank_batt_ctx;
static uint16_t _batt_volt_adc_table[BATT_ADC_UPDATE_MAX_CNT] = { 0 };

static uint8_t _soc_element_num = 0;

const batt_ntc_temprature
    batt_default_ntc_temprature_table[batt_ntc_temprature_max_span] = {
        { batt_ntc_temprature_m_25, 3672 }, { batt_ntc_temprature_m_24, 3653 },
        { batt_ntc_temprature_m_23, 3633 }, { batt_ntc_temprature_m_22, 3612 },
        { batt_ntc_temprature_m_21, 3591 }, { batt_ntc_temprature_m_20, 3569 },
        { batt_ntc_temprature_m_19, 3547 }, { batt_ntc_temprature_m_18, 3524 },
        { batt_ntc_temprature_m_17, 3500 }, { batt_ntc_temprature_m_16, 3476 },
        { batt_ntc_temprature_m_15, 3451 }, { batt_ntc_temprature_m_14, 3425 },
        { batt_ntc_temprature_m_13, 3398 }, { batt_ntc_temprature_m_12, 3371 },
        { batt_ntc_temprature_m_11, 3343 }, { batt_ntc_temprature_m_10, 3315 },
        { batt_ntc_temprature_m_09, 3286 }, { batt_ntc_temprature_m_08, 3256 },
        { batt_ntc_temprature_m_07, 3226 }, { batt_ntc_temprature_m_06, 3195 },
        { batt_ntc_temprature_m_05, 3164 }, { batt_ntc_temprature_m_04, 3131 },
        { batt_ntc_temprature_m_03, 3099 }, { batt_ntc_temprature_m_02, 3066 },
        { batt_ntc_temprature_m_01, 3032 }, { batt_ntc_temprature_0, 2997 },
        { batt_ntc_temprature_p_01, 2963 }, { batt_ntc_temprature_p_02, 2927 },
        { batt_ntc_temprature_p_03, 2892 }, { batt_ntc_temprature_p_04, 2856 },
        { batt_ntc_temprature_p_05, 2819 }, { batt_ntc_temprature_p_06, 2782 },
        { batt_ntc_temprature_p_07, 2745 }, { batt_ntc_temprature_p_08, 2707 },
        { batt_ntc_temprature_p_09, 2669 }, { batt_ntc_temprature_p_10, 2631 },
        { batt_ntc_temprature_p_11, 2593 }, { batt_ntc_temprature_p_12, 2554 },
        { batt_ntc_temprature_p_13, 2515 }, { batt_ntc_temprature_p_14, 2476 },
        { batt_ntc_temprature_p_15, 2438 }, { batt_ntc_temprature_p_16, 2399 },
        { batt_ntc_temprature_p_17, 2360 }, { batt_ntc_temprature_p_18, 2321 },
        { batt_ntc_temprature_p_19, 2281 }, { batt_ntc_temprature_p_20, 2242 },
        { batt_ntc_temprature_p_21, 2203 }, { batt_ntc_temprature_p_22, 2164 },
        { batt_ntc_temprature_p_23, 2125 }, { batt_ntc_temprature_p_24, 2086 },
        { batt_ntc_temprature_p_25, 2048 }, { batt_ntc_temprature_p_26, 2010 },
        { batt_ntc_temprature_p_27, 1972 }, { batt_ntc_temprature_p_28, 1934 },
        { batt_ntc_temprature_p_29, 1896 }, { batt_ntc_temprature_p_30, 1859 },
        { batt_ntc_temprature_p_31, 1822 }, { batt_ntc_temprature_p_32, 1786 },
        { batt_ntc_temprature_p_33, 1750 }, { batt_ntc_temprature_p_34, 1714 },
        { batt_ntc_temprature_p_35, 1678 }, { batt_ntc_temprature_p_36, 1643 },
        { batt_ntc_temprature_p_37, 1609 }, { batt_ntc_temprature_p_38, 1575 },
        { batt_ntc_temprature_p_39, 1541 }, { batt_ntc_temprature_p_40, 1508 },
        { batt_ntc_temprature_p_41, 1475 }, { batt_ntc_temprature_p_42, 1443 },
        { batt_ntc_temprature_p_43, 1411 }, { batt_ntc_temprature_p_44, 1380 },
        { batt_ntc_temprature_p_45, 1349 }, { batt_ntc_temprature_p_46, 1319 },
        { batt_ntc_temprature_p_47, 1289 }, { batt_ntc_temprature_p_48, 1260 },
        { batt_ntc_temprature_p_49, 1231 }, { batt_ntc_temprature_p_50, 1203 },
        { batt_ntc_temprature_p_51, 1175 }, { batt_ntc_temprature_p_52, 1148 },
        { batt_ntc_temprature_p_53, 1122 }, { batt_ntc_temprature_p_54, 1095 },
        { batt_ntc_temprature_p_55, 1070 }, { batt_ntc_temprature_p_56, 1045 },
        { batt_ntc_temprature_p_57, 1020 }, { batt_ntc_temprature_p_58, 996 },
        { batt_ntc_temprature_p_59, 973 },  { batt_ntc_temprature_p_60, 950 },
        { batt_ntc_temprature_p_61, 927 },  { batt_ntc_temprature_p_62, 905 },
        { batt_ntc_temprature_p_63, 884 },  { batt_ntc_temprature_p_64, 863 },
        { batt_ntc_temprature_p_65, 842 },  { batt_ntc_temprature_p_66, 822 },
        { batt_ntc_temprature_p_67, 802 },  { batt_ntc_temprature_p_68, 783 },
        { batt_ntc_temprature_p_69, 765 },  { batt_ntc_temprature_p_70, 746 },
        { batt_ntc_temprature_p_71, 728 },  { batt_ntc_temprature_p_72, 711 },
        { batt_ntc_temprature_p_73, 694 },  { batt_ntc_temprature_p_74, 677 },
        { batt_ntc_temprature_p_75, 661 },  { batt_ntc_temprature_p_76, 645 },
        { batt_ntc_temprature_p_77, 630 },  { batt_ntc_temprature_p_78, 614 },
        { batt_ntc_temprature_p_79, 600 },  { batt_ntc_temprature_p_80, 586 },
        { batt_ntc_temprature_p_81, 572 },  { batt_ntc_temprature_p_82, 558 },
        { batt_ntc_temprature_p_83, 545 },  { batt_ntc_temprature_p_84, 532 },
        { batt_ntc_temprature_p_85, 519 },  { batt_ntc_temprature_p_86, 507 },
        { batt_ntc_temprature_p_87, 495 },  { batt_ntc_temprature_p_88, 483 },
        { batt_ntc_temprature_p_89, 472 },  { batt_ntc_temprature_p_90, 461 },
        { batt_ntc_temprature_p_91, 450 },  { batt_ntc_temprature_p_92, 439 },
        { batt_ntc_temprature_p_93, 429 },  { batt_ntc_temprature_p_94, 419 },
        { batt_ntc_temprature_p_95, 409 },  { batt_ntc_temprature_p_96, 400 },
        { batt_ntc_temprature_p_97, 391 },  { batt_ntc_temprature_p_98, 381 },
        { batt_ntc_temprature_p_99, 373 },  { batt_ntc_temprature_p_100, 364 },
        { batt_ntc_temprature_p_101, 356 },
    };

/* Private function prototypes  ----------------------------------------------*/
static int32_t batt_update_configuration(void);
static int32_t batt_volt_temp_init(void);
static uint8_t batt_update_soc(uint8_t at_once);
static uint8_t batt_update_soh(uint8_t at_once);

static uint8_t batt_update_temp(void);
static uint8_t batt_update_volt(void);
static uint8_t batt_update_soc_table(void);
static uint8_t batt_update_soc_direction(void);
static uint8_t batt_update_temp_band(void);
static uint8_t batt_update_age(uint32_t exp);
static uint8_t batt_update_target_volt_current(void);
static bank_batt_level_t compute_soc(
    const bank_batt_soc_element_t *soc_table, const uint16_t critical_level,
    soc_direction_t direction);
static uint8_t compute_soc_percent(
    const bank_batt_soc_element_t *soc_table, const uint16_t v100percent,
    uint16_t critical_level, soc_direction_t direction);

static uint8_t compute_soc_percent(
    const bank_batt_soc_element_t *soc_table, const uint16_t v100percent,
    uint16_t critical_level, soc_direction_t direction);
static uint8_t soc_percent_extra_limitation(
    const bank_batt_soc_element_t *soc_table, bank_batt_level_t batt_level,
    uint8_t new_percent, soc_direction_t direction);
static uint16_t _adc_to_voltage(uint16_t adc_value);
static int8_t _adc_to_temp(uint16_t ntc_value);
static uint16_t _adc_median(uint16_t arr[], uint8_t n);
static bank_batt_temp_band_t _temp_to_band(int8_t tempband);
static bank_batt_age_t _aging_level(uint32_t exp);


/* Exported functions
   --------------------------------------------------------*/
int32_t bank_batt_init(void)
{
    batt_update_configuration();
    batt_volt_temp_init();
    batt_update_age(BANK_CFG_BATT_AGE_THRESHOLD1 - 1);
    batt_update_temp_band();
    _bank_batt_ctx.last_state = bank_chrg_state_idle;
    batt_update_soc_direction();
#if BANK_SUPPORT_FUEL_GAUGE
    cntlr_fuelgauge_init(dev_info, 100, ture);
    cntlr_fuelgauge_dev_info(dev_info);
    cntlr_fuelgauge_bc_info(cell_info);
#endif
    batt_update_soc_table();
    batt_update_target_volt_current();
    batt_update_soc(soc_update_at_once);
    batt_update_soh(soc_update_at_once);
    return 0;
}

int32_t bank_batt_process(const bank_msg_t *mail)
{
    int32_t ret                      = bank_error_none;
    static uint16_t batt_process_cnt = 0;

    batt_update_temp();
    batt_update_volt();
    batt_update_temp_band();
    batt_update_age(BANK_CFG_BATT_AGE_THRESHOLD1 - 1);
    batt_update_target_volt_current();
    batt_update_soc_direction();
    batt_update_soc_table();
    if (mail->msg == bank_msg_batt_force_update
        || batt_process_cnt >= BANK_CFG_PROCESS_15S_CNT) {
        batt_process_cnt = 0;
        batt_update_soc(soc_update_normal);
        batt_update_soh(soc_update_normal);
    }

    if (mail->msg == bank_msg_periodic_update
        && batt_process_cnt < BANK_CFG_PROCESS_15S_CNT) {
        batt_process_cnt++;
    }

    // TODO msg stop heating.. batt_process_cnt  = 0
    // overheat...
exit:
    return ret;
}


/* Private functions---------------------------------------------------------*/


static int32_t batt_update_configuration(void)
{
    // get config core from otp
    // get configuartiong from eeprom

    switch (_bank_batt_ctx.mfg) {

    case bank_batt_mfg_atl:
    default:
        _bank_batt_ctx.idle_protect_mv  = BANK_CFG_DEFAULT_IDLE_PROTECT_MV;
        _bank_batt_ctx.idle_flat_mv     = BANK_CFG_DEFAULT_IDLE_FLAT_MV;
        _bank_batt_ctx.idle_critical_mv = BANK_CFG_DEFAULT_IDLE_CRITICAL_MV;
        _bank_batt_ctx.low_current_charging_critical_mv =
            BANK_CFG_DEFAULT_LO_CHARGING_CRITICAL_MV;
        _bank_batt_ctx.high_current_charging_critical_mv =
            BANK_CFG_DEFAULT_HI_CHARGING_CRITICAL_MV;
        _bank_batt_ctx.ntc_temp_table = batt_default_ntc_temprature_table;
        break;
    }
    // battery serieals critical value
    //
    return 0;
}

static int32_t batt_volt_temp_init(void)
{
    int32_t ret = bank_error_none;
    uint16_t ntc_adc_value;
    uint16_t ambient_adc_value;
    int16_t ambient_temp;

    for (int i = 0; i < BATT_ADC_UPDATE_MAX_CNT; i++) {
        // TODO need add ambient temp
        if ((cntlr_adc_get(cntlr_adc_channel_ext_temp_sensor, &ntc_adc_value)
             == false)
            || (cntlr_adc_get(
                    cntlr_adc_channel_inner_temp_sensor, &ambient_adc_value)
                == false)
            || (cntlr_adc_get(
                    cntlr_adc_channel_batt_volt, &_batt_volt_adc_table[i])
                == false)) {
            ret = bank_error_batt_adc;
            goto exit;
        }
        vTaskDelay(20);
    };

    // get last temp value
    bank_batt_set_temp_adc(ntc_adc_value);
    bank_batt_set_temp(_adc_to_temp(ntc_adc_value));

    // TODO need add ambient temp
    cntrl_adc_get_inner_temp(&ambient_temp);
    bank_batt_set_ambient_temp_adc(ambient_adc_value);
    if ((ambient_temp > BANK_CFG_BATT_MIN_AMBIENT_TEMP)
        && (ambient_temp < BANK_CFG_BATT_MAX_AMBIENT_TEMP)) {
        bank_batt_set_ambient_temp(ambient_temp);
    } else {
        ret = bank_error_batt_adc;
    }

    bank_batt_set_volt(_adc_to_voltage(
        _adc_median(_batt_volt_adc_table, BATT_ADC_UPDATE_MAX_CNT)));

exit:
    return ret;
}

/**
 * @brief Battery soc update
 * - soc level update
 * - soc percent update
 *
 * @return return if soc critical.
 */
static bank_batt_level_t batt_update_soc(uint8_t at_once)
{
    uint8_t new_percent = 0;
    bank_batt_level_t new_level;
    const bank_batt_soc_element_t *soc_element;
    uint16_t critical_level;
    uint16_t v100percent;
    soc_direction_t soc_direction = _bank_batt_ctx.soc_direction;

#if BANK_USE_FUEL_GAUGE_SOC

#else
    if (soc_direction == soc_direction_up) {
        if (_bank_batt_ctx.target_profile->current
                > BANK_CFG_MAX_MID_CHRG_CURRENT_MA
            && bank_chrg_high_current_source) {
            critical_level = _bank_batt_ctx.high_current_charging_critical_mv;
        } else {
            critical_level = _bank_batt_ctx.low_current_charging_critical_mv;
        }
        v100percent = _bank_batt_ctx.v100_charging;
        soc_element = _bank_batt_ctx.soc_charging;
    } else {
        critical_level = _bank_batt_ctx.idle_critical_mv;
        v100percent    = _bank_batt_ctx.v100_idle;
        soc_element    = _bank_batt_ctx.soc_idle;
    }

    new_level = compute_soc(soc_element, critical_level, soc_direction);
    if ((new_level > bank_batt_level() && soc_direction == soc_direction_up)
        || (new_level < bank_batt_level()
            && soc_direction == soc_direction_down)) {
        bank_batt_set_level(new_level);
        // TODO: notify..
    }
    new_percent = compute_soc_percent(
        soc_element, v100percent, critical_level, soc_direction);
    // TODO lock percent (new_percent, new_level, soc_direction)
    new_percent = soc_percent_extra_limitation(
        soc_element, new_level, new_percent, soc_direction);
    if ((new_percent > bank_batt_soc() && soc_direction == soc_direction_up)
        || (new_percent < bank_batt_soc()
            && soc_direction == soc_direction_down)) {
        bank_batt_set_soc(new_percent);
    }
#endif
    LOGD(TAG, "critical_level =%d\n", critical_level);
    LOGD(TAG, "v100percent =%d\n", v100percent);
    LOGD(TAG, "new_level =%d\n", new_level);
    LOGD(TAG, "new_percent =%d\n", new_percent);
    LOGD(TAG, "battery voltage=%d\n", bank_batt_volt());
    return new_level;
}

static uint8_t batt_update_soh(uint8_t at_once)
{
    uint8_t ret = false;
#if BANK_USE_FUEL_GAUGE_SOH

#else

#endif
    return ret;
}

static uint8_t batt_low_check(void)
{
    //     if ((bank_batt_level_unknown == bb_bank_batt_level())
    //     || _bank_batt_ctx.force_soc_level_update
    //     || (soc_direction_up == direction
    //         && soc_level > bb_bank_batt_level())
    //     || (soc_direction_down == direction
    //         && soc_level < bb_bank_batt_level())) {
    //     bb_set_bank_batt_level(soc_level);
    //     save_batt_level(soc_level);
    //     check_low_batt();
    // }
    return 0;
}

static uint8_t batt_update_temp(void)
{
    int8_t batt_temp = 0;
    int16_t ambient_temp;
    uint16_t batt_ntc_adc;
    uint16_t ambient_adc_value;
#if BANK_USE_FUEL_GAUGE_BATT_TEMP

#else
    // TODO need add ambient channel
    if (cntlr_adc_get(cntlr_adc_channel_ext_temp_sensor, &batt_ntc_adc)
        || cntlr_adc_get(
            cntlr_adc_channel_inner_temp_sensor, &ambient_adc_value)) {

        bank_batt_set_temp_adc(batt_ntc_adc);
        // bank_batt_set_temp(_adc_to_temp(batt_ntc_adc));
        batt_temp = _adc_to_temp(batt_ntc_adc);
        // TODO ambient temp handle
        cntrl_adc_get_inner_temp(&ambient_temp);
        bank_batt_set_ambient_temp_adc(batt_ntc_adc);
        if ((ambient_temp >= -45) && (ambient_temp < 128)) {
            bank_batt_set_ambient_temp(ambient_temp);
        }

    } else {
        return false;
    }

    if (batt_temp != bank_batt_temp()) {
        bank_batt_set_temp(batt_temp);
        return true;
    } else {
        return false;
    }
#endif
}

static uint8_t batt_update_volt(void)
{
    static uint8_t batt_volt_adc_update_cnt = 0;

    uint8_t ret = true;


#if BANK_USE_FUEL_GAUGE_BATT_VOL

#else
    if (cntlr_adc_get(
            cntlr_adc_channel_batt_volt,
            &_batt_volt_adc_table[batt_volt_adc_update_cnt])) {
        bank_batt_set_volt_adc(_batt_volt_adc_table[batt_volt_adc_update_cnt]);
    } else {
        ret = false;
        goto exit;
    }

    if (batt_volt_adc_update_cnt < BATT_ADC_UPDATE_MAX_CNT) {
        batt_volt_adc_update_cnt++;
    } else {
        bank_batt_set_volt(_adc_to_voltage(
            _adc_median(_batt_volt_adc_table, BATT_ADC_UPDATE_MAX_CNT)));
        batt_volt_adc_update_cnt = 0;
    }
#endif
    // TODO, overcharge = target volt + 200mV?
exit:
    return ret;
}

static uint8_t batt_update_soc_table(void)
{
    uint8_t ret = true;
    switch (_bank_batt_ctx.mfg) {
    case bank_batt_mfg_atl:
    default:
        _bank_batt_ctx.soc_idle =
            batt_atl_soc_idle[bank_batt_age()][bank_batt_temp_band()];
        _bank_batt_ctx.soc_charging =
            batt_atl_soc_charging[bank_batt_age()][bank_batt_temp_band()];
        _bank_batt_ctx.v100_idle =
            batt_atl_v100_idle[bank_batt_age()][bank_batt_temp_band()];
        _bank_batt_ctx.v100_charging =
            batt_atl_v100_charging[bank_batt_age()][bank_batt_temp_band()];
        _bank_batt_ctx.target_profile =
            &batt_atl_profile[bank_batt_age()][bank_batt_temp_band()];

        _soc_element_num = ARRAY_LENGTH(batt_atl_soc_idle);

        break;
    }
    return ret;
}

static uint8_t batt_update_soc_direction(void)
{
    if (bank_chrg_state_precharging == bank_chrg_state()
        || bank_chrg_state_charging == bank_chrg_state()
        || bank_chrg_state_ghost_charge == bank_chrg_state()) {
        _bank_batt_ctx.soc_direction = soc_direction_up;
    } else {
        _bank_batt_ctx.soc_direction = soc_direction_down;
    }

    return true;
}

static uint8_t batt_update_target_volt_current(void)
{
    uint8_t ret = false;
    bank_chrg_set_target_current(_bank_batt_ctx.target_profile->current);
    bank_chrg_set_target_volt(_bank_batt_ctx.target_profile->volt);
    return ret;

    // get batt temp , amb_temp
}

static uint8_t batt_update_temp_band(void)
{
    bank_batt_temp_band_t band = _temp_to_band(bank_batt_temp());

    if (band != bank_batt_temp_band()) {
        bank_batt_set_temp_band(band);
        return true;
    } else {
        return false;
    }
}

static uint8_t batt_update_age(uint32_t exp)
{
    bank_batt_age_t age = _aging_level(exp);

    if (age != bank_batt_age()) {
        bank_batt_set_age(age);
        return true;
    } else {
        return false;
    }
}

static bank_batt_level_t compute_soc(
    const bank_batt_soc_element_t *soc_table, const uint16_t critical_level,
    soc_direction_t direction)
{
    bank_batt_level_t soc_level = bank_batt_level();
    uint16_t batt_voltage       = bank_batt_volt();

    for (uint8_t i = 0; i < BANK_CFG_BATT_SOC_LEVEL_SIZE; ++i) {
        if (BANK_CFG_BATT_SOC_LEVEL_SIZE - 1 == i) {
            if (batt_voltage > critical_level) {
                soc_level = bank_batt_level_low;
            } else if (batt_voltage > soc_table[i].voltage) {
                soc_level = bank_batt_level_critical;
                break;
            } else {
                soc_level = bank_batt_level_flat;
            }
        } else if (batt_voltage >= soc_table[i].voltage) {
            soc_level = soc_table[i].level;
            break;
        }
    }

    return soc_level;
}

static uint8_t compute_soc_percent(
    const bank_batt_soc_element_t *soc_table, const uint16_t v100_percent,
    uint16_t critical_level, soc_direction_t direction)
{
    uint8_t soc_level_percent     = 0;
    uint16_t higher_level_voltage = 0;
    uint16_t lower_level_voltage  = 0;
    uint16_t batt_voltage         = bank_batt_volt();
    uint8_t critical_percent;
    uint8_t base_percent;
    uint8_t level_scrop = 0;
    uint8_t table_index = 0;


    if (batt_voltage >= v100_percent) {
        soc_level_percent = 100;
        return soc_level_percent;
    }

    /** lower than flat level, equal to 0. */
    if (batt_voltage <= soc_table[BANK_CFG_BATT_SOC_LEVEL_SIZE - 1].voltage) {
        soc_level_percent = 0;
        return soc_level_percent;
    }

    if (soc_direction_down == direction) {
        critical_percent = BANK_CFG_DEFAULT_IDLE_CRITICAL_LEVEL;
    } else {
        critical_percent = BANK_CFG_DEFAULT_CHRG_CRITICAL_LEVEL;
    }

    /** search current voltage table index. */
    for (table_index = 0; table_index < BANK_CFG_BATT_SOC_LEVEL_SIZE;
         ++table_index) {

        if (batt_voltage >= soc_table[table_index].voltage) {
            lower_level_voltage = soc_table[table_index].voltage;
            if (BANK_BATT_SOC_TABLE_FIRST_ELEMENT == table_index) {
                higher_level_voltage = v100_percent;
            } else {
                higher_level_voltage = soc_table[table_index - 1].voltage;
            }

            break;
        }
    }

    if (table_index
        < (BANK_CFG_BATT_SOC_LEVEL_SIZE
           - BANK_BATT_SOC_CRITICAL_VIRTURAL_INDEX)) {
        base_percent = (100 / BANK_CFG_BATT_SOC_LEVEL_SIZE)
            * (BANK_CFG_BATT_SOC_LEVEL_SIZE
               - BANK_BATT_SOC_CRITICAL_VIRTURAL_INDEX - table_index);
        level_scrop = 100 / BANK_CFG_BATT_SOC_LEVEL_SIZE;
    } else {
        if (batt_voltage >= critical_level) {
            base_percent = critical_percent;
            level_scrop = 100 / BANK_CFG_BATT_SOC_LEVEL_SIZE - critical_percent;
            lower_level_voltage = critical_level;

        } else {
            base_percent         = 0;
            level_scrop          = critical_percent;
            higher_level_voltage = critical_level;
        }
    }
    LOGD(TAG, "GET INDEX..%d\n", table_index);
    LOGD(
        TAG, "soc_table[table_index].voltage=%d\n",
        soc_table[table_index].voltage);
    LOGD(TAG, "batt_voltage:%d\n", batt_voltage);
    LOGD(TAG, "GET base_percent..%d\n", base_percent);
    LOGD(TAG, "GET level_scrop..%d\n", level_scrop);

    /**
     * -5/7- 25-5/7 --- 25 --- --- 25 --- --- 25 ---
     * |____|______|__________|____t_____|__________|
     * 0  crit     25        50         75         100
     * Round = (value*10 + 5) / 10
     * Pb: Base Percent, 50, for Pt
     * level_scrop: 25, for Pt
     * Pt: Perecnt of t = ((Round)(Vt-V25)/(V50-V25)*level_scrop) + Pb
     */
    soc_level_percent = ((batt_voltage - lower_level_voltage) * level_scrop * 10
                             / (higher_level_voltage - lower_level_voltage)
                         + 5)
            / 10
        + base_percent;

    return soc_level_percent;
}

/**
 * @brief extra limit soc percent by ui spec
 * like , level is
 * @param soc_table
 * @param batt_level
 * @param new_percent
 * @param direction
 * @return
 */
static uint8_t soc_percent_extra_limitation(
    const bank_batt_soc_element_t *soc_table, bank_batt_level_t batt_level,
    uint8_t new_percent, soc_direction_t direction)
{
    uint8_t limit_percent = new_percent;

    /* critical limitation. */
    if (bank_batt_level_critical == batt_level) {
        if (new_percent < BANK_CFG_DEFAULT_IDLE_CRITICAL_LEVEL) {
            limit_percent = BANK_CFG_DEFAULT_IDLE_CRITICAL_LEVEL;
        }
        if (new_percent
            > soc_table[BANK_BATT_SOC_CRITICAL_VIRTURAL_INDEX].level_percent) {
            limit_percent =
                soc_table[BANK_BATT_SOC_CRITICAL_VIRTURAL_INDEX].level_percent;
        }
        return limit_percent;
    }

    for (int i = 0; i < BANK_CFG_BATT_SOC_LEVEL_SIZE - 1; ++i) {
        /**
         * @brief idle limitation.
         * i.e.
         * batt_level = bank_batt_level_high which  base percent
         * BANK_BATT_SOC_LEVEL_50 , higher leve is BANK_BATT_SOC_LEVEL_75
         * if new_percent = BANK_BATT_SOC_LEVEL_50
         *    cal_percent  = BANK_BATT_SOC_LEVEL_50 + 1
         * if new_percent > BANK_BATT_SOC_LEVEL_75
         *    cal_percent  = BANK_BATT_SOC_LEVEL_75
         */
        if (soc_table[i].level == batt_level
            && direction == soc_direction_down) {

            if (new_percent < soc_table[i].level_percent + 1) {
                limit_percent = soc_table[i].level_percent + 1;
            }

            if ((i != BANK_BATT_SOC_TABLE_FIRST_ELEMENT)
                && new_percent
                    > soc_table[i - BANK_BATT_SOC_TABLE_FIRST_ELEMENT]
                          .level_percent) {
                limit_percent = soc_table[i - BANK_BATT_SOC_TABLE_FIRST_ELEMENT]
                                    .level_percent;
            }
            break;
        }

        /**
         * @brief charging limitation.
         * i.e.
         * batt_level = bank_batt_level_high which  base percent
         * BANK_BATT_SOC_LEVEL_50 , higher leve is BANK_BATT_SOC_LEVEL_75
         * if new_percent < BANK_BATT_SOC_LEVEL_50
         *    cal_percent  = BANK_BATT_SOC_LEVEL_50
         * if new_percent > BANK_BATT_SOC_LEVEL_75-1
         *    cal_percent  = BANK_BATT_SOC_LEVEL_75-1
         */
        if (soc_table[i].level == batt_level && direction == soc_direction_up) {

            if (new_percent < soc_table[i].level_percent) {
                limit_percent = soc_table[i].level_percent;
            }

            if ((i != BANK_BATT_SOC_TABLE_FIRST_ELEMENT)
                && new_percent
                    > soc_table[i - BANK_BATT_SOC_TABLE_FIRST_ELEMENT]
                            .level_percent
                        - 1) {
                limit_percent = soc_table[i - BANK_BATT_SOC_TABLE_FIRST_ELEMENT]
                                    .level_percent
                    - 1;
            }
            break;
        }
    }

    return limit_percent;
}

/**
 * @brief battery adc convert to voltage
 *
 * @param adc_value
 * @return uint16_t  adc_value/ 4096 * 2.5 *1000 * 22 / 10 =
 */
static uint16_t _adc_to_voltage(uint16_t adc_value)
{
    return BANK_CFG_ADC_TO_VOLTAGE(adc_value);
}

/**
 * @brief
 *
 * @param ntc_value
 * @return
 */
static int8_t _adc_to_temp(uint16_t ntc_value)
{
    int8_t batt_temp =
        _bank_batt_ctx.ntc_temp_table[batt_ntc_temprature_max_span]
            .temprature; // maximum
    for (uint8_t i = 0; i < batt_ntc_temprature_max_span; i++) {
        if (ntc_value > (_bank_batt_ctx.ntc_temp_table[i].ntc_value)) {
            batt_temp = _bank_batt_ctx.ntc_temp_table[i].temprature;
            break;
        }
    }

    return batt_temp;
}

/**
 * @brief
 *
 * @param arr
 * @param n
 * @return
 */
static uint16_t _adc_median(uint16_t arr[], uint8_t n)
{
    int i, j, temp;

    // bubble sort
    for (i = 0; i < n - 1; i++) {
        for (j = 0; j < n - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                temp       = arr[j];
                arr[j]     = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }

    return arr[(n >> 1) + 1];
}


/**
 * @brief
 *
 * @param temperature
 * @return bank_batt_temp_band_t
 */
static bank_batt_temp_band_t _temp_to_band(int8_t temperature)
{
#if (BANK_CFG_BATT_TEMP_BAND_MAX == 3)
    if (temperature <= BANK_BATT_TEMP_COOL) {
        return bank_batt_temp_band_0_to_10;
    } else if (temperature <= BANK_BATT_TEMP_WARM) {
        return bank_batt_temp_band_11_to_45;
    } else {
        return bank_batt_temp_band_46_to_60;
    }
#endif
}

static bank_batt_age_t _aging_level(uint32_t exp)
{
    if (exp < BANK_CFG_BATT_AGE_THRESHOLD1) {
        return bank_batt_age_1;
#if (BANK_CFG_BATT_AGE_MAX <= 2)
    } else {
        return bank_batt_age_2;
#elif (BANK_CFG_BATT_AGE_MAX <= 3)
    } else if (exp < BANK_CFG_BATT_AGE_THRESHOLD2) {
        return bank_batt_age_2;
    } else {
        return bank_batt_age_3;
#else
#error "BANK_CFG_BATT_AGE_MAX defined value is not supported"
#endif
    }
}
