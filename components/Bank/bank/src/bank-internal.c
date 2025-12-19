#include "bank-internal.h"

bank_info_t bank_info;
bank_event_hdlr_t bank_event_hdlr = NULL;

/* Exported functions --------------------------------------------------------*/

/** Bank charge state */
extern inline void bank_chrg_set_state(bank_chrg_state_t state);
extern inline bank_chrg_state_t bank_chrg_state(void);

/** Bank charge faults. */
extern inline void bank_chrg_set_fault(uint32_t fault);
extern inline uint32_t bank_chrg_fault(void);
extern inline void bank_clear_chrg_fault(uint32_t fault);

/** Suspend charging state bit for Bank charger. */
extern inline void bank_chrg_set_suspend(uint8_t suspend);
extern inline uint8_t bank_chrg_suspend(void);

/** Flag to indicate if recharge is requested. */
extern inline void bank_chrg_set_recharge_req(uint8_t req);
extern inline uint8_t bank_chrg_recharge_req(void);

/** Flag to indicate bank battery has been full charged. */
extern inline void bank_chrg_set_charged(uint8_t charged);
extern inline uint8_t bank_chrg_charged(void);

/** Last/Current charging timer type. */
extern inline void bank_chrg_set_timer(bank_chrg_timer_t timer);
extern inline bank_chrg_timer_t bank_chrg_timer(void);

/** Last/Current charging duration. */
extern inline void bank_chrg_set_duration(uint16_t duration);
extern inline uint16_t bank_chrg_duration(void);

/** Last/Current target duration. */
extern inline void bank_chrg_set_target_duration(uint16_t duration);
extern inline uint16_t bank_chrg_target_duration(void);

/** Last/Current charging energy in Joule. */
extern inline void bank_chrg_set_energy(uint16_t energy);
extern inline uint16_t bank_chrg_energy(void);

/** Total charge energy in Joule.*/
extern inline void bank_chrg_set_total_energy(uint32_t energy);
extern inline uint32_t bank_chrg_total_energy(void);


/** Last/Current charging source. */
extern inline void bank_chrg_set_source(bank_chrg_source_t source);
extern inline bank_chrg_source_t bank_chrg_source(void);


/** Last/Current charging chip type. */
extern inline void bank_chrg_set_chip(bank_chrg_chip_t chip);
extern inline bank_chrg_chip_t bank_chrg_chip(void);

/** Input current limit for Bank charger in mA. */
extern inline void bank_chrg_set_input_current(uint16_t current);
extern inline uint16_t bank_chrg_input_current(void);

/** Precharge current limit for Bank charger in mA. */
extern inline void bank_chrg_set_precharge_current(uint16_t current);
extern inline uint16_t bank_chrg_precharge_current(void);

/** Target current limit for Bank charger in mA. */
extern inline void bank_chrg_set_target_current(uint16_t current);
extern inline uint16_t bank_chrg_target_current(void);

/** Termination current limit for Bank charger in mA. */
extern inline void bank_chrg_set_term_current(uint16_t current);
extern inline uint16_t bank_chrg_term_current(void);

/** Input voltage limit for Bank charger in mV. */
extern inline void bank_chrg_set_input_volt(uint16_t volt);
extern inline uint16_t bank_chrg_input_volt(void);

/** Target voltage limit for Bank charger in mV. */
extern inline void bank_chrg_set_target_volt(uint16_t volt);
extern inline uint16_t bank_chrg_target_volt(void);

/** Battery voltae rising threshold from pre-charge to fast charge. */
extern inline void bank_chrg_set_volt_lowvz(uint16_t volt);
extern inline uint16_t bank_chrg_volt_lowvz(void);

/** Charge start reason. */
extern inline void bank_chrg_set_start_reason(bank_chrg_start_reason_t reason);
extern inline bank_chrg_start_reason_t bank_chrg_start_reason(void);

/** Charge stop reason. */
extern inline void bank_chrg_set_stop_reason(bank_chrg_stop_reason_t reason);
extern inline bank_chrg_stop_reason_t bank_chrg_stop_reason(void);

/** Battery age. */
extern inline void bank_batt_set_age(bank_batt_age_t age);

extern inline bank_batt_age_t bank_batt_age(void);

/** Battery full charge capacity in mAh. */
extern inline void bank_batt_set_full_cap(uint16_t cap);
extern inline uint16_t bank_batt_full_cap(void);

/** Battery remain capacity in mAh. */
extern inline void bank_batt_set_rem_cap(uint16_t cap);
extern inline uint16_t bank_batt_rem_cap(void);

/** Measured current of  battery in mA. */
extern inline void bank_batt_set_current(int16_t current);
extern inline int16_t bank_batt_current(void);

/** Bank battery voltage. */
extern inline void bank_batt_set_volt(uint16_t volt);
extern inline uint16_t bank_batt_volt(void);

/** Battery voltage adc. */
extern inline void bank_batt_set_volt_adc(uint16_t volt_adc);
extern inline uint16_t bank_batt_volt_adc(void);

/** Battery power level. */
extern inline void bank_batt_set_level(bank_batt_level_t level);
extern inline bank_batt_level_t bank_batt_level(void);

/** State of charge in percentage i.e. power level percentage. */
extern inline void bank_batt_set_soc(uint8_t soc);
extern inline uint8_t bank_batt_soc(void);

/** Battery state of health in percentage. */
extern inline void bank_batt_set_soh(uint8_t soh);
extern inline uint8_t bank_batt_soh(void);

/** Battery temperature. */
extern inline void bank_batt_set_temp(int8_t temp);
extern inline int8_t bank_batt_temp(void);

/** Battery temperature adc value */
extern inline void bank_batt_set_temp_adc(uint16_t temp_adc);
extern inline uint16_t bank_batt_temp_adc(void);

/** Battery temperature. */
extern inline void bank_batt_set_ambient_temp(int8_t temp);
extern inline int8_t bank_batt_ambient_temp(void);

/** Battery temperature adc value */
extern inline void bank_batt_set_ambient_temp_adc(uint16_t temp_adc);
extern inline uint16_t bank_batt_ambient_temp_adc(void);

/** Battery temperature band. */
extern inline void bank_batt_set_temp_band(bank_batt_temp_band_t band);
extern inline bank_batt_temp_band_t bank_batt_temp_band(void);

/** Check whether the charging source supports high current. */
extern inline uint8_t bank_chrg_high_current_source(void);

void bank_event_hdlr_register(bank_event_hdlr_t hdlr)
{
    bank_event_hdlr = hdlr;
}

void bank_event_send(bank_event_t event, const void *data)
{
    if (bank_event_hdlr != NULL) {
        bank_event_hdlr(event, data);
    }
}
