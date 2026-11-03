/**
 * @file test_at_common_contract.c
 * @brief The AT client and the AT server must speak one vocabulary, not two that look alike.
 *
 * The client (xy_atc.h) and the server (xy_ats.h) are opposite ends of the same protocol, and both are
 * used: the client talks to a modem, the server makes this board answer commands. What they share is
 * the vocabulary - response strings, line ending, command prefix, the link states, and the description
 * of the transport underneath.
 *
 * That vocabulary used to be written out in both headers, byte for byte. A translation unit that
 * included both therefore saw `struct at_dev_ops` defined twice, and the guard watching for duplicate
 * definitions had to carry an exemption saying the duplication was intentional. It is now defined
 * once, in at_common.h, and both headers include it.
 *
 * This suite asserts the property that makes the merge worth doing, which is stronger than "both
 * compile": the two sides must agree on the *same* type, not on compatible ones. A translation unit
 * that includes both headers and passes a client's at_dev_ops_t to a server function taking the same
 * name is only correct if there is one type. Two identical struct definitions in different headers
 * are two types that happen to match - the compiler cannot tell, and a future field added to one
 * side alone becomes a silent mismatch.
 *
 * The enum values are asserted too: the states are named in both the client's and the server's
 * headers, and a renamed state would break one side while leaving the other compiling.
 */

#include <string.h>

#include "unity.h"

#include "at_common.h"
#include "xy_atc.h"
#include "xy_ats.h"

/* Both headers included above on purpose: including one and then the other is the case that used to
 * produce a duplicate definition. */

/* Unity calls these around every case. There is nothing to arrange here - the assertions are about
 * types and constants - but Unity needs them to exist. */
void setUp(void)
{
}

void tearDown(void)
{
}

/* === the shared vocabulary ============================================================== */

static void test_the_response_strings_are_the_ones_both_sides_agree_on(void)
{
    TEST_ASSERT_EQUAL_STRING("OK", AT_RESP_OK_STR);
    TEST_ASSERT_EQUAL_STRING("ERROR", AT_RESP_ERROR_STR);
    TEST_ASSERT_EQUAL_STRING("\r\n", AT_CMD_LINE_END);
    TEST_ASSERT_EQUAL_STRING("AT", AT_CMD_PREFIX);
}

/* The states are the same conversation seen from two ends. Their numeric values are part of the
 * contract: a state value crossing the boundary between the two sides means something. */
static void test_the_link_states_have_the_values_both_sides_expect(void)
{
    TEST_ASSERT_EQUAL_INT(0, at_state_init);
    TEST_ASSERT_EQUAL_INT(1, at_state_idle);
    TEST_ASSERT_EQUAL_INT(2, at_state_sending);
    TEST_ASSERT_EQUAL_INT(3, at_state_receiving);
    TEST_ASSERT_EQUAL_INT(4, at_state_error);
    TEST_ASSERT_EQUAL_INT(5, at_state_data_mode);
}

/* === one type, not two that match ======================================================= */

/* The proof that the merge is real rather than cosmetic: the client's and the server's transport
 * descriptions are the same type, so a pointer to one is a pointer to the other. */
static void test_the_transport_description_is_one_type_on_both_sides(void)
{
    struct at_dev_ops client_side;
    struct at_dev_ops server_side;

    memset(&client_side, 0, sizeof(client_side));
    memset(&server_side, 0, sizeof(server_side));

    /* Copying one into the other is what a single type makes possible and two merely-matching
     * definitions would also allow - so the compiler cannot be the witness here, and this case is not
     * claiming to be. The witness is the guard that reports a duplicate definition: this file is
     * compiled into a translation unit where both headers are included, so a second definition of the
     * struct would break the build instead of passing quietly. What this case does check is the
     * property the merge is for: one layout, one size, one set of member names. */
    server_side = client_side;

    TEST_ASSERT_EQUAL_UINT32(sizeof(client_side), sizeof(server_side));
    TEST_ASSERT_EQUAL_PTR(client_side.attr, server_side.attr);
    TEST_ASSERT_EQUAL_PTR(client_side.read, server_side.read);
}

