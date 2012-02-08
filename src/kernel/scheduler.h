#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"

void scheduler_switch_to_process(ps_t *ps);
ps_t *scheduler_get_current_process();

#endif /* SCHEDULER_H */
