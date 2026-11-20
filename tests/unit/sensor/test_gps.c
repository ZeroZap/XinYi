#include "unity.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "xy_gps.h"

static uint32_t g_tick;

uint32_t xy_hal_sys_get_tick_count(void)
{
    return g_tick;
}


void setUp(void)
{
    g_tick = 1000U;
}

void tearDown(void)
{
}

static gps_device_t make_gps(const char *name)
{
    gps_device_t dev;
    memset(&dev, 0, sizeof(dev));
    strncpy(dev.name, name, GPS_NAME_MAX_LEN - 1U);
    dev.name[GPS_NAME_MAX_LEN - 1U] = '\0';
    dev.config.baudrate = 9600U;
    dev.config.update_rate = 1U;
    dev.config.nmea_output = GPS_NMEA_GGA | GPS_NMEA_RMC | GPS_NMEA_GSV;
    dev.config.checksum_check = false;
    return dev;
}

static void test_parse_gga_updates_position_fix_and_last_valid_snapshot(void)
{
    gps_device_t dev = make_gps("gps-gga");
    const char gga[] = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47";

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, gps_parse_nmea(&dev, gga, (uint16_t)strlen(gga)));

    TEST_ASSERT_TRUE(dev.data.data_valid);
    TEST_ASSERT_EQUAL_INT(GPS_QUALITY_GPS_SPS, dev.data.quality);
    TEST_ASSERT_BITS_HIGH(GPS_FLAG_FIXED, dev.data.status_flags);
    TEST_ASSERT_EQUAL_UINT8(8U, dev.data.satellites_used);
    TEST_ASSERT_FLOAT_WITHIN(0.000001f, 48.1173f, (float)dev.data.latitude);
    TEST_ASSERT_FLOAT_WITHIN(0.000001f, 11.516666f, (float)dev.data.longitude);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 545.4f, dev.data.altitude);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 46.9f, dev.data.geoid_height);
    TEST_ASSERT_EQUAL_UINT32(12U, dev.data.hour);
    TEST_ASSERT_EQUAL_UINT32(35U, dev.data.minute);
    TEST_ASSERT_EQUAL_UINT32(19U, dev.data.second);
    TEST_ASSERT_EQUAL_UINT32(g_tick, dev.last_valid_time);
    TEST_ASSERT_FLOAT_WITHIN(0.000001f, (float)dev.data.latitude,
                             (float)dev.last_valid_data.latitude);
}

static void test_checksum_failure_is_rejected_and_counted(void)
{
    gps_device_t dev = make_gps("gps-checksum");
    const char bad_gga[] = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*00";
    dev.config.checksum_check = true;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_ERROR, gps_parse_nmea(&dev, bad_gga, (uint16_t)strlen(bad_gga)));

    TEST_ASSERT_EQUAL_UINT32(1U, dev.parse_errors);
    TEST_ASSERT_FALSE(dev.data.data_valid);
}

static void test_parse_byte_frames_sentence_and_increments_count(void)
{
    gps_device_t dev = make_gps("gps-byte");
    const char rmc[] = "$GPRMC,092204,A,4250.5589,S,14718.5084,E,000.5,054.7,191194,020.3,E*68\n";

    for (size_t i = 0U; i < strlen(rmc); ++i) {
        TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, gps_parse_byte(&dev, (uint8_t)rmc[i]));
    }

    TEST_ASSERT_EQUAL_UINT32(1U, dev.frame_count);
    TEST_ASSERT_TRUE(dev.data.data_valid);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.257222f, dev.data.speed);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 54.7f, dev.data.course);
    TEST_ASSERT_EQUAL_UINT32(19U, dev.data.day);
    TEST_ASSERT_EQUAL_UINT32(11U, dev.data.month);
    TEST_ASSERT_EQUAL_UINT32(2094U, dev.data.year);
}

static void test_gsv_updates_visible_satellite_fields(void)
{
    gps_device_t dev = make_gps("gps-gsv");
    const char gsv[] = "$GPGSV,1,1,02,07,79,048,42,08,62,165,43*70";

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, gps_parse_nmea(&dev, gsv, (uint16_t)strlen(gsv)));

    TEST_ASSERT_EQUAL_UINT8(2U, dev.satellite_count);
    TEST_ASSERT_EQUAL_UINT8(7U, dev.satellites[0].id);
    TEST_ASSERT_EQUAL_UINT8(79U, dev.satellites[0].elevation);
    TEST_ASSERT_EQUAL_UINT8(48U, dev.satellites[0].azimuth);
    TEST_ASSERT_EQUAL_UINT8(42U, dev.satellites[0].snr);
    TEST_ASSERT_EQUAL_UINT8(8U, dev.satellites[1].id);
    TEST_ASSERT_EQUAL_UINT8(62U, dev.satellites[1].elevation);
    TEST_ASSERT_EQUAL_UINT8(165U, dev.satellites[1].azimuth);
    TEST_ASSERT_EQUAL_UINT8(43U, dev.satellites[1].snr);
}

static void test_register_find_unregister_and_capacity_guard(void)
{
    gps_device_t first = make_gps("gps-reg");
    gps_device_t duplicate = make_gps("gps-reg");
    gps_device_t filler[GPS_SATELLITE_MAX];

    TEST_ASSERT_EQUAL_UINT8(0U, gps_get_count());
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, gps_register(&first));
    TEST_ASSERT_EQUAL_INT(GPS_STATUS_READY, first.status);
    TEST_ASSERT_EQUAL_UINT8(1U, gps_get_count());
    TEST_ASSERT_EQUAL_PTR(&first, gps_find("gps-reg"));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, gps_register(&duplicate));

    for (uint8_t i = 1U; i < GPS_SATELLITE_MAX; ++i) {
        char name[16];
        snprintf(name, sizeof(name), "gps-%u", (unsigned int)i);
        filler[i] = make_gps(name);
        TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, gps_register(&filler[i]));
    }
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_NO_MEM, gps_register(&duplicate));

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, gps_unregister(&first));
    TEST_ASSERT_EQUAL_INT(GPS_STATUS_IDLE, first.status);
    TEST_ASSERT_NULL(gps_find("gps-reg"));
    for (uint8_t i = 1U; i < GPS_SATELLITE_MAX; ++i) {
        TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, gps_unregister(&filler[i]));
    }
    TEST_ASSERT_EQUAL_UINT8(0U, gps_get_count());
}

static void test_sleep_and_wakeup_reject_null_before_touching_state(void)
{
    gps_device_t dev = make_gps("gps-power");

    dev.status = GPS_STATUS_ERROR;

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, gps_sleep(NULL));
    TEST_ASSERT_EQUAL_INT(XY_DEVICE_INVALID_PARAM, gps_wakeup(NULL));

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, gps_sleep(&dev));
    TEST_ASSERT_EQUAL_INT(GPS_STATUS_IDLE, dev.status);

    TEST_ASSERT_EQUAL_INT(XY_DEVICE_OK, gps_wakeup(&dev));
    TEST_ASSERT_EQUAL_INT(GPS_STATUS_READY, dev.status);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_parse_gga_updates_position_fix_and_last_valid_snapshot);
    RUN_TEST(test_checksum_failure_is_rejected_and_counted);
    RUN_TEST(test_parse_byte_frames_sentence_and_increments_count);
    RUN_TEST(test_gsv_updates_visible_satellite_fields);
    RUN_TEST(test_register_find_unregister_and_capacity_guard);
    RUN_TEST(test_sleep_and_wakeup_reject_null_before_touching_state);
    return UNITY_END();
}