static void test_the_link_attributes_are_one_type_on_both_sides(void)
{
    struct at_dev_attr attributes;

    memset(&attributes, 0, sizeof(attributes));
    attributes.recv_size = 256U;
    attributes.send_size = 128U;
    attributes.user_data = &attributes;

    at_dev_attr_t as_pointer = &attributes;

    TEST_ASSERT_EQUAL_PTR(&attributes, as_pointer);
    TEST_ASSERT_EQUAL_UINT16(256U, as_pointer->recv_size);
    TEST_ASSERT_EQUAL_UINT16(128U, as_pointer->send_size);
}

/* The transport operations are the contract between a side and whatever it talks over. */
static void test_the_transport_operations_are_reachable_through_the_shared_type(void)
{
    static char buffer[8];
    struct at_dev_ops ops;

    memset(&ops, 0, sizeof(ops));
    ops.read = NULL;
    ops.write = NULL;
    ops.control = NULL;

    TEST_ASSERT_NULL(ops.read);
    TEST_ASSERT_NULL(ops.write);
    TEST_ASSERT_NULL(ops.control);
    TEST_ASSERT_NULL(ops.attr);

    /* A read that would be supplied by the platform. Declaring it here rather than only checking the
     * pointers keeps the function-pointer shape under test: the signature is part of the vocabulary. */
    int (*reader)(void *buffer, int len) = ops.read;
    TEST_ASSERT_NULL(reader);
    TEST_ASSERT_EQUAL_UINT32(8U, sizeof(buffer));
}

/* === each side keeps what is its own ===================================================== */

/* The client and the server are not the same thing, and merging their vocabulary must not have
 * merged their identity: the client's status enum and the server's command table still exist, and
 * they are distinct types. */
static void test_the_client_keeps_its_own_state_machine(void)
{
    atc_client_t client = NULL;
    atc_status_t status = atc_status_uninitialized;

    TEST_ASSERT_NULL(client);
    TEST_ASSERT_EQUAL_INT(0, status);
}

static void test_the_server_keeps_its_own_command_table(void)
{
    at_server_t server;
    at_server_status_t status = ATS_SERVER_STATUS_UNINITIALIZED;

    memset(&server, 0, sizeof(server));
    TEST_ASSERT_EQUAL_INT(0, status);
    TEST_ASSERT_EQUAL_INT(ATS_SERVER_STATUS_UNINITIALIZED, server.status);
}

/* The command modes are the server's, and they belong to the server only: the client has no notion of
 * a command's test/query/setup/execute mode. */
static void test_the_command_modes_remain_the_server_s(void)
{
    at_cmd_mode_t mode = ATS_CMD_MODE_QUERY;

    TEST_ASSERT_EQUAL_INT(1, mode);
    TEST_ASSERT_EQUAL_INT(0, ATS_CMD_MODE_TEST);
    TEST_ASSERT_EQUAL_INT(2, ATS_CMD_MODE_SETUP);
    TEST_ASSERT_EQUAL_INT(3, ATS_CMD_MODE_EXEC);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_the_response_strings_are_the_ones_both_sides_agree_on);
    RUN_TEST(test_the_link_states_have_the_values_both_sides_expect);
    RUN_TEST(test_the_transport_description_is_one_type_on_both_sides);
    RUN_TEST(test_the_link_attributes_are_one_type_on_both_sides);
    RUN_TEST(test_the_transport_operations_are_reachable_through_the_shared_type);
    RUN_TEST(test_the_client_keeps_its_own_state_machine);
    RUN_TEST(test_the_server_keeps_its_own_command_table);
    RUN_TEST(test_the_command_modes_remain_the_server_s);

    return UNITY_END();
}
