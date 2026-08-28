#include "unity.h"

#include <string.h>

#include "xy_json.h"

void xy_log_char(char ch)
{
    (void)ch;
}

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_parse_reads_nested_values(void)
{
    xy_json_t *root = xy_json_parse(
        "{\"name\":\"XinYi\",\"enabled\":true,\"rate\":12.5,\"items\":[1,null]}");

    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_EQUAL(XY_JSON_OBJECT, root->type);
    TEST_ASSERT_EQUAL_STRING("XinYi", xy_json_get_string(root, "name", "missing"));
    TEST_ASSERT_TRUE(xy_json_get_bool(root, "enabled", false));
    TEST_ASSERT_TRUE(xy_json_get_number(root, "rate", 0.0) == 12.5);

    xy_json_t *items = xy_json_object_get(root, "items");
    TEST_ASSERT_NOT_NULL(items);
    TEST_ASSERT_EQUAL_UINT16(2U, xy_json_array_size(items));
    TEST_ASSERT_EQUAL(XY_JSON_NULL, xy_json_array_get(items, 1U)->type);

    xy_json_free(root);
}

static void test_parse_rejects_trailing_or_incomplete_input(void)
{
    TEST_ASSERT_NULL(xy_json_parse("{\"a\":1} trailing"));
    TEST_ASSERT_NULL(xy_json_parse("\"unterminated"));
    TEST_ASSERT_NULL(xy_json_parse("[1,]"));
    TEST_ASSERT_NULL(xy_json_parse("{\"a\":1"));
    TEST_ASSERT_NULL(xy_json_parse("1x"));
}

static void test_parse_rejects_invalid_number_tokens(void)
{
    TEST_ASSERT_NULL(xy_json_parse("-"));
    TEST_ASSERT_NULL(xy_json_parse("01"));
    TEST_ASSERT_NULL(xy_json_parse("1."));
    TEST_ASSERT_NULL(xy_json_parse("1e"));
    TEST_ASSERT_NULL(xy_json_parse("1e+"));
}

static void test_object_and_array_mutations_preserve_contract(void)
{
    xy_json_t *obj = xy_json_new_object();
    xy_json_t *arr = xy_json_new_array();

    TEST_ASSERT_NOT_NULL(obj);
    TEST_ASSERT_NOT_NULL(arr);
    TEST_ASSERT_EQUAL(XY_JSON_OK, xy_json_object_set(obj, "value", xy_json_new_number(1.0)));
    TEST_ASSERT_EQUAL(XY_JSON_OK, xy_json_object_set(obj, "value", xy_json_new_number(2.0)));
    TEST_ASSERT_EQUAL_UINT16(1U, obj->value.object.count);
    TEST_ASSERT_TRUE(xy_json_get_number(obj, "value", 0.0) == 2.0);

    TEST_ASSERT_EQUAL(XY_JSON_OK, xy_json_array_append(arr, xy_json_new_number(1.0)));
    TEST_ASSERT_EQUAL(XY_JSON_OK, xy_json_array_insert(arr, 0U, xy_json_new_number(0.0)));
    TEST_ASSERT_EQUAL_UINT16(2U, xy_json_array_size(arr));
    TEST_ASSERT_EQUAL(XY_JSON_OK, xy_json_array_remove(arr, 1U));
    TEST_ASSERT_EQUAL_UINT16(1U, xy_json_array_size(arr));
    TEST_ASSERT_EQUAL(XY_JSON_ERROR_INVALID_PARAM, xy_json_array_remove(arr, 1U));

    xy_json_free(obj);
    xy_json_free(arr);
}

static void test_public_guards_return_safe_defaults(void)
{
    TEST_ASSERT_NULL(xy_json_parse(NULL));
    TEST_ASSERT_NULL(xy_json_object_get(NULL, "key"));
    TEST_ASSERT_EQUAL(XY_JSON_ERROR_INVALID_PARAM, xy_json_object_set(NULL, "key", NULL));
    TEST_ASSERT_EQUAL(XY_JSON_ERROR_INVALID_PARAM, xy_json_array_append(NULL, NULL));
    TEST_ASSERT_EQUAL_UINT16(0U, xy_json_array_size(NULL));
    TEST_ASSERT_EQUAL_STRING("fallback", xy_json_get_string(NULL, NULL, "fallback"));
    TEST_ASSERT_TRUE(xy_json_get_number(NULL, NULL, 3.0) == 3.0);
    TEST_ASSERT_TRUE(xy_json_get_bool(NULL, NULL, true));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_parse_reads_nested_values);
    RUN_TEST(test_parse_rejects_trailing_or_incomplete_input);
    RUN_TEST(test_parse_rejects_invalid_number_tokens);
    RUN_TEST(test_object_and_array_mutations_preserve_contract);
    RUN_TEST(test_public_guards_return_safe_defaults);
    return UNITY_END();
}
