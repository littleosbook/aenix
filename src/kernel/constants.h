#ifndef CONSTANTS_H
#define CONSTANTS_H

/* numeric contants */
#define FOUR_KB     0x1000
#define ONE_MB      0x100000
#define FOUR_MB     0x400000
#define EIGHT_MB    0x800000

/* virtual memory */
#define KERNEL_START_VADDR  0xC0000000
#define KERNEL_PDT_IDX      (KERNEL_START_VADDR >> 22)

/* kernel stack */
#define KERNEL_STACK_SIZE FOUR_KB

/* interrupts */
#define SYSCALL_INT_IDX 0xAE

/* segements */
#define SEGSEL_KERNEL_CS 0x08
#define SEGSEL_KERNEL_DS 0x10
#define SEGSEL_USER_SPACE_CS 0x18
#define SEGSEL_USER_SPACE_DS 0x20

/* registers */
#define REG_EFLAGS_DEFAULT 0x202

#endif
