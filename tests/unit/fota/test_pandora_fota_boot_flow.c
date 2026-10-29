/*
 * Host contract for the Pandora resident-bootloader decision sequence.
 *
 * `pandora_fota_boot_flow.c` owns the order the real bootloader runs:
 *
 *   validate candidate -> [reviewed restage] -> install once
 *     -> confirm request ? confirm + ack : record attempt (+ rollback abort)
 *     -> jump to application
 *
 * These cases pin that order, the fail-closed short circuits, and the UART marker
 * sequence the board capture validators rely on. Every board operation is faked,
 * so nothing here touches Flash, the RTC backup registers or the application
 * vector table.
 *
 * Evidence boundary: Host contract only. It does not prove real Flash erase/program,
 * RTC backup retention, power-loss behaviour or vector handoff on silicon.
 */

#include "unity.h"

#include "pandora_fota_boot_flow.h"

#include <stdint.h>
#include <string.h>

enum {
    EV_CANDIDATE_VALID = 1,
    EV_VECTORS_VALID = 2,
    EV_INSTALL = 3,
    EV_CONFIRM = 4,
    EV_ATTEMPT = 5,
    EV_RESTAGE = 6,
    EV_RTC_READ_BASE = 16,
    EV_RTC_WRITE_BASE = 32,
};

#define EV_RTC_READ(slot) (EV_RTC_READ_BASE + (int)(slot))
#define EV_RTC_WRITE(slot) (EV_RTC_WRITE_BASE + (int)(slot))

#define MAX_EVENTS 24U
#define MAX_MARKERS 8U
#define MARKER_LENGTH 64U

static int events[MAX_EVENTS];
static unsigned int event_count;

static char markers[MAX_MARKERS][MARKER_LENGTH];
static unsigned int marker_count;

static uint32_t rtc_slots[4];

static int rtc_read_fail_slot;
static int rtc_write_fail_slot;
static int candidate_valid_result;
static int vectors_valid_result;
static int install_result;
static int install_wrote;
static int confirm_result;
static int confirm_writes_ack;
static int attempt_result;
static int attempt_rollback;

static void record_event(int event) {
    if (event_count < MAX_EVENTS) {
        events[event_count] = event;
        ++event_count;
    }
}

static int event_position(int event) {
    for (unsigned int index = 0U; index < event_count; ++index) {
        if (events[index] == event) {
            return (int)index;
        }
    }
    return -1;
}

static unsigned int event_count_of(int event) {
    unsigned int total = 0U;

    for (unsigned int index = 0U; index < event_count; ++index) {
        if (events[index] == event) {
            ++total;
        }
    }
    return total;
}

static int marker_index(const char* marker) {
    for (unsigned int index = 0U; index < marker_count; ++index) {
        if (strcmp(markers[index], marker) == 0) {
            return (int)index;
        }
    }
    return -1;
}

static int fake_rtc_read(uint8_t slot, uint32_t* value) {
    record_event(EV_RTC_READ_BASE + (int)slot);
    if ((int)slot == rtc_read_fail_slot) {
        return -1;
    }
    *value = rtc_slots[slot];
    return 0;
}

static int fake_rtc_write(uint8_t slot, uint32_t value) {
    record_event(EV_RTC_WRITE_BASE + (int)slot);
    if ((int)slot == rtc_write_fail_slot) {
        return -1;
    }
    rtc_slots[slot] = value;
    return 0;
}

static int fake_candidate_valid(void) {
    record_event(EV_CANDIDATE_VALID);
    return candidate_valid_result;
}

static int fake_vectors_valid(void) {
    record_event(EV_VECTORS_VALID);
    return vectors_valid_result;
}

static int fake_install_once(int* installed) {
    record_event(EV_INSTALL);
    if (install_result != 0) {
        return install_result;
    }
    *installed = install_wrote;
    return 0;
}

static int fake_confirm(void) {
    record_event(EV_CONFIRM);
    if (confirm_result != 0) {
        return confirm_result;
    }
    if (confirm_writes_ack) {
        rtc_slots[PANDORA_FOTA_RTC_SLOT_CONFIRM] = PANDORA_FOTA_CONFIRM_ACK_MAGIC;
    }
    return 0;
}

static int fake_record_attempt(int* rollback_required) {
    record_event(EV_ATTEMPT);
    if (attempt_result != 0) {
        return attempt_result;
    }
    *rollback_required = attempt_rollback;
    return 0;
}

