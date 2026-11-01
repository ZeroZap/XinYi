/**
 * SPDX-License-Identifier: MIT
 * @file    sensor_demo.c
 * @brief   Sensor sample - canonical Device model, self-checking and host-runnable
 * @version 2.0.0
 *
 * 演示 XinYi 当前的传感器路径（canonical = `components/device/`）：
 *
 *     应用   sensor_demo.c
 *       └─ 驱动   components/drivers/sensor/temperature/sht30/xy_sht30.c
 *            └─ 总线  xy_i2c_device_init()/write()/read()
 *                     （components/device/src/xy_device_bus_helpers.c）
 *                 └─ HAL   xy_hal_i2c_master_transmit()/receive()
 *
 * 这份样张是**可执行的契约**，不是说明文字：它由 CTest `sensor_device_example`
 * 编译并运行，任何一条 check 失败都让 `main()` 返回非零。因此
 *
 *  - 样张里出现的每个函数、每个字段都必须在公开头里真实存在（编译期强制，
 *    目标带 `-Werror=implicit-function-declaration`）；
 *  - 期望值是从驱动源码算出来的（`0x6666` → 2500 是精确值，推导见 `load_valid_frame()`），
 *    不是"看起来合理"的数字；
 *  - 最底层那个假 HAL 是**测试替身**，本文件不主张任何 MCU 运行时或实板证据。
 *
 * 刻意**不使用 `assert()`**：Release 构建会定义 `NDEBUG`，`assert()` 退化成空语句，
 * 样张就什么也检查不了。
 *
 * 取代旧版本（2026-11-02）：旧 `sensor_demo.c` 教的是 `components/sensor/inc/xy_sensor.h`
 * 里的旧框架——它调用的 12 个 `xy_sensor_*` 入口里，10 个在全仓**没有任何定义**，另外 2 个
 * （`xy_sensor_sample_fetch`/`xy_sensor_channel_get`）只有 signature 不同的 canonical 版本；
 * `xy_sensor_wakeup`/`xy_sensor_sleep`/`xy_sensor_print_all` 连声明都没有；第 99 行还是一个
 * C++ lambda（`[](...){...}`）写在 .c 文件里。它连语法检查都过不去，而没有任何构建目标编译过它。
 */

#include <stdio.h>
#include <string.h>

#include "xy_hal_delay.h"
#include "xy_hal_i2c.h"
#include "xy_sht30.h"

/* ==================== 自检骨架 ==================== */

static int g_checks;
static int g_failures;

static void check(int condition, const char *what)
{
    g_checks++;
    if (!condition) {
        g_failures++;
        printf("FAIL: %s\n", what);
    }
}

/* ==================== 假 HAL ====================
 * 只实现这条路径真正用到的三个入口。其余 HAL 入口不定义：驱动一旦改走别的传输
 * 路径（DMA、mem_read、另一条总线），链接期就会红，而不是被一个空替身静默放过。
 */

typedef struct {
    uint16_t addr;
    uint8_t data[4];
    size_t len;
    uint32_t timeout;
} i2c_write_t;

static int g_bus; /* 只是一个非空句柄：xy_i2c_device_init() 要求 i2c_handle != NULL */
#define FAKE_I2C_HANDLE ((void *)&g_bus)

static i2c_write_t g_writes[8];
static size_t g_write_count;
static uint8_t g_read_frame[6];
static size_t g_read_frame_len;
static size_t g_read_count;
static xy_hal_error_t g_write_status;
static xy_hal_error_t g_read_status;
static uint32_t g_delay_ms;

