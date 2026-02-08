/**
 * @file driver-bq2562x.h
 * @author Xusheng Chen
 * @brief The header file of TI bq2562x(bq25620/bq25622) battery charger driver.
 * @version 0.1
 * @date 2024-12-24
 *
 * @copyright Copyright (c) 2024 Philip Morris Products S.A.
 * All rights reserved.
 *
 */
#ifndef _DRIVER_BQ2562X_H_
#define _DRIVER_BQ2562X_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "driver-charger-type0.h"
#include "target.h"

/**
 * @brief BQ2562x I2C chip address.
 */
#define BQ25620_I2C_SLAVE_ADDRESS 0x6B

/**
 * @name Register numbers definition extracted from TI specification.
 * @{
 */
#define BQ2562X_REG02_CHARGE_CURRENT_LIMIT 0x02
#define BQ2562X_REG04_CHARGE_VOLTAGE_LIMIT 0x04
#define BQ2562X_REG06_INPUT_CURRENT_LIMIT 0x06
#define BQ2562X_REG08_INPUT_VOLTAGE_LIMIT 0x08
#define BQ2562X_REG0A_IOTG_REGULATION 0x0A
#define BQ2562X_REG0C_VOTG_REGULATION 0x0C
#define BQ2562X_REG0E_MINIMAL_SYSTEM_VOLTAGE 0x0E
#define BQ2562X_REG10_PRECHARGE_CONTROL 0x10
#define BQ2562X_REG12_TERMINATION_CONTROL 0x12
#define BQ2562X_REG14_CHARGE_CONTROL_0 0x14
#define BQ2562X_REG15_CHARGE_TIMER_CONTROL 0x15
#define BQ2562X_REG16_CHARGER_CONTROL_1 0x16
#define BQ2562X_REG17_CHARGER_CONTROL_2 0x17
#define BQ2562X_REG18_CHARGER_CONTROL_3 0x18
#define BQ2562X_REG19_CHARGER_CONTROL_4 0x19
#define BQ2562X_REG1A_NTC_CONTROL_0 0x1A
#define BQ2562X_REG1B_NTC_CONTROL_1 0x1B
#define BQ2562X_REG1C_NTC_CONTROL_2 0x1C
#define BQ2562X_REG1D_CHARGER_STATUS_0 0x1D
#define BQ2562X_REG1E_CHARGER_STATUS_1 0x1E
#define BQ2562X_REG1F_FAULT_STATUS_0 0x1F
#define BQ2562X_REG20_CHARGER_FLAG_0 0x20
#define BQ2562X_REG21_CHARGER_FLAG_1 0x21
#define BQ2562X_REG22_FAULT_FLAG_0 0x22
#define BQ2562X_REG23_CHARGER_MASK_0 0x23
#define BQ2562X_REG24_CHARGER_MASK_1 0x24
#define BQ2562X_REG25_FAULT_MASK_0 0x25
#define BQ2562X_REG26_ADC_CONTROL 0x26
#define BQ2562X_REG27_ADC_FUNCTION_DISABLE_0 0x27
#define BQ2562X_REG28_IBUS_ADC 0x28
#define BQ2562X_REG2A_IBAT_ADC 0x2A
#define BQ2562X_REG2C_VBUS_ADC 0x2C
#define BQ2562X_REG2E_VPMID_ADC 0x2E
#define BQ2562X_REG30_VBAT_ADC 0x30
#define BQ2562X_REG32_VSYS_ADC 0x32
#define BQ2562X_REG34_TS_ADC 0x34
#define BQ2562X_REG36_TDIE_ADC 0x36
#define BQ2562X_REG38_PART_INFORMATION 0x38

/** @} */

/**
 * @brief Device id.
 *
 */
#define BQ25620_DEVICE_ID 0x00
#define BQ25622_DEVICE_ID 0x08

/**
 * @brief Device faults
 *
 */