static int fake_authorize_restage(void) {
    record_event(EV_RESTAGE);
    return 0;
}

static int fake_authorize_restage_failing(void) {
    record_event(EV_RESTAGE);
    return -1;
}

static void fake_mark(const char* marker) {
    if (marker_count < MAX_MARKERS) {
        size_t length = strlen(marker);
        if (length >= MARKER_LENGTH) {
            length = MARKER_LENGTH - 1U;
        }
        memcpy(markers[marker_count], marker, length);
        markers[marker_count][length] = '\0';
        ++marker_count;
    }
}

static pandora_fota_boot_flow_ops_t make_ops(void) {
    const pandora_fota_boot_flow_ops_t ops = {
        .rtc_backup_read = fake_rtc_read,
        .rtc_backup_write = fake_rtc_write,
        .candidate_valid = fake_candidate_valid,
        .application_vectors_valid = fake_vectors_valid,
        .install_once = fake_install_once,
        .confirm = fake_confirm,
        .record_attempt = fake_record_attempt,
        .authorize_restage = NULL,
        .mark = fake_mark,
    };
    return ops;
}

void setUp(void) {
    event_count = 0U;
    marker_count = 0U;
    memset(events, 0, sizeof(events));
    memset(markers, 0, sizeof(markers));
    memset(rtc_slots, 0, sizeof(rtc_slots));

    rtc_read_fail_slot = -1;
    rtc_write_fail_slot = -1;
    candidate_valid_result = 1;
    vectors_valid_result = 1;
    install_result = 0;
    install_wrote = 1;
    confirm_result = 0;
    confirm_writes_ack = 0;
    attempt_result = 0;
    attempt_rollback = 0;
}

void tearDown(void) {}

static void test_attempt_path_orders_install_before_attempt_and_jumps(void) {
    pandora_fota_boot_flow_ops_t ops = make_ops();
    pandora_fota_boot_outcome_t outcome = pandora_fota_boot_flow_run(&ops);

    TEST_ASSERT_EQUAL_INT(PANDORA_FOTA_BOOT_OUTCOME_JUMP, outcome);
    TEST_ASSERT_EQUAL_INT(1, (int)event_count_of(EV_INSTALL));
    TEST_ASSERT_EQUAL_INT(1, (int)event_count_of(EV_ATTEMPT));
    TEST_ASSERT_EQUAL_INT(0, (int)event_count_of(EV_CONFIRM));
    TEST_ASSERT_EQUAL_INT(1, (int)event_count_of(EV_RTC_READ(PANDORA_FOTA_RTC_SLOT_CONFIRM)));
    TEST_ASSERT_TRUE(event_position(EV_CANDIDATE_VALID) < event_position(EV_INSTALL));
    TEST_ASSERT_TRUE(event_position(EV_INSTALL) <
                     event_position(EV_RTC_READ(PANDORA_FOTA_RTC_SLOT_CONFIRM)));
    TEST_ASSERT_TRUE(event_position(EV_RTC_READ(PANDORA_FOTA_RTC_SLOT_CONFIRM)) <
                     event_position(EV_ATTEMPT));

    TEST_ASSERT_EQUAL_UINT(2U, marker_count);
    TEST_ASSERT_EQUAL_INT(0, marker_index("PANDORA_BOOT_CANDIDATE_INSTALLED\r\n"));
    TEST_ASSERT_EQUAL_INT(1, marker_index("PANDORA_BOOT_ATTEMPT_COMMITTED\r\n"));
}

static void test_attempt_path_reports_already_installed(void) {
    pandora_fota_boot_flow_ops_t ops = make_ops();
    install_wrote = 0;

    TEST_ASSERT_EQUAL_INT(PANDORA_FOTA_BOOT_OUTCOME_JUMP, pandora_fota_boot_flow_run(&ops));
    TEST_ASSERT_EQUAL_INT(0, marker_index("PANDORA_BOOT_CANDIDATE_ALREADY_INSTALLED\r\n"));
    TEST_ASSERT_EQUAL_INT(-1, marker_index("PANDORA_BOOT_CANDIDATE_INSTALLED\r\n"));
}

