/**
 * SPDX-License-Identifier: MIT
 * @file    xy_gps.h
 * @brief   GPS Sensor Driver Interface for XinYi Framework
 * @version 1.0.0
 *
 * 位置传感器 - GPS/GNSS 模块
 * 适用于 NMEA 协议输出的 GPS 模块（UBLOX、AT6558、LC86L 等）
 */

#ifndef __XY_GPS_H__
#define __XY_GPS_H__

#include "xy_device_error.h"

#include <stdint.h>
#include <stdbool.h>

/* ==================== 常量定义 ==================== */
#define GPS_NAME_MAX_LEN       32
#define GPS_SATELLITE_MAX      64
#define GPS_NMEA_BUF_SIZE      256
#define GPSgga_MAX_FIELDS     15

/* ==================== 定位质量 ==================== */
typedef enum {
    GPS_QUALITY_INVALID      = 0,    ///< 无效定位
    GPS_QUALITY_GPS_SPS      = 1,    ///< GPS 单点定位
    GPS_QUALITY_DGPS         = 2,    ///< 差分 GPS
    GPS_QUALITY_PPS          = 3,    ///< 精密定位服务
    GPS_QUALITY_RTK_FIXED    = 4,    ///< RTK 固定解
    GPS_QUALITY_RTK_FLOAT    = 5,   ///< RTK 浮点解
    GPS_QUALITY_ESTIMATED    = 6,   ///< 估算模式
    GPS_QUALITY_MANUAL       = 7,   ///< 手动输入
    GPS_QUALITY_SIMULATION   = 8,   ///< 模拟模式
} gps_quality_t;

/* ==================== 定位模式 ==================== */
typedef enum {
    GPS_MODE_NONE            = 0,    ///< 无定位
    GPS_MODE_2D              = 1,    ///< 2D 定位
    GPS_MODE_3D              = 2,    ///< 3D 定位
    GPS_MODE_DGPS            = 3,    ///< 差分定位
    GPS_MODE_RTK             = 4,   ///< RTK 定位
} gps_mode_t;

/* ==================== 设备状态 ==================== */
typedef enum {
    GPS_STATUS_IDLE          = 0,    ///< 未注册 / 待机
    GPS_STATUS_READY         = 1,    ///< 已注册 / 就绪
    GPS_STATUS_ERROR         = 2,    ///< 错误
} gps_status_t;

/* ==================== NMEA 语句类型 ==================== */
typedef enum {
    GPS_NMEA_GGA             = 0x01,  ///< 定位数据
    GPS_NMEA_RMC             = 0x02,  ///< 推荐最小定位
    GPS_NMEA_GSV             = 0x04,  ///< 可见卫星
    GPS_NMEA_GLL             = 0x08,  ///< 地理坐标
    GPS_NMEA_GSA             = 0x10,  ///< 卫星状态
    GPS_NMEA_VTG             = 0x20,  ///< 地面速度
    GPS_NMEA_ZDA            = 0x40,  ///< 时间日期
} gps_nmea_type_t;

/* ==================== GPS 数据结构 ==================== */
typedef struct {
    /* 位置信息 */
    double latitude;                  ///< 纬度 (度，-90~90，北正南负)
    double longitude;                 ///< 经度 (度，-180~180，东正西负)
    float altitude;                   ///< 海拔高度 (米)
    float geoid_height;               ///< 大地水准面高度 (米)

    /* 运动信息 */
    float speed;                      ///< 速度 (米/秒)
    float course;                     ///< 航向角 (度，0~360，从北顺时针)
    float magnetic_variation;         ///< 磁偏角 (度)

    /* 时间信息 */
    uint32_t hour;                    ///< 小时 (0-23)
    uint32_t minute;                  ///< 分钟 (0-59)
    uint32_t second;                  ///< 秒 (0-59)
    uint32_t millisecond;             ///< 毫秒 (0-999)
    uint32_t day;                     ///< 日 (1-31)
    uint32_t month;                   ///< 月 (1-12)
    uint32_t year;                    ///< 年 (4位数)
    uint32_t timestamp;               ///< Unix 时间戳 (秒)

    /* 定位状态 */
    gps_quality_t quality;            ///< 定位质量
    gps_mode_t mode;                  ///< 定位模式
    uint8_t satellites_used;          ///< 使用的卫星数 (0-64)
    float hdop;                       ///< 水平精度因子
    float pdop;                       ///< 位置精度因子
    float vdop;                       ///< 垂直精度因子

    /* 状态标志 */
    bool data_valid;                  ///< 数据是否有效
    uint8_t status_flags;             ///< 状态标志位
} gps_data_t;