#define BQ2562X_FAULT_NONE 0x00
#define BQ2562X_FAULT_BAT_TEMP 0x01
#define BQ2562X_FAULT_CHRG_TIMER_EXPIRATION 0x02
#define BQ2562X_FAULT_WTD_TIMER_EXPIRATION 0x04
#define BQ2562X_FAULT_THERMAL_SHUTDOWN 0x08
#define BQ2562X_FAULT_BOOST 0x10
#define BQ2562X_FAULT_SYS 0x20
#define BQ2562X_FAULT_BAT 0x40
#define BQ2562X_FAULT_VBUS 0x80

/**
 * @brief Device charge status
 *
 */
#define BQ2562X_CHG_STAT_DONE_OR_IDLE 0x00
#define BQ2562X_CHG_STAT_CC 0x01
#define BQ2562X_CHG_STAT_CV 0x02
#define BQ2562X_CHG_STAT_TOP_OFF 0x03

/**
 * @brief charger current limit: 0x02 chrg_curr_limit[11:6]
 */
#define BQ2562X_ICHG_SFT (6)
#define BQ2562X_ICHG_MSK (0x3f << BQ2562X_ICHG_SFT)
#define BQ2562X_ICHG_MIN (80)
#define BQ2562X_ICHG_MAX (3520)
#define BQ2562X_ICHG_STEP (80)

/**
 * @brief charger voltage limit: 0x04 chrg_volt_limit [11:3]
 */
#define BQ2562X_VREG_SFT (3)
#define BQ2562X_VREG_MSK (0x1ff << BQ2562X_VREG_SFT)
#define BQ2562X_VREG_MIN (3500)
#define BQ2562X_VREG_MAX (4800)
#define BQ2562X_VREG_STEP (10)

/**
 * @brief input current limit: 0x06 input_curr_limit[11:4]
 */
#define BQ2562X_IINDPM_SFT (4)
#define BQ2562X_IINDPM_MSK (0xff << BQ2562X_IINDPM_SFT)
#define BQ2562X_IINDPM_MIN (100)
#define BQ2562X_IINDPM_MAX (3200)
#define BQ2562X_IINDPM_STEP (20)

/**
 * @brief input voltage limit: 0x08 input_volt_limit[13:5]
 */
#define BQ2562X_VINDPM_SFT (5)
#define BQ2562X_VINDPM_MSK (0x3ff << BQ2562X_VINDPM_SFT)
#define BQ2562X_VINDPM_MIN (3800)
#define BQ2562X_VINDPM_MAX (16800)
#define BQ2562X_VINDPM_STEP (40)

/**
 * @brief Pre-Charge current Limit: 0x10 prechg_curr_limit[8:4]
 *
 */
#define BQ2562X_IPRECHG_SFT (4)
#define BQ2562X_IPRECHG_MSK (0x3f << BQ2562X_IPRECHG_SFT)
#define BQ2562X_IPRECHG_MIN (20)
#define BQ2562X_IPRECHG_MAX (620)
#define BQ2562X_IPRECHG_STEP (20)

/**
 * @brief termination current threshold: 0x12 term_cntrl[8:3]
 */
#define BQ2562X_ITERM_SFT (3)
#define BQ2562X_ITERM_MSK (0x3f << BQ2562X_ITERM_SFT)
#define BQ2562X_ITERM_MIN (10)
#define BQ2562X_ITERM_MAX (620)
#define BQ2562X_ITERM_STEP (10)
/** @} end of group BQ2562X_REG12_Termination_Current_Limit */

/**
 * @brief Auto DPDM Disable : 0x15 chrg_timer_cntrl[6]
 */
#define BQ2562X_AUTO_DPDM_SFT (6)
#define BQ2562X_AUDO_DPDM_MSK (0x1 << BQ2562X_AUTO_DPDM_SFT)
#define BQ2562X_AUDO_DPDM_ENABLE (1)
#define BQ2562X_AUDO_DPDM_DISABLE (0)

/**
 * @brief Force DPDM Enable : 0x15 chrg_timer_cntrl[5]
 */
