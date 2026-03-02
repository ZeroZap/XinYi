/**
 * @file xy_json.c
 * @brief Lightweight JSON Parser Implementation
 * @version 1.0.0
 * @date 2026-03-01 自主任务
 */

#include "xy_json.h"
#include "xy_log.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

static void json_skip_whitespace(json_parser_t *p)
{
    while (p->pos < p->len && isspace(p->json[p->pos])) {
        p->pos++;
    }
}

static int json_parse_value(json_parser_t *p, json_value_t *value);

static int json_parse_string(json_parser_t *p, json_value_t *value)
{
    if (p->json[p->pos] != '"') {
        return JSON_ERROR_INVALID;
    }
    p->pos++;
    
    uint16_t start = p->pos;
    while (p->pos < p->len && p->json[p->pos] != '"') {
        if (p->json[p->pos] == '\\') {
            p->pos++;  /* 跳过转义字符 */
        }
        p->pos++;
    }
    
    if (p->pos >= p->len) {
        return JSON_ERROR_INVALID;
    }
    
    value->type = JSON_TYPE_STRING;
    value->data.string.str = &p->json[start];
    value->data.string.len = p->pos - start;
    p->pos++;  /* 跳过结束引号 */
    
    return JSON_OK;
}

static int json_parse_number(json_parser_t *p, json_value_t *value)
{
    uint16_t start = p->pos;
    bool is_float = false;
    
    if (p->json[p->pos] == '-') {
        p->pos++;
    }
    
    while (p->pos < p->len && (isdigit(p->json[p->pos]) || 
           p->json[p->pos] == '.' || p->json[p->pos] == 'e' || 
           p->json[p->pos] == 'E' || p->json[p->pos] == '+' ||
           p->json[p->pos] == '-')) {
        if (p->json[p->pos] == '.' || p->json[p->pos] == 'e' || p->json[p->pos] == 'E') {
            is_float = true;
        }
        p->pos++;
    }
    
    if (is_float) {
        value->type = JSON_TYPE_NUMBER;
        char buf[32];
        uint16_t len = p->pos - start;
        if (len >= sizeof(buf)) len = sizeof(buf) - 1;
        memcpy(buf, &p->json[start], len);
        buf[len] = '\0';
        value->data.number = atof(buf);
    } else {
        value->type = JSON_TYPE_NUMBER;
        char buf[32];
        uint16_t len = p->pos - start;
        if (len >= sizeof(buf)) len = sizeof(buf) - 1;
        memcpy(buf, &p->json[start], len);
        buf[len] = '\0';
        value->data.integer = atoll(buf);
    }
    
    return JSON_OK;
}

static int json_parse_object(json_parser_t *p, json_value_t *value)
{
    if (p->json[p->pos] != '{') {
        return JSON_ERROR_INVALID;
    }
    p->pos++;
    
    value->type = JSON_TYPE_OBJECT;
    value->data.object.keys = NULL;
    value->data.object.values = NULL;
    value->data.object.count = 0;
    
    json_skip_whitespace(p);
    if (p->json[p->pos] == '}') {
        p->pos++;
        return JSON_OK;
    }
    
    /* 简化实现：仅解析不存储 */
    /* TODO: 完整实现对象解析 */
    
    while (p->pos < p->len && p->json[p->pos] != '}') {
        json_value_t key, val;
        
        json_parse_string(p, &key);
        json_skip_whitespace(p);
        
        if (p->json[p->pos] != ':') {
            return JSON_ERROR_INVALID;
        }
        p->pos++;
        
        json_parse_value(p, &val);
        json_skip_whitespace(p);
        
        if (p->json[p->pos] == ',') {
            p->pos++;
        }
    }
    
    if (p->pos >= p->len) {
        return JSON_ERROR_INVALID;
    }
    
    p->pos++;  /* 跳过 } */
    return JSON_OK;
}

