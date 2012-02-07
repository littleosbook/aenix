#include "idt.h"
#include "stdint.h"
#include "gdt.h"
#include "interrupt.h"
#include "constants.h"

#define IDT_INTERRUPT_GATE_TYPE 0
#define IDT_TRAP_GATE_TYPE		1

#define IDT_TIMER_INTERRUPT_INDEX    0x20
#define IDT_KEYBOARD_INTERRUPT_INDEX 0x21

#define CREATE_IDT_GATE(idx) \
    create_idt_gate(idx, (uint32_t) &interrupt_handler_##idx,\
                    IDT_TRAP_GATE_TYPE, PL0);

#define DECLARE_INTERRUPT_HANDLER(i) void interrupt_handler_##i(void)

void debug_handler(void);
/* Protected mode exceptions interrupts */
DECLARE_INTERRUPT_HANDLER(0);
DECLARE_INTERRUPT_HANDLER(1);
DECLARE_INTERRUPT_HANDLER(2);
DECLARE_INTERRUPT_HANDLER(3);
DECLARE_INTERRUPT_HANDLER(4);
DECLARE_INTERRUPT_HANDLER(5);
DECLARE_INTERRUPT_HANDLER(6);
DECLARE_INTERRUPT_HANDLER(7);
DECLARE_INTERRUPT_HANDLER(8);
DECLARE_INTERRUPT_HANDLER(9);
DECLARE_INTERRUPT_HANDLER(10);
DECLARE_INTERRUPT_HANDLER(11);
DECLARE_INTERRUPT_HANDLER(12);
DECLARE_INTERRUPT_HANDLER(13);
DECLARE_INTERRUPT_HANDLER(14);
DECLARE_INTERRUPT_HANDLER(15);
DECLARE_INTERRUPT_HANDLER(16);
DECLARE_INTERRUPT_HANDLER(17);
DECLARE_INTERRUPT_HANDLER(18);
DECLARE_INTERRUPT_HANDLER(19);

/* IRQs */
DECLARE_INTERRUPT_HANDLER(32);
DECLARE_INTERRUPT_HANDLER(33);
DECLARE_INTERRUPT_HANDLER(34);
DECLARE_INTERRUPT_HANDLER(35);
DECLARE_INTERRUPT_HANDLER(36);
DECLARE_INTERRUPT_HANDLER(37);
DECLARE_INTERRUPT_HANDLER(38);
DECLARE_INTERRUPT_HANDLER(39);
DECLARE_INTERRUPT_HANDLER(40);
DECLARE_INTERRUPT_HANDLER(41);
DECLARE_INTERRUPT_HANDLER(42);
DECLARE_INTERRUPT_HANDLER(43);
DECLARE_INTERRUPT_HANDLER(44);
DECLARE_INTERRUPT_HANDLER(45);
DECLARE_INTERRUPT_HANDLER(46);
DECLARE_INTERRUPT_HANDLER(47);

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

/* external assembly function for handling syscalls */
void handle_syscall(void);

static void create_idt_gate(uint8_t n, uint32_t handler, uint8_t type,
                            uint8_t pl);

void idt_init(void)
{
    idt_ptr_t idt_ptr;
    idt_ptr.limit = IDT_NUM_ENTRIES * sizeof(idt_gate_t) - 1;
    idt_ptr.base  = (uint32_t) &idt;

    /* Protected mode exceptions */
    CREATE_IDT_GATE(0);
    CREATE_IDT_GATE(1);
    CREATE_IDT_GATE(2);
    CREATE_IDT_GATE(3);
    CREATE_IDT_GATE(4);
    CREATE_IDT_GATE(5);
    CREATE_IDT_GATE(6);
    CREATE_IDT_GATE(7);
    CREATE_IDT_GATE(8);
    CREATE_IDT_GATE(9);
    CREATE_IDT_GATE(10);
    CREATE_IDT_GATE(11);
    CREATE_IDT_GATE(12);
    CREATE_IDT_GATE(13);
    CREATE_IDT_GATE(14);
    CREATE_IDT_GATE(15);
    CREATE_IDT_GATE(16);
    CREATE_IDT_GATE(17);
    CREATE_IDT_GATE(18);
    CREATE_IDT_GATE(19);

    /* IRQs */
    CREATE_IDT_GATE(32);
    CREATE_IDT_GATE(33);
    CREATE_IDT_GATE(34);
    CREATE_IDT_GATE(35);
    CREATE_IDT_GATE(36);
    CREATE_IDT_GATE(37);
    CREATE_IDT_GATE(38);
    CREATE_IDT_GATE(39);
    CREATE_IDT_GATE(40);
    CREATE_IDT_GATE(41);
    CREATE_IDT_GATE(42);
    CREATE_IDT_GATE(43);
    CREATE_IDT_GATE(44);
    CREATE_IDT_GATE(45);
    CREATE_IDT_GATE(46);
    CREATE_IDT_GATE(47);

    /* System call interrupt */
    create_idt_gate(SYSCALL_INT_IDX, (uint32_t) handle_syscall,
                    IDT_TRAP_GATE_TYPE, PL3);

    idt_load_and_set((uint32_t) &idt_ptr);
}

static void create_idt_gate(uint8_t n, uint32_t handler, uint8_t type,
                            uint8_t pl)
{
    idt[n].handler_low = handler & 0x0000FFFF;
    idt[n].handler_high = (handler >> 16) & 0x0000FFFF;

    idt[n].segsel = SEGSEL_KERNEL_CS;
    idt[n].zero = 0;

    /* name | value | size | desc
     * --------------------------
     * P    |     1 |    1 | segment present in memory
     * DPL  |    pl |    2 | privilege level
     * 0    |     0 |    1 | a zero bit
     * D    |     1 |    1 | size of gate, 1 = 32 bits, 0 = 16 bits
     * 1    |     1 |    1 | a one bit
     * 1    |     1 |    1 | a one bit
     * T    |  type |    1 | the type of the gate, 1 = trap, 0 = interrupt
     */
    idt[n].config =
        (0x01 << 7)          |
        ((pl & 0x03)  << 5)  |
        (0x01 << 3)          |
        (0x01 << 2)          |
        (0x01 << 1)          |
        type;
}

