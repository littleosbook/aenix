#ifndef GDT_H
#define GDT_H

#define SEGSEL_KERNEL_CS 0x08
#define SEGSEL_KERNEL_DS 0x10

#define PL0 0x0
#define PL3 0x3

void gdt_init();

#endif /* GDT_H */

