#include "pandora_fota_boot_flow.h"

#include <stddef.h>

static void flow_mark(const pandora_fota_boot_flow_ops_t *ops, const char *marker) {
    if (ops->mark != NULL) {
        ops->mark(marker);
    }
}

static int flow_ops_complete(const pandora_fota_boot_flow_ops_t *ops) {
    return ops != NULL && ops->rtc_backup_read != NULL && ops->rtc_backup_write != NULL &&
           ops->candidate_valid != NULL && ops->application_vectors_valid != NULL &&
           ops->install_once != NULL && ops->confirm != NULL && ops->record_attempt != NULL;
}

static int flow_restage(const pandora_fota_boot_flow_ops_t *ops) {
    uint32_t done_value = 0U;

    if (ops->authorize_restage == NULL) {
        return 0;
    }
    if (ops->rtc_backup_read(PANDORA_FOTA_RTC_SLOT_RESTAGE, &done_value) != 0) {
        flow_mark(ops, "PANDORA_BOOT_RESTAGE_STATE_ERROR\r\n");
        return -1;
    }
    if (done_value == PANDORA_FOTA_RESTAGE_DONE_MAGIC) {
        return 0;
    }
    if (ops->authorize_restage() != 0) {
        /* The board callback reports which authorization step failed. */
        return -1;
    }
    if (ops->rtc_backup_write(PANDORA_FOTA_RTC_SLOT_RESTAGE, PANDORA_FOTA_RESTAGE_DONE_MAGIC) !=
        0) {
        flow_mark(ops, "PANDORA_BOOT_RESTAGE_DONE_ERROR\r\n");
        return -1;
    }
    flow_mark(ops, "PANDORA_BOOT_RESTAGE_AUTHORIZED\r\n");
    return 0;
}

static int flow_install(const pandora_fota_boot_flow_ops_t *ops) {
    int installed = 0;

    if (ops->install_once(&installed) != 0) {
        flow_mark(ops, "PANDORA_BOOT_INSTALL_ERROR\r\n");
        return -1;
    }
    flow_mark(ops, installed != 0 ? "PANDORA_BOOT_CANDIDATE_INSTALLED\r\n"
                                  : "PANDORA_BOOT_CANDIDATE_ALREADY_INSTALLED\r\n");
    if (installed == 0) {
        return 0;
    }
    /*
     * The confirm request is written by the application, and an application only runs
     * after its own install. A request that is already in the slot when install_once()
     * replaces the execution image therefore was written by the image that just got
     * replaced - it cannot refer to the candidate that replaced it. Honouring it would
     * confirm the new image on its first boot, before it ever ran, and skip the whole
     * boot-attempt budget the budget exists to enforce. Discard it with the install.
     *
     * An acknowledgement from an earlier cycle is cleared by the same write, so the
     * slot always describes the current install cycle instead of accumulating.
     */
    if (ops->rtc_backup_write(PANDORA_FOTA_RTC_SLOT_CONFIRM, 0U) != 0) {
        flow_mark(ops, "PANDORA_BOOT_CONFIRM_REQUEST_ERROR\r\n");
        return -1;
    }
    return 0;
}

static int flow_confirm(const pandora_fota_boot_flow_ops_t *ops) {
    if (ops->confirm() != 0) {
        flow_mark(ops, "PANDORA_BOOT_CONFIRM_ERROR\r\n");
        return -1;
    }
    if (ops->rtc_backup_write(PANDORA_FOTA_RTC_SLOT_CONFIRM, PANDORA_FOTA_CONFIRM_ACK_MAGIC) != 0) {
        /* The journal already holds the confirmation; what failed is publishing the
         * acknowledgement the application reads back after the jump. Not CONFIRM_ERROR. */
        flow_mark(ops, "PANDORA_BOOT_CONFIRM_ACK_ERROR\r\n");
        return -1;
    }
    flow_mark(ops, "PANDORA_BOOT_CANDIDATE_CONFIRMED\r\n");
    return 0;
}

static int flow_attempt(const pandora_fota_boot_flow_ops_t *ops) {
    int rollback_required = 0;

    if (ops->record_attempt(&rollback_required) != 0) {
        flow_mark(ops, "PANDORA_BOOT_ATTEMPT_ERROR\r\n");
        return -1;
    }
    if (rollback_required) {
        flow_mark(ops, "PANDORA_BOOT_ROLLBACK_REQUIRED\r\n");
        return -1;
    }
    flow_mark(ops, "PANDORA_BOOT_ATTEMPT_COMMITTED\r\n");
    return 0;
}

pandora_fota_boot_outcome_t pandora_fota_boot_flow_run(const pandora_fota_boot_flow_ops_t *ops) {
    uint32_t confirm_value = 0U;

    if (!flow_ops_complete(ops)) {
        return PANDORA_FOTA_BOOT_OUTCOME_HALT;
    }
    if (!ops->candidate_valid()) {
        if (!ops->application_vectors_valid()) {
            flow_mark(ops, "PANDORA_BOOT_NO_VALID_IMAGE\r\n");
            return PANDORA_FOTA_BOOT_OUTCOME_HALT;
        }
        return PANDORA_FOTA_BOOT_OUTCOME_JUMP;
    }
    if (flow_restage(ops) != 0) {
        return PANDORA_FOTA_BOOT_OUTCOME_HALT;
    }
    if (flow_install(ops) != 0) {
        return PANDORA_FOTA_BOOT_OUTCOME_HALT;
    }
    if (ops->rtc_backup_read(PANDORA_FOTA_RTC_SLOT_CONFIRM, &confirm_value) != 0) {
        /* A halt with no marker is reported by the capture validator as a truncated
         * boot frame, so the step that could not read the slot has to name itself. */
        flow_mark(ops, "PANDORA_BOOT_CONFIRM_STATE_ERROR\r\n");
        return PANDORA_FOTA_BOOT_OUTCOME_HALT;
    }
    if (confirm_value == PANDORA_FOTA_CONFIRM_REQUEST_MAGIC) {
        if (flow_confirm(ops) != 0) {
            return PANDORA_FOTA_BOOT_OUTCOME_HALT;
        }
    } else if (flow_attempt(ops) != 0) {
        return PANDORA_FOTA_BOOT_OUTCOME_HALT;
    }
    return PANDORA_FOTA_BOOT_OUTCOME_JUMP;
}