static void test_confirm_request_confirms_and_acknowledges_without_attempt(void) {
    pandora_fota_boot_flow_ops_t ops = make_ops();
    /*
     * The request is read on the boot after the install: this image was installed by an
     * earlier boot and has now been running long enough to ask for confirmation. A boot
     * that also writes the image discards the request instead (see the discard tests).
     */
    install_wrote = 0;
    rtc_slots[PANDORA_FOTA_RTC_SLOT_CONFIRM] = PANDORA_FOTA_CONFIRM_REQUEST_MAGIC;

    TEST_ASSERT_EQUAL_INT(PANDORA_FOTA_BOOT_OUTCOME_JUMP, pandora_fota_boot_flow_run(&ops));
    TEST_ASSERT_EQUAL_INT(1, (int)event_count_of(EV_CONFIRM));
    TEST_ASSERT_EQUAL_INT(0, (int)event_count_of(EV_ATTEMPT));
    TEST_ASSERT_EQUAL_INT(1, (int)event_count_of(EV_RTC_WRITE(PANDORA_FOTA_RTC_SLOT_CONFIRM)));
    TEST_ASSERT_EQUAL_UINT32(PANDORA_FOTA_CONFIRM_ACK_MAGIC,
                             rtc_slots[PANDORA_FOTA_RTC_SLOT_CONFIRM]);
    TEST_ASSERT_TRUE(event_position(EV_INSTALL) < event_position(EV_CONFIRM));
    TEST_ASSERT_TRUE(event_position(EV_CONFIRM) <
                     event_position(EV_RTC_WRITE(PANDORA_FOTA_RTC_SLOT_CONFIRM)));

    TEST_ASSERT_EQUAL_UINT(2U, marker_count);
    TEST_ASSERT_EQUAL_INT(1, marker_index("PANDORA_BOOT_CANDIDATE_CONFIRMED\r\n"));
    TEST_ASSERT_EQUAL_INT(-1, marker_index("PANDORA_BOOT_ATTEMPT_COMMITTED\r\n"));
}

static void test_invalid_candidate_with_valid_vectors_jumps_without_side_effects(void) {
    pandora_fota_boot_flow_ops_t ops = make_ops();
    candidate_valid_result = 0;

    TEST_ASSERT_EQUAL_INT(PANDORA_FOTA_BOOT_OUTCOME_JUMP, pandora_fota_boot_flow_run(&ops));
    TEST_ASSERT_EQUAL_INT(1, (int)event_count_of(EV_VECTORS_VALID));
    TEST_ASSERT_EQUAL_INT(0, (int)event_count_of(EV_INSTALL));
    TEST_ASSERT_EQUAL_INT(0, (int)event_count_of(EV_CONFIRM));
    TEST_ASSERT_EQUAL_INT(0, (int)event_count_of(EV_ATTEMPT));
    TEST_ASSERT_EQUAL_INT(0, (int)event_count_of(EV_RTC_READ(PANDORA_FOTA_RTC_SLOT_CONFIRM)));
    TEST_ASSERT_EQUAL_UINT(0U, marker_count);
}

static void test_invalid_candidate_with_invalid_vectors_halts(void) {
    pandora_fota_boot_flow_ops_t ops = make_ops();
    candidate_valid_result = 0;
    vectors_valid_result = 0;

    TEST_ASSERT_EQUAL_INT(PANDORA_FOTA_BOOT_OUTCOME_HALT, pandora_fota_boot_flow_run(&ops));
    TEST_ASSERT_EQUAL_INT(0, (int)event_count_of(EV_INSTALL));
    TEST_ASSERT_EQUAL_UINT(1U, marker_count);
    TEST_ASSERT_EQUAL_INT(0, marker_index("PANDORA_BOOT_NO_VALID_IMAGE\r\n"));
}

static void test_install_failure_halts_before_confirm_and_attempt(void) {
    pandora_fota_boot_flow_ops_t ops = make_ops();
    install_result = -1;

    TEST_ASSERT_EQUAL_INT(PANDORA_FOTA_BOOT_OUTCOME_HALT, pandora_fota_boot_flow_run(&ops));
    TEST_ASSERT_EQUAL_INT(0, (int)event_count_of(EV_CONFIRM));
    TEST_ASSERT_EQUAL_INT(0, (int)event_count_of(EV_ATTEMPT));
    TEST_ASSERT_EQUAL_INT(0, (int)event_count_of(EV_RTC_READ(PANDORA_FOTA_RTC_SLOT_CONFIRM)));
    TEST_ASSERT_EQUAL_UINT(1U, marker_count);
    TEST_ASSERT_EQUAL_INT(0, marker_index("PANDORA_BOOT_INSTALL_ERROR\r\n"));
}

