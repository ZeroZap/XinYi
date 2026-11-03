/**
 * SPDX-License-Identifier: MIT
 * @file    gps_example.c
 * @brief   GPS (NMEA) sensor sample - self-checking, host-runnable
 * @version 2.0.0
 *
 * 演示 `xy_gps.h` 的真实公开面：注册/查找/注销、NMEA 整句与逐字节解析、
 * 位置/时间/卫星/DOP 查询、距离与方位计算、待机与唤醒、参数设置。
 *
 * 这份样张是**可执行的契约**，不是说明文字：它由 CTest `sensor_gps_example`
 * 编译并运行，任何一条 check 失败都让 `main()` 返回非零。因此
 *
 *  - 样张里出现的每个函数都必须在 `xy_gps.h` 里有声明（编译期强制）；
 *  - 每条断言的期望值都是从驱动源码算出来的真实值，不是"看起来合理"的值。
 *
 * 刻意**不使用 `assert()`**：Release 构建会定义 `NDEBUG`，`assert()` 会退化成
 * 空语句，样张就什么也检查不了。也刻意不依赖任何真实 UART/GPS 模块——NMEA
 * 语句作为字符串喂给解析器，因此在 PC 上可复现。
 */

#include <stdio.h>
#include <string.h>

#include "xy_gps.h"
#include "xy_hal_sys.h"

/* ==================== 自检骨架 ==================== */

static int g_checks;
static int g_failures;

static void check(bool condition, const char *what)
{
    g_checks++;
    if (!condition) {
        g_failures++;
        printf("FAIL: %s\n", what);
    }
}

static void check_str(const char *what, const char *expected, const char *actual)
{
    g_checks++;
    if (actual == NULL || strcmp(expected, actual) != 0) {
        g_failures++;
        printf("FAIL: %s (expected \"%s\", got \"%s\")\n", what, expected,
               actual == NULL ? "(null)" : actual);
    }
}

/* ==================== 样例数据 ==================== */

/* 一条真实的 GGA 语句（1 Hz 输出，8 颗卫星参与定位，海拔 545.4 m）。 */
static const char GGA_SENTENCE[] =
    "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47";

/* 同一条语句，校验和故意改成 *00。 */
static const char GGA_BAD_CHECKSUM[] =
    "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*00";

/* 带 CRLF 结尾的 RMC 语句，用于逐字节喂入路径。校验和 *61 是按
 * "$GPRMC,092204,A,4250.5589,S,14718.5084,E,000.5,054.7,191194,020.3,E" 的
 * 逐字节 XOR 算出来的真值（tests/unit/sensor/test_gps.c 里同一条语句写的是
 * *68，只是因为那个用例把 checksum_check 关了，所以从来没被验证过）。 */
static const char RMC_SENTENCE[] =
    "$GPRMC,092204,A,4250.5589,S,14718.5084,E,000.5,054.7,191194,020.3,E*61\r\n";

static void gps_sample_init(gps_device_t *dev, const char *name)
{
    memset(dev, 0, sizeof(*dev));

    /* `name` 是定长数组，必须拷贝，不能整体赋值。 */
    strncpy(dev->name, name, GPS_NAME_MAX_LEN - 1U);
    dev->name[GPS_NAME_MAX_LEN - 1U] = '\0';

    dev->config.baudrate = 9600U;
    dev->config.data_bits = 8U;
    dev->config.stop_bits = 1U;
    dev->config.parity = 'N';
    dev->config.update_rate = 1U;
    dev->config.nmea_output = GPS_NMEA_GGA | GPS_NMEA_RMC | GPS_NMEA_GSV;
    dev->config.checksum_check = true;
}

/* ==================== 1. 注册 / 查找 / 注销 ==================== */

