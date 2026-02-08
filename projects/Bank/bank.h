#ifndef _BANK_DEF_H_
#define _BANK_DEF_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { bank_chrg_state_init = 0, bank_chrg_state } bank_chrg_state_t;

typedef enum {
    chrg_stop_by_extraction = 0x00,
    chrg_stop_by_charged,
    chrg_stop_by_cmd,
    chrg_stop_by_heating,
    chrg_stop_by_temp_too_low,  // cold
    chrg_stop_by_temp_top_higt, // over heating
} bank_chrg_stop_reason_t;

typedef enum {
    bank_batt_soc_level_low,
    bank_batt_soc_level_critical,
    bank_batt_soc_level_20,
    bank_batt_soc_level_25,
    bank_batt_soc_level_40,
    bank_batt_soc_level_50,
    bank_batt_soc_level_60,
    bank_batt_soc_level_75,
    bank_batt_soc_level_80,
    bank_batt_soc_level_99,
    bank_batt_soc_level_fulled,
} bank_batt_soc_level_t;

typedef enum {

} bank_batt_soh_t;


typedef enum {
    bank_batt_temp_band_cold,     // < 0 C
    bank_batt_temp_band_cool,     // [0,10]
    bank_batt_temp_band_good,     // [11,25]
    bank_batt_temp_band_warm,     // [26,45]
    bank_batt_temp_band_hot,      // [46, 60]
    bank_batt_temp_band_over_heat // > 60
} bank_batt_temp_band;

typedef struct {
    bank_batt_soc_level_t bank_soc_level;
    uint8_t bank_batt_sco;

} bank_info_t;

#ifdef __cplusplus
}
#endif
#endif /* _BANK_DEF_H_ */
