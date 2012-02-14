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
#include "constants.h"

#define PROC_INITIAL_STACK_SIZE 1 /* in page frames */
#define PROC_INITIAL_STACK_VADDR (KERNEL_START_VADDR - FOUR_KB)
#define PROC_INITIAL_ESP (KERNEL_START_VADDR - 4)

static int process_load_code(ps_t *ps, char const *path, uint32_t vaddr)
{
    uint32_t pfs, paddr, kernel_vaddr, mapped_memory_size;

    vnode_t node;
    if (vfs_lookup(path, &node)) {
        log_error("process_load_code",
                  "Could not find vnode for path: %s\n", path);
        return -1;
    }

    vattr_t attr;
    if (vfs_getattr(&node, &attr)) {
        log_error("process_load_code",
                  "Could not get attributes for path: %s\n", path);
        return -1;
    }

    pfs = div_ceil(attr.file_size, FOUR_KB);
    paddr = pfa_allocate(pfs);
    if (paddr == 0) {
        log_error("process_load_code",
                  "Could not allocate page frames for process code. "
                  "size: %u, pfs: %u\n",
                  attr.file_size, pfs);
        return -1;
    }

    kernel_vaddr = pdt_kernel_find_next_vaddr(attr.file_size);
    if (kernel_vaddr == 0) {
        log_error("process_load_code",
                  "Could not find virtual memory for proc code in kernel. "
                  "paddr: %X, size: %u\n", paddr, attr.file_size);
        return -1;
    }

    mapped_memory_size =
        pdt_map_kernel_memory(paddr, kernel_vaddr, attr.file_size,
                              PAGING_READ_WRITE, PAGING_PL0);
    if (mapped_memory_size < attr.file_size) {
        pdt_unmap_kernel_memory(kernel_vaddr, attr.file_size);
        log_error("process_load_code",
                  "Could not map memory in kernel. "
                  "kernel_vaddr: %X, paddr: %X, size: %u\n",
                  kernel_vaddr, paddr, attr.file_size);
        return -1;
    }

    if(vfs_read(&node, (void *) kernel_vaddr, attr.file_size) !=
       (int) attr.file_size) {
        pdt_unmap_kernel_memory(kernel_vaddr, attr.file_size);
        log_error("process_load_code",
                  "Could not copy the process code. "
                  "kernel_vaddr: %X, attr.file_size: %u\n",
                  kernel_vaddr, attr.file_size);
        return -1;
    }

    pdt_unmap_kernel_memory(kernel_vaddr, attr.file_size);

    mapped_memory_size =
        pdt_map_memory(ps->pdt, paddr, vaddr, attr.file_size,
                       PAGING_READ_WRITE, PAGING_PL3);
    if (mapped_memory_size < attr.file_size) {
        log_error("process_load_code",
                  "Could not map memory in proc PDT. "
                  "vaddr: %X, paddr: %X, size: %u, pdt: %X\n",
                  vaddr, paddr, attr.file_size, (uint32_t) ps->pdt);
        return -1;
    }

    ps->code_paddrs = kmalloc(sizeof(paddr_list_t));
    if (ps->code_paddrs == NULL) {
        log_error("process_load_code",
                  "Could not allocated memory for code paddr list\n");
        return -1;
    }

    ps->code_paddrs->paddr = paddr;
    ps->code_paddrs->count = pfs;
    ps->registers.eip = vaddr;

    return 0;
}

static int process_load_stack(ps_t *ps)
{
    uint32_t paddr, bytes, pfs, mapped_memory_size;

    pfs = div_ceil(PROC_INITIAL_STACK_SIZE, FOUR_KB);
    paddr = pfa_allocate(pfs);
    if (paddr == 0) {
        log_error("process_load_stack",
                  "Could not allocate page frames for stack. pfs: %u\n", pfs);
        return -1;
    }

    bytes = pfs * FOUR_KB;
    mapped_memory_size =
        pdt_map_memory(ps->pdt, paddr, PROC_INITIAL_STACK_VADDR, bytes,
                       PAGING_READ_WRITE, PAGING_PL3);
    if (mapped_memory_size < bytes) {
        log_error("process_load_stack",
                  "Could not map memory for stack in given pdt. "
                  "vaddr: %X, paddr: %X, size: %u, pdt: %X\n",
                  PROC_INITIAL_STACK_VADDR, paddr, bytes, (uint32_t) ps->pdt);
        return -1;
    }

    ps->stack_paddrs = kmalloc(sizeof(paddr_list_t));
    if (ps->stack_paddrs == NULL) {
        log_error("process_load_stack",
                  "Could not allocated memory for stack paddr list\n");
        return -1;
    }

    ps->stack_paddrs->paddr = paddr;
    ps->stack_paddrs->count = pfs;
    ps->registers.esp = PROC_INITIAL_ESP;

    return 0;
}

