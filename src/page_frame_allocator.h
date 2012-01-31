#ifndef PAGE_FRAME_ALLOCATOR_H
#define PAGE_FRAME_ALLOCATOR_H

#include "stdint.h"

#define MEMORY_MAP_MAPPED_FLAG  0x01

struct memory_map {
    uint32_t addr;
    uint32_t len;
    uint8_t flags;
};
typedef struct memory_map memory_map_t;

void pfa_init(memory_map_t *mmap, uint32_t n);

#endif /* PAGE_FRAME_ALLOCATOR_H */
