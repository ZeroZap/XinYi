
#include "sm.h"

void xy_sm_init(xy_sm_t *self)
{
    self->process         = NULL;
    self->exit            = NULL;
    self->timeout_entry   = NULL;
    self->timeout_process = NULL;
    self->timeout_exit    = NULL;
    self->timeout_counter = 0;
}

void xy_sm_transition(xy_sm_t *self, xy_sm_fn *entry, xy_sm_fn *process, xy_sm_fn *exit)
{
    if (self->exit) {
        self->exit(self);
    }

    self->process         = process;
    self->exit            = exit;
    self->timeout_entry   = NULL;
    self->timeout_process = NULL;
    self->timeout_exit    = NULL;
    self->timeout_counter = 0;

    if (entry) {
        entry(self);
    }
}

void xy_sm_transition_timeout(xy_sm_t *self, xy_sm_fn *entry, xy_sm_fn *process,
                           xy_sm_fn *exit, xy_sm_fn *timeout_entry,
                           xy_sm_fn *timeout_process, xy_sm_fn *timeout_exit,
                           size_t timeout)
{
    xy_sm_transition(self, entry, process, exit);
    self->timeout_entry   = timeout_entry;
    self->timeout_process = timeout_process;
    self->timeout_exit    = timeout_exit;
    self->timeout_counter = timeout;
}

void xy_sm_transition_delay(xy_sm_t *self, xy_sm_fn *timeout_entry,
                         xy_sm_fn *timeout_process, xy_sm_fn *timeout_exit,
                         size_t timeout)
{
    xy_sm_transition_timeout(self, NULL, NULL, NULL, timeout_entry,
                          timeout_process, timeout_exit, timeout);
}


void xy_sm_process_sample(xy_sm_t *self, size_t timeout)
{
#if 0
xy_sm_process_begin(self);

xy_sm_process_end(self);
#else
    if (self->process) {
        self->process(self);
    }

    if (self->timeout_process) {
        if (self->timeout_counter >= timeout) {
            xy_sm_transition(self->timeout_entry, self->timeout_process,
                          self->timeout_exit, self->timeout);
        } else {
            // TODO add elapsed tick
            self->timeout += 1;
        }
    }
#endif
}