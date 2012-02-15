#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "stdint.h"
#include "process.h"

uint32_t scheduler_next_pid(void);

int scheduler_add_runnable_process(ps_t *ps);
int scheduler_replace_process(ps_t *old, ps_t *new);
void scheduler_terminate_process(ps_t *ps);

void scheduler_schedule(void);
ps_t *scheduler_get_current_process();

#endif /* SCHEDULER_H */
