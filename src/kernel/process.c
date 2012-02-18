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
    paddr_ele_t *code_paddrs;

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

    code_paddrs = kmalloc(sizeof(paddr_ele_t));
    if (code_paddrs == NULL) {
        log_error("process_load_code",
                  "Could not allocated memory for code paddr list\n");
        return -1;
    }

    code_paddrs->paddr = paddr;
    code_paddrs->count = pfs;
    code_paddrs->next = NULL;

    ps->code_paddrs.start = code_paddrs;
    ps->code_paddrs.end = code_paddrs;
    ps->user_mode.eip = vaddr;
    ps->code_start_vaddr = vaddr;

    return 0;
}

static int process_load_stack(ps_t *ps)
{
    uint32_t paddr, bytes, pfs, mapped_memory_size;
    paddr_ele_t *stack_paddrs;

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

    stack_paddrs = kmalloc(sizeof(paddr_ele_t));
    if (stack_paddrs == NULL) {
        log_error("process_load_stack",
                  "Could not allocated memory for stack paddr list\n");
        return -1;
    }

    stack_paddrs->paddr = paddr;
    stack_paddrs->count = pfs;
    stack_paddrs->next = NULL;

    ps->stack_paddrs.start = stack_paddrs;
    ps->stack_paddrs.end = stack_paddrs;
    ps->stack_start_vaddr = PROC_INITIAL_STACK_VADDR;
    ps->user_mode.esp = PROC_INITIAL_ESP;

    return 0;
}

static int process_load_kernel_stack(ps_t *ps)
{
    uint32_t pfs, bytes, vaddr, paddr, mapped_memory_size;
    paddr_ele_t *kernel_stack_paddrs;

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

    kernel_stack_paddrs = kmalloc(sizeof(paddr_ele_t));
    if (kernel_stack_paddrs == NULL) {
        log_error("process_load_kernel_stack",
                  "Could not allocated memory for kernel stack paddr list\n");
        return -1;
    }

    kernel_stack_paddrs->count = pfs;
    kernel_stack_paddrs->paddr = paddr;
    kernel_stack_paddrs->next = NULL;

    ps->kernel_stack_paddrs.start = kernel_stack_paddrs;
    ps->kernel_stack_paddrs.end = kernel_stack_paddrs;
    ps->kernel_stack_start_vaddr = vaddr + bytes - 4;

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
    paddr_ele_t *current, *tmp;

    current = l->start;
    while (current != NULL) {
        size += current->count;
        tmp = current->next;
        pfa_free_cont(current->paddr, current->count);
        kfree(current);
        current = tmp;
    }

    return size * FOUR_KB;
}

void process_delete_resources(ps_t *ps)
{
    uint32_t size, i;

    if (ps->pdt != 0) {
        pdt_delete(ps->pdt);
    }

    if (ps->kernel_stack_start_vaddr != 0) {
        size = delete_paddr_list(&ps->kernel_stack_paddrs);
        pdt_unmap_kernel_memory(ps->kernel_stack_start_vaddr, size);
    }

    delete_paddr_list(&ps->code_paddrs);
    delete_paddr_list(&ps->stack_paddrs);

    for (i = 0; i < PROCESS_MAX_NUM_FD; ++i) {
        if (ps->file_descriptors[i].vnode != NULL) {
            /* TODO: implement reference counting for vnodes to support
             *       freeing them
             */
        }
    }
}

static void process_delete_and_free(ps_t *ps)
{
    process_delete_resources(ps);
    kfree(ps);
}

