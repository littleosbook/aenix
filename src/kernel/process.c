#include "process.h"
#include "vfs.h"
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

static int fill_paddr_lists(ps_t *ps, uint32_t code_paddr, uint32_t code_pfs,
                             uint32_t stack_paddr, uint32_t stack_pfs)
{
    ps->code_paddrs = kmalloc(sizeof(paddr_list_t));
    if (ps->code_paddrs == NULL) {
        log_error("fill_paddr_lists",
                  "Couldn't kmalloc memory for code_paddrs\n");
        return -1;
    }
    ps->code_paddrs->paddr = code_paddr;
    ps->code_paddrs->count = code_pfs;
    ps->code_paddrs->next = NULL;

    ps->stack_paddrs = kmalloc(sizeof(paddr_list_t));
    if (ps->stack_paddrs == NULL) {
        log_error("fill_paddr_lists",
                  "Couldn't kmalloc memory for stack_paddrs\n");
        return -1;
    }
    ps->stack_paddrs->paddr = stack_paddr;
    ps->stack_paddrs->count = stack_pfs;
    ps->stack_paddrs->next = NULL;

    ps->heap_paddrs = NULL;

    return 0;
}

ps_t *process_create(char const *path, uint32_t id)
{
    pde_t *pdt;
    uint32_t bytes, page_frames;
    uint32_t code_pfs, code_paddr, code_vaddr, stack_paddr;
    uint32_t pdt_paddr;
    uint32_t mapped_memory_size;
    uint32_t kernel_stack_paddr, kernel_stack_vaddr;
    ps_t *proc;

    vnode_t node;
    if (vfs_lookup(path, &node)) {
        log_error("create_process",
                  "Could not find vnode for path: %s\n", path);
        return NULL;
    }

    vattr_t attr;
    if (vfs_getattr(&node, &attr)) {
        log_error("create_process",
                  "Could not get attributes for path: %s\n", path);
        return NULL;
    }

    code_pfs = div_ceil(attr.file_size, FOUR_KB);

    code_paddr = pfa_allocate(code_pfs);

    if (code_paddr == 0) {
        log_error("create_process",
                  "Could not allocate page frames for process code. "
                  "size: %u, pfs: %u\n",
                  attr.file_size, code_pfs);
        return NULL;
    }

    code_vaddr = pdt_kernel_find_next_vaddr(attr.file_size);

    if (code_vaddr == 0) {
        log_error("create_process",
                  "Could not find virtual memory for proc code in kernel. "
                  "paddr: %X, size: %u\n", code_paddr, attr.file_size);
        return NULL;
    }

    mapped_memory_size =
        pdt_map_kernel_memory(code_paddr, code_vaddr, attr.file_size,
                              PAGING_READ_WRITE, PAGING_PL0);

    if (mapped_memory_size < attr.file_size) {
        pdt_unmap_kernel_memory(code_vaddr, attr.file_size);
        log_error("create_process",
                  "Could not map memory in kernel. "
                  "vaddr: %X, paddr: %X, size: %u\n",
                  code_vaddr, code_paddr, attr.file_size);
        return NULL;
    }

    if(vfs_read(&node, (void *) code_vaddr, attr.file_size) !=
       (int) attr.file_size) {
        pdt_unmap_kernel_memory(code_vaddr, attr.file_size);
        log_error("process_create",
                  "Could not copy the process code. "
                  "code_vaddr: %X, attr.file_size: %u\n",
                  code_vaddr, attr.file_size);
        return NULL;
    }

    log_debug("process_create",
              "code_vaddr[0]: %X, code_vaddr[1]: %X\n",
              *((uint8_t *) code_vaddr), *(((uint8_t *) code_vaddr) + 1));

    pdt_unmap_kernel_memory(code_vaddr, attr.file_size);

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
                       attr.file_size, PAGING_READ_WRITE, PAGING_PL3);
    if (mapped_memory_size < attr.file_size) {
        log_error("create_process",
                  "Could not map memory in proc PDT. "
                  "vaddr: %X, paddr: %X, size: %u, pdt: %X\n",
                  code_vaddr, code_paddr, attr.file_size, (uint32_t) pdt);
        pdt_delete(pdt);
        log_info("create_process", "Proc PDT deleted\n");
        return NULL;
    }

    stack_paddr = pfa_allocate(PROC_INITIAL_STACK_SIZE);
    bytes = PROC_INITIAL_STACK_SIZE * FOUR_KB;
    if (stack_paddr == 0) {
        log_error("process_create",
                  "Could not allocate page frames for stack. "
                  "pfs: %u\n", PROC_INITIAL_STACK_SIZE);
        pdt_delete(pdt);
        log_info("create_process", "Process PDT deleted\n");
        return NULL;
    }

    mapped_memory_size =
        pdt_map_memory(pdt, stack_paddr, PROC_INITIAL_STACK_VADDR, bytes,
                       PAGING_READ_WRITE, PAGING_PL3);

    if (mapped_memory_size < bytes) {
        log_error("process_create",
                  "Could not map memory for stack in given pdt. "
                  "vaddr: %X, paddr: %X, size: %u, pdt: %X\n",
                  PROC_INITIAL_STACK_VADDR, stack_paddr, bytes, (uint32_t) pdt);
        pdt_delete(pdt);
        log_info("create_process", "Process PDT deleted\n");
        return NULL;
    }

    page_frames = div_ceil(KERNEL_STACK_SIZE, FOUR_KB);
    kernel_stack_paddr = pfa_allocate(page_frames);
    if (kernel_stack_paddr == 0) {
        log_error("create_process",
                  "Could not allocate page for kernel stack. "
                  "pfs: %u\n", page_frames);
        pdt_delete(pdt);
        log_info("create_process", "Process PDT deleted\n");
        return NULL;
    }

    bytes = page_frames * FOUR_KB;
    kernel_stack_vaddr = pdt_kernel_find_next_vaddr(bytes);
    if (kernel_stack_vaddr == 0) {
        log_error("create_process",
                  "Could not find virtual address for kernel stack."
                  "bytes: %u\n", bytes);
        pdt_delete(pdt);
        log_info("create_process", "Process PDT deleted\n");
        return NULL;
    }

    mapped_memory_size =
        pdt_map_kernel_memory(kernel_stack_paddr, kernel_stack_vaddr, bytes,
                              PAGING_READ_WRITE, PAGING_PL0);
    if (mapped_memory_size != bytes) {
        log_error("create_process",
                  "Could not map memory for kernel stack."
                  "kernel_stack_paddr: %X, kernel_stack_vaddr: %X, bytes: %u\n",
                  kernel_stack_paddr, kernel_stack_vaddr, bytes);
        pdt_delete(pdt);
        log_info("create_process", "Process PDT deleted\n");
        return NULL;
    }

    proc = (ps_t *) kmalloc(sizeof(ps_t));
    if (proc == NULL) {
        log_error("create_process",
                  "kmalloc return NULL pointer for proc.\n");
        pdt_delete(pdt);
        log_info("create_process", "Process PDT deleted\n");
        return NULL;
    }

    proc->id = id;
    proc->pdt = pdt;
    proc->pdt_paddr = pdt_paddr;
    proc->code_vaddr  = code_vaddr;
    proc->stack_vaddr = PROC_INITIAL_ESP;
    proc->kernel_stack_vaddr = kernel_stack_vaddr;

    memset(proc->file_descriptors, 0, PROCESS_MAX_NUM_FD * sizeof(fd_t));

    if (fill_paddr_lists(proc, code_paddr, code_pfs, stack_paddr, 1)) {
        log_error("create_process", "Couldn't fill paddr lists");
        return NULL;
    }
    return proc;
}

int process_replace(ps_t *ps, char const *path)
{
    UNUSED_ARGUMENT(ps);
    UNUSED_ARGUMENT(path);
    return -1;
}
