#ifndef PAGE_FRAME_ALLOCATOR_H
#define PAGE_FRAME_ALLOCATOR_H

#include "stdint.h"
#include "kernel.h"
#include "multiboot.h"

void pfa_init(const multiboot_info_t *mbinfo, kernel_meminfo_t *mem);

#endif /* PAGE_FRAME_ALLOCATOR_H */
