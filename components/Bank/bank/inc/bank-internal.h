#ifndef _BANK_INTERNAL_H_
#define _BANK_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "bank-battery.h"
#include "bank-cfg.h"
#include "bank-charge.h"
#include "bank-def.h"
#include "controller-charger.h"
#include "controller-log.h"
#include "controller-usb.h"
#include "event_groups.h"
#include "task.h"
#include "timers.h"

extern bank_info_t bank_info;

/* Exported functions --------------------------------------------------------*/
/**
 * @ingroup Bank_Property
 * @{
 */

/** Bank charge state */
inline void bank_chrg_set_state(bank_chrg_state_t state)
{
    bank_info.chrg_state = state;
}

inline bank_chrg_state_t bank_chrg_state(void)
{
    return bank_info.chrg_state;
}

/** Bank charge faults. */
inline void bank_chrg_set_fault(uint32_t fault)
{
    bank_info.chrg_faults |= fault;
}

inline uint32_t bank_chrg_fault(void)
{
    return bank_info.chrg_faults;
}

inline void bank_chrg_clear_fault(uint32_t fault)
{
    bank_info.chrg_faults &= (~fault);
}

/** Suspend charging state bit for Bank charger. */
inline void bank_chrg_set_suspend(uint8_t suspend)
{
    bank_info.chrg_suspend = suspend;
}

inline uint8_t bank_chrg_suspend(void)
{
    return bank_info.chrg_suspend;
}

/** Flag to indicate if recharge is requested. */
inline void bank_chrg_set_recharge_req(uint8_t req)
{
    bank_info.chrg_recharge_req = req;
}

inline uint8_t bank_chrg_recharge_req(void)
{
    return bank_info.chrg_recharge_req;
}

/** Flag to indicate bank battery has been full charged. */
inline void bank_chrg_set_charged(uint8_t charged)
{
    bank_info.chrg_charged = charged;
}

inline uint8_t bank_chrg_charged(void)
{
    return bank_info.chrg_charged;
}

/** Last/Current charging timer type. */
inline void bank_chrg_set_timer(bank_chrg_timer_t timer)
{
    bank_info.chrg_timer = timer;
}

inline bank_chrg_timer_t bank_chrg_timer(void)
{
    return bank_info.chrg_timer;
}

/** Last/Current charging duration. */
inline void bank_chrg_set_duration(uint16_t duration)
{
    bank_info.chrg_duration = duration;
}

inline uint16_t bank_chrg_duration(void)
{
    return bank_info.chrg_duration;
}

/** Last/Current target duration. */
inline void bank_chrg_set_target_duration(uint16_t duration)
{
    bank_info.chrg_target_duration = duration;
}

inline uint16_t bank_chrg_target_duration(void)
{
    return bank_info.chrg_target_duration;
}

/** Last/Current charging energy in Joule. */
inline void bank_chrg_set_energy(uint16_t energy)
{
    bank_info.chrg_energy = energy;
}

inline uint16_t bank_chrg_energy(void)
{
    return bank_info.chrg_energy;
}

/** Total charge energy in Joule.*/
inline void bank_chrg_set_total_energy(uint32_t energy)
{
    bank_info.total_energy = energy;
}

inline uint32_t bank_chrg_total_energy(void)
{
    return bank_info.total_energy;
}

/** Last/Current charging source. */
inline void bank_chrg_set_source(bank_chrg_source_t source)
{
    bank_info.chrg_source = source;
}

inline bank_chrg_source_t bank_chrg_source(void)
{
    return bank_info.chrg_source;
}

/** Last/Current charging chip type. */
inline void bank_chrg_set_chip(bank_chrg_chip_t chip)
{
    bank_info.chrg_chip = chip;
}

inline bank_chrg_chip_t bank_chrg_chip(void)
{
    return bank_info.chrg_chip;
}

/** Input current limit for Bank charger in mA. */
inline void bank_chrg_set_input_current(uint16_t current)
{
    bank_info.chrg_input_current = current;
}

inline uint16_t bank_chrg_input_current(void)
{
    return bank_info.chrg_input_current;
}

/** Precharge current limit for Bank charger in mA. */
inline void bank_chrg_set_precharge_current(uint16_t current)
{
    bank_info.chrg_precharge_current = current;
}

inline uint16_t bank_chrg_precharge_current(void)
{
    return bank_info.chrg_precharge_current;
}

/** Target current limit for Bank charger in mA. */
inline void bank_chrg_set_target_current(uint16_t current)
{
    bank_info.chrg_target_current = current;
}

inline uint16_t bank_chrg_target_current(void)
{
    return bank_info.chrg_target_current;
}

/** Termination current limit for Bank charger in mA. */
inline void bank_chrg_set_term_current(uint16_t current)
{
    bank_info.chrg_term_current = current;
}

inline uint16_t bank_chrg_term_current(void)
{
    return bank_info.chrg_term_current;
}

/** Input voltage limit for Bank charger in mV. */
inline void bank_chrg_set_input_volt(uint16_t volt)
{
    bank_info.chrg_input_volt = volt;
}

inline uint16_t bank_chrg_input_volt(void)
{
    return bank_info.chrg_input_volt;
}

/** Target voltage limit for Bank charger in mV. */
inline void bank_chrg_set_target_volt(uint16_t volt)
{
    bank_info.chrg_target_vol = volt;
}

inline uint16_t bank_chrg_target_volt(void)
{
    return bank_info.chrg_target_vol;
}