static void test_confirm_slot_read_failure_halts_after_install(void) {
    pandora_fota_boot_flow_ops_t ops = make_ops();
    rtc_read_fail_slot = (int)PANDORA_FOTA_RTC_SLOT_CONFIRM;

    TEST_ASSERT_EQUAL_INT(PANDORA_FOTA_BOOT_OUTCOME_HALT, pandora_fota_boot_flow_run(&ops));
    TEST_ASSERT_EQUAL_INT(1, (int)event_count_of(EV_INSTALL));
    TEST_ASSERT_EQUAL_INT(0, (int)event_count_of(EV_CONFIRM));
    TEST_ASSERT_EQUAL_INT(0, (int)event_count_of(EV_ATTEMPT));
    TEST_ASSERT_EQUAL_UINT(1U, marker_count);
    TEST_ASSERT_EQUAL_INT(0, marker_index("PANDORA_BOOT_CANDIDATE_INSTALLED\r\n"));
}

static void test_attempt_failure_halts_without_jump(void) {
    pandora_fota_boot_flow_ops_t ops = make_ops();
    attempt_result = -1;

    TEST_ASSERT_EQUAL_INT(PANDORA_FOTA_BOOT_OUTCOME_HALT, pandora_fota_boot_flow_run(&ops));
    TEST_ASSERT_EQUAL_UINT(2U, marker_count);
    TEST_ASSERT_EQUAL_INT(1, marker_index("PANDORA_BOOT_ATTEMPT_ERROR\r\n"));
    TEST_ASSERT_EQUAL_INT(-1, marker_index("PANDORA_BOOT_ATTEMPT_COMMITTED\r\n"));
}

static void test_exhausted_attempts_halt_with_rollback_marker(void) {
    pandora_fota_boot_flow_ops_t ops = make_ops();
    attempt_rollback = 1;

    TEST_ASSERT_EQUAL_INT(PANDORA_FOTA_BOOT_OUTCOME_HALT, pandora_fota_boot_flow_run(&ops));
    TEST_ASSERT_EQUAL_UINT(2U, marker_count);
    TEST_ASSERT_EQUAL_INT(1, marker_index("PANDORA_BOOT_ROLLBACK_REQUIRED\r\n"));
    TEST_ASSERT_EQUAL_INT(-1, marker_index("PANDORA_BOOT_ATTEMPT_COMMITTED\r\n"));
    TEST_ASSERT_EQUAL_INT(0, marker_index("PANDORA_BOOT_CANDIDATE_INSTALLED\r\n"));
}

static void test_confirm_failure_halts_before_ack_write(void) {
    pandora_fota_boot_flow_ops_t ops = make_ops();
    install_wrote = 0;
    rtc_slots[PANDORA_FOTA_RTC_SLOT_CONFIRM] = PANDORA_FOTA_CONFIRM_REQUEST_MAGIC;
    confirm_result = -1;

    TEST_ASSERT_EQUAL_INT(PANDORA_FOTA_BOOT_OUTCOME_HALT, pandora_fota_boot_flow_run(&ops));
    TEST_ASSERT_EQUAL_INT(1, (int)event_count_of(EV_CONFIRM));
    TEST_ASSERT_EQUAL_INT(0, (int)event_count_of(EV_RTC_WRITE(PANDORA_FOTA_RTC_SLOT_CONFIRM)));
    TEST_ASSERT_EQUAL_UINT32(PANDORA_FOTA_CONFIRM_REQUEST_MAGIC,
                             rtc_slots[PANDORA_FOTA_RTC_SLOT_CONFIRM]);
    TEST_ASSERT_EQUAL_UINT(2U, marker_count);
    TEST_ASSERT_EQUAL_INT(1, marker_index("PANDORA_BOOT_CONFIRM_ERROR\r\n"));
    TEST_ASSERT_EQUAL_INT(-1, marker_index("PANDORA_BOOT_CANDIDATE_CONFIRMED\r\n"));
}