static void demo_registry(void)
{
    gps_device_t dev;
    gps_device_t duplicate;

    gps_sample_init(&dev, "GPS_AT6558");
    gps_sample_init(&duplicate, "GPS_AT6558");

    check(gps_register(NULL) == XY_DEVICE_INVALID_PARAM, "gps_register(NULL) rejected");

    check(gps_register(&dev) == XY_DEVICE_OK, "gps_register(dev) accepted");
    check(dev.status == GPS_STATUS_READY, "register marks device READY");
    check(gps_get_count() == 1U, "gps_get_count() == 1 after register");

    check(gps_find("GPS_AT6558") == &dev, "gps_find() returns registered device");
    check(gps_find(NULL) == NULL, "gps_find(NULL) returns NULL");
    check(gps_find("no-such-gps") == NULL, "gps_find(unknown) returns NULL");

    /* 重名设备被拒绝，注册表不变。 */
    check(gps_register(&duplicate) == XY_DEVICE_INVALID_PARAM, "duplicate name rejected");
    check(gps_get_count() == 1U, "rejected register leaves count at 1");

    check(gps_unregister(NULL) == XY_DEVICE_INVALID_PARAM, "gps_unregister(NULL) rejected");
    check(gps_unregister(&dev) == XY_DEVICE_OK, "gps_unregister(dev) accepted");
    check(dev.status == GPS_STATUS_IDLE, "unregister marks device IDLE");
    check(gps_get_count() == 0U, "gps_get_count() == 0 after unregister");
    check(gps_unregister(&dev) == XY_DEVICE_NOT_FOUND, "second unregister -> XY_DEVICE_NOT_FOUND");
}

/* ==================== 2. 整句解析与查询 ==================== */