/** Battery voltage rising threshold from pre-charge to fast charge. */
inline void bank_chrg_set_volt_lowvz(uint16_t volt)
{
    bank_info.chrg_volt_lowvz = volt;
}


inline uint16_t bank_chrg_volt_lowvz(void)
{
    return bank_info.chrg_volt_lowvz;
}

/** Charge stop reason. */
inline void bank_chrg_set_start_reason(bank_chrg_start_reason_t reason)
{
    bank_info.chrg_start_reason = reason;
}

inline bank_chrg_start_reason_t bank_chrg_start_reason(void)
{
    return bank_info.chrg_start_reason;
}

/** Charge stop reason. */
inline void bank_chrg_set_stop_reason(bank_chrg_stop_reason_t reason)
{
    bank_info.chrg_stop_reason = reason;
}

inline bank_chrg_stop_reason_t bank_chrg_stop_reason(void)
{
    return bank_info.chrg_stop_reason;
}

/** Battery age . */
inline void bank_batt_set_age(bank_batt_age_t age)
{
    bank_info.age = age;
}

inline bank_batt_age_t bank_batt_age(void)
{
    return bank_info.age;
}

/** Battery full charge capacity in mAh. */
inline void bank_batt_set_full_cap(uint16_t cap)
{
    bank_info.batt_full_cap = cap;
}

inline uint16_t bank_batt_full_cap(void)
{
    return bank_info.batt_full_cap;
}

/** Battery remain capacity in mAh. */
inline void bank_batt_set_rem_cap(uint16_t cap)
{
    bank_info.batt_rem_cap = cap;
}

inline uint16_t bank_batt_rem_cap(void)
{
    return bank_info.batt_rem_cap;
}

/** Measured current of  battery in mA. */
inline void bank_batt_set_current(int16_t current)
{
    bank_info.batt_current = current;
}

inline int16_t bank_batt_current(void)
{
    return bank_info.batt_current;
}

/** Bank battery voltage. */
inline void bank_batt_set_volt(uint16_t volt)
{
    bank_info.batt_volt = volt;
}

inline uint16_t bank_batt_volt(void)
{
    return bank_info.batt_volt;
}

/** Battery voltage adc. */
inline void bank_batt_set_volt_adc(uint16_t volt_adc)
{
    bank_info.batt_volt_adc = volt_adc;
}

inline uint16_t bank_batt_volt_adc(void)
{
    return bank_info.batt_volt_adc;
}

/** Battery power level. */
inline void bank_batt_set_level(bank_batt_level_t level)
{
    bank_info.batt_level = level;
}

inline bank_batt_level_t bank_batt_level(void)
{
    return bank_info.batt_level;
}

/** State of charge in percentage i.e. power level percentage. */
inline void bank_batt_set_soc(uint8_t soc)
{
    bank_info.batt_soc = soc;
}

inline uint8_t bank_batt_soc(void)
{
    return bank_info.batt_soc;
}

/** Battery state of health in percentage. */
inline void bank_batt_set_soh(uint8_t soh)
{
    bank_info.batt_soh = soh;
}

inline uint8_t bank_batt_soh(void)
{
    return bank_info.batt_soh;
}

/** Battery temperature. */
inline void bank_batt_set_temp(int8_t temp)
{
    bank_info.batt_temperature = temp;
}

inline int8_t bank_batt_temp(void)
{
    return bank_info.batt_temperature;
}

/** Battery temperature adc value */
inline void bank_batt_set_temp_adc(uint16_t temp_adc)
{
    bank_info.batt_temperature_adc = temp_adc;
}

inline uint16_t bank_batt_temp_adc(void)
{
    return bank_info.batt_temperature_adc;
}

/** Battery ambient temperature. */
inline void bank_batt_set_ambient_temp(int8_t temp)
{
    bank_info.ambient_temperature = temp;
}

inline int8_t bank_batt_ambient_temp(void)
{
    return bank_info.ambient_temperature;
}

/** Battery temperature adc value */
inline void bank_batt_set_ambient_temp_adc(uint16_t temp_adc)
{
    bank_info.ambient_temperature_adc = temp_adc;
}

inline uint16_t bank_batt_ambient_temp_adc(void)
{
    return bank_info.ambient_temperature_adc;
}

/** Battery temperature band. */
inline void bank_batt_set_temp_band(bank_batt_temp_band_t band)
{
    bank_info.batt_temp_band = band;
}

inline bank_batt_temp_band_t bank_batt_temp_band(void)
{
    return bank_info.batt_temp_band;
}

/** Check whether the charging source supports high current. */
inline uint8_t bank_chrg_high_current_source(void)
{
    if ((bank_info.chrg_source == bank_chrg_source_sdp_100ma)
        || (bank_info.chrg_source == bank_chrg_source_sdp_500ma)
        || (bank_info.chrg_source == bank_chrg_source_unknown_500ma)
        || (bank_info.chrg_source == bank_chrg_source_non_standard_1000ma)
        || (bank_info.chrg_source == bank_chrg_source_error)) {
        return false;
    }
    return true;
}


/** @{ end of group Bank_Property */

/* Exported macro ------------------------------------------------------------*/


/* Exported functions --------------------------------------------------------*/
/**
 * @defgroup Bank_Notify
 * @{
 */
void bank_event_send(bank_event_t event, const void *data);
void bank_event_hdlr_register(bank_event_hdlr_t hdlr);
/** @{ end of group Bank_Notify */

#ifdef __cplusplus
}
#endif

#endif /* _BANK_INTERNAL_H_ */
