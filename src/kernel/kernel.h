#ifndef KERNEL_H
#define KERNEL_H

#include "stdint.h"

#define KERNEL_VIRTUAL_ADDRESS 0xC040000

struct kernel_meminfo {
    uint32_t kernel_physical_start;
    uint32_t kernel_physical_end;
    uint32_t kernel_virtual_start;
    uint32_t kernel_virtual_end;
} __attribute__((packed));
typedef struct kernel_meminfo kernel_meminfo_t;

#endif /* KERNEL_H */