static void demo_parse_and_query(void)
{
    gps_device_t dev;
    gps_device_t unfixed;
    gps_data_t data;
    double lat = 0.0;
    double lon = 0.0;
    float alt = 0.0f;
    float hdop = 0.0f;
    float pdop = 0.0f;
    float vdop = 0.0f;
    uint32_t timestamp = 0U;
    uint8_t count = 0xFFU;
    uint8_t used = 0xFFU;
    uint8_t visible = 0xFFU;

    gps_sample_init(&dev, "GPS_query");
    gps_sample_init(&unfixed, "GPS_unfixed");

    check(gps_read_data(NULL, &data) == XY_DEVICE_INVALID_PARAM, "gps_read_data(NULL, data) rejected");
    check(gps_read_data(&dev, NULL) == XY_DEVICE_INVALID_PARAM, "gps_read_data(dev, NULL) rejected");
    check(gps_is_fixed(NULL) == false, "gps_is_fixed(NULL) is false");

    /* 解析之前没有定位。 */
    check(gps_is_fixed(&dev) == false, "device is not fixed before any sentence");

    /* 未定位时 gps_get_position 明确失败，且不写输出。 */
    check(gps_get_position(NULL, &lat, &lon, &alt) == XY_DEVICE_INVALID_PARAM,
          "gps_get_position(NULL, ...) rejected");
    check(gps_get_position(&unfixed, &lat, &lon, &alt) == XY_DEVICE_ERROR,
          "gps_get_position() before fix -> XY_DEVICE_ERROR");

    /* 参数校验。 */
    check(gps_parse_nmea(NULL, GGA_SENTENCE, (uint16_t)strlen(GGA_SENTENCE)) == XY_DEVICE_INVALID_PARAM,
          "gps_parse_nmea(NULL, ...) rejected");
    check(gps_parse_nmea(&dev, NULL, 10U) == XY_DEVICE_INVALID_PARAM, "gps_parse_nmea(dev, NULL) rejected");
    check(gps_parse_nmea(&dev, GGA_SENTENCE, 0U) == XY_DEVICE_INVALID_PARAM,
          "gps_parse_nmea(dev, sentence, 0) rejected");

    /* 整句解析。 */
    check(gps_parse_nmea(&dev, GGA_SENTENCE, (uint16_t)strlen(GGA_SENTENCE)) == XY_DEVICE_OK,
          "GGA sentence parsed");
    check(dev.frame_count == 0U, "整句路径不计 frame_count（只有逐字节路径计）");

    check(gps_is_fixed(&dev) == true, "device is fixed after a valid GGA");
    check(dev.data.quality == GPS_QUALITY_GPS_SPS, "GGA quality -> GPS_QUALITY_GPS_SPS");
    check(dev.data.satellites_used == 8U, "GGA satellites_used == 8");
    check(dev.data.hour == 12U && dev.data.minute == 35U && dev.data.second == 19U,
          "GGA time fields 12:35:19");

    /* 4807.038 N / 01131.000 E -> 48.1173 / 11.51666 度 */
    check(gps_read_data(&dev, &data) == XY_DEVICE_OK, "gps_read_data(dev, data) accepted");
    check(data.data_valid, "cached data is valid");
    check(data.latitude > 48.1172 && data.latitude < 48.1174, "GGA latitude == 48.1173 deg");
    check(data.longitude > 11.5166 && data.longitude < 11.5168, "GGA longitude == 11.51666 deg");
    check(data.altitude > 545.39f && data.altitude < 545.41f, "GGA altitude == 545.4 m");
    check(data.hdop > 0.89f && data.hdop < 0.91f, "GGA hdop == 0.9");

    check(gps_get_position(&dev, &lat, &lon, &alt) == XY_DEVICE_OK, "gps_get_position() after fix");
    check(lat > 48.1172 && lat < 48.1174, "gps_get_position() latitude");
    check(lon > 11.5166 && lon < 11.5168, "gps_get_position() longitude");
    check(alt > 545.39f && alt < 545.41f, "gps_get_position() altitude");

    /* 输出指针可以被单独忽略。 */
    check(gps_get_position(&dev, NULL, NULL, NULL) == XY_DEVICE_OK,
          "gps_get_position() accepts all-NULL outputs");

    check(gps_get_time(&dev, &timestamp) == XY_DEVICE_OK, "gps_get_time() accepted");
    check(timestamp > 0U, "gps_get_time() returns a non-zero stamp");
    check(gps_get_time(NULL, &timestamp) == XY_DEVICE_INVALID_PARAM, "gps_get_time(NULL, ...) rejected");

    check(gps_get_satellites(&dev, &count, &used, &visible) == XY_DEVICE_OK,
          "gps_get_satellites() accepted");
    check(used == 8U, "gps_get_satellites() used == 8");
    check(count == 0U, "gps_get_satellites() visible count stays 0 without GSV");
    check(gps_get_satellites(&dev, NULL, NULL, NULL) == XY_DEVICE_OK,
          "gps_get_satellites() accepts all-NULL outputs");

    check(gps_get_dop(&dev, &hdop, &pdop, &vdop) == XY_DEVICE_OK, "gps_get_dop() accepted");
    check(hdop > 0.89f && hdop < 0.91f, "gps_get_dop() hdop == 0.9");
    check(pdop == 0.0f && vdop == 0.0f, "GGA leaves pdop/vdop at 0");
    check(gps_get_dop(NULL, &hdop, &pdop, &vdop) == XY_DEVICE_INVALID_PARAM, "gps_get_dop(NULL, ...) rejected");

    /* 校验和错误的语句被拒绝、计入 parse_errors，且缓存不被破坏。 */
    check(gps_parse_nmea(&dev, GGA_BAD_CHECKSUM, (uint16_t)strlen(GGA_BAD_CHECKSUM)) == XY_DEVICE_ERROR,
          "bad checksum -> XY_DEVICE_ERROR");
    check(dev.parse_errors == 1U, "bad checksum counted in parse_errors");
    check(gps_is_fixed(&dev) == true, "rejected sentence leaves the cached fix intact");

    /* 关闭校验和后同一条语句被接受。 */
    dev.config.checksum_check = false;
    check(gps_parse_nmea(&dev, GGA_BAD_CHECKSUM, (uint16_t)strlen(GGA_BAD_CHECKSUM)) == XY_DEVICE_OK,
          "checksum_check=false accepts it");
}

/* ==================== 3. 逐字节（UART）解析 ==================== */

static void demo_byte_stream(void)
{
    gps_device_t dev;
    size_t i;

    gps_sample_init(&dev, "GPS_bytes");
    dev.config.checksum_check = true;

    check(gps_parse_byte(NULL, '$') == XY_DEVICE_INVALID_PARAM, "gps_parse_byte(NULL) rejected");

    for (i = 0U; i < strlen(RMC_SENTENCE); i++) {
        if (gps_parse_byte(&dev, (uint8_t)RMC_SENTENCE[i]) != XY_DEVICE_OK) {
            check(false, "gps_parse_byte() failed mid-sentence");
            return;
        }
    }

    check(dev.frame_count == 1U, "one frame counted after CRLF");
    check(dev.nmea_buf_idx == 0U, "frame buffer resets after a complete frame");
    check(dev.data.data_valid == true, "RMC status 'A' sets data_valid");
    /* RMC 不写 quality，而 gps_is_fixed() 要求 quality > INVALID：只收 RMC 时
     * "数据有效"与"已定位"是两件事，这条不对称是驱动当前的真实契约。 */
    check(gps_is_fixed(&dev) == false, "RMC alone leaves gps_is_fixed() false (quality untouched)");
    check(dev.data.speed > 0.257f && dev.data.speed < 0.258f, "RMC speed == 0.257 m/s");
    check(dev.data.course > 54.69f && dev.data.course < 54.71f, "RMC course == 54.7 deg");
    check(dev.data.day == 19U && dev.data.month == 11U && dev.data.year == 2094U,
          "RMC date fields 19-11-2094");
}

