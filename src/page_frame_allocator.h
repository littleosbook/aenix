#ifndef PAGE_FRAME_ALLOCATOR_H
#define PAGE_FRAME_ALLOCATOR_H

#include "stdint.h"
#include "kernel.h"
#include "multiboot.h"

void pfa_init(const multiboot_info_t *mbinfo, kernel_meminfo_t *mem);
uint32_t pfa_allocate(uint32_t num_page_frames);
void pfa_free(uint32_t paddr);

#endif /* PAGE_FRAME_ALLOCATOR_H */
