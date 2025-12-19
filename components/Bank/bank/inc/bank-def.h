#ifndef _BANK_DEF_H_
#define _BANK_DEF_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "bank-cfg.h"
#include <stdint.h>

/**
 * @addtogroup Bank
 * @{
 */
/* Exported types ------------------------------------------------------------*/
/**
 * @addtogroup Bank_Exported_Teyps
 * @{
 */

typedef enum {
    bank_error_none         = 0,
    bank_error_invalid_hdlr = -1,
    bank_error_msq_full     = -2,
    bank_error_msq_empty    = -3,

    bank_error_chrg_init    = -1000,
    bank_error_chrg_start   = -1001,
    bank_error_chrg_resume  = -1002,
    bank_error_chrg_stop    = -1002,
    bank_error_chrg_suspend = -1002,
    bank_error_chrg_cable   = -1003,
    bank_error_chrg_fault   = -1100,
    bank_error_batt_init    = -2000,
    bank_error_batt_adc     = -2001,
    bank_error_unknown      = -10000
} bank_error_t;

typedef enum {
    bank_chrg_state_reseting = 0,
    bank_chrg_state_reset,
    bank_chrg_state_configuring,
    bank_chrg_state_initializing,
    bank_chrg_state_initialized,
    bank_chrg_state_idle,
    bank_chrg_state_detecting,
    bank_chrg_state_detecting_non_standard,
    bank_chrg_state_enabling,
    bank_chrg_state_precharging,
    bank_chrg_state_charging,
    bank_chrg_state_ghost_charge,
    bank_chrg_state_topoff_charge,
    bank_chrg_state_charged,
    bank_chrg_state_fault,
    bank_chrg_state_toor,
    bank_chrg_state_max
} bank_chrg_state_t;

typedef enum {
    bank_chrg_fault_none = 0,
    bank_chrg_fault_ovp  = 1,
    bank_chrg_fault_batt = 2,
    bank_chrg_fault_sys  = 4,
    bank_chrg_fault_all =
        (bank_chrg_fault_ovp | bank_chrg_fault_batt | bank_chrg_fault_sys)
} bank_chrg_fault_t;

typedef enum {
    bank_chrg_timer_1hrs = 1,
    bank_chrg_timer_2hrs,
    bank_chrg_timer_3hrs,
    bank_chrg_timer_4hrs,
    bank_chrg_timer_5hrs,
    bank_chrg_timer_6hrs,
    bank_chrg_timer_7hrs,
    bank_chrg_timer_8hrs,
    bank_chrg_timer_9hrs,
    bank_chrg_timer_10hrs,
    bank_chrg_timer_11hrs,
    bank_chrg_timer_12hrs,
    bank_chrg_timer_13hrs,
    bank_chrg_timer_14hrs,
    bank_chrg_timer_15hrs,
    bank_chrg_timer_16hrs,
    bank_chrg_timer_17hrs,
    bank_chrg_timer_18hrs,
    bank_chrg_timer_19hrs,
    bank_chrg_timer_20hrs,
    bank_chrg_timer_21hrs,
    bank_chrg_timer_22hrs,
    bank_chrg_timer_23hrs,
    bank_chrg_timer_24hrs,
    bank_chrg_timer_25hrs,
    bank_chrg_timer_26hrs,
    bank_chrg_timer_27hrs,
    bank_chrg_timer_28hrs,
} bank_chrg_timer_t;