/* ==================== 4. 距离 / 方位 ==================== */

static void demo_geo_math(void)
{
    /* 赤道上经度差 1 度 = 111194.9 m（R = 6371 km）。 */
    float d = gps_distance(0.0, 0.0, 0.0, 1.0);
    float b = gps_bearing(0.0, 0.0, 0.0, 1.0);
    float same = gps_distance(39.9088, 116.3974, 39.9088, 116.3974);

    check(d > 111194.0f && d < 111196.0f, "gps_distance(1 deg at equator) == 111194.9 m");
    check(b > 89.99f && b < 90.01f, "gps_bearing(east) == 90 deg");
    check(same > -0.001f && same < 0.001f, "gps_distance(same point) == 0");

    /* 方位角归一到 [0, 360)：西向应为 270。 */
    b = gps_bearing(0.0, 0.0, 0.0, -1.0);
    check(b > 269.99f && b < 270.01f, "gps_bearing(west) == 270 deg");
}

/* ==================== 5. 功耗 / 命令 / 配置 ==================== */

static void demo_control(void)
{
    gps_device_t dev;

    gps_sample_init(&dev, "GPS_control");

    check(gps_sleep(NULL) == XY_DEVICE_INVALID_PARAM, "gps_sleep(NULL) rejected");
    check(gps_wakeup(NULL) == XY_DEVICE_INVALID_PARAM, "gps_wakeup(NULL) rejected");

    check(gps_sleep(&dev) == XY_DEVICE_OK, "gps_sleep() accepted");
    check(dev.status == GPS_STATUS_IDLE, "sleep marks device IDLE");
    check(gps_wakeup(&dev) == XY_DEVICE_OK, "gps_wakeup() accepted");
    check(dev.status == GPS_STATUS_READY, "wakeup marks device READY");

    check(gps_send_command(&dev, "$PCAS02,1000*2E\r\n", 18U) == XY_DEVICE_OK,
          "gps_send_command() placeholder returns XY_DEVICE_OK");

    check(gps_set_baudrate(NULL, 9600U) == XY_DEVICE_INVALID_PARAM, "gps_set_baudrate(NULL) rejected");
    check(gps_set_baudrate(&dev, 115200U) == XY_DEVICE_OK, "gps_set_baudrate() accepted");
    check(dev.config.baudrate == 115200U, "set_baudrate commits the new value");

    check(gps_set_update_rate(NULL, 1U) == XY_DEVICE_INVALID_PARAM, "gps_set_update_rate(NULL) rejected");
    check(gps_set_update_rate(&dev, 0U) == XY_DEVICE_INVALID_PARAM, "update_rate 0 rejected");
    check(gps_set_update_rate(&dev, 11U) == XY_DEVICE_INVALID_PARAM, "update_rate 11 rejected");
    check(dev.config.update_rate == 1U, "rejected update_rate leaves config unchanged");
    check(gps_set_update_rate(&dev, 5U) == XY_DEVICE_OK, "update_rate 5 accepted");
    check(dev.config.update_rate == 5U, "set_update_rate commits the new value");

    /* gps_get_uptime() 的当前语义是"平台 tick"（见头文件说明），不是数据老化时长。 */
    check(gps_get_uptime(NULL) == 0U, "gps_get_uptime(NULL) == 0");
    check(gps_get_uptime(&dev) == xy_hal_sys_get_tick_count(),
          "gps_get_uptime() == OS tick while last_update_time has no writer");

    check_str("name round-trips through the fixed-size field", "GPS_control", dev.name);
}

/* ==================== 入口 ==================== */

int main(void)
{
    demo_registry();
    demo_parse_and_query();
    demo_byte_stream();
    demo_geo_math();
    demo_control();

    printf("gps_example: %d checks, %d failed\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