static int process_load_kernel_stack(ps_t *ps)
{
    uint32_t pfs, bytes, vaddr, paddr, mapped_memory_size;

    pfs = div_ceil(KERNEL_STACK_SIZE, FOUR_KB);
    paddr = pfa_allocate(pfs);
    if (paddr == 0) {
        log_error("process_load_kernel_stack",
                  "Could not allocate page for kernel stack. pfs: %u\n", pfs);
        return -1;
    }

    bytes = pfs * FOUR_KB;
    vaddr = pdt_kernel_find_next_vaddr(bytes);
    if (vaddr == 0) {
        log_error("process_load_kernel_stack",
                  "Could not find virtual address for kernel stack."
                  "bytes: %u\n", bytes);
        return -1;
    }

    mapped_memory_size =
        pdt_map_kernel_memory(paddr, vaddr, bytes, PAGING_READ_WRITE,
                              PAGING_PL0);
    if (mapped_memory_size != bytes) {
        log_error("process_load_kernel_stack",
                  "Could not map memory for kernel stack."
                  "paddr: %X, vaddr: %X, bytes: %u\n",
                  paddr, vaddr, bytes);
        return -1;
    }

    ps->kernel_stack_paddrs = kmalloc(sizeof(paddr_list_t));
    if (ps->kernel_stack_paddrs == NULL) {
        log_error("process_load_kernel_stack",
                  "Could not allocated memory for kernel stack paddr list\n");
        return -1;
    }

    ps->kernel_stack_paddrs->paddr = paddr;
    ps->kernel_stack_paddrs->count = pfs;
    ps->kernel_stack_vaddr = vaddr + bytes - 4;

    return 0;
}

static int process_load_pdt(ps_t *ps)
{
    pde_t *pdt;
    uint32_t paddr;

    pdt = pdt_create(&paddr);
    if (pdt == NULL || paddr == 0) {
        log_error("process_load_pdt",
                  "Could not create PDT for process."
                  "pdt: %X, pdt_paddr: %u\n",
                  (uint32_t) pdt, paddr);
        return -1;
    }

    ps->pdt = pdt;
    ps->pdt_paddr = paddr;

    return 0;
}

static uint32_t delete_paddr_list(paddr_list_t *l)
{
    uint32_t size = 0;
    paddr_list_t *current, *tmp;

    current = l;
    while (current != NULL) {
        size += current->count;
        tmp = current->next;
        pfa_free_cont(current->paddr, current->count);
        kfree(current);
        current = tmp;
    }

    return size * FOUR_KB;
}

void process_delete(ps_t *ps)
{
    uint32_t size;

    if (ps->pdt != 0) {
        pdt_delete(ps->pdt);
    }

    if (ps->kernel_stack_vaddr != 0) {
        size = delete_paddr_list(ps->kernel_stack_paddrs);
        pdt_unmap_kernel_memory(ps->kernel_stack_vaddr, size);
    }

    delete_paddr_list(ps->code_paddrs);
    delete_paddr_list(ps->stack_paddrs);

    kfree(ps);
}

ps_t *process_create(char const *path, uint32_t id)
{
    ps_t *ps;

    ps = (ps_t *) kmalloc(sizeof(ps_t));
    if (ps == NULL) {
        log_error("process_create",
                  "kmalloc return NULL pointer for proc.\n");
        return NULL;
    }
    ps->pdt = 0;
    ps->id = id;
    ps->pdt_paddr = 0;
    ps->code_paddrs = NULL;
    ps->stack_paddrs = NULL;
    ps->kernel_stack_paddrs = NULL;
    ps->kernel_stack_vaddr = 0;
    memset(ps->file_descriptors, 0, PROCESS_MAX_NUM_FD * sizeof(fd_t));
    memset(&ps->registers, 0, sizeof(registers_t));
    ps->registers.eflags = REG_EFLAGS_DEFAULT;

    if (process_load_pdt(ps)) {
        log_error("process_create",
                  "Couldn't create pdt for process %u\n", id);
        process_delete(ps);
        return NULL;
    }

    if (process_load_code(ps, path, 0)) {
        log_error("process_create",
                  "Couldn't load code at path %s for process %u\n", path, id);
        process_delete(ps);
        return NULL;
    }

    if (process_load_stack(ps)) {
        log_error("process_create",
                  "Couldn't load stack for process %u\n", id);
        process_delete(ps);
        return NULL;
    }

    if (process_load_kernel_stack(ps)) {
        log_error("process_create",
                  "Couldn't load kernel stack for process %u\n", id);
        process_delete(ps);
        return NULL;
    }

    return ps;
}


ps_t *process_create_replacement(ps_t *parent, char const *path)
{
    int i;
    ps_t *child;

    /* create the new process */
    child = process_create(path, parent->id);
    if (child == NULL) {
        return NULL;
    }

    /* copy the old data */
    for (i = 0; i < PROCESS_MAX_NUM_FD; ++i) {
        if (parent->file_descriptors[i].vnode != NULL) {
            child->file_descriptors[i].vnode =
                parent->file_descriptors[i].vnode;
        }
    }

    return child;
}
