#ifndef PROCESS_H
#define PROCESS_H

#include "stdint.h"
#include "vnode.h"
#include "paging.h"

#define PROCESS_MAX_NUM_FD 64

/* do not change order of variables in the struct, asm code depends on it! */
struct registers {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t esp;
    uint32_t eflags;
    uint32_t eip;
} __attribute__((packed));
typedef struct registers registers_t;

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

    registers_t registers;

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
ps_t *process_create_replacement(ps_t *parent, char const *path);

#endif /* PROCESS_H */