/** List of values for the bank charger power source. */
typedef enum {
    bank_chrg_source_dcp_2050ma = BANK_CFG_CHRG_SOURCE_DCP_2050MA,
    bank_chrg_source_sdp_500ma  = BANK_CFG_CHRG_SOURCE_SDP_500MA,
    bank_chrg_source_sdp_100ma  = BANK_CFG_CHRG_SOURCE_SDP_100MA,
    bank_chrg_source_cdp_default_2050ma =
        BANK_CFG_CHRG_SOURCE_CDP_DEFAULT_2050MA,
    bank_chrg_source_cdp_audio_500ma   = BANK_CFG_CHRG_SOURCE_CDP_AUDIO_500MA,
    bank_chrg_source_cdp_medium_1500ma = BANK_CFG_CHRG_SOURCE_CDP_MEDIUM_1500MA,
    bank_chrg_source_cdp_high_2050ma   = BANK_CFG_CHRG_SOURCE_CDP_HIGH_2050MA,
    bank_chrg_source_sdp_scp_dock_2050ma =
        BANK_CFG_CHRG_SOURCE_SDP_SCP_DOCK_2050MA,
    bank_chrg_source_non_standard_1000ma = BANK_CFG_CHRG_SOURCE_NSA_1000MA,
    bank_chrg_source_non_standard_2000ma = BANK_CFG_CHRG_SOURCE_NSA_2000MA,
    bank_chrg_source_non_standard_2100ma = BANK_CFG_CHRG_SOURCE_NSA_2100MA,
    bank_chrg_source_non_standard_2400ma = BANK_CFG_CHRG_SOURCE_NSA_2400MA,
    bank_chrg_source_unknown_500ma       = BANK_CFG_CHRG_SOURCE_UNKNOWN_500MA,
    bank_chrg_source_error               = 0xFF,
} bank_chrg_source_t;

/**
 * List of values for bank charger chips.
 *
 */
typedef enum {
    bank_chrg_chip_first   = 0,
    bank_chrg_chip_bq2452x = bank_chrg_chip_first,
    bank_chrg_chip_bq2589x,
    bank_chrg_chip_bq2562x,
    bank_chrg_chip_unknown,
    bank_chrg_chip_count = bank_chrg_chip_unknown
} bank_chrg_chip_t;

/**
 * List of values for bank charger start reason.
 *
 */
typedef enum {
    bank_chrg_start_reason_insertion   = BANK_CFG_CHRG_START_REASON_INSERTION,
    bank_chrg_start_reason_user        = BANK_CFG_CHRG_START_REASON_USER,
    bank_chrg_start_reason_command     = BANK_CFG_CHRG_START_REASON_COMMAND,
    bank_chrg_start_reason_heating_off = BANK_CFG_CHRG_START_REASON_HEATING_OFF,
    bank_chrg_start_reason_unknown     = BANK_CFG_CHRG_START_REASON_UNKNOWN
} bank_chrg_start_reason_t;

/**
 * List of values for bank charger stop reason.
 *
 */
typedef enum {
    bank_chrg_stop_reason_completed   = BANK_CFG_CHRG_STOP_REASON_COMPLETED,
    bank_chrg_stop_reason_user_action = BANK_CFG_CHRG_STOP_REASON_USER,
    bank_chrg_stop_reason_overheat    = BANK_CFG_CHRG_STOP_REASON_OVERHEAT,
    bank_chrg_stop_reason_vin_out_of_range = BANK_CFG_CHRG_STOP_REASON_VOR,
    bank_chrg_stop_reason_hard_fault = BANK_CFG_CHRG_STOP_REASON_HARD_FAULT,
    bank_chrg_stop_reason_timeout    = BANK_CFG_CHRG_STOP_REASON_TIMEOUT,
    bank_chrg_stop_reason_chip_fault = BANK_CFG_CHRG_STOP_REASON_CHIPFAULT,
    bank_chrg_stop_reason_cold_temp  = BANK_CFG_CHRG_STOP_REASON_COLD_TEMP,
    bank_chrg_stop_reason_heating    = BANK_CFG_CHRG_STOP_REASON_HEATING,
    bank_chrg_stop_reason_command    = BANK_CFG_CHRG_STOP_REASON_COMMAND,
    bank_chrg_stop_reason_extraction = BANK_CFG_CHRG_STOP_REASON_EXTRACTION,
    bank_chrg_stop_reason_unknown    = BANK_CFG_CHRG_STOP_REASON_UNKNOWN
} bank_chrg_stop_reason_t;

/** List of values for the bank charger power cable status. */
typedef enum {
    bank_chrg_cable_in       = BANK_CFG_CHRG_CABLE_OUT,
    bank_chrg_cable_detected = BANK_CFG_CHRG_CABLE_DETECT,
    bank_chrg_cable_out      = BANK_CFG_CHRG_CABLE_IN
} bank_chrg_cable_status_t;

