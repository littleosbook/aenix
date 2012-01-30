#include "paging.h"
#include "fs.h"
#include "kmalloc.h"
#include "string.h"
#include "kernel.h"
#include "common.h"
#include "log.h"

#define PROCESS_MEMORY_SIZE 4096 /* in bytes, 4kB heap and stack per process */

struct ps {
    pde_t *pdt;
    uint32_t code_addr;
    uint32_t stack_addr;
};

#include "process.h"

static uint32_t next_ps_start_addr; /* physical address */

void ps_init(uint32_t addr)
{
    next_ps_start_addr = addr;
}

ps_t *create_process(char *path)
{
    pde_t *pdt;
    uint32_t mapped_memory_size, total_mapped_size = 0;
    ps_t *proc;
    uint32_t code_fs_addr, code_virtual_addr = 0;

    inode_t *node = fs_find_inode(path);
    if (node == NULL) {
        return NULL;
    }
    code_fs_addr = fs_get_addr(node);

    mapped_memory_size =
        pdt_map_kernel_memory(next_ps_start_addr, code_virtual_addr,
                              node->size, PAGING_READ_WRITE, PAGING_PL0);

    if (mapped_memory_size < node->size) {
        pdt_unmap_kernel_memory(code_virtual_addr, node->size);
        log_printf("mapped_memory_size: %u, node->size: %u\n", mapped_memory_size, node->size);
        return NULL;
    }

    log_printf("mapped kernel memory\n");

    memcpy((void *) code_virtual_addr, (void *) code_fs_addr, node->size);

    log_printf("transferred code\n");

    pdt_unmap_kernel_memory(code_virtual_addr, node->size);

    log_printf("unmapped kernel pages\n");

    pdt = pdt_create();
    if (pdt == NULL) {
        return NULL;
    }

    log_printf("created_pdt\n");

    mapped_memory_size =
        pdt_map_memory(pdt, next_ps_start_addr, code_virtual_addr,
                       node->size, PAGING_READ_ONLY, PAGING_PL3);
    if (mapped_memory_size < node->size) {
        pdt_delete(pdt);
        return NULL;
    }
    total_mapped_size += mapped_memory_size;

    mapped_memory_size =
        pdt_map_memory(pdt, next_ps_start_addr + mapped_memory_size,
                       KERNEL_VIRTUAL_ADDRESS - PROCESS_MEMORY_SIZE,
                       PROCESS_MEMORY_SIZE, PAGING_READ_WRITE, PAGING_PL3);
    if (mapped_memory_size < PROCESS_MEMORY_SIZE) {
        pdt_delete(pdt);
        return NULL;
    }
    total_mapped_size += mapped_memory_size;

    log_printf("mapped all pages\n");

    proc = (ps_t *) kmalloc(sizeof(ps_t));
    proc->pdt = pdt;
    proc->code_addr  = code_virtual_addr;
    proc->stack_addr = KERNEL_VIRTUAL_ADDRESS - 4;

    next_ps_start_addr = NEXT_ADDR(next_ps_start_addr + total_mapped_size);

    log_printf("it's a wrap!\n");

    return proc;
}

uint32_t get_code_addr(ps_t *ps)
{
    return ps->code_addr;
}

uint32_t get_stack_addr(ps_t *ps)
{
    return ps->stack_addr;
}
