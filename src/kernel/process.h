#ifndef PROCESS_H
#define PROCESS_H

#include "stdint.h"
#include "vnode.h"
#include "paging.h"

#define PROCESS_MAX_NUM_FD 64

struct paddr_list {
    uint32_t paddr;
    uint32_t count;
    struct paddr_list *next;
};
typedef struct paddr_list paddr_list_t;


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

    paddr_list_t *code_paddrs;
    paddr_list_t *stack_paddrs;
    paddr_list_t *kernel_stack_paddrs;
};
typedef struct ps ps_t;

ps_t *process_create(char const *path, uint32_t id);
ps_t *process_replace(ps_t *ps, char const *path);
void process_delete(ps_t *ps);
ps_t *process_create_fork(ps_t *parent, char const *path);

#endif /* PROCESS_H */