static void test_confirm_ack_write_failure_halts_before_confirm_marker(void) {
    pandora_fota_boot_flow_ops_t ops = make_ops();
    install_wrote = 0;
    rtc_slots[PANDORA_FOTA_RTC_SLOT_CONFIRM] = PANDORA_FOTA_CONFIRM_REQUEST_MAGIC;
    rtc_write_fail_slot = (int)PANDORA_FOTA_RTC_SLOT_CONFIRM;

    TEST_ASSERT_EQUAL_INT(PANDORA_FOTA_BOOT_OUTCOME_HALT, pandora_fota_boot_flow_run(&ops));
    TEST_ASSERT_EQUAL_INT(1, (int)event_count_of(EV_CONFIRM));
    TEST_ASSERT_EQUAL_UINT(1U, marker_count);
    TEST_ASSERT_EQUAL_INT(0, marker_index("PANDORA_BOOT_CANDIDATE_ALREADY_INSTALLED\r\n"));
    TEST_ASSERT_EQUAL_INT(-1, marker_index("PANDORA_BOOT_CANDIDATE_CONFIRMED\r\n"));
}

static void test_a_replaced_image_cannot_leave_its_request_behind(void) {
    /*
     * The regression: the app stages a new candidate, its reset handshake leaves a
     * request in the slot, and the next boot installs that candidate. Before the fix the
     * bootloader read the request and confirmed an image that had never executed once,
     * which also skipped the three-attempt budget in one step.
     */
    pandora_fota_boot_flow_ops_t ops = make_ops();
    rtc_slots[PANDORA_FOTA_RTC_SLOT_CONFIRM] = PANDORA_FOTA_CONFIRM_REQUEST_MAGIC;

    TEST_ASSERT_EQUAL_INT(PANDORA_FOTA_BOOT_OUTCOME_JUMP, pandora_fota_boot_flow_run(&ops));
    TEST_ASSERT_EQUAL_INT(0, (int)event_count_of(EV_CONFIRM));
    TEST_ASSERT_EQUAL_INT(1, (int)event_count_of(EV_ATTEMPT));
    TEST_ASSERT_EQUAL_UINT32(0U, rtc_slots[PANDORA_FOTA_RTC_SLOT_CONFIRM]);
    TEST_ASSERT_EQUAL_UINT(2U, marker_count);
    TEST_ASSERT_EQUAL_INT(-1, marker_index("PANDORA_BOOT_CANDIDATE_CONFIRMED\r\n"));
    TEST_ASSERT_EQUAL_INT(1, marker_index("PANDORA_BOOT_ATTEMPT_COMMITTED\r\n"));
}

static void test_an_install_clears_the_previous_acknowledgement(void) {
    pandora_fota_boot_flow_ops_t ops = make_ops();
    rtc_slots[PANDORA_FOTA_RTC_SLOT_CONFIRM] = PANDORA_FOTA_CONFIRM_ACK_MAGIC;

    TEST_ASSERT_EQUAL_INT(PANDORA_FOTA_BOOT_OUTCOME_JUMP, pandora_fota_boot_flow_run(&ops));
    TEST_ASSERT_EQUAL_UINT32(0U, rtc_slots[PANDORA_FOTA_RTC_SLOT_CONFIRM]);
    TEST_ASSERT_EQUAL_INT(0, (int)event_count_of(EV_CONFIRM));
    TEST_ASSERT_EQUAL_INT(1, (int)event_count_of(EV_ATTEMPT));
}

static void test_the_request_is_cleared_before_the_slot_is_read(void) {
    pandora_fota_boot_flow_ops_t ops = make_ops();

    TEST_ASSERT_EQUAL_INT(PANDORA_FOTA_BOOT_OUTCOME_JUMP, pandora_fota_boot_flow_run(&ops));
    TEST_ASSERT_EQUAL_INT(1, (int)event_count_of(EV_RTC_WRITE(PANDORA_FOTA_RTC_SLOT_CONFIRM)));
    TEST_ASSERT_TRUE(event_position(EV_INSTALL) <
                     event_position(EV_RTC_WRITE(PANDORA_FOTA_RTC_SLOT_CONFIRM)));
    TEST_ASSERT_TRUE(event_position(EV_RTC_WRITE(PANDORA_FOTA_RTC_SLOT_CONFIRM)) <
                     event_position(EV_RTC_READ(PANDORA_FOTA_RTC_SLOT_CONFIRM)));
}