/** List of values for the bank charger communication status. */
typedef enum {
    bank_chrg_com_disconnected = BANK_CFG_CHRG_COM_DISCONNECTED,
    bank_chrg_com_detected     = BANK_CFG_CHRG_COM_DETECTED,
    bank_chrg_com_connected    = BANK_CFG_CHRG_COM_CONNECTED
} bank_chrg_cxn_status_t;

/**
 * @brief list of values for bank battery temperature band.
 *
 */
typedef enum {
#if (BANK_CFG_BATT_TEMP_BAND_MAX == 3)
    bank_batt_temp_band_0_to_10 = 0, // 0 - 10 deg C.
    bank_batt_temp_band_11_to_45,    // 11 - 45 deg C.
    bank_batt_temp_band_46_to_60,    // 46 - 60 deg C.
#elif (BANK_CFG_BATT_TEMP_BAND_MAX == 4)
    bank_batt_temp_band_0_to_10 = 0, // 0 - 10 deg C.
    bank_batt_temp_band_11_to_25,    // 11 - 25 deg C.
    bank_batt_temp_band_26_to_45,    // 26 - 45 deg C.
    bank_batt_temp_band_46_to_60,    // 46 - 60 deg C.
#else
#error "Undefine or unspport BANK_CFG_BATT_TEMP_BAND_MAX"
#endif
    bank_batt_temp_band_count
} bank_batt_temp_band_t;

/**
 * @brief list of values for bank battery age.
 *
 */
typedef enum {
    bank_batt_age_fresh = 0,
    bank_batt_age_1     = bank_batt_age_fresh,
    bank_batt_age_2,
    bank_batt_age_3,
    bank_batt_age_max
} bank_batt_age_t;

/**
 * @brief list of values for bank battery level.
 *
 */
typedef enum {
    bank_batt_level_unknown    = BANK_CFG_BATT_LEVEL_UNKNOWN,
    bank_batt_level_flat       = BANK_CFG_BATT_LEVEL_FLAT,
    bank_batt_level_critical   = BANK_CFG_BATT_LEVEL_CRITICAL,
    bank_batt_level_2_last_exp = BANK_CFG_BATT_LEVEL_2_LAST_EXP,
    bank_batt_level_low        = BANK_CFG_BATT_LEVEL_LOW,
    bank_batt_level_medium     = BANK_CFG_BATT_LEVEL_MEDIUM,
    bank_batt_level_high       = BANK_CFG_BATT_LEVEL_HIGH,
    bank_batt_level_full       = BANK_CFG_BATT_LEVEL_FULL
} bank_batt_level_t;

typedef enum {
    bank_event_none = 0,

    /* system mode */
    bank_event_system_in_bist_mode,
    bank_event_system_in_replacement_mode,
    bank_event_system_in_normal,

    /* bus status changed. */
    bank_event_chrg_bus_in,
    bank_event_chrg_bus_out,

    /* charge state changed. */
    bank_event_chrg_start,
    bank_event_chrg_suspend,
    bank_event_chrg_resume,
    bank_event_chrg_stop,

    /* charge fault */
    bank_event_chrg_fault,
    bank_event_chrg_fault_ovp,
    bank_event_chrg_fault_batt,
    bank_event_chrg_fault_sys,
    bank_event_chrg_fault_timeout,

    /* charge misc. */
    bank_event_chrg_int,

    /* battery temperature. */
    bank_event_batt_temp,
    bank_event_batt_temp_hot,
    bank_event_batt_temp_cold,

    /* battery voltage. */
    bank_event_batt_vol,
    bank_event_batt_volt_too_low,
    bank_event_batt_volt_too_high,

    /* battery current. */
    bank_event_batt_current,
    bank_event_batt_current_short,
    bank_event_batt_current_open,

    /* battery misc current. */
    bank_event_batt_empty,

    bank_event_max

} bank_event_t;

typedef enum {
    bank_msg_init = 0,
    bank_msg_periodic_update,
    bank_msg_chrg_init,
    bank_msg_chrg_start,
    bank_msg_chrg_resume,
    bank_msg_chrg_top,
    bank_msg_chrg_suspend,
    bank_msg_chrg_ghost,
    bank_msg_chrg_idle,
    bank_msg_batt_force_update,
    bank_msg_batt_enter_replacement,
    bank_msg_max,
    bank_msg_none = 0xFFFF
} bank_msg_type_t;

