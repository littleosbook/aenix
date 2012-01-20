#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "pic.h"

#define TIMER_INTERRUPT_INDEX    PIC1_START
#define KEYBOARD_INTERRUPT_INDEX PIC1_START + 1

void enable_interrupts(void);
void disable_interrupts(void);

#endif /* INTERRUPT_H */
