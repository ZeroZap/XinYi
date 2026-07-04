#ifndef TEST_HAL_RTC_H
#define TEST_HAL_RTC_H

#include <stdint.h>

void hal_rtc_get_data(uint32_t offset, char *buf, uint32_t len);
void hal_rtc_set_data(uint32_t offset, const char *buf, uint32_t len);

#endif /* TEST_HAL_RTC_H */