#define BQ2562X_FORCE_DPDM_SFT (5)
#define BQ2562X_FORCE_DPDM_MSK (0x1 << BQ2562X_FORCE_DPDM_SFT)
#define BQ2562X_FORCE_DPDM_ENABLE (1)
#define BQ2562X_FORCE_DPDM_DISABLE (0)

/**
 * @brief Check DPDM Dection Done : 0x15 chrg_timer_cntrl[5]
 */
#define BQ2562X_DPDM_DONE_SFT (5)
#define BQ2562X_DPDM_DONE_MSK (0x1 << BQ2562X_DPDM_DONE_SFT)
#define BQ2562X_DPDM_DONE (0)
#define BQ2562X_DPDM_DETECTING (1)

/**
 * @brief charge timer enable: 0x15 chrg_timer_cntrl[2]
 */
#define BQ2562X_CHG_TMR_EN_SFT (2)
#define BQ2562X_CHG_TMR_EN_MSK (0x1 << BQ2562X_CHG_TMR_EN_SFT)
#define BQ2562X_CHG_TMR_ENABLE (1)
#define BQ2562X_CHG_TMR_DISABLE (0)

/**
 * @brief pre-charge timer : 0x15 chrg_timer_cntrl[1]
 */
#define BQ2562X_PRECHG_TMR_EN_SFT (1)
#define BQ2562X_PRECHG_TMR_EN_MSK (0x1 << BQ2562X_PRECHG_TMR_EN_SFT)
#define BQ2562X_PRECHG_TMR_ENABLE (1)
#define BQ2562X_PRECHG_TMR_DISABLE (0)

/**
 * @brief charge timer control: 0x15 chrg_timer_cntrl[0]
 */
#define BQ2562X_CHG_TMR_SET_SFT (0)
#define BQ2562X_CHG_TMR_SET_MSK (0x1 << BQ2562X_CHG_TMR_SET_SFT)
#define BQ2562X_CHG_TMR_14P5H 0
#define BQ2562X_CHG_TMR_28H 1

/**
 * @brief charge enable: 0x16 chrg_cntrl1[5]
 */
#define BQ2562X_EN_CHG_SFT (5)
#define BQ2562X_EN_CHG_MSK (0x1 << BQ2562X_EN_CHG_SFT)
#define BQ2562X_EN_CHG_ENABLE (1)
#define BQ2562X_EN_CHG_DISABLE (0)

/**
 * @brief watchdog reset: 0x16 chrg_cntrl1[2]
 */
#define BQ2562X_IWDG_RESET (0x01 << 2)

/**
 * @brief watchdog setting: 0x16 chrg_cntrl1[1:0]
 */
#define BQ2562X_IWDG_SFT (0)
#define BQ2562X_IWDG_MSK (0x03 << BQ2562X_IWDG_SFT)
#define BQ2562X_IWDG_DISABLE (0x0)
#define BQ2562X_IWDG_50S (0x1)
#define BQ2562X_IWDG_100S (0x2)
#define BQ2562X_IWDG_200S (0x3)

/**
 * @brief reset registers: 0x17 chrg_cntrl2[7]
 */
#define BQ2562X_REG_RST_RESET (0x01 << 7)

/**
 * @brief Vbus OVP setting: 0x17 chrg_cntrl2[0]
 */
#define BQ2562X_VBUS_OVP_SFT (0)
#define BQ2562X_VBUS_OVP_OVP_MSK (0x01 << BQ2562X_VBUS_OVP_SFT)
#define BQ2562X_VBUS_OVP_6_3V 0
#define BQ2562X_VBUS_OVP_18_5V 1

/**
 * @brief OTG mode control: 0x18 chrg_cntrl3[6]
 */
#define BQ2562X_EN_OTG_SFT (6)
#define BQ2562X_EN_OTG_MSK (0x1 << BQ2562X_EN_OTG_SFT)
#define BQ2562X_EN_OTG_ENABLE (1)
#define BQ2562X_EN_OTG_DISABLE (0)

/**
 * @brief BATFET control: 0x18 chrg_ctrl3[1:0]
 */
