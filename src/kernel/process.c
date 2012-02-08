#include "process.h"
#include "fs.h"
#include "kmalloc.h"
#include "page_frame_allocator.h"
#include "string.h"
#include "kernel.h"
#include "common.h"
#include "log.h"
#include "mem.h"
#include "math.h"
#include "tss.h"
#include "gdt.h"

#define PROC_INITIAL_STACK_SIZE 1 /* in page frames */
#define PROC_INITIAL_STACK_VADDR (KERNEL_START_VADDR - FOUR_KB)
#define PROC_INITIAL_ESP (KERNEL_START_VADDR - 4)
#define PROC_INITIAL_HEAP_SIZE 1 /* in page frames */

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

ps_t *process_create(char *path)
{
    pde_t *pdt;
    uint32_t error, bytes, page_frames;
    uint32_t code_fs_vaddr, code_pfs, code_paddr, code_vaddr;
    uint32_t heap_vaddr, pdt_paddr;
    uint32_t mapped_memory_size;
    uint32_t kernel_stack_paddr, kernel_stack_vaddr;
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

    memcpy((void *) code_vaddr, (void *) code_fs_vaddr, node->size);

    log_debug("process_create",
              "code_vaddr[0]: %X, code_vaddr[1]: %X\n",
              *((uint8_t *) code_vaddr), *(((uint8_t *) code_vaddr) + 1));

    pdt_unmap_kernel_memory(code_vaddr, node->size);

    /* create proc early so that it is mapped both into the kernel PDT and
     * the proc PDT
     */
    proc = (ps_t *) kmalloc(sizeof(ps_t));
    if (proc == NULL) {
        log_error("create_process",
                  "kmalloc return NULL pointer for proc.\n");
        return NULL;
    }

    pdt = pdt_create(&pdt_paddr);
    if (pdt == NULL || pdt_paddr == 0) {
        log_error("create_process",
                  "Could not create PDT for process %s. "
                  "pdt: %X, pdt_paddr: %u\n",
                  path, (uint32_t) pdt, pdt_paddr);
        return NULL;
    }

    code_vaddr = 0;
    mapped_memory_size =
        pdt_map_memory(pdt, code_paddr, code_vaddr,
                       node->size, PAGING_READ_WRITE, PAGING_PL3);
    if (mapped_memory_size < node->size) {
        kfree(proc);
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
        kfree(proc);
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
        kfree(proc);
        log_error("create_process",
                  "Could not allocate and map memory for heap.\n");
        pdt_delete(pdt);
        log_info("create_process", "Process PDT deleted\n");
        return NULL;
    }

    page_frames = div_ceil(KERNEL_STACK_SIZE, FOUR_KB);
    kernel_stack_paddr = pfa_allocate(page_frames);
    if (kernel_stack_paddr == 0) {
        kfree(proc);
        log_error("create_process",
                  "Could not allocate page for kernel stack. "
                  "pfs: %u\n", page_frames);
        return NULL;
    }

    bytes = page_frames * FOUR_KB;
    kernel_stack_vaddr = pdt_kernel_find_next_vaddr(bytes);
    if (kernel_stack_vaddr == 0) {
        kfree(proc);
        log_error("create_process",
                  "Could not find virtual address for kernel stack."
                  "bytes: %u\n", bytes);
        return NULL;
    }

    mapped_memory_size =
        pdt_map_kernel_memory(kernel_stack_paddr, kernel_stack_vaddr, bytes,
                              PAGING_READ_WRITE, PAGING_PL0);
    if (mapped_memory_size != bytes) {
        kfree(proc);
        log_error("create_process",
                  "Could not map memory for kernel stack."
                  "kernel_stack_paddr: %X, kernel_stack_vaddr: %X, bytes: %u\n",
                  kernel_stack_paddr, kernel_stack_vaddr, bytes);
        return NULL;
    }

    proc->pdt = pdt;
    proc->pdt_paddr = pdt_paddr;
    proc->code_vaddr  = code_vaddr;
    proc->stack_vaddr = PROC_INITIAL_ESP;
    proc->heap_vaddr = heap_vaddr;
    proc->kernel_stack_vaddr = kernel_stack_vaddr;

    return proc;
}
