#include "idt.h"
#include "stdint.h"
#include "gdt.h"
#include "interrupt.h"

#define IDT_NUM_ENTRIES 256

#define IDT_INTERRUPT_GATE_TYPE 0
#define IDT_TRAP_GATE_TYPE		1

#define IDT_TIMER_INTERRUPT_INDEX    0x20
#define IDT_KEYBOARD_INTERRUPT_INDEX 0x21

struct idt_gate {
	uint16_t handler_low;
	uint16_t segsel;
	uint8_t zero;
	uint8_t config;
	uint16_t handler_high;
} __attribute__((packed)); 
typedef struct idt_gate idt_gate_t;

struct idt_ptr {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed));
typedef struct idt_ptr idt_ptr_t;

idt_gate_t idt[IDT_NUM_ENTRIES];

/* external assembly function for loading the ldt */
void idt_load_and_set(uint32_t idt_ptr);

static void create_idt_gate(uint8_t n, uint32_t handler, uint8_t type);

void idt_init(void)
{
    idt_ptr_t idt_ptr;
    int i;
    idt_ptr.limit = IDT_NUM_ENTRIES * sizeof(idt_gate_t) - 1;
    idt_ptr.base  = (uint32_t) &idt;
    for (i = 0; i < 20; ++i) {
        create_idt_gate(i, (uint32_t) &handle_cpu_int, IDT_TRAP_GATE_TYPE);
    }
    for (i = 0x20; i < 0x20 + 16; ++i) {
        create_idt_gate(i, (uint32_t) &handle_pic_int, IDT_TRAP_GATE_TYPE);
    }
    idt_load_and_set((uint32_t) &idt_ptr);
}

static void create_idt_gate(uint8_t n, uint32_t handler, uint8_t type)
{
    idt[n].handler_low = handler & 0x0000FFFF;
    idt[n].handler_high = (handler >> 16) & 0x0000FFFF;

    idt[n].segsel = SEGSEL_KERNEL_CS;
    idt[n].zero = 0;

    /* name | value | size | desc
     * --------------------------
     * P    |     1 |    1 | segment present in memory
     * DPL  |   PL0 |    2 | privilege level
     * 0    |     0 |    1 | a zero bit
     * D    |     1 |    1 | size of gate, 1 = 32 bits, 0 = 16 bits
     * 1    |     1 |    1 | a one bit
     * 1    |     1 |    1 | a one bit
     * T    |  type |    1 | the type of the gate, 1 = trap, 0 = interrupt
     */
    idt[n].config = 
        (0x01 << 7)          | 
        ((PL0 & 0x03)  << 5) | 
        (0x01 << 3)          | 
        (0x01 << 2)          | 
        (0x01 << 1)          | 
        type;
}
