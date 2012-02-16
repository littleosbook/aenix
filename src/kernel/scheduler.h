#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "stdint.h"
#include "process.h"

uint32_t scheduler_next_pid(void);

int scheduler_init(void);

int scheduler_add_runnable_process(ps_t *ps);
int scheduler_replace_process(ps_t *old, ps_t *new);
void scheduler_terminate_process(ps_t *ps);
int scheduler_has_any_child_terminated(ps_t *parent);
int scheduler_num_children(uint32_t pid);

void scheduler_schedule(void);
ps_t *scheduler_get_current_process();

void snapshot_and_schedule(registers_t *current);

#endif /* SCHEDULER_H */
