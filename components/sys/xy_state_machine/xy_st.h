#ifndef _STATE_MACHINE_H_
#define _STATE_MACHINE_H_
typedef struct state_machine xy_sm_t;
typedef void xy_sm_fn(xy_sm_t *self);

struct state_machine {
    xy_sm_fn *process;
    xy_sm_fn *exit;
    xy_sm_fn *timeout_entry;
    xy_sm_fn *timeout_process;
    xy_sm_fn *timeout_exit;
    size_t timeout_counter;
};

void xy_sm_init(xy_sm_t *self);
void xy_sm_transition(xy_sm_t *self, xy_sm_fn *entry, xy_sm_fn *process, xy_sm_fn *exit);
void xy_sm_transition_timeout(xy_sm_t *self, xy_sm_fn *entry, xy_sm_fn *process,
                           xy_sm_fn *exit, xy_sm_fn *timeout_entry,
                           xy_sm_fn *timeout_process, xy_sm_fn *timeout_exit,
                           size_t timeout);
void xy_sm_transition_delay(xy_sm_t *self, xy_sm_fn *timeout_entry,
                         xy_sm_fn *timeout_process, xy_sm_fn *timeout_exit,
                         size_t timeout);

void xy_sm_process_sample(xy_sm_t *self);

inline void xy_sm_process_begin(xy_sm_t *self);

inline void xy_sm_process_end(xy_sm_t *self);
#endif