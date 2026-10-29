/**
 * @file pandora_sys.h
 * @brief Pandora board SYS owner: retained reset request and reset-cause attribution.
 *
 * `xy_sys_reboot_reason()` reports the platform reset-cause bitmask, which can
 * carry several bits at once: an internal reset asserts the NRST flag too, so a
 * software boot was measured as 0x5 and a watchdog boot as 0x9 on this board.
 * The boot dispatch needs one name, so the choice is a contract rather than an
 * inline `if` chain, and it is covered by host tests.
 */

#ifndef PANDORA_SYS_H
#define PANDORA_SYS_H

#include <stdint.h>

/** The single cause the boot dispatch acts on. */
typedef enum {
    PANDORA_RESET_KIND_UNKNOWN = 0,  /**< flags the decoder does not recognize */
    PANDORA_RESET_KIND_POWER_ON,     /**< no reset flag was set at all */
    PANDORA_RESET_KIND_EXTERNAL_PIN, /**< NRST, with no internal cause alongside */
    PANDORA_RESET_KIND_BROWNOUT,     /**< power event on the supply rail */
    PANDORA_RESET_KIND_SOFTWARE,     /**< requested through xy_sys_reset() */
    PANDORA_RESET_KIND_WATCHDOG,     /**< IWDG/WWDG expiry */
    PANDORA_RESET_KIND_LOW_POWER,    /**< exit from a low-power mode */
} pandora_reset_kind_t;

/**
 * @brief Attribute a reset-cause bitmask to one cause.
 *
 * Priority, highest first: WATCHDOG, SOFTWARE, BROWNOUT, LOW_POWER, EXTERNAL_PIN.
 * An intentional reset explains the pin flag it asserts, and a power event must
 * never be reported as a clean power-on. Only an empty mask is POWER_ON; a mask
 * carrying a bit this decoder does not know is UNKNOWN, never POWER_ON.
 */
pandora_reset_kind_t pandora_reset_kind_from_reason(uint32_t reason);

/** Marker name for a kind; out-of-range values report "UNKNOWN". */
const char *pandora_reset_kind_name(pandora_reset_kind_t kind);

/** Kind latched by the last xy_sys_init(); UNKNOWN until then. */
pandora_reset_kind_t pandora_sys_reset_kind(void);

#endif /* PANDORA_SYS_H */