static void test_a_request_that_cannot_be_cleared_halts_before_the_decision(void) {
    pandora_fota_boot_flow_ops_t ops = make_ops();
    rtc_slots[PANDORA_FOTA_RTC_SLOT_CONFIRM] = PANDORA_FOTA_CONFIRM_REQUEST_MAGIC;
    rtc_write_fail_slot = (int)PANDORA_FOTA_RTC_SLOT_CONFIRM;

    TEST_ASSERT_EQUAL_INT(PANDORA_FOTA_BOOT_OUTCOME_HALT, pandora_fota_boot_flow_run(&ops));
    TEST_ASSERT_EQUAL_INT(0, (int)event_count_of(EV_CONFIRM));
    TEST_ASSERT_EQUAL_INT(0, (int)event_count_of(EV_ATTEMPT));
    TEST_ASSERT_EQUAL_INT(0, (int)event_count_of(EV_RTC_READ(PANDORA_FOTA_RTC_SLOT_CONFIRM)));
    TEST_ASSERT_EQUAL_UINT(2U, marker_count);
    TEST_ASSERT_EQUAL_INT(0, marker_index("PANDORA_BOOT_CANDIDATE_INSTALLED\r\n"));
    TEST_ASSERT_EQUAL_INT(1, marker_index("PANDORA_BOOT_CONFIRM_REQUEST_ERROR\r\n"));
}

static void test_restage_runs_between_validation_and_install_only_once(void) {
    pandora_fota_boot_flow_ops_t ops = make_ops();
    ops.authorize_restage = fake_authorize_restage;

    TEST_ASSERT_EQUAL_INT(PANDORA_FOTA_BOOT_OUTCOME_JUMP, pandora_fota_boot_flow_run(&ops));
    TEST_ASSERT_EQUAL_INT(1, (int)event_count_of(EV_RESTAGE));
    TEST_ASSERT_TRUE(event_position(EV_CANDIDATE_VALID) < event_position(EV_RESTAGE));
    TEST_ASSERT_TRUE(event_position(EV_RESTAGE) < event_position(EV_INSTALL));
    TEST_ASSERT_EQUAL_INT(1, (int)event_count_of(EV_RTC_READ(PANDORA_FOTA_RTC_SLOT_RESTAGE)));
    TEST_ASSERT_EQUAL_INT(1, (int)event_count_of(EV_RTC_WRITE(PANDORA_FOTA_RTC_SLOT_RESTAGE)));
    TEST_ASSERT_EQUAL_UINT32(PANDORA_FOTA_RESTAGE_DONE_MAGIC,
                             rtc_slots[PANDORA_FOTA_RTC_SLOT_RESTAGE]);
    TEST_ASSERT_EQUAL_INT(0, marker_index("PANDORA_BOOT_RESTAGE_AUTHORIZED\r\n"));

    /* The completed restage must not be authorized again on the next boot. */
    event_count = 0U;
    marker_count = 0U;
    TEST_ASSERT_EQUAL_INT(PANDORA_FOTA_BOOT_OUTCOME_JUMP, pandora_fota_boot_flow_run(&ops));
    TEST_ASSERT_EQUAL_INT(0, (int)event_count_of(EV_RESTAGE));
    TEST_ASSERT_EQUAL_INT(-1, marker_index("PANDORA_BOOT_RESTAGE_AUTHORIZED\r\n"));
}

static void test_restage_authorization_failure_halts_before_install(void) {
    pandora_fota_boot_flow_ops_t ops = make_ops();
    ops.authorize_restage = fake_authorize_restage_failing;

    TEST_ASSERT_EQUAL_INT(PANDORA_FOTA_BOOT_OUTCOME_HALT, pandora_fota_boot_flow_run(&ops));
    TEST_ASSERT_EQUAL_INT(1, (int)event_count_of(EV_RESTAGE));
    TEST_ASSERT_EQUAL_INT(0, (int)event_count_of(EV_INSTALL));
    TEST_ASSERT_EQUAL_INT(0, (int)event_count_of(EV_RTC_WRITE(PANDORA_FOTA_RTC_SLOT_RESTAGE)));
    TEST_ASSERT_EQUAL_UINT32(0U, rtc_slots[PANDORA_FOTA_RTC_SLOT_RESTAGE]);
    TEST_ASSERT_EQUAL_UINT(0U, marker_count);
}

static void test_null_ops_halts(void) {
    TEST_ASSERT_EQUAL_INT(PANDORA_FOTA_BOOT_OUTCOME_HALT, pandora_fota_boot_flow_run(NULL));
}