static void process_init(ps_t *ps, uint32_t id)
{
    ps->id = id;
    ps->parent_id = 0;
    ps->pdt = 0;
    ps->pdt_paddr = 0;
    ps->kernel_stack_start_vaddr = 0;
    ps->code_start_vaddr = 0;
    ps->stack_start_vaddr = PROC_INITIAL_STACK_VADDR;
    ps->code_paddrs.start = NULL;
    ps->code_paddrs.end = NULL;
    ps->stack_paddrs.start = NULL;
    ps->stack_paddrs.end = NULL;
    ps->kernel_stack_paddrs.start = NULL;
    ps->kernel_stack_paddrs.end = NULL;

    memset(ps->file_descriptors, 0, PROCESS_MAX_NUM_FD * sizeof(fd_t));
    memset(&ps->user_mode, 0, sizeof(registers_t));
    memset(&ps->current, 0, sizeof(registers_t));

    ps->user_mode.eflags = REG_EFLAGS_DEFAULT;
    ps->user_mode.ss = (SEGSEL_USER_SPACE_DS | 0x03);
    ps->user_mode.cs = (SEGSEL_USER_SPACE_CS | 0x03);
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

    process_init(ps, id);

    if (process_load_pdt(ps)) {
        log_error("process_create",
                  "Couldn't create pdt for process %u\n", id);
        process_delete_and_free(ps);
        return NULL;
    }

    if (process_load_code(ps, path, 0)) {
        log_error("process_create",
                  "Couldn't load code at path %s for process %u\n", path, id);
        process_delete_and_free(ps);
        return NULL;
    }

    if (process_load_stack(ps)) {
        log_error("process_create",
                  "Couldn't load stack for process %u\n", id);
        process_delete_and_free(ps);
        return NULL;
    }

    if (process_load_kernel_stack(ps)) {
        log_error("process_create",
                  "Couldn't load kernel stack for process %u\n", id);
        process_delete_and_free(ps);
        return NULL;
    }

    ps->current = ps->user_mode;
    return ps;
}

static int process_copy_file_descriptors(ps_t *from, ps_t *to)
{
    vnode_t *copy;
    int i;
    for (i = 0; i < PROCESS_MAX_NUM_FD; ++i) {
        if (from->file_descriptors[i].vnode != NULL) {
            copy = kmalloc(sizeof(vnode_t));
            if (copy == NULL) {
                log_error("process_copy_file_descriptors",
                          "Couldn't allocate memory for vnode. "
                          "from: %u, to: %u\n", from->id, to->id);
                return -1; /* process_delete_resources will free prev vnodes */
            }
            vnode_copy(from->file_descriptors[i].vnode, copy);
            to->file_descriptors[i].vnode = copy;
        }
    }

    return 0;
}

ps_t *process_create_replacement(ps_t *parent, char const *path)
{
    ps_t *child;

    /* create the new process */
    child = process_create(path, parent->id);
    if (child == NULL) {
        log_error("process_create_replacement",
                  "Could not create child process. parent: %u.\n", parent->id);
        return NULL;
    }

    child->parent_id = parent->parent_id;

    /* copy the old data */
    if (process_copy_file_descriptors(parent, child)) {
        log_error("process_create_replacement",
                  "Could not copy file descriptors from. "
                  "parent: %u, child: %u.\n", parent->id, child->id);
        process_delete_and_free(child);
        return NULL;
    }

    return child;
}