/* ==================== GPS 配置 ==================== */
typedef struct {
    /* 串口配置 */
    uint32_t baudrate;                ///< 波特率 (默认 9600)
    uint8_t data_bits;                ///< 数据位 (默认 8)
    uint8_t stop_bits;                ///< 停止位 (默认 1)
    char parity;                      ///< 校验位 ('N', 'O', 'E')

    /* GPS 配置 */
    uint32_t update_rate;             ///< 更新频率 (Hz，默认 1)
    gps_nmea_type_t nmea_output;     ///< 输出 NMEA 语句类型
    bool checksum_check;              ///< 是否校验 NMEA 校验和
    bool datum_set;                   ///< 是否设置坐标系
} gps_config_t;

/* ==================== GPS 设备扩展 ==================== */
typedef struct gps_device {
    char name[GPS_NAME_MAX_LEN];
    gps_status_t status;

    /* GPS 特定配置 */
    gps_config_t config;

    /* GPS 数据缓存 */
    gps_data_t data;
    gps_data_t last_valid_data;       ///< 上次有效数据

    /* NMEA 解析状态 */
    char nmea_buf[GPS_NMEA_BUF_SIZE];
    uint16_t nmea_buf_idx;
    uint32_t parse_errors;
    uint32_t frame_count;

    /* 卫星信息 */
    struct {
        uint8_t id;
        uint8_t elevation;            ///< 仰角 (度)
        uint8_t azimuth;              ///< 方位角 (度)
        uint8_t snr;                  ///< 信噪比 (dB-Hz)
    } satellites[GPS_SATELLITE_MAX];
    uint8_t satellite_count;

    /* 时间戳 */
    uint32_t last_update_time;
    uint32_t last_valid_time;
} gps_device_t;

/* ==================== 标准 NMEA 解析宏 ==================== */
#define GPS_NMEA_HEAD(_str, _talker, _type) \
    ((_str)[0] == '$' && \
     (_str)[1] == (_talker)[0] && \
     (_str)[2] == (_talker)[1] && \
     (_str)[3] == (_type)[0] && \
     (_str)[4] == (_type)[1])

#define GPS_NMEA_CHECKSUM(_str) \
    ({                                                         \
        const char *_s = (_str);                               \
        uint8_t _cs = 0;                                       \
        while (*_s && *_s != '*') _cs ^= (uint8_t)*_s++;       \
        _cs;                                                   \
    })

/* ==================== GPS 类型定义简化 ==================== */

/* ==================== 状态标志位 ==================== */
#define GPS_FLAG_FIXED         (1 << 0)   ///< 已定位
#define GPS_FLAG_DGPS          (1 << 1)   ///< 差分定位
#define GPS_FLAG_RTK           (1 << 2)   ///< RTK 定位
#define GPS_FLAG_SBAS          (1 << 3)   ///< SBAS 增强
#define GPS_FLAG_COLD_START    (1 << 4)   ///< 冷启动
#define GPS_FLAG_ANTENNA_SHORT (1 << 5)   ///< 天线短路
#define GPS_FLAG_ANTENNA_OPEN  (1 << 6)   ///< 天线开路

/* ==================== 常用 GPS 模块支持 ==================== */
/**
 * @brief 支持的 GPS 模块类型
 */
typedef enum {
    GPS_CHIP_UNKNOWN = 0,
    GPS_CHIP_UBLOX,          ///< U-Blox (NEO-6M, NEO-7M, NEO-M8N, etc.)
    GPS_CHIP_AT6558,         ///< AT6558 (华大北斗)
    GPS_CHIP_LC86L,          ///< LC86L (中科微)
    GPS_CHIP_MTK3339,        ///< MediaTek MTK3339
    GPS_CHIP_SIRF,           ///< SiRF Star III/IV
    GPS_CHIP_Quectel_L26,    ///< Quectel L26
    GPS_CHIP_SIMCOM_SIM68,   ///< SIMCom SIM68
} gps_chip_type_t;

/* ==================== GPS 工具函数 ==================== */

/**
 * @brief   将 NMEA 纬度格式转换为度
 * @param   dmm: 度分格式 (DDMM.MMMM)
 * @param   dir: 方向 ('N'/'S')
 * @return  度数
 */
static inline double gps_nmea_lat_to_deg(double dmm, char dir)
{
    double deg = (double)((uint32_t)(dmm / 100));
    double min = dmm - deg * 100;
    double result = deg + min / 60.0;
    return (dir == 'S' || dir == 's') ? -result : result;
}

/**
 * @brief   将 NMEA 经度格式转换为度
 * @param   dmm: 度分格式 (DDDMM.MMMM)
 * @param   dir: 方向 ('E'/'W')
 * @return  度数
 */
