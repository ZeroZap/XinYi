#include "unity.h"

#include <string.h>

#include "core_json.h"

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_validate_rejects_bad_parameters(void)
{
    TEST_ASSERT_EQUAL(JSONNullParameter, JSON_Validate(NULL, 1U));
    TEST_ASSERT_EQUAL(JSONBadParameter, JSON_Validate("{}", 0U));
}

static void test_validate_accepts_objects_arrays_and_scalars(void)
{
    TEST_ASSERT_EQUAL(JSONSuccess,
                      JSON_Validate(" { \"foo\": [1, true, null], \"bar\": \"baz\" } ",
                                    strlen(" { \"foo\": [1, true, null], \"bar\": \"baz\" } ")));
    TEST_ASSERT_EQUAL(JSONSuccess, JSON_Validate("[1,2,3]", strlen("[1,2,3]")));
    TEST_ASSERT_EQUAL(JSONSuccess, JSON_Validate("true", strlen("true")));
    TEST_ASSERT_EQUAL(JSONSuccess, JSON_Validate("123.45", strlen("123.45")));
}

static void test_validate_rejects_malformed_or_trailing_data(void)
{
    TEST_ASSERT_EQUAL(JSONPartial, JSON_Validate("{\"a\":1", strlen("{\"a\":1")));
    TEST_ASSERT_EQUAL(JSONIllegalDocument, JSON_Validate("{\"a\":1} extra", strlen("{\"a\":1} extra")));
}

static void test_search_finds_nested_values_and_types(void)
{
    char json[] = "{\"foo\":\"abc\",\"bar\":{\"answer\":42,\"ok\":true},\"list\":[10,{\"name\":\"xy\"}]}";
    char *value = NULL;
    size_t value_len = 0U;
    JSONTypes_t value_type = JSONInvalid;

    TEST_ASSERT_EQUAL(JSONSuccess,
                      JSON_SearchT(json, strlen(json), "bar.answer", strlen("bar.answer"), &value,
                                   &value_len, &value_type));
    TEST_ASSERT_NOT_NULL(value);
    TEST_ASSERT_EQUAL(JSONNumber, value_type);
    TEST_ASSERT_EQUAL_UINT(2U, value_len);
    TEST_ASSERT_EQUAL_MEMORY("42", value, value_len);

    TEST_ASSERT_EQUAL(JSONSuccess,
                      JSON_SearchT(json, strlen(json), "list[1].name", strlen("list[1].name"),
                                   &value, &value_len, &value_type));
    TEST_ASSERT_EQUAL(JSONString, value_type);
    TEST_ASSERT_EQUAL_UINT(2U, value_len);
    TEST_ASSERT_EQUAL_MEMORY("xy", value, value_len);
}

static void test_search_reports_not_found_and_bad_parameters(void)
{
    char json[] = "{\"foo\":\"abc\"}";
    char *value = NULL;
    size_t value_len = 0U;

    TEST_ASSERT_EQUAL(JSONNotFound,
                      JSON_Search(json, strlen(json), "missing", strlen("missing"), &value,
                                  &value_len));
    TEST_ASSERT_EQUAL(JSONNullParameter,
                      JSON_Search(NULL, strlen(json), "foo", strlen("foo"), &value, &value_len));
    TEST_ASSERT_EQUAL(JSONBadParameter,
                      JSON_Search(json, strlen(json), "", 0U, &value, &value_len));
}

static void test_iterate_walks_object_pairs(void)
{
    const char json[] = "{\"a\":1,\"b\":false}";
    size_t start = 0U;
    size_t next = 0U;
    JSONPair_t pair;

    memset(&pair, 0, sizeof(pair));
    TEST_ASSERT_EQUAL(JSONSuccess, JSON_Iterate(json, strlen(json), &start, &next, &pair));
    TEST_ASSERT_EQUAL_UINT(1U, pair.keyLength);
    TEST_ASSERT_EQUAL_MEMORY("a", pair.key, pair.keyLength);
    TEST_ASSERT_EQUAL(JSONNumber, pair.jsonType);
    TEST_ASSERT_EQUAL_UINT(1U, pair.valueLength);
    TEST_ASSERT_EQUAL_MEMORY("1", pair.value, pair.valueLength);

    start = 0U;
    memset(&pair, 0, sizeof(pair));
    TEST_ASSERT_EQUAL(JSONSuccess, JSON_Iterate(json, strlen(json), &start, &next, &pair));
    TEST_ASSERT_EQUAL_UINT(1U, pair.keyLength);
    TEST_ASSERT_EQUAL_MEMORY("b", pair.key, pair.keyLength);
    TEST_ASSERT_EQUAL(JSONFalse, pair.jsonType);
    TEST_ASSERT_EQUAL_UINT(strlen("false"), pair.valueLength);
    TEST_ASSERT_EQUAL_MEMORY("false", pair.value, pair.valueLength);

    start = 0U;
    TEST_ASSERT_EQUAL(JSONNotFound, JSON_Iterate(json, strlen(json), &start, &next, &pair));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_validate_rejects_bad_parameters);
    RUN_TEST(test_validate_accepts_objects_arrays_and_scalars);
    RUN_TEST(test_validate_rejects_malformed_or_trailing_data);
    RUN_TEST(test_search_finds_nested_values_and_types);
    RUN_TEST(test_search_reports_not_found_and_bad_parameters);
    RUN_TEST(test_iterate_walks_object_pairs);
    return UNITY_END();
}
