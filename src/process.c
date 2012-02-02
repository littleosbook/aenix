#include "paging.h"
#include "fs.h"
#include "kmalloc.h"
#include "page_frame_allocator.h"
#include "string.h"
#include "kernel.h"
#include "common.h"
#include "log.h"
#include "mem.h"
#include "math.h"

#define PROC_INITIAL_STACK_SIZE 1 /* in page frames */
#define PROC_INITIAL_STACK_VADDR (KERNEL_START_VADDR - FOUR_KB)
#define PROC_INITIAL_ESP (KERNEL_START_VADDR - 4)
#define PROC_INITIAL_HEAP_SIZE 1 /* in page frames */

struct ps {
    pde_t *pdt;

    uint32_t code_vaddr;
    uint32_t stack_vaddr;
    uint32_t heap_vaddr;
};

#include "process.h"

static uint32_t allocate_and_map(pde_t *pdt,
                                 uint32_t vaddr,
                                 uint32_t pfs,
                                 uint8_t rw,
                                 uint8_t pl)
{
    uint32_t mapped_memory_size;
    uint32_t paddr = pfa_allocate(pfs);
    uint32_t byte_size = pfs * FOUR_KB;
    if (paddr == 0) {
        log_error("allocate_and_map",
                  "Could not allocate page frames. "
                  "paddr: %X, pfs: %u\n",
                  paddr, pfs);
        return 1;
    }

    mapped_memory_size = pdt_map_memory(pdt, paddr, vaddr, byte_size, rw, pl);

    if (mapped_memory_size < byte_size) {
        log_error("allocate_and_map",
                  "Could not map memory in given pdt. "
                  "vaddr: %X, paddr: %X, size: %u, pdt: %X\n",
                  vaddr, paddr, byte_size, (uint32_t) pdt);
        return 1;
    }

    return 0;
}

ps_t *create_process(char *path)
{
    pde_t *pdt;
    uint32_t error;
    uint32_t code_fs_vaddr, code_pfs, code_paddr, code_vaddr;
    uint32_t heap_vaddr;
    uint32_t mapped_memory_size;
    ps_t *proc;

    inode_t *node = fs_find_inode(path);
    if (node == NULL) {
        log_error("create_process", "Could not find path to process %s\n",
                   path);
        return NULL;
    }
    code_fs_vaddr = fs_get_addr(node);
    code_pfs = div_ceil(node->size, FOUR_KB);

    code_paddr = pfa_allocate(code_pfs);

    if (code_paddr == 0) {
        log_error("create_process",
                  "Could not allocate page frames for process code. "
                  "size: %u, pfs: %u\n",
                  node->size, code_pfs);
        return NULL;
    }

    code_vaddr = pdt_kernel_find_next_vaddr(node->size);

    if (code_vaddr == 0) {
        log_error("create_process",
                  "Could not find virtual memory for proc code in kernel. "
                  "paddr: %X, size: %u\n", code_paddr, node->size);
        return NULL;
    }

    mapped_memory_size =
        pdt_map_kernel_memory(code_paddr, code_vaddr,
                              node->size, PAGING_READ_WRITE, PAGING_PL0);

    if (mapped_memory_size < node->size) {
        pdt_unmap_kernel_memory(code_vaddr, node->size);
        log_error("create_process",
                  "Could not map memory in kernel. "
                  "vaddr: %X, paddr: %X, size: %u\n",
                  code_vaddr, code_paddr, node->size);
        return NULL;
    }

    log_debug("create_process", "mapped kernel memory\n");

    memcpy((void *) code_vaddr, (void *) code_fs_vaddr, node->size);

    log_debug("create_process", "transferred code\n");

    pdt_unmap_kernel_memory(code_vaddr, node->size);

    log_debug("create_process", "unmapped kernel pages\n");

    pdt = pdt_create();
    if (pdt == NULL) {
        log_error("create_process",
                  "Could not create PDT for process %s\n",
                  path);
        return NULL;
    }

    log_debug("create_process", "created_pdt\n");

    code_vaddr = 0;
    mapped_memory_size =
        pdt_map_memory(pdt, code_paddr, code_vaddr,
                       node->size, PAGING_READ_WRITE, PAGING_PL3);
    if (mapped_memory_size < node->size) {
        log_error("create_process",
                  "Could not map memory in proc PDT. "
                  "vaddr: %X, paddr: %X, size: %u, pdt: %X\n",
                  code_vaddr, code_paddr, node->size, (uint32_t) pdt);
        pdt_delete(pdt);
        log_info("create_process", "Proc PDT deleted\n");
        return NULL;
    }

    error = allocate_and_map(pdt, PROC_INITIAL_STACK_VADDR,
                             PROC_INITIAL_STACK_SIZE, PAGING_READ_WRITE,
                             PAGING_PL3);

    if (error) {
        log_error("create_process",
                  "Could not allocate and map memory for stack.\n");
        pdt_delete(pdt);
        log_info("create_process", "Process PDT deleted\n");
        return NULL;
    }

    heap_vaddr = code_vaddr + code_pfs * FOUR_KB;
    error =
        allocate_and_map(pdt, heap_vaddr, PROC_INITIAL_HEAP_SIZE,
                         PAGING_READ_WRITE, PAGING_PL3);
    if (error) {
        log_error("create_process",
                  "Could not allocate and map memory for heap.\n");
        pdt_delete(pdt);
        log_info("create_process", "Process PDT deleted\n");
        return NULL;
    }

    proc = (ps_t *) kmalloc(sizeof(ps_t));
    if (proc == NULL) {
        log_error("create_process",
                  "kmalloc return NULL pointer for proc.\n");
        return NULL;
    }

    proc->pdt = pdt;
    proc->code_vaddr  = code_vaddr;
    proc->stack_vaddr = PROC_INITIAL_STACK_VADDR;
    proc->heap_vaddr = heap_vaddr;

    log_debug("create_process", "all done!\n");

    return proc;
}