static int json_parse_array(json_parser_t *p, json_value_t *value)
{
    if (p->json[p->pos] != '[') {
        return JSON_ERROR_INVALID;
    }
    p->pos++;
    
    value->type = JSON_TYPE_ARRAY;
    value->data.array.count = 0;
    
    json_skip_whitespace(p);
    if (p->json[p->pos] == ']') {
        p->pos++;
        return JSON_OK;
    }
    
    while (p->pos < p->len && p->json[p->pos] != ']') {
        json_value_t elem;
        json_parse_value(p, &elem);
        value->data.array.count++;
        
        json_skip_whitespace(p);
        if (p->json[p->pos] == ',') {
            p->pos++;
        }
    }
    
    if (p->pos >= p->len) {
        return JSON_ERROR_INVALID;
    }
    
    p->pos++;  /* 跳过 ] */
    return JSON_OK;
}

static int json_parse_value(json_parser_t *p, json_value_t *value)
{
    json_skip_whitespace(p);
    
    if (p->pos >= p->len) {
        return JSON_ERROR_INVALID;
    }
    
    char c = p->json[p->pos];
    
    if (c == '"') {
        return json_parse_string(p, value);
    } else if (c == '{') {
        return json_parse_object(p, value);
    } else if (c == '[') {
        return json_parse_array(p, value);
    } else if (c == 't') {  /* true */
        value->type = JSON_TYPE_BOOL;
        value->data.boolean = true;
        p->pos += 4;
        return JSON_OK;
    } else if (c == 'f') {  /* false */
        value->type = JSON_TYPE_BOOL;
        value->data.boolean = false;
        p->pos += 5;
        return JSON_OK;
    } else if (c == 'n') {  /* null */
        value->type = JSON_TYPE_NULL;
        p->pos += 4;
        return JSON_OK;
    } else if (c == '-' || isdigit(c)) {
        return json_parse_number(p, value);
    }
    
    return JSON_ERROR_INVALID;
}

int json_parse(json_parser_t *parser, const char *json, uint16_t len)
{
    if (!parser || !json || len == 0) {
        return JSON_ERROR_INVALID;
    }
    
    memset(parser, 0, sizeof(*parser));
    parser->json = json;
    parser->len = len;
    parser->pos = 0;
    
    parser->root = malloc(sizeof(json_value_t));
    if (!parser->root) {
        return JSON_ERROR_OUT_OF_MEM;
    }
    
    int ret = json_parse_value(parser, parser->root);
    if (ret != JSON_OK) {
        snprintf(parser->error_msg, sizeof(parser->error_msg), 
                 "Parse error at position %d", parser->pos);
        free(parser->root);
        parser->root = NULL;
    }
    
    return ret;
}

void json_free(json_value_t *value)
{
    if (!value) return;
    
    if (value->type == JSON_TYPE_ARRAY) {
        /* TODO: 释放数组元素 */
    } else if (value->type == JSON_TYPE_OBJECT) {
        /* TODO: 释放对象成员 */
    }
    
    free(value);
}

json_value_t* json_object_get(json_value_t *obj, const char *key)
{
    if (!obj || obj->type != JSON_TYPE_OBJECT || !key) {
        return NULL;
    }
    /* TODO: 实现对象查找 */
    return NULL;
}

json_value_t* json_array_get(json_value_t *arr, uint16_t index)
{
    if (!arr || arr->type != JSON_TYPE_ARRAY) {
        return NULL;
    }
    if (index >= arr->data.array.count) {
        return NULL;
    }
    /* TODO: 实现数组索引 */
    return NULL;
}

int json_get_int(json_value_t *value, int64_t *out)
{
    if (!value || !out || value->type != JSON_TYPE_NUMBER) {
        return JSON_ERROR_INVALID;
    }
    *out = value->data.integer;
    return JSON_OK;
}

int json_get_number(json_value_t *value, double *out)
{
    if (!value || !out || value->type != JSON_TYPE_NUMBER) {
        return JSON_ERROR_INVALID;
    }
    *out = value->data.number;
    return JSON_OK;
}

int json_get_string(json_value_t *value, const char **out, uint16_t *len)
{
    if (!value || !out || value->type != JSON_TYPE_STRING) {
        return JSON_ERROR_INVALID;
    }
    *out = value->data.string.str;
    if (len) {
        *len = value->data.string.len;
    }
    return JSON_OK;
}

int json_get_bool(json_value_t *value, bool *out)
{
    if (!value || !out || value->type != JSON_TYPE_BOOL) {
        return JSON_ERROR_INVALID;
    }
    *out = value->data.boolean;
    return JSON_OK;
}
