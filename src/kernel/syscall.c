#include "common.h"
#include "interrupt.h"
#include "log.h"
#include "stddef.h"
#include "vfs.h"
#include "scheduler.h"
#include "kmalloc.h"
#include "process.h"

#define NUM_SYSCALLS 12
#define NEXT_STACK_ITEM(stack) ((uint32_t *) (stack) + 1)
#define PEEK_STACK(stack, type) (*((type *) (stack)))

struct stack_state {
    uint32_t user_esp;
    uint32_t user_ss;
} __attribute__((packed));
typedef struct stack_state stack_state_t;

typedef int (*syscall_handler_t)(uint32_t syscall, void *stack);

static ps_t *tmp; /* used for holding data when switching stacks */

static int sys_not_supported(uint32_t syscall, void *stack)
{
    UNUSED_ARGUMENT(syscall);
    UNUSED_ARGUMENT(stack);

    return -1;
}

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


static void continue_execve(void)
{
    ps_t *old = scheduler_get_current_process();
    process_delete(old);

    scheduler_switch_to_process(tmp);
}

static int sys_execve(uint32_t syscall, void *stack)
{
    UNUSED_ARGUMENT(syscall);
    char const *path;
    ps_t *current, *new;

    disable_interrupts();

    path = PEEK_STACK(stack, char const *);
    current = scheduler_get_current_process();
    new = process_create_fork(current, path);
    if (new == NULL) {
        return -1;
    }

    tmp = new;

    switch_to_kernel_stack(&continue_execve);
    return 0;
}

static syscall_handler_t handlers[NUM_SYSCALLS] = {
/* 0 */  sys_not_supported,
/* 1 */  sys_not_supported,
/* 2 */  sys_not_supported,
/* 3 */  sys_read,
/* 4 */  sys_write,
/* 5 */  sys_open,
/* 6 */  sys_not_supported,
/* 7 */  sys_not_supported,
/* 8 */  sys_not_supported,
/* 9 */  sys_not_supported,
/* 10 */ sys_not_supported,
/* 11 */ sys_execve
    };


int syscall_handle_interrupt(cpu_state_t cpu_state,
                                  exec_state_t exec_state,
                                  stack_state_t ss)
{
    UNUSED_ARGUMENT(cpu_state);
    UNUSED_ARGUMENT(exec_state);

    /* TODO: Take care of security issues regarding syscall params */
    uint32_t *user_stack = (uint32_t *) ss.user_esp;
    uint32_t syscall = *user_stack;

    if (syscall >= NUM_SYSCALLS) {
        log_info("syscall_handle_interrupt",
                 "bad syscall used."
                 "syscall: %X, ss.user_esp: %X, ss.user_ss: %X\n",
                 syscall, ss.user_esp, ss.user_ss);
        return 1;
    }

    return handlers[syscall](syscall, user_stack + 1);
}
