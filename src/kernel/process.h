#ifndef PROCESS_H
#define PROCESS_H

#include "stdint.h"
#include "vnode.h"
#include "paging.h"

#define PROCESS_MAX_NUM_FD 3

struct fd {
    vnode_t *vnode;
};
typedef struct fd fd_t;

struct ps {
    uint32_t id;

    pde_t *pdt;

    uint32_t pdt_paddr;
    uint32_t code_vaddr;
    uint32_t stack_vaddr;

    uint32_t kernel_stack_vaddr;

    fd_t file_descriptors[PROCESS_MAX_NUM_FD];
};
typedef struct ps ps_t;

ps_t *process_create(char const *path, uint32_t id);
int process_replace(ps_t *ps, char const *path);

#endif /* PROCESS_H */
