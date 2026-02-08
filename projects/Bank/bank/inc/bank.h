#ifndef _BANK_H_
#define _BANK_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
/**
 * @addtogroup HNB_U3
 * @{
 */

/**
 * @defgroup Bank
 * @note Bank module
 * @{
 */

/* Exported types ------------------------------------------------------------*/
/**
 * @defgroup Bank_Exported_Teyps
 * @{
 */
typedef enum {
    bank_ctrl_cmd_start_charging,
    bank_ctrl_cmd_stop_charging,
    bank_ctrl_cmd_start_heating,
    bank_ctrl_cmd_stop_heating,
    bank_ctrl_cmd_period_update_on,
    bank_ctrl_cmd_period_update_off,
    bank_ctrl_cmd_enter_sleep,
    bank_ctrl_cmd_max
} bank_ctrl_cmd_t;

typedef enum {
    bank_ctrl_ok = 0,
    bank_ctrl_timeout,
    bank_ctrl_not_allowed,
    bank_ctrl_nonsupport = 0xff
} bank_ctrl_stat;
/** @} end of group Bank_Exported_Teyps */

/* Exported constants
 * ------------------------------------------------------------*/
/**
 * @defgroup Bank_Exported_Constants
 * @{
 */
/**
 * Spec of charge state
 */
#define BANK_CHARGED 0
#define BANK_CHARING 1
#define BANK_IDLE 2
/** @} end of group Bank_Exported_Constants */

/* Exported functions --------------------------------------------------------*/
/**
 * @defgroup Bank_Exported_Functions
 * @{
 */
/** Bank process. */
int32_t bank_init(void);
void bank_process(void);


/** Bank get propterty. */
uint16_t bank_get_battery_voltage(void);
uint16_t bank_get_battery_voltage_rt(void);
uint16_t bang_get_battery_voltage_adc(void);
uint8_t bank_get_battery_percent(void);
uint8_t bank_get_battery_level(void);
uint8_t bank_get_battery_ui_level(void);
uint8_t bank_get_charge_state(void);

/**
 * @defgroup Bank_Coontrol_Handler  Bank Coontrol Handler
 * @{
 */
bank_ctrl_stat bank_control(bank_ctrl_cmd_t cmd, uint16_t tick_wait);

uint8_t bank_allow_sleeping(void);
uint8_t bank_allow_heating(void);

/** @} end of group Bank_Coontrol_Handler */

/** @} end of group Bank_Exported_Functions */

/* Private types ------------------------------------------------------------*/
/**
 * @defgroup Bank_Private_Teyps
 * @{
 */
/** @} end of group Bank_Private_Teyps */

/* Private constants
 * ------------------------------------------------------------*/
/**
 * @defgroup Bank_Private_Constants
 * @{
 */
/** @} end of group Bank_Private_Constants */

/* Private functions --------------------------------------------------------*/
/**
 * @defgroup Bank_Private_Functions
 * @{
 */
/** @} end of group Bank_Private_Functions */

/* Private submodule --------------------------------------------------------*/
/**
 * @defgroup Bank_Private_Modules
 * @{
 */
/** @} end of group Bank_Private_Modules */

/** @} end of group Bank */

/** @} end of group HNB_U3 */

#ifdef __cplusplus
}
#endif
#endif
