#ifndef KERNEL_H
#define KERNEL_H

#include "stdint.h"

struct kernel_meminfo {
    uint32_t kernel_physical_start;
    uint32_t kernel_physical_end;
    uint32_t kernel_virtual_start;
    uint32_t kernel_virtual_end;
} __attribute__((packed));
typedef struct kernel_meminfo kernel_meminfo_t;

#endif /* KERNEL_H */
