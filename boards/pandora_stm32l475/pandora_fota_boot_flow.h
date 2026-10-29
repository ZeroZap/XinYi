#ifndef PANDORA_FOTA_BOOT_FLOW_H
#define PANDORA_FOTA_BOOT_FLOW_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Board-owned resident-bootloader decision sequence.
 *
 * The exact ordering below is what the board runs, and the UART markers it emits
 * are what the capture validators check, so the sequence lives in one host-testable
 * owner instead of inline in the bootloader entry point:
 *
 *   validate candidate -> [reviewed restage] -> install once
 *     -> confirm request ? confirm + ack : record attempt (+ rollback abort)
 *     -> jump to application
 *
 * Any failing step halts before the next one runs; nothing here jumps on failure.
 *
 * The confirm slot is scoped to one install cycle. install_once() reporting a write
 * clears it, because the only writer of a request is the application - which by
 * definition runs after its own install, so a request present at install time belongs
 * to the image that was just replaced. Without that clear, a single stale request
 * would confirm every later candidate on its first boot and bypass the attempt budget
 * (PANDORA_BOOT_CONFIRM_REQUEST_ERROR reports a slot that could not be cleared).
 */

#define PANDORA_FOTA_CONFIRM_REQUEST_MAGIC 0x46525132U
#define PANDORA_FOTA_CONFIRM_ACK_MAGIC 0x46414332U
#define PANDORA_FOTA_RESTAGE_DONE_MAGIC 0x52535444U
#define PANDORA_FOTA_FLOW_MAX_ATTEMPTS 3U

/* RTC backup slots; 0..2 are owned by the W25Q128/FOTA handshake. */
#define PANDORA_FOTA_RTC_SLOT_CONFIRM 1U
#define PANDORA_FOTA_RTC_SLOT_RESTAGE 2U

typedef enum {
    PANDORA_FOTA_BOOT_OUTCOME_JUMP = 0,
    PANDORA_FOTA_BOOT_OUTCOME_HALT = 1,
} pandora_fota_boot_outcome_t;

/*
 * Injected board operations. Every callback returns 0 on success and non-zero on
 * failure so the flow owns the sequencing contract rather than the vendor APIs.
 */
typedef struct {
    /* RTC backup register access. */
    int (*rtc_backup_read)(uint8_t slot, uint32_t *value);
    int (*rtc_backup_write)(uint8_t slot, uint32_t value);
    /* 1 when a valid W25Q128 candidate envelope was found. */
    int (*candidate_valid)(void);
    /* 1 when the application vector table at PANDORA_FOTA_APP_BASE is plausible. */
    int (*application_vectors_valid)(void);
    /* Install the candidate at most once; *installed reports whether it wrote. */
    int (*install_once)(int *installed);
    /* Commit the pending candidate that the application just confirmed. */
    int (*confirm)(void);
    /* Record one boot attempt; *rollback_required aborts the jump when exhausted. */
    int (*record_attempt)(int *rollback_required);
    /*
     * Optional one-shot reviewed restage authorization. NULL when the restage
     * build option is absent. The callback emits its own restage markers.
     */
    int (*authorize_restage)(void);
    /* Emit one boot marker line. */
    void (*mark)(const char *marker);
} pandora_fota_boot_flow_ops_t;

/**
 * Run the resident bootloader decision sequence.
 *
 * @param ops Board operations; every member except authorize_restage is required.
 * @return JUMP to hand off to the application image, HALT to stop in the bootloader.
 */
pandora_fota_boot_outcome_t pandora_fota_boot_flow_run(const pandora_fota_boot_flow_ops_t *ops);

#ifdef __cplusplus
}
#endif

#endif /* PANDORA_FOTA_BOOT_FLOW_H */
