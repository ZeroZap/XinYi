#ifndef TEST_BOOTREASON_PORTABLE_H
#define TEST_BOOTREASON_PORTABLE_H

#include "bootreason_check.h"

bootreason_status_t bootreason_check_wdt_timeout_reset(void);
bootreason_status_t bootreason_check_normal_power_on(void);

#endif /* TEST_BOOTREASON_PORTABLE_H */
