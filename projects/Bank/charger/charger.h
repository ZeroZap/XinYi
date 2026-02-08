#ifndef _CHARGER_H_
#define _CHARGER_H_

typedef enum {
    charger_src_no_input = 0,
    charger_src_sdp_100,
    charger_src_sdp_500,
    charger_src_sdp_1000,
    charger_src_sdp_scp_dock_2050,
    charger_src_cdp_audio_1500,
    charger_src_cdp_medium_1500,
    charger_src_cdp_default_2050,
    charger_src_cdp_high_2050,
    charger_src_nsa_1000,
    charger_src_nsa_2000,
    charger_src_nsa_2100,
    charger_src_nsa_2400,
    charger_src_dcp,
    charger_src_unknown
} charger_src_t;

typedef enum {
    charger_status_no_charging,
    charger_status_trickle_charge,
    charger_status_pre_charge,
    charger_status_fast_charge,  // CC mode
    charger_status_taper_charge, // CV mode
    charger_status_top_off_timer_charging,
    charger_status_terminated,
    charger_status_otg,
    charger_state_error,
} charger_status;

typedef enum {
    charger_fault_none,
    charger_fault_power_poor,
    charger_fault_input_ovp,
    charger_fault_input_ulp
} charger_fault_t;

typedef enum {
    charger_health_unknown = 0,
    charger_health_good,
    charger_health_overheat,
    charger_health_overvoltage,
    charger_health_cold,
    charger_health_watchdog_timer_expire,
    charger_health_safety_timer_expire,
    charger_health_warm,
    charger_health_cool,
    charger_health_hot,
    charger_health_no_battery,
} charger_health_t;

union chager_proval {
    enum
};

// more ref
// https://docs.zephyrproject.org/latest/doxygen/html/charger_8h_source.html
#endif