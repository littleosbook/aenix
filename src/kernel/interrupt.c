#include "interrupt.h"
#include "stddef.h"
#include "pic.h"
#include "keyboard.h"
#include "common.h"
#include "pit.h"
#include "stdio.h"
#include "log.h"
#include "constants.h"

static interrupt_handler_t interrupt_handlers[IDT_NUM_ENTRIES];

uint32_t register_interrupt_handler(uint32_t interrupt,
                                    interrupt_handler_t handler)
{
    if (interrupt > 255) {
        return 1;
    }
    if (interrupt == SYSCALL_INT_IDX) {
        return 1;
    }
    if (interrupt_handlers[interrupt] != NULL) {
        return 1;
    }

    interrupt_handlers[interrupt] = handler;
    return 0;
}

void interrupt_handler(cpu_state_t state, idt_info_t info, stack_state_t exec)
{
    if (interrupt_handlers[info.idt_index] != NULL) {
        interrupt_handlers[info.idt_index](state, info, exec);
    } else {
        log_info("interrupt_handler",
                  "unhandled interrupt: %u, eip: %X, cs: %X, eflags: %X\n",
                  info.idt_index, exec.eip, exec.cs, exec.eflags);
    }
}
