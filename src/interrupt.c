#include "stdint.h"
#include "fb.h"
#include "pic.h"
#include "keyboard.h"
#include "common.h"

struct irq_info {
	uint32_t idt_index;
	uint32_t error_code;
} __attribute__((packed));
typedef struct irq_info irq_info_t;

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

void interrupt_handler(cpu_state_t state, irq_info_t info)
{
	UNUSED_ARGUMENT(state);
	fb_puts("Interrupt number: ");
	fb_putui(info.idt_index);
	fb_puts("\n");
	if (info.idt_index == 0x21) {
		kbd_read();
	}
	if (info.idt_index >= 0x20 && info.idt_index < (0x20+16)) {
		pic_acknowledge();
	}
}