/**
 * @ingroup Config_Code
 * @{
 */

/** Battery technology bit shift. */
#define CONFIG_CODE_BATT_TECH_SHIFT 30

/** Battery manufacturer bit shift. */
#define CONFIG_CODE_BATT_MFG_SHIFT 24

/** Battery technology generation bit shift. */
#define CONFIG_CODE_BATT_TECH_GEN_SHIFT 16

/** Battery technology bit mask. */
#define CONFIG_CODE_BATT_TECH_MASK 0xC0000000

/** Battery manufacturer bit mask. */
#define CONFIG_CODE_BATT_MFG_MASK 0x0F000000

/** Battery technology generation bit mask. */
#define CONFIG_CODE_BATT_TECH_GEN_MASK 0x00070000

/** List of values for bank battery technology. */
typedef enum {
    bank_batt_tech_lco = 0,
    bank_batt_tech_ncm,
    bank_batt_tech_lifepo4,
    bank_batt_tech_nca
} bank_batt_tech_t;

/** List of values for bank battery manufacturer. */
typedef enum {
    bank_batt_mfg_unknown   = 0x00,
    bank_batt_mfg_fullriver = 0x01,
    bank_batt_mfg_panasonic = 0x02,
    bank_batt_mfg_lgc       = 0x03,
    bank_batt_mfg_aucopo    = 0x04,
    bank_batt_mfg_rempus    = 0x05,
    bank_batt_mfg_desay     = 0x06,
    bank_batt_mfg_atl       = 0x07,
    bank_batt_mfg_cosmx     = 0x08,
    bank_batt_mfg_byd_dak   = 0x09,
    bank_batt_mfg_mic       = 0x0A,
    bank_batt_mfg_cpw       = 0x0B
} bank_batt_mfg_t;

/** List of values for bank battery technology generation. */
typedef enum {
    bank_batt_tech_gen_0 = 0,
    bank_batt_tech_gen_1,
    bank_batt_tech_gen_2,
    bank_batt_tech_gen_3,
    bank_batt_tech_gen_4,
    bank_batt_tech_gen_5,
    bank_batt_tech_gen_6,
    bank_batt_tech_gen_7
} bank_batt_tech_gen_t;

/** List of bank battery configuration. */
typedef enum bank_batt_config_index {
    bank_batt_config_version,
    bank_batt_config_charging_critical_mv,
    bank_batt_config_idle_critical_mv,
    bank_batt_config_idle_flat_mv
} bank_batt_config_t;

/** Bank battery state of charge. */
typedef struct {
    uint16_t voltage;
    uint8_t level_percent;
    uint8_t level;
} bank_batt_soc_element_t;

/** Bank battery charge target profile. */
typedef struct {
    uint16_t volt;
    uint16_t current;
} bank_batt_target_profile_t;


/** @} end of group Config_Code */

/**
 * @brief The information of bank property.
 */
