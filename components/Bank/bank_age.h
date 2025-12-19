#include "bank_cfg.h"

#if BOARD_TYPE == USB2P_V1
uint8_t bank_batt_age(uint32_t energy)
{
    if (energy < 1000) {
        return 0;
    }
}
#endif
