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
#define PROC_INITIAL_STACK_SIZE_BYTES PROC_INITIAL_STACK_SIZE * FOUR_KB
#define PROC_INITIAL_STACK_VADDR (KERNEL_START_VADDR - FOUR_KB)
#define PROC_INITIAL_ESP (KERNEL_START_VADDR - 4)
#define PROC_INITIAL_HEAP_SIZE 1 /* in page frames */
#define PROC_INITIAL_HEAP_SIZE_BYTES PROC_INITIAL_HEAP_SIZE * FOUR_KB

struct ps {
    pde_t *pdt;

    uint32_t code_vaddr;
    uint32_t stack_vaddr;
    uint32_t heap_vaddr;
};

#include "process.h"

ps_t *create_process(char *path)
{
    pde_t *pdt;
    uint32_t code_fs_vaddr, code_pfs, code_paddr, code_vaddr;
    uint32_t stack_paddr;
    uint32_t heap_paddr, heap_vaddr;
    uint32_t mapped_memory_size;
    /*ps_t *proc;*/

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

    stack_paddr = pfa_allocate(PROC_INITIAL_STACK_SIZE);
    if (stack_paddr == 0) {
        log_error("create_process",
                  "Could not allocate page frames for process stack. "
                  "paddr: %X, pfs: %u\n",
                  stack_paddr, PROC_INITIAL_STACK_SIZE);
        return NULL;
    }

    mapped_memory_size = pdt_map_memory(pdt, stack_paddr,
                                        PROC_INITIAL_STACK_VADDR,
                                        PROC_INITIAL_STACK_SIZE_BYTES,
                                        PAGING_READ_WRITE, PAGING_PL3);

    if (mapped_memory_size < PROC_INITIAL_STACK_SIZE_BYTES) {
        log_error("create_process",
                  "Could not map memory in proc PDT for stack. "
                  "vaddr: %X, paddr: %X, size: %u, pdt: %X\n",
                  PROC_INITIAL_STACK_VADDR, stack_paddr,
                  PROC_INITIAL_STACK_SIZE_BYTES, (uint32_t) pdt);
        pdt_delete(pdt);
        log_info("create_process", "Proc PDT deleted\n");
        return NULL;
    }

    heap_paddr = pfa_allocate(PROC_INITIAL_HEAP_SIZE);
    if (heap_paddr == 0) {
        log_error("create_process",
                  "Could not allocate page frames for process heap. "
                  "paddr: %X, pfs: %u\n",
                  heap_paddr, PROC_INITIAL_HEAP_SIZE);
        return NULL;
    }

    heap_vaddr = code_vaddr + code_pfs * FOUR_KB;
    mapped_memory_size = pdt_map_memory(pdt, heap_paddr, heap_vaddr,
                                        PROC_INITIAL_HEAP_SIZE_BYTES,
                                        PAGING_READ_WRITE, PAGING_PL3);

    if (mapped_memory_size < PROC_INITIAL_HEAP_SIZE_BYTES) {
        log_error("create_process",
                  "Could not map memory in proc PDT for heap. "
                  "vaddr: %X, paddr: %X, size: %u, pdt: %X\n",
                  heap_vaddr, heap_paddr,
                  PROC_INITIAL_HEAP_SIZE_BYTES, (uint32_t) pdt);
        pdt_delete(pdt);
        log_info("create_process", "Proc PDT deleted\n");
        return NULL;
    }


    return NULL;

    /*proc = (ps_t *) kmalloc(sizeof(ps_t));*/
    /*proc->pdt = pdt;*/
    /*proc->code_addr  = code_virtual_addr;*/
    /*proc->stack_addr = KERNEL_VIRTUAL_ADDRESS - 4;*/

    /*next_ps_start_addr = NEXT_ADDR(next_ps_start_addr + total_mapped_size);*/

    /*log_printf("it's a wrap!\n");*/

    /*return proc;*/
}
