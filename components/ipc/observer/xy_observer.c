/**
 * @file xy_observer.c
 * @brief Observer Pattern Implementation
 * @version 1.0.0
 * @date 2026-02-28
 */

#include "xy_observer.h"
#include <string.h>

/* ==================== Observer Implementation ==================== */

static bool observer_matches(const xy_observer_t *lhs, const xy_observer_t *rhs)
{
    return lhs->callback == rhs->callback && lhs->user_data == rhs->user_data;
}

int xy_observer_init(xy_observer_t *observer, const char *name,
                     observer_callback_t callback, void *user_data)
{
    if (!observer) {
        return XY_OBSERVER_INVALID_PARAM;
    }

    memset(observer, 0, sizeof(*observer));

    if (name) {
        strncpy(observer->name, name, sizeof(observer->name) - 1);
        observer->name[sizeof(observer->name) - 1] = '\0';
    }

    observer->callback = callback;
    observer->user_data = user_data;
    observer->active = true;

    return XY_OBSERVER_OK;
}

int xy_observer_deinit(xy_observer_t *observer)
{
    if (!observer) {
        return XY_OBSERVER_INVALID_PARAM;
    }

    observer->callback = NULL;
    observer->user_data = NULL;
    observer->active = false;

    return XY_OBSERVER_OK;
}

/* ==================== Subject Implementation ==================== */

int xy_subject_init(xy_subject_t *subject, const char *name)
{
    if (!subject) {
        return XY_OBSERVER_INVALID_PARAM;
    }

    memset(subject, 0, sizeof(*subject));

    if (name) {
        strncpy(subject->name, name, sizeof(subject->name) - 1);
        subject->name[sizeof(subject->name) - 1] = '\0';
    }

    subject->observer_count = 0;
    subject->notifying = false;

    return XY_OBSERVER_OK;
}

int xy_subject_deinit(xy_subject_t *subject)
{
    if (!subject) {
        return XY_OBSERVER_INVALID_PARAM;
    }

    return xy_subject_clear(subject);
}

int xy_subject_attach(xy_subject_t *subject, xy_observer_t *observer)
{
    if (!subject || !observer) {
        return XY_OBSERVER_INVALID_PARAM;
    }

    if (!observer->active || !observer->callback) {
        return XY_OBSERVER_INVALID_PARAM;
    }

    if (subject->observer_count >= XY_OBSERVER_MAX_OBSERVERS) {
        return XY_OBSERVER_FULL;
    }

    /* Check if already attached */
    for (size_t i = 0; i < subject->observer_count; i++) {
        if (observer_matches(&subject->observers[i], observer)) {
            return XY_OBSERVER_OK; /* Already attached */
        }
    }

    /* Add observer */
    memcpy(&subject->observers[subject->observer_count], observer, sizeof(*observer));
    subject->observer_count++;

    return XY_OBSERVER_OK;
}

int xy_subject_detach(xy_subject_t *subject, xy_observer_t *observer)
{
    if (!subject || !observer) {
        return XY_OBSERVER_INVALID_PARAM;
    }

    for (size_t i = 0; i < subject->observer_count; i++) {
        if (observer_matches(&subject->observers[i], observer)) {
            /* Remove observer by shifting remaining observers */
            for (size_t j = i; j < subject->observer_count - 1; j++) {
                memcpy(&subject->observers[j], &subject->observers[j + 1], sizeof(*observer));
            }
            subject->observer_count--;
            return XY_OBSERVER_OK;
        }
    }

    return XY_OBSERVER_NOT_FOUND;
}

int xy_subject_notify(xy_subject_t *subject, const void *data)
{
    xy_observer_t snapshot[XY_OBSERVER_MAX_OBSERVERS];
    size_t snapshot_count;

    if (!subject) {
        return XY_OBSERVER_INVALID_PARAM;
    }

    snapshot_count = subject->observer_count;
    memcpy(snapshot, subject->observers, snapshot_count * sizeof(snapshot[0]));
    subject->notifying = true;

    /* Notify the observers that were attached at the start of this cycle.
     * Reentrant detach suppresses callbacks later in the same cycle, while
     * reentrant attach is deferred until the next notification cycle.
     */
    for (size_t i = 0; i < snapshot_count; i++) {
        bool still_attached = false;

        for (size_t j = 0; j < subject->observer_count; j++) {
            if (observer_matches(&subject->observers[j], &snapshot[i])) {
                still_attached = true;
                break;
            }
        }

        if (still_attached && snapshot[i].active && snapshot[i].callback) {
            snapshot[i].callback(subject, data, snapshot[i].user_data);
        }
    }

    subject->notifying = false;

    return XY_OBSERVER_OK;
}

size_t xy_subject_observer_count(xy_subject_t *subject)
{
    if (!subject) {
        return 0;
    }

    return subject->observer_count;
}

int xy_subject_clear(xy_subject_t *subject)
{
    if (!subject) {
        return XY_OBSERVER_INVALID_PARAM;
    }

    memset(subject->observers, 0, sizeof(subject->observers));
    subject->observer_count = 0;
    subject->notifying = false;

    return XY_OBSERVER_OK;
}