#define BQ2562X_BATFET_CTRL_SFT (0)
#define BQ2562X_BATFET_CTRL_MSK (0x3 << BQ2562X_BATFET_CTRL_SFT)
#define BQ2562X_BATFET_CTRL_IDLE (0x0)
#define BQ2562X_BATFET_CTRL_SHUTDOWN (0x1)
#define BQ2562X_BATFET_CTRL_SHIP (0x2)
#define BQ2562X_BATFET_CTRL_SYS_RESET (0x3)

/**
 * @brief safety timer status: 0x1d chrg_status[1]
 */
#define BQ2562X_CHG_TMR_EXPIRED (0x1 << 1)

/**
 * @brief watchdog timer status: 0x1d chrg_status[1]
 */
#define BQ2562X_IWTD_TMR_EXPIRED (0x1 << 0)

/**
 * @brief charge source type: 0x1E chrg_status1[0:2]
 */
#define BQ2562X_VBUS_TYPE_SFT (0)
#define BQ2562X_VBUS_TYPE_MSK (0x07 << BQ2562X_VBUS_TYPE_SFT)
#define BQ2562X_VBUS_TYPE_NO_ADAPTER 0 /* No adapter. */
#define BQ2562X_VBUS_TYPE_SDP 1        /* SDP adapter. */
#define BQ2562X_VBUS_TYPE_CDP 2        /* CDP adapter. */
#define BQ2562X_VBUS_TYPE_DCP 3        /* DCP adapter. */
#define BQ2562X_VBUS_TYPE_UNKNOWN 4    /* Unknown adapter. */
#define BQ2562X_VBUS_TYPE_NSA 5        /* NSA adapter. */
#define BQ2562X_VBUS_TYPE_HVDCP 6      /* HVDP adapter. */
#define BQ2562X_VBUS_TYPE_OTG 7        /* OTG . */

/**
 * @brief extra charge source type
 * 1. When BQ2562X_VBUS_TYPE = BQ2562X_VBUS_TYPE_NSA
 * 2. Check Input current:
 * BQ2562X_VBUS_TYPE_NSA_1A : 1000mA
 * BQ2562X_VBUS_TYPE_NSA_2P1A: 2100mA
 * BQ2562X_VBUS_TYPE_NSA_2P4A: 2400mA
 */
#define BQ2562X_VBUS_TYPE_NSA_1A 8    /* NSA-1A adapter */
#define BQ2562X_VBUS_TYPE_NSA_2P1A 9  /* NSA-2.1A adapter */
#define BQ2562X_VBUS_TYPE_NSA_2P4A 10 /* NSA-2.4A adapter */

/**
 * @brief charge status: 0x1E chrg_status1[4:3]
 */
#define BQ2562X_CHG_STAT_SFT (3)
#define BQ2562X_CHG_STAT_MSK (0x03 << BQ2562X_CHG_STAT_SFT)
#define BQ2562X_CHG_STAT_NOT_CHARGING (0x00)
#define BQ2562X_CHG_STAT_FAST_CHARGING (0x01)
#define BQ2562X_CHG_STAT_TOPOFF_TIME_ACTIVE_CHARGING (0x02)

/**
 * @brief charge fault: 0x1f fault_status[7:0]
 */
#define BQ2562X_CHG_FAULT_TS_NORMAL 0x00
#define BQ2562X_CHG_FAULT_TS_COLD 0x01
#define BQ2562X_CHG_FAULT_TS_HOT 0x02
#define BQ2562X_CHG_FAULT_TS_COOL 0x03
#define BQ2562X_CHG_FAULT_TS_WARM 0x04
#define BQ2562X_CHG_FAULT_TS_PRE_COOL 0x05
#define BQ2562X_CHG_FAULT_TS_PRE_WARM 0x06
#define BQ2562X_CHG_FAULT_TS_SHORT 0x08
#define BQ2562X_CHG_FAULT_OTG_FAULT 0x10
#define BQ2562X_CHG_FAULT_VSYS_FAULT 0x20
#define BQ2562X_CHG_FAULT_BATT_FAULT 0x40
#define BQ2562X_CHG_FAULT_VBUS_FAULT 0x80

