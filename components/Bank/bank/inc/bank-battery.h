#ifndef _BANK_BATTERY_H_
#define _BANK_BATTERY_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "bank-def.h"

/**
 * @defgroup Bank_Battery
 * @{
 */

/* Exported types ------------------------------------------------------------*/
/**
 * @defgroup Bank_Battery_Exported_Teyps
 * @{
 */
typedef enum {
    batt_ntc_temprature_m_25     = -25,
    batt_ntc_temprature_m_24     = -24,
    batt_ntc_temprature_m_23     = -23,
    batt_ntc_temprature_m_22     = -22,
    batt_ntc_temprature_m_21     = -21,
    batt_ntc_temprature_m_20     = -20,
    batt_ntc_temprature_m_19     = -19,
    batt_ntc_temprature_m_18     = -18,
    batt_ntc_temprature_m_17     = -17,
    batt_ntc_temprature_m_16     = -16,
    batt_ntc_temprature_m_15     = -15,
    batt_ntc_temprature_m_14     = -14,
    batt_ntc_temprature_m_13     = -13,
    batt_ntc_temprature_m_12     = -12,
    batt_ntc_temprature_m_11     = -11,
    batt_ntc_temprature_m_10     = -10,
    batt_ntc_temprature_m_09     = -9,
    batt_ntc_temprature_m_08     = -8,
    batt_ntc_temprature_m_07     = -7,
    batt_ntc_temprature_m_06     = -6,
    batt_ntc_temprature_m_05     = -5,
    batt_ntc_temprature_m_04     = -4,
    batt_ntc_temprature_m_03     = -3,
    batt_ntc_temprature_m_02     = -2,
    batt_ntc_temprature_m_01     = -1,
    batt_ntc_temprature_0        = 0,
    batt_ntc_temprature_p_01     = 1,
    batt_ntc_temprature_p_02     = 2,
    batt_ntc_temprature_p_03     = 3,
    batt_ntc_temprature_p_04     = 4,
    batt_ntc_temprature_p_05     = 5,
    batt_ntc_temprature_p_06     = 6,
    batt_ntc_temprature_p_07     = 7,
    batt_ntc_temprature_p_08     = 8,
    batt_ntc_temprature_p_09     = 9,
    batt_ntc_temprature_p_10     = 10,
    batt_ntc_temprature_p_11     = 11,
    batt_ntc_temprature_p_12     = 12,
    batt_ntc_temprature_p_13     = 13,
    batt_ntc_temprature_p_14     = 14,
    batt_ntc_temprature_p_15     = 15,
    batt_ntc_temprature_p_16     = 16,
    batt_ntc_temprature_p_17     = 17,
    batt_ntc_temprature_p_18     = 18,
    batt_ntc_temprature_p_19     = 19,
    batt_ntc_temprature_p_20     = 20,
    batt_ntc_temprature_p_21     = 21,
    batt_ntc_temprature_p_22     = 22,
    batt_ntc_temprature_p_23     = 23,
    batt_ntc_temprature_p_24     = 24,
    batt_ntc_temprature_p_25     = 25,
    batt_ntc_temprature_p_26     = 26,
    batt_ntc_temprature_p_27     = 27,
    batt_ntc_temprature_p_28     = 28,
    batt_ntc_temprature_p_29     = 29,
    batt_ntc_temprature_p_30     = 30,
    batt_ntc_temprature_p_31     = 31,
    batt_ntc_temprature_p_32     = 32,
    batt_ntc_temprature_p_33     = 33,
    batt_ntc_temprature_p_34     = 34,
    batt_ntc_temprature_p_35     = 35,
    batt_ntc_temprature_p_36     = 36,
    batt_ntc_temprature_p_37     = 37,
    batt_ntc_temprature_p_38     = 38,
    batt_ntc_temprature_p_39     = 39,
    batt_ntc_temprature_p_40     = 40,
    batt_ntc_temprature_p_41     = 41,
    batt_ntc_temprature_p_42     = 42,
    batt_ntc_temprature_p_43     = 43,
    batt_ntc_temprature_p_44     = 44,
    batt_ntc_temprature_p_45     = 45,
    batt_ntc_temprature_p_46     = 46,
    batt_ntc_temprature_p_47     = 47,
    batt_ntc_temprature_p_48     = 48,
    batt_ntc_temprature_p_49     = 49,
    batt_ntc_temprature_p_50     = 50,
    batt_ntc_temprature_p_51     = 51,
    batt_ntc_temprature_p_52     = 52,
    batt_ntc_temprature_p_53     = 53,
    batt_ntc_temprature_p_54     = 54,
    batt_ntc_temprature_p_55     = 55,
    batt_ntc_temprature_p_56     = 56,
    batt_ntc_temprature_p_57     = 57,
    batt_ntc_temprature_p_58     = 58,
    batt_ntc_temprature_p_59     = 59,
    batt_ntc_temprature_p_60     = 60,
    batt_ntc_temprature_p_61     = 61,
    batt_ntc_temprature_p_62     = 62,
    batt_ntc_temprature_p_63     = 63,
    batt_ntc_temprature_p_64     = 64,
    batt_ntc_temprature_p_65     = 65,
    batt_ntc_temprature_p_66     = 66,
    batt_ntc_temprature_p_67     = 67,
    batt_ntc_temprature_p_68     = 68,
    batt_ntc_temprature_p_69     = 69,
    batt_ntc_temprature_p_70     = 70,
    batt_ntc_temprature_p_71     = 71,
    batt_ntc_temprature_p_72     = 72,
    batt_ntc_temprature_p_73     = 73,
    batt_ntc_temprature_p_74     = 74,
    batt_ntc_temprature_p_75     = 75,
    batt_ntc_temprature_p_76     = 76,
    batt_ntc_temprature_p_77     = 77,
    batt_ntc_temprature_p_78     = 78,
    batt_ntc_temprature_p_79     = 79,
    batt_ntc_temprature_p_80     = 80,
    batt_ntc_temprature_p_81     = 81,
    batt_ntc_temprature_p_82     = 82,
    batt_ntc_temprature_p_83     = 83,
    batt_ntc_temprature_p_84     = 84,
    batt_ntc_temprature_p_85     = 85,
    batt_ntc_temprature_p_86     = 86,
    batt_ntc_temprature_p_87     = 87,
    batt_ntc_temprature_p_88     = 88,
    batt_ntc_temprature_p_89     = 89,
    batt_ntc_temprature_p_90     = 90,
    batt_ntc_temprature_p_91     = 91,
    batt_ntc_temprature_p_92     = 92,
    batt_ntc_temprature_p_93     = 93,
    batt_ntc_temprature_p_94     = 94,
    batt_ntc_temprature_p_95     = 95,
    batt_ntc_temprature_p_96     = 96,
    batt_ntc_temprature_p_97     = 97,
    batt_ntc_temprature_p_98     = 98,
    batt_ntc_temprature_p_99     = 99,
    batt_ntc_temprature_p_100    = 100,
    batt_ntc_temprature_p_101    = 101,
    batt_ntc_temprature_max_span = 127 // [-25, 101] degree Celsius
} batt_ntc_temprature_level;

typedef struct {
    int8_t temprature;
    uint16_t ntc_value;
} batt_ntc_temprature;

typedef uint8_t (*batt_age_convert)(uint32_t energy);
typedef uint8_t (*batt_temp_band_convert)(int8_t temp);

/** @} end of group Bank_Battery_Exported_Teyps */

/* Exported constants
 * ------------------------------------------------------------*/
/**
 * @defgroup Bank_Battery_Exported_Constants
 * @{
 */

/** @} end of group Bank_Battery_Exported_Constants */

/* Exported functions --------------------------------------------------------*/
/**
 * @defgroup Bank_Battery_Exported_Functions
 * @{
 */
int32_t bank_batt_init(void);
int32_t bank_batt_process(const bank_msg_t *mail);


/** @} end of group Bank_Battery_Exported_Functions */

/** @} end of group Bank_Battery */
#ifdef __cplusplus
}
#endif

#endif
