#include "interrupt.h"
#include "stdint.h"
#include "pic.h"
#include "keyboard.h"
#include "common.h"
#include "pit.h"
#include "stdio.h"
#include "log.h"

struct idt_info {
	uint32_t idt_index;
	uint32_t error_code;
} __attribute__((packed));
typedef struct idt_info idt_info_t;

struct cpu_state {
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
} __attribute__((packed));
typedef struct cpu_state cpu_state_t;

struct instr_state {
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
} __attribute__((packed));
typedef struct instr_state instr_state_t;

void print_keyboard_input(void)
{
    uint8_t ch = kbd_scan_code_to_ascii(kbd_read_scan_code());
    if (ch != 0) {
        printf("%c", ch);
    }
}

void interrupt_handler(cpu_state_t state, idt_info_t info, instr_state_t instr)
{
    UNUSED_ARGUMENT(state);

    if (info.idt_index == KEYBOARD_INTERRUPT_INDEX) {
        print_keyboard_input();
    }

    if (info.idt_index == TIMER_INTERRUPT_INDEX) {
        pit_handle_interrupt();
    }

    if (info.idt_index == COM1_INTERRUPT_INDEX) {
        printf("data on com1\n");
    }

    if (info.idt_index >= PIC1_START &&
        info.idt_index < (PIC1_START + PIC_NUM_IRQS)) {
        pic_acknowledge();
    }

    if (info.idt_index == SYS_CALL_INTERRUPT_INDEX) {
        log_debug("interrupt_handler", "sys call interrupt\n");
    }

    log_debug("interrupt_handler",
              "interrupt: %u, eip: %X, cs: %X, eflags: %X\n",
              info.idt_index, instr.eip, instr.cs, instr.eflags);

}
