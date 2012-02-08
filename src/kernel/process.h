#ifndef PROCESS_H
#define PROCESS_H

#include "stdint.h"
#include "inode.h"
#include "paging.h"

struct ps {
    pde_t *pdt;

    uint32_t pdt_paddr;
    uint32_t code_vaddr;
    uint32_t stack_vaddr;
    uint32_t heap_vaddr;

    uint32_t kernel_stack_vaddr;

    inode_t file_descriptors[3];
};
typedef struct ps ps_t;

ps_t *process_create(char *path);

#endif /* PROCESS_H */