static inline double gps_nmea_lon_to_deg(double dmm, char dir)
{
    double deg = (double)((uint32_t)(dmm / 100));
    double min = dmm - deg * 100;
    double result = deg + min / 60.0;
    return (dir == 'W' || dir == 'w') ? -result : result;
}

/**
 * @brief   计算两点间的距离 (Haversine)
 * @param   lat1, lon1: 起点坐标
 * @param   lat2, lon2: 终点坐标
 * @return  距离 (米)
 */
float gps_distance(double lat1, double lon1, double lat2, double lon2);

/**
 * @brief   计算方位角
 * @param   lat1, lon1: 起点坐标
 * @param   lat2, lon2: 终点坐标
 * @return  方位角 (度，0-360)
 */
float gps_bearing(double lat1, double lon1, double lat2, double lon2);

/**
 * @brief   检查坐标是否有效
 * @param   lat: 纬度
 * @param   lon: 经度
 * @return  true=有效
 */
static inline bool gps_position_valid(double lat, double lon)
{
    return (lat >= -90.0 && lat <= 90.0 && lon >= -180.0 && lon <= 180.0);
}

/* ==================== GPS 驱动注册 ==================== */

/**
 * @brief   注册 GPS 设备
 * @param   dev: GPS 设备指针
 * @return  传感器错误码
 */
xy_error_t gps_register(gps_device_t *dev);

/**
 * @brief   注销 GPS 设备
 * @param   dev: GPS 设备指针
 * @return  传感器错误码
 */
xy_error_t gps_unregister(gps_device_t *dev);

/**
 * @brief   查找 GPS 设备
 * @param   name: 设备名
 * @return  GPS 设备指针
 */
gps_device_t *gps_find(const char *name);

/**
 * @brief   获取 GPS 设备数量
 * @return  已注册设备数
 */
uint8_t gps_get_count(void);

/* ==================== NMEA 数据输入 ==================== */

/**
 * @brief   解析一条完整 NMEA 语句
 * @param   dev:       GPS 设备指针
 * @param   nmea_str:  以 '\0' 结尾的语句（是否带 CRLF 均可）
 * @param   len:       语句长度（不含结尾 '\0'）
 * @return  XY_DEVICE_OK=已解析；XY_DEVICE_ERROR=校验和拒绝；XY_DEVICE_INVALID_PARAM=参数无效
 * @note    `dev->config.checksum_check` 为 true 时，校验和不符的语句被拒绝并
 *          计入 `dev->parse_errors`，缓存数据保持不变。无法识别的语句类型
 *          返回 XY_DEVICE_OK 且不修改缓存。
 */
xy_error_t gps_parse_nmea(gps_device_t *dev, const char *nmea_str, uint16_t len);

/**
 * @brief   逐字节送入 NMEA 数据流（用于 UART 中断/轮询）
 * @param   dev:  GPS 设备指针
 * @param   byte: 收到的字节
 * @return  XY_DEVICE_OK / XY_DEVICE_INVALID_PARAM
 * @note    以 '$' 复位帧缓冲，以 '\n' 结束一帧并解析；每收满一帧
 *          `dev->frame_count` 加一。
 */
xy_error_t gps_parse_byte(gps_device_t *dev, uint8_t byte);

/* ==================== 数据读取 ==================== */

/**
 * @brief   拷贝缓存的定位数据
 * @param   dev:  GPS 设备指针
 * @param   data: 输出缓冲（失败时不修改）
 * @return  XY_DEVICE_OK / XY_DEVICE_INVALID_PARAM
 */
xy_error_t gps_read_data(gps_device_t *dev, gps_data_t *data);

/**
 * @brief   是否已定位
 * @return  true=`data_valid` 且 `quality > GPS_QUALITY_INVALID`
 * @note    **两半来自不同语句**：`data_valid` 由 RMC 的状态字段（A/V）或 GGA 的
 *          质量字段设置，而 `quality` 只有 GGA 会写。因此只收到 RMC 时
 *          `data_valid == true` 但 `gps_is_fixed() == false`——要判断"能给出坐标"，
 *          必须以 GGA 为准（或同时判 `data_valid`）。CTest `sensor_gps_example`
 *          把这条不对称钉成断言。
 */
bool gps_is_fixed(gps_device_t *dev);

/**
 * @brief   读取位置
 * @param   lat/lon/alt: 输出指针（任一可为 NULL 表示不关心）
 * @return  XY_DEVICE_OK / XY_DEVICE_ERROR=尚未定位 / XY_DEVICE_INVALID_PARAM
 */