typedef struct bank_info {
    /** Bank charge state */
    bank_chrg_state_t chrg_state;

    /** Bank charge faults. */
    uint8_t chrg_faults;

    /** Suspend charging state bit for Bank charger. */
    uint8_t chrg_suspend;

    /** Flag to indicate if recharge is requested. */
    uint8_t chrg_recharge_req;

    /** Flag to indicate bank battery has been full charged. */
    uint8_t chrg_charged;

    /** Last/Current charging timer type. */
    bank_chrg_timer_t chrg_timer;

    /** Last/Current charging duration. */
    uint16_t chrg_duration;

    /** Last/Current target duration. */
    uint16_t chrg_target_duration;

    /** Last/Current charging energy in Joule. */
    uint16_t chrg_energy;

    /** Total charge energy in Joule.*/
    uint32_t total_energy;

    /** Last/Current charging source. */
    bank_chrg_source_t chrg_source;

    /** Last/Current charging chip type. */
    bank_chrg_chip_t chrg_chip;

    /** Input current limit for Bank charger in mA. */
    uint16_t chrg_input_current;

    /** Precharge current limit for Bank charger in mA. */
    uint16_t chrg_precharge_current;

    /** Target current limit for Bank charger in mA. */
    uint16_t chrg_target_current;

    /** Termination current limit for Bank charger in mA. */
    uint16_t chrg_term_current;

    /** Input voltage limit for Bank charger in mV. */
    uint16_t chrg_input_volt;

    /** Target voltage limit for Bank charger in mV. */
    uint16_t chrg_target_vol;

    /** Charge stop reason. */
    bank_chrg_start_reason_t chrg_start_reason;

    /** Charge stop reason. */
    bank_chrg_stop_reason_t chrg_stop_reason;

    /** Battery age. */
    bank_batt_age_t age;

    /** Battery full charge capacity in mAh. */
    uint16_t batt_full_cap;

    /** Battery remain capacity in mAh. */
    uint16_t batt_rem_cap;

    /** Measured current of Bank charger in mA. */
    int16_t batt_current;

    /** Battery voltage. */
    uint16_t batt_volt;

    /** Battery voltage rising threshold from pre-charge to fast charge. */
    uint16_t chrg_volt_lowvz;

    /** Battery voltage adc. */
    uint16_t batt_volt_adc;

    /** Battery power level. */
    bank_batt_level_t batt_level;

    /** State of charge in percentage i.e. power level percentage. */
    uint8_t batt_soc;

    /** Battery state of health in percentage. */
    uint8_t batt_soh;

    /** Battery temperature. */
    int8_t batt_temperature;

    /** Battery temperature adc value */
    uint16_t batt_temperature_adc;

    /** Battery temperature band. */
    bank_batt_temp_band_t batt_temp_band;

    /** Device ambient_temperature. */
    int8_t ambient_temperature;

    /** Device ambient_temperature adc value. */
    uint16_t ambient_temperature_adc;

} bank_info_t;

typedef struct {
    bank_msg_type_t msg;
    uint32_t p1;
    uint32_t p2;
} bank_msg_t;

typedef enum {
    bank_cntl_int = 0,
    bank_cntl_enter_normal_mode,
    bank_cntl_enter_sleep_mode,
    bank_cntl_enter_critical_mode,
    bank_cntl_enter_replacement_mode,
    bank_cntl_enter_bist_mode,
    // bank_cntl_enter_extreme_mode, not allow

    bank_cntrl_batt_critical,
    bank_cntrl_batt_low,

    bank_cntl_periodic_update, /**< periodic update. */
    bank_cntl_chrg_start = 128,
    bank_cntl_chrg_stop,
    bank_cntl_chrg_susspend,
    bank_cntl_chrg_resume,

    bank_cntrl_batt_cfg = 256
} bank_cntl_cmd_t;

typedef enum {
    bank_mode_normal = 0,
    bank_mode_bist,
    bank_mode_ciritcal,
    bank_mode_replacement,
    bank_mode_sleep
} bank_mode_t;


typedef void (*bank_event_hdlr_t)(bank_event_t event, const void *data);
typedef int32_t (*bank_managment_cntl_func)(void *arg);


/* Exported macro ------------------------------------------------------------*/


/* Exported constants --------------------------------------------------------*/
#define BANK_BATT_TEMP_COLD (0)
#define BANK_BATT_TEMP_COOL (10)
#define BANK_BATT_TEMP_GOOD (25)
#define BANK_BATT_TEMP_WARM (45)
#define BANK_BATT_TEMP_HOT (60)
#define BANK_AMBIENT_TEMP_COLD (-10)
#define BANK_AMBIENT_TEMP_HOT (90)

#define BANK_BATT_SOC_LEVEL_0 (0)
#define BANK_BATT_SOC_LEVEL_20 (20)
#define BANK_BATT_SOC_LEVEL_25 (25)
#define BANK_BATT_SOC_LEVEL_40 (40)
#define BANK_BATT_SOC_LEVEL_50 (50)
#define BANK_BATT_SOC_LEVEL_60 (60)
#define BANK_BATT_SOC_LEVEL_75 (75)
#define BANK_BATT_SOC_LEVEL_80 (80)
/* Must equal 1 since the threshold is usually below level 20. */
#define BANK_BATT_SOC_CRITICAL_VIRTURAL_INDEX (1)
#define BANK_BATT_SOC_TABLE_FIRST_ELEMENT (0)

/** @} end of group Bank_Exported_Teyps */

/** @} end of group Bank */

#ifdef __cplusplus
}
#endif

#endif /* _BANK_DEF_H_ */