#define EXPECT_HALT_WITHOUT_SIDE_EFFECTS(field)                                                    \
    do {                                                                                           \
        pandora_fota_boot_flow_ops_t ops = make_ops();                                             \
        ops.field = NULL;                                                                          \
        event_count = 0U;                                                                          \
        marker_count = 0U;                                                                         \
        TEST_ASSERT_EQUAL_INT(PANDORA_FOTA_BOOT_OUTCOME_HALT, pandora_fota_boot_flow_run(&ops));   \
        TEST_ASSERT_EQUAL_UINT(0U, event_count);                                                   \
        TEST_ASSERT_EQUAL_UINT(0U, marker_count);                                                  \
    } while (0)

static void test_every_required_callback_is_checked_before_any_side_effect(void) {
    EXPECT_HALT_WITHOUT_SIDE_EFFECTS(rtc_backup_read);
    EXPECT_HALT_WITHOUT_SIDE_EFFECTS(rtc_backup_write);
    EXPECT_HALT_WITHOUT_SIDE_EFFECTS(candidate_valid);
    EXPECT_HALT_WITHOUT_SIDE_EFFECTS(application_vectors_valid);
    EXPECT_HALT_WITHOUT_SIDE_EFFECTS(install_once);
    EXPECT_HALT_WITHOUT_SIDE_EFFECTS(confirm);
    EXPECT_HALT_WITHOUT_SIDE_EFFECTS(record_attempt);
}

static void test_missing_marker_callback_still_runs_the_sequence(void) {
    pandora_fota_boot_flow_ops_t ops = make_ops();
    ops.mark = NULL;

    TEST_ASSERT_EQUAL_INT(PANDORA_FOTA_BOOT_OUTCOME_JUMP, pandora_fota_boot_flow_run(&ops));
    TEST_ASSERT_EQUAL_INT(1, (int)event_count_of(EV_INSTALL));
    TEST_ASSERT_EQUAL_INT(1, (int)event_count_of(EV_ATTEMPT));
    TEST_ASSERT_EQUAL_UINT(0U, marker_count);
}

void test_restage_read_failure_halts_before_authorization(void) {
    pandora_fota_boot_flow_ops_t ops = make_ops();
    ops.authorize_restage = fake_authorize_restage;
    rtc_read_fail_slot = (int)PANDORA_FOTA_RTC_SLOT_RESTAGE;

    TEST_ASSERT_EQUAL_INT(PANDORA_FOTA_BOOT_OUTCOME_HALT, pandora_fota_boot_flow_run(&ops));
    TEST_ASSERT_EQUAL_INT(0, (int)event_count_of(EV_RESTAGE));
    TEST_ASSERT_EQUAL_INT(0, (int)event_count_of(EV_INSTALL));
    TEST_ASSERT_EQUAL_UINT(0U, marker_count);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_attempt_path_orders_install_before_attempt_and_jumps);
    RUN_TEST(test_attempt_path_reports_already_installed);
    RUN_TEST(test_confirm_request_confirms_and_acknowledges_without_attempt);
    RUN_TEST(test_invalid_candidate_with_valid_vectors_jumps_without_side_effects);
    RUN_TEST(test_invalid_candidate_with_invalid_vectors_halts);
    RUN_TEST(test_install_failure_halts_before_confirm_and_attempt);
    RUN_TEST(test_confirm_slot_read_failure_halts_after_install);
    RUN_TEST(test_attempt_failure_halts_without_jump);
    RUN_TEST(test_exhausted_attempts_halt_with_rollback_marker);
    RUN_TEST(test_confirm_failure_halts_before_ack_write);
    RUN_TEST(test_confirm_ack_write_failure_halts_before_confirm_marker);
    RUN_TEST(test_a_replaced_image_cannot_leave_its_request_behind);
    RUN_TEST(test_an_install_clears_the_previous_acknowledgement);
    RUN_TEST(test_the_request_is_cleared_before_the_slot_is_read);
    RUN_TEST(test_a_request_that_cannot_be_cleared_halts_before_the_decision);
    RUN_TEST(test_restage_runs_between_validation_and_install_only_once);
    RUN_TEST(test_restage_authorization_failure_halts_before_install);
    RUN_TEST(test_restage_read_failure_halts_before_authorization);
    RUN_TEST(test_null_ops_halts);
    RUN_TEST(test_every_required_callback_is_checked_before_any_side_effect);
    RUN_TEST(test_missing_marker_callback_still_runs_the_sequence);
    return UNITY_END();
}
