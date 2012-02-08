#include "common.h"
#include "interrupt.h"
#include "log.h"
#include "stddef.h"
#include "fs.h"
#include "scheduler.h"

#define NUM_SYSCALLS 6
#define NEXT_STACK_ITEM(stack) ((uint32_t *) (stack) + 1)
#define PEEK_STACK(stack, type) (*((type *) (stack)))

struct stack_state {
    uint32_t user_esp;
    uint32_t user_ss;
} __attribute__((packed));
typedef struct stack_state stack_state_t;

typedef int (*syscall_handler_t)(uint32_t syscall, void *stack);

static int sys_not_supported(uint32_t syscall, void *stack)
{
    UNUSED_ARGUMENT(syscall);
    UNUSED_ARGUMENT(stack);

    return -1;
}

static int sys_write(uint32_t syscall, void *stack)
{
    UNUSED_ARGUMENT(syscall);

    uint32_t fd = PEEK_STACK(stack, uint32_t);
    stack = NEXT_STACK_ITEM(stack);

    char const *str = PEEK_STACK(stack, char const *);
    stack = NEXT_STACK_ITEM(stack);

    size_t len = PEEK_STACK(stack, size_t);

    log_debug("sys_write",
              "fd: %u, str: %s, len: %u\n",
              fd, str, len);
    return 0;
}

static int get_next_fd(fd_t *fds, uint32_t num_fds)
{
    uint32_t i;

    for (i = 0; i < num_fds; ++i) {
        if (fds[i].inode != NULL) {
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

    inode_t *inode = fs_find_inode(path);
    if (inode == NULL) {
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
        return -1;
    }

    ps->file_descriptors[fd].inode = inode;

    return fd;
}

static syscall_handler_t handlers[NUM_SYSCALLS] = {
        sys_not_supported,
        sys_not_supported,
        sys_not_supported,
        sys_not_supported,
        sys_write,
        sys_open
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
