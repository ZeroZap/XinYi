/**
 * @file xy_json.h
 * @brief Lightweight JSON Parser
 * @version 1.0.0
 * @date 2026-03-01 自主任务
 */

#ifndef NANO_JSON_H
#define NANO_JSON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief JSON 值类型
 */
typedef enum {
    JSON_TYPE_NULL = 0,
    JSON_TYPE_BOOL,
    JSON_TYPE_NUMBER,
    JSON_TYPE_STRING,
    JSON_TYPE_ARRAY,
    JSON_TYPE_OBJECT,
} json_type_t;

/**
 * @brief JSON 错误码
 */
#define JSON_OK                 0
#define JSON_ERROR              (-1)
#define JSON_ERROR_INVALID      (-2)
#define JSON_ERROR_NOT_FOUND    (-3)
#define JSON_ERROR_OUT_OF_MEM   (-4)

/**
 * @brief JSON 值
 */
typedef struct json_value {
    json_type_t type;
    union {
        bool boolean;
        int64_t integer;
        double number;
        struct {
            const char *str;
            uint16_t len;
        } string;
        struct {
            struct json_value *values;
            uint16_t count;
        } array;
        struct {
            const char **keys;
            struct json_value *values;
            uint16_t count;
        } object;
    } data;
} json_value_t;

/**
 * @brief JSON 解析器
 */
typedef struct {
    const char *json;
    uint16_t len;
    uint16_t pos;
    json_value_t *root;
    char error_msg[64];
} json_parser_t;

/**
 * @brief 解析 JSON 字符串
 */
int json_parse(json_parser_t *parser, const char *json, uint16_t len);

/**
 * @brief 释放 JSON 值
 */
void json_free(json_value_t *value);

/**
 * @brief 获取对象中的值
 */
json_value_t* json_object_get(json_value_t *obj, const char *key);

/**
 * @brief 获取数组中的值
 */
json_value_t* json_array_get(json_value_t *arr, uint16_t index);

/**
 * @brief 获取整数值
 */
int json_get_int(json_value_t *value, int64_t *out);

/**
 * @brief 获取浮点数值
 */
int json_get_number(json_value_t *value, double *out);

/**
 * @brief 获取字符串值
 */
int json_get_string(json_value_t *value, const char **out, uint16_t *len);

/**
 * @brief 获取布尔值
 */
int json_get_bool(json_value_t *value, bool *out);

#ifdef __cplusplus
}
#endif

#endif
