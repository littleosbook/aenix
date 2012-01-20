#ifndef PIT_H
#define PIT_H

#include "stdint.h"

void pit_init(void);
void pit_set_callback(uint16_t interval, void (*callback)(void));
void pit_handle_interrupt(void);

#endif /* PIT_H */
