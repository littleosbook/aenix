#ifndef PAGE_FRAME_ALLOCATOR_H
#define PAGE_FRAME_ALLOCATOR_H

#include "stdint.h"
#include "kernel.h"
#include "multiboot.h"

uint32_t pfa_init(multiboot_info_t const *mbinfo,
              kernel_meminfo_t const *mem,
              uint32_t fs_paddr, uint32_t fs_size);
uint32_t pfa_allocate(uint32_t num_page_frames);
void pfa_free(uint32_t paddr);
void pfa_free_cont(uint32_t paddr, uint32_t n);

#endif /* PAGE_FRAME_ALLOCATOR_H */