static int process_copy_paddr_list(pde_t *pdt, uint32_t vaddr,
                                   paddr_list_t *from, paddr_list_t *to)
{
    uint32_t bytes, mapped, paddr, kernel_vaddr;

    paddr_ele_t *p = from->start;
    while (p != NULL) {
        /* allocated and copy the pages */
        paddr = pfa_allocate(p->count);
        if (paddr == 0) {
            log_error("process_copy_paddr_list",
                      "Could not allocate page frames. pfs: %u.\n", p->count);
            return -1;
        }

        bytes = p->count * FOUR_KB;
        kernel_vaddr = pdt_kernel_find_next_vaddr(bytes);
        if (kernel_vaddr == 0) {
            log_error("process_copy_paddr_list",
                      "Could not find virtual memory for proc code in kernel. "
                      "paddr: %X, bytes: %u\n", paddr, bytes);
            pfa_free_cont(paddr, p->count);
            return -1;
        }

        mapped = pdt_map_kernel_memory(paddr, kernel_vaddr, bytes,
                                       PAGING_READ_WRITE, PAGING_PL0);
        if (mapped < bytes) {
            log_error("process_copy_paddr_list",
                      "Could not map memory in kernel. "
                      "kernel_vaddr: %X, paddr: %X, bytes: %u\n",
                      kernel_vaddr, paddr, bytes);
            pdt_unmap_kernel_memory(kernel_vaddr, bytes);
            pfa_free_cont(paddr, p->count);
            return -1;
        }

        memcpy((void *) kernel_vaddr, (void *) vaddr, bytes);
        pdt_unmap_kernel_memory(kernel_vaddr, bytes);

        /* set up childs paddr_list_t */
        paddr_ele_t *pe = kmalloc(sizeof(paddr_ele_t));
        if (pe == NULL) {
            log_error("process_copy_paddr_list",
                      "Could not allocate memory for paddr_ele_t struct\n");
            pfa_free_cont(paddr, p->count);
            return -1;
        }
        pe->count = p->count;
        pe->paddr = paddr;
        pe->next = NULL;

        if (to->start == NULL) {
            to->start = pe;
        } else {
            to->end->next = pe;
        }
        to->end = pe;

        mapped = pdt_map_memory(pdt, paddr, vaddr, bytes,
                                PAGING_READ_WRITE, PAGING_PL3);
        if (mapped < bytes) {
            log_error("process_copy_paddr_list",
                      "Could not map memory in PDT."
                      "vaddr: %X, paddr: %X, bytes: %X", vaddr, paddr, bytes);
            kfree(pe);
            pfa_free_cont(paddr, p->count);
            return -1;
        }

        vaddr += bytes;
        p = p->next;
    }

    return 0;
}

ps_t *process_clone(ps_t *parent, uint32_t id)
{
    uint32_t error;

    /* initialize the new process */
    ps_t *child = kmalloc(sizeof(ps_t));
    if (child == NULL) {
        log_error("process_clone",
                  "Couldn't allocate memory for ps_t struct\n");
        return NULL;
    }
    process_init(child, id);
    child->parent_id = parent->id;

    /* copy user mode registers */
    child->user_mode = parent->user_mode;

    /* create a new PDT */
    error = process_load_pdt(child);
    if (error) {
        log_error("process_clone",
                  "Could not create PDT for child process %u.\n", id);
        process_delete_and_free(child);
        return NULL;
    }

    /* copy code */
    child->code_start_vaddr = parent->code_start_vaddr;
    error = process_copy_paddr_list(child->pdt,
                                    parent->code_start_vaddr,
                                    &parent->code_paddrs,
                                    &child->code_paddrs);
    if (error) {
        log_error("process_clone",
                  "couldn't copy code from parent ps. parent: %u, child: %u\n",
                  parent->id, id);
        process_delete_and_free(child);
        return NULL;
    }

    /* copy stack */
    child->stack_start_vaddr = parent->stack_start_vaddr;
    error = process_copy_paddr_list(child->pdt,
                                    parent->stack_start_vaddr,
                                    &parent->stack_paddrs,
                                    &child->stack_paddrs);
    if (error) {
        log_error("process_clone",
                  "couldn't stack from parent ps. parent: %u, child: %u\n",
                  parent->id, id);
        process_delete_and_free(child);
        return NULL;
    }

    /* create a new kernel stack */
    error = process_load_kernel_stack(child);
    if (error) {
        log_error("process_clone",
                  "Couldn't create kernel stack for process %u\n", id);
        process_delete_and_free(child);
        return NULL;
    }

    /* copy the file descriptors */
    error = process_copy_file_descriptors(parent, child);
    if (error) {
        log_error("process_clone", "Error copying file descriptors. "
                  "parent: %u, child: %u.\n", parent->id, id);
        process_delete_and_free(child);
        return NULL;
    }

    return child;
}