xy_hal_error_t xy_hal_i2c_master_transmit(void *i2c, uint16_t dev_addr, const uint8_t *data,
                                          size_t len, uint32_t timeout)
{
    check(i2c == FAKE_I2C_HANDLE, "HAL transmit receives the handle the application passed in");
    if (g_write_status != XY_HAL_OK) {
        return g_write_status;
    }
    if (g_write_count < sizeof(g_writes) / sizeof(g_writes[0])) {
        i2c_write_t *write = &g_writes[g_write_count];
        write->addr = dev_addr;
        write->len = len;
        write->timeout = timeout;
        memcpy(write->data, data, len < sizeof(write->data) ? len : sizeof(write->data));
    }
    g_write_count++;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_master_receive(void *i2c, uint16_t dev_addr, uint8_t *data,
                                         size_t len, uint32_t timeout)
{
    (void)dev_addr;
    (void)timeout;
    check(i2c == FAKE_I2C_HANDLE, "HAL receive receives the handle the application passed in");
    if (g_read_status != XY_HAL_OK) {
        return g_read_status;
    }
    if (len > g_read_frame_len) {
        return XY_HAL_ERROR_INVALID_PARAM; /* 假件没有那么多字节，不替驱动编数据 */
    }
    memcpy(data, g_read_frame, len);
    g_read_count++;
    return XY_HAL_OK;
}

void xy_hal_delay_ms(uint32_t ms)
{
    g_delay_ms += ms;
}

/* ==================== 测量帧 ====================
 * SHT30 的帧格式：每 2 字节数据后跟 1 字节 CRC（多项式 0x31、初值 0xFF、MSB first）。
 * 驱动在 `xy_sht30_read()` 里逐字校验它，所以样张必须自己拼出合法帧。
 */

static uint8_t sht30_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFFU;

    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (unsigned bit = 0; bit < 8U; ++bit) {
            crc = (crc & 0x80U) ? (uint8_t)((crc << 1) ^ 0x31U) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

/* 一帧真实形状的测量数据：温度原始值 0x6666 (=26214)、湿度原始值 0x8000 (=32768)。
 *
 * 温度：(int32_t)26214 * 17500 = 458,745,000 = 65535 * 7000，恰好整除 → 7000 - 4500 = 2500
 *       即 25.00 °C（所以这个期望值不是"接近 25 度"，是精确的）。
 * 湿度：32768 * 10000 / 65535 = 5000.07… → 整数截断 5000，即 50.00 %RH。
 */
static void load_valid_frame(void)
{
    g_read_frame[0] = 0x66U;
    g_read_frame[1] = 0x66U;
    g_read_frame[2] = sht30_crc8(&g_read_frame[0], 2U);
    g_read_frame[3] = 0x80U;
    g_read_frame[4] = 0x00U;
    g_read_frame[5] = sht30_crc8(&g_read_frame[3], 2U);
    g_read_frame_len = sizeof(g_read_frame);
}

/* ==================== 场景 ==================== */

int main(void)
{
    xy_sht30_t sht;

    /* 0. 夹具自身：CRC 常数与上面推导的两个期望值必须一致 */
    load_valid_frame();
    check(g_read_frame[2] == 0x93U, "CRC of the temperature word is 0x93");
    check(g_read_frame[5] == 0xA2U, "CRC of the humidity word is 0xA2");

    /* 1. 参数被拒时不产生任何总线流量 */
    memset(&sht, 0, sizeof(sht));
    g_write_count = 0;
    check(xy_sht30_init(NULL, FAKE_I2C_HANDLE) == XY_DEVICE_INVALID_PARAM, "init(NULL) rejected");
    check(xy_sht30_init(&sht, NULL) == XY_DEVICE_INVALID_PARAM, "init(handle = NULL) rejected");
    check(xy_sht30_init_addr(&sht, FAKE_I2C_HANDLE, 0x43U) == XY_DEVICE_INVALID_PARAM,
          "init at 0x43 rejected: SHT30 only exists at 0x44/0x45");
    check(g_write_count == 0U, "rejected init produced no I2C traffic");

    /* 2. 初始化：一次软复位写；地址与超时由驱动自己决定 */
    check(xy_sht30_init(&sht, FAKE_I2C_HANDLE) == XY_DEVICE_OK, "init at 0x44 accepted");
    check(g_write_count == 1U, "init wrote exactly one command");
    check(g_writes[0].addr == 0x44U, "init addressed 0x44");
    check(g_writes[0].len == 2U && g_writes[0].data[0] == 0x30U && g_writes[0].data[1] == 0xA2U,
          "init sent the 0x30A2 soft reset");
    check(g_writes[0].timeout == 1000U, "init kept the driver's 1000 ms transfer budget");
    check(sht.i2c_dev.base.initialized, "init marked the bus initialised");

    /* 3. 一次完整采样：命令 → 15 ms 转换 → 6 字节帧 → 换算 */
    g_write_count = 0;
    g_read_count = 0;
    g_delay_ms = 0;
    check(xy_sht30_read(&sht) == XY_DEVICE_OK, "read() succeeded");
    check(g_write_count == 1U, "read sent one command");
    check(g_writes[0].data[0] == 0x2CU && g_writes[0].data[1] == 0x06U,
          "read sent the 0x2C06 high-repeatability measurement");
    check(g_delay_ms == 15U, "read waited the 15 ms conversion time");
    check(g_read_count == 1U, "read fetched exactly one 6-byte frame");
    check(sht.temperature == 2500, "temperature raw 0x6666 converts to 25.00 °C");
    check(sht.humidity == 5000U, "humidity raw 0x8000 converts to 50.00 %RH");

    /* 4. CRC 错帧被拒，缓存不被污染 */
    g_read_frame[2] ^= 0xFFU;
    check(xy_sht30_read(&sht) == XY_DEVICE_IO_ERROR, "a bad CRC is refused");
    check(sht.temperature == 2500, "refused frame left the temperature cache alone");
    check(sht.humidity == 5000U, "refused frame left the humidity cache alone");
    load_valid_frame();

    /* 5. 传输失败按原样向上传播，且失败命令之后不再读 */
    g_write_status = XY_HAL_ERROR_TIMEOUT;
    g_read_count = 0;
    check(xy_sht30_read(&sht) == XY_DEVICE_IO_ERROR, "a failed command is reported");
    check(g_read_count == 0U, "a failed command does not go on to read");
    g_write_status = XY_HAL_OK;

    g_read_status = XY_HAL_ERROR_TIMEOUT;
    check(xy_sht30_read(&sht) == XY_DEVICE_IO_ERROR, "a failed frame read is reported");
    check(sht.temperature == 2500, "failed frame read left the cache alone");
    g_read_status = XY_HAL_OK;

    /* 6. 便捷 getter 自带一次采样：失败返回哨兵，而不是旧值 */
    check(xy_sht30_read_temperature(&sht) == 2500,
          "read_temperature() re-samples and reports 25.00 °C");
    g_read_status = XY_HAL_ERROR_TIMEOUT;
    check(xy_sht30_read_temperature(&sht) == INT16_MIN,
          "read_temperature() reports INT16_MIN when the sample fails");
    check(xy_sht30_read_humidity(&sht) == UINT16_MAX,
          "read_humidity() reports UINT16_MAX when the sample fails");
    g_read_status = XY_HAL_OK;

    /* 7. 反初始化之后不再有任何总线流量 */
    check(xy_sht30_deinit(&sht) == XY_DEVICE_OK, "deinit() succeeded");
    g_write_count = 0;
    check(xy_sht30_read(&sht) == XY_DEVICE_INVALID_PARAM, "read after deinit is refused");
    check(g_write_count == 0U, "read after deinit produced no I2C traffic");
    check(xy_sht30_deinit(&sht) == XY_DEVICE_INVALID_PARAM, "a second deinit is refused");

    printf("sensor_demo: %d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
