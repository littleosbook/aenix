#ifndef PIT_H
#define PIT_H

#include "stdint.h"

void pit_init(void);
/* interval is in ms */
void pit_set_interval(uint32_t interval);

#endif /* PIT_H */
