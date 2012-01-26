#ifndef INTERRUPT_H
#define INTERRUPT_H

#define TIMER_INTERRUPT_INDEX    PIC1_START
#define KEYBOARD_INTERRUPT_INDEX PIC1_START + 1
#define COM1_INTERRUPT_INDEX     PIC1_START + 4
#define SYS_CALL_INTERRUPT_INDEX 0xAE

void enable_interrupts(void);
void disable_interrupts(void);

#endif /* INTERRUPT_H */
