#ifndef _BANK_CHARGE_H_
#define _BANK_CHARGE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "bank-def.h"

/**
 * @defgroup Bank_Charge
 * @{
 */

/* Exported types ------------------------------------------------------------*/
/**
 * @defgroup Bank_Charge_Exported_Teyps
 * @{
 */

/** @} end of group Bank_Charge_Exported_Teyps */

/* Exported constants
 * ------------------------------------------------------------*/
/**
 * @defgroup Bank_Charge_Exported_Constants
 * @{
 */

/** @} end of group Bank_Charge_Exported_Constants */

/* Exported functions --------------------------------------------------------*/
/**
 * @defgroup Bank_Charge_Exported_Functions
 * @{
 */
int32_t bank_chrg_init(void);
int32_t bank_chrg_process(const bank_msg_t *mail);

uint8_t bank_chrg_cable_status(void);
int32_t chrg_cable_connected(void);

/** @} end of group Bank_Charge_Exported_Functions */

/** end of group Bank_Charge*/

#ifdef __cplusplus
}
#endif

#endif /* _CHARGE_H_ */