/**
 * @brief adc enable: 0x26 adc_control[7]
 */
#define BQ2562X_ADC_EN_SFT (7)
#define BQ2562X_ADC_EN_MSK (0x01 << BQ2562X_ADC_EN_SFT)
#define BQ2562X_ADC_EN_DISABLE 0
#define BQ2562X_ADC_EN_ENABLE 1

/**
 * @brief adc rate: 0x26 adc_control[6]
 */
#define BQ2562X_ADC_RATE_SFT (6)
#define BQ2562X_ADC_RATE_MSK (0x01 << BQ2562X_ADC_RATE_SFT)
#define BQ2562X_ADC_RATE_CONTINUOUS 0
#define BQ2562X_ADC_RATE_ONE_SHOT 1

/**
 * @brief adc sample speed: 0x26 adc_control[5:4]
 */
#define BQ2562X_ADC_SAMPLE_SFT (5)
#define BQ2562X_ADC_SAMPLE_MSK (0x03 << BQ2562X_ADC_SAMPLE_SFT)
#define BQ2562X_ADC_SAMPLE_12BIT 0
#define BQ2562X_ADC_SAMPLE_11BIT 1
#define BQ2562X_ADC_SAMPLE_10BIT 2
#define BQ2562X_ADC_SAMPLE_9BIT 3

/**
 * @brief adc average: 0x26 adc_control[3]
 */
#define BQ2562X_ADC_AVG_SFT (3)
#define BQ2562X_ADC_AVG_MSK (0x01 << BQ2562X_ADC_AVG_SFT)
#define BQ2562X_ADC_AVG_SINGLE_VALUE  = 0
#define BQ2562X_ADC_AVG_AVERAGE_VALUE = 1

/**
 * @brief adc average init value: 0x26 adc_control[2]
 */
#define BQ2562X_ADC_AVG_INIT_SFT (2)
#define BQ2562X_ADC_AVG_INIT_MSK (0x01 << BQ2562X_ADC_AVG_INIT_SFT)
#define BQ2562X_ADC_AVG_INIT_USING_EXIT 0
#define BQ2562X_ADC_AVG_INIT_USING_NEW 1

/**
 * @brief Bus current : 0x28[15:1]
 */
#define BQ2562X_IBUS_SFT (1)
#define BQ2562X_IBUS_MSK (0xFFFE << BQ2562X_IBUS_SFT)
#define BQ2562X_IBUS_MIN (-4000)
#define BQ2562X_IBUS__MAX (4000)
#define BQ2562X_IBUS_STEP (2)

/**
 * @brief Battery current : 0x2A[15:2]
 */
#define BQ2562X_IBAT_SFT (2)
#define BQ2562X_IBAT_MSK (0xFFFC << BQ2562X_IBAT_SFT)
#define BQ2562X_IBAT_MIN (-7500)
#define BQ2562X_IBAT__MAX (4000)
#define BQ2562X_IBAT_STEP (4)

/**
 * @brief Bus voltage : 0x2E[14:2]
 */
#define BQ2562X_VBUS_SFT (2)
#define BQ2562X_VBUS_MSK (0x7FFC << BQ2562X_VBUS_SFT)
#define BQ2562X_VBUS_MIN (0)
#define BQ2562X_VBUS_MAX (18000)
#define BQ2562X_VBUS_STEP (3.97)

/**
 * @brief Bus current : 0x30[12:1]
 */
#define BQ2562X_VBAT_SFT (1)
#define BQ2562X_VBAT_MSK (0x1FFE << BQ2562X_VBAT_SFT)
#define BQ2562X_VBAT_MIN (0)
#define BQ2562X_VBAT_MAX (5572)
#define BQ2562X_VBAT_STEP (1.99)

/**
 * @brief chip id: 0x3f part_infor[5:3]
 */
#define BQ2562X_PN_SFT (2)
#define BQ2562X_PN_MSK (0x3 << BQ2562X_PN_SFT)

extern driver_charger_type0_t driver_bq2562x;

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_BQ2562X_H_ */
