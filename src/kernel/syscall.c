#include "common.h"
#include "interrupt.h"
#include "log.h"
#include "stddef.h"
#include "vfs.h"
#include "scheduler.h"
#include "kmalloc.h"
#include "process.h"

#define NUM_SYSCALLS 8
#define NEXT_STACK_ITEM(stack) ((uint32_t *) (stack) + 1)
#define PEEK_STACK(stack, type) (*((type *) (stack)))

typedef int (*syscall_handler_t)(uint32_t syscall, void *stack);

static int sys_read(uint32_t syscall, void *stack)
{
    UNUSED_ARGUMENT(syscall);

    uint32_t fd = PEEK_STACK(stack, uint32_t);
    stack = NEXT_STACK_ITEM(stack);

    char *buf = PEEK_STACK(stack, char *);
    stack = NEXT_STACK_ITEM(stack);

    size_t count = PEEK_STACK(stack, size_t);

    ps_t *ps = scheduler_get_current_process();

    if (fd >= PROCESS_MAX_NUM_FD) {
        log_info("sys_read", "pid %u tried to open bad fd %u\n",
                 ps->id, fd);
        return -1;
    }

    vnode_t *vnode = ps->file_descriptors[fd].vnode;
    if (vnode == NULL) {
        log_info("sys_read", "Couldn't find vnode for fd %u, pid %u\n",
                 fd, ps->id);
        return -1;
    }

    return vnode->v_op->vn_read(vnode, buf, count);
}

static int sys_write(uint32_t syscall, void *stack)
{
    UNUSED_ARGUMENT(syscall);

    uint32_t fd = PEEK_STACK(stack, uint32_t);
    stack = NEXT_STACK_ITEM(stack);

    char const *str = PEEK_STACK(stack, char const *);
    stack = NEXT_STACK_ITEM(stack);

    size_t len = PEEK_STACK(stack, size_t);

    ps_t *ps = scheduler_get_current_process();

    vnode_t *vn = ps->file_descriptors[fd].vnode;
    if (vn == NULL) {
    	log_error("sys_write",
    		  "trying to write to empty fd. fd: %u, pid: %u\n",
    		  fd, ps->id);
    	return -1;
    }

    return vfs_write(vn, str, len);
}

static int get_next_fd(fd_t *fds, uint32_t num_fds)
{
    uint32_t i;

    for (i = 0; i < num_fds; ++i) {
        if (fds[i].vnode == NULL) {
            return i;
        }
    }

    return -1;
}

static int sys_open(uint32_t syscall, void *stack)
{
    UNUSED_ARGUMENT(syscall);

    char const *path = PEEK_STACK(stack, char const *);
    stack = NEXT_STACK_ITEM(stack);

    /* flags and mode isn't used at the moment */

    ps_t *ps = scheduler_get_current_process();

    vnode_t *vnode = kmalloc(sizeof(vnode_t));
    if(vfs_lookup(path, vnode)) {
        log_info("sys_open",
                 "process %u tried to open non existing file %s.\n",
                 ps->id, path);
        return -1;
    }

    int fd = get_next_fd(ps->file_descriptors, PROCESS_MAX_NUM_FD);
    if (fd == -1) {
        log_info("sys_open",
                 "File descriptor table for ps %u is full.\n",
                 ps->id);
        kfree(vnode);
        return -1;
    }

    if (vnode->v_op->vn_open(vnode)) {
        log_error("sys_open",
                  "Can't open the vnode for path %s for ps %u\n",
                  path, ps->id);
        kfree(vnode);
        return -1;
    }

    ps->file_descriptors[fd].vnode = vnode;

    return fd;
}


static void continue_execve(uint32_t data)
{
    ps_t *new = (ps_t *) data;
    ps_t *old = scheduler_get_current_process();

    scheduler_replace_process(old, new);

    scheduler_schedule();
    /* will never reach this code */
}

static int sys_execve(uint32_t syscall, void *stack)
{
    UNUSED_ARGUMENT(syscall);

    char const *path;
    ps_t *current, *new;

    path = PEEK_STACK(stack, char const *);
    current = scheduler_get_current_process();
    new = process_create_replacement(current, path);
    if (new == NULL) {
        log_error("sys_execve", "Could not create replacement process. "
                  "current: %u, path: %s\n", current->id, path);
        return -1;
    }

    switch_to_kernel_stack(&continue_execve, (uint32_t) new);

    /* will never reach this code */

    return 0;
}

