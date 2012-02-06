#ifndef TSS_H
#define TSS_H

#include "stdint.h"

struct tss {
    uint16_t prev_task_link;
    uint16_t reserved;
    uint32_t esp0;
    uint16_t ss0;
    uint16_t reserved0;
    uint32_t esp1;
    uint16_t ss1;
    uint16_t reserved1;
    uint32_t esp2;
    uint16_t ss2;
    uint16_t reserved2;

    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;

    uint16_t es;
    uint16_t reserved3;
    uint16_t cs;
    uint16_t reserved4;
    uint16_t ss;
    uint16_t reserved5;
    uint16_t ds;
    uint16_t reserved6;
    uint16_t fs;
    uint16_t reserved7;
    uint16_t gs;
    uint16_t reserved8;

    uint16_t ldt_ss;
    uint16_t reserved9;

    uint16_t debug_and_reserved; /* The lowest bit is for debug */
    uint16_t io_map_base;

} __attribute__((packed));

typedef struct tss tss_t;

uint32_t tss_init();

void tss_load_and_set(uint16_t tss_segsel); /* defined in tss_asm.s */

void tss_set_kernel_stack(uint16_t segsel, uint32_t vaddr);

#endif /* TSS_H */
