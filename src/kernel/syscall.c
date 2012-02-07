#include "common.h"
#include "interrupt.h"
#include "log.h"

#define NUM_SYSCALLS 5

struct stack_state {
    uint32_t user_esp;
    uint32_t user_ss;
} __attribute__((packed));
typedef struct stack_state stack_state_t;

typedef uint32_t (*syscall_handler_t)(uint32_t syscall, void *stack);

uint32_t sys_not_supported(uint32_t syscall, void *stack)
{
    UNUSED_ARGUMENT(syscall);
    UNUSED_ARGUMENT(stack);

    return 1;
}

uint32_t sys_write(uint32_t syscall, void *stack)
{
    UNUSED_ARGUMENT(syscall);
    UNUSED_ARGUMENT(stack);

    log_debug("sys_write", "taking care of the syscall write\n");
    return 0;
}

syscall_handler_t handlers[NUM_SYSCALLS] = {
        sys_not_supported,
        sys_not_supported,
        sys_not_supported,
        sys_not_supported,
        sys_write
    };


uint32_t syscall_handle_interrupt(cpu_state_t cpu_state,
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