static int sys_fork(uint32_t syscall, void *stack)
{
    UNUSED_ARGUMENT(syscall);
    UNUSED_ARGUMENT(stack);

    ps_t *parent, *new;

    uint32_t new_pid = scheduler_next_pid();
    parent = scheduler_get_current_process();
    new = process_clone(parent, new_pid);
    if (new == NULL) {
        log_error("sys_fork", "can't create fork of process %u\n",
                  parent->id);
        return -1;
    }

    new->user_mode.eax = 0;
    new->current = new->user_mode;

    scheduler_add_runnable_process(new);

    return new_pid;
}

static int sys_yield(uint32_t syscall, void *stack)
{
    UNUSED_ARGUMENT(syscall);
    UNUSED_ARGUMENT(stack);

    ps_t *ps = scheduler_get_current_process();
    ps->user_mode.eax = 0;

    disable_interrupts();
    ps->current = ps->user_mode;

    scheduler_schedule();

    /* we should not get here */

    return -1;
}

static void continue_exit(uint32_t data)
{
    UNUSED_ARGUMENT(data);
    ps_t *ps = scheduler_get_current_process();

    scheduler_terminate_process(ps);

    scheduler_schedule();
    /* we should never get here */
}

static int sys_exit(uint32_t syscall, void *stack)
{
    UNUSED_ARGUMENT(syscall);
    UNUSED_ARGUMENT(stack);

    /* TODO: use exit status from stack */

    switch_to_kernel_stack(continue_exit, 0);

    /* we should never get here */
    return -1;
}

static int sys_wait(uint32_t syscall, void *stack)
{
    UNUSED_ARGUMENT(syscall);
    UNUSED_ARGUMENT(stack);

    ps_t *ps = scheduler_get_current_process();

    if (!scheduler_num_children(ps->id)) {
        return -1;
    }

    while (1) {
        if (scheduler_has_any_child_terminated(ps)) {
            /* will be turned to user mode process by wrapper */
            return 0;
        } else {
            /* should continue to be kernel process */
            snapshot_and_schedule(&ps->current);
        }
    }

    /* can't reach this code */

    return -1;
}

static syscall_handler_t handlers[NUM_SYSCALLS] = {
/* 0 */ sys_open,
/* 1 */ sys_read,
/* 2 */ sys_write,
/* 3 */ sys_execve,
/* 4 */ sys_fork,
/* 5 */ sys_yield,
/* 6 */ sys_exit,
/* 7 */ sys_wait,
    };

static void update_user_mode_registers(ps_t *ps, cpu_state_t cs,
                                       stack_state_t es)
{
    ps->user_mode.eax = cs.eax;
    ps->user_mode.ebx = cs.ebx;
    ps->user_mode.ecx = cs.ecx;
    ps->user_mode.edx = cs.edx;
    ps->user_mode.ebp = cs.ebp;
    ps->user_mode.esi = cs.esi;
    ps->user_mode.edi = cs.edi;
    ps->user_mode.eflags = es.eflags;
    ps->user_mode.eip = es.eip;
    ps->user_mode.esp = es.user_esp;
    ps->user_mode.ss = es.user_ss;
    ps->user_mode.cs = es.cs;
}

registers_t *syscall_handle_interrupt(cpu_state_t cpu_state,
                                      stack_state_t exec_state)
{
    ps_t *ps = scheduler_get_current_process();
    update_user_mode_registers(ps, cpu_state, exec_state);

    uint32_t *user_stack = (uint32_t *) exec_state.user_esp;
    uint32_t syscall = *user_stack;

    if (syscall >= NUM_SYSCALLS) {
        log_info("syscall_handle_interrupt",
                 "bad syscall used." "syscall: %u, ps: %u\n", syscall, ps->id);
        ps->user_mode.eax = -1;
        return &ps->user_mode;
    }

    int eax = handlers[syscall](syscall, user_stack + 1);

    disable_interrupts();
    ps->user_mode.eax = eax;
    return &ps->user_mode;
}