xy_error_t gps_get_position(gps_device_t *dev, double *lat, double *lon, float *alt);

/**
 * @brief   读取时间戳
 * @param   timestamp: 输出 Unix 时间戳（秒）
 * @return  XY_DEVICE_OK / XY_DEVICE_INVALID_PARAM
 * @note    当前实现按每月 30 天、每年 365 天近似换算，跨月/跨年的绝对误差
 *          未消除；用于同一会话内的时间先后判断是安全的，不可用作日历真值。
 */
xy_error_t gps_get_time(gps_device_t *dev, uint32_t *timestamp);

/**
 * @brief   读取卫星数量
 * @param   count:             可见卫星数（GSV 累计，任一输出可为 NULL）
 * @param   satellites_used:   参与定位的卫星数（GGA）
 * @param   satellites_visible: 同 count
 */
xy_error_t gps_get_satellites(gps_device_t *dev, uint8_t *count,
                                uint8_t *satellites_used, uint8_t *satellites_visible);

/**
 * @brief   读取精度因子
 * @param   hdop/pdop/vdop: 输出指针（任一可为 NULL）
 */
xy_error_t gps_get_dop(gps_device_t *dev, float *hdop, float *pdop, float *vdop);

/**
 * @brief   距最近一次数据更新的毫秒数
 * @return  当前实现返回 **OS tick**（`xy_os_tick_get()`）；`dev` 为 NULL 时为 0
 * @note    实现是 `xy_hal_sys_get_tick_count() - dev->last_update_time`，而 `last_update_time`
 *          目前**没有任何写入方**，减法项恒为 0，所以现在等价于"上电至今的毫秒数"。
 *          要让它变成"数据老化"，必须先有地方写 `last_update_time`——那会改变本
 *          函数的语义，属修改公开契约，需先决定。CTest `sensor_gps_example` 把当前
 *          语义（= OS tick）钉成断言，因此这个改动不会悄悄发生。
 */
uint32_t gps_get_uptime(gps_device_t *dev);

/* ==================== 控制与功耗 ==================== */

/**
 * @brief   进入待机（仅提交 `status = GPS_STATUS_IDLE`）
 * @return  XY_DEVICE_OK / XY_DEVICE_INVALID_PARAM
 * @note    模块侧命令尚未实现（依赖具体 UART 驱动）。
 */
xy_error_t gps_sleep(gps_device_t *dev);

/**
 * @brief   唤醒（仅提交 `status = GPS_STATUS_READY`）
 * @return  XY_DEVICE_OK / XY_DEVICE_INVALID_PARAM
 */
xy_error_t gps_wakeup(gps_device_t *dev);

/**
 * @brief   向模块发送命令
 * @note    占位实现：当前不做任何 I/O 也不需要 dev，永远返回 XY_DEVICE_OK。
 */
xy_error_t gps_send_command(gps_device_t *dev, const char *cmd, uint16_t len);

/**
 * @brief   设置波特率（只更新 `dev->config.baudrate`，不产生总线流量）
 */
xy_error_t gps_set_baudrate(gps_device_t *dev, uint32_t baudrate);

/**
 * @brief   设置更新率
 * @param   rate_hz: 1..10
 * @return  XY_DEVICE_OK / XY_DEVICE_INVALID_PARAM（0 或 >10 被拒绝，配置保持不变）
 */
xy_error_t gps_set_update_rate(gps_device_t *dev, uint32_t rate_hz);

/* ==================== 示例: 注册并使用 GPS 设备 ==================== */
/*
 * 完整可编译、可运行的样张见 `components/sensor/examples/gps_example.c`
 * （由 CTest `sensor_gps_example` 编译并执行）。
 *
 * gps_device_t my_gps;
 * memset(&my_gps, 0, sizeof(my_gps));
 * strncpy(my_gps.name, "GPS_AT6558", GPS_NAME_MAX_LEN - 1U);
 * my_gps.config.baudrate = 9600U;
 * my_gps.config.checksum_check = true;
 *
 * gps_register(&my_gps);
 *
 * // 在 UART 中断或轮询中:
 * gps_parse_byte(&my_gps, byte);
 *
 * // 读取数据:
 * gps_data_t data;
 * if (gps_read_data(&my_gps, &data) == XY_DEVICE_OK && data.data_valid) {
 *     printf("Lat: %.6f, Lon: %.6f, Alt: %.1f\n",
 *            data.latitude, data.longitude, data.altitude);
 * }
 *
 * // 注意: `name` 是定长数组，不能用 `dev->name = "..."` 赋值。
 */

#endif /* __XY_GPS_H__ */