#include "common.h"
#include "interrupt.h"
#include "log.h"

struct stack_state {
    uint32_t user_esp;
    uint32_t user_ss;
};
typedef struct stack_state stack_state_t;

uint32_t syscall_handle_interrupt(cpu_state_t cpu_state,
                                  exec_state_t exec_state,
                                  stack_state_t ss)
{
    UNUSED_ARGUMENT(cpu_state);
    UNUSED_ARGUMENT(exec_state);
    UNUSED_ARGUMENT(ss);

    log_debug("syscall_handle_interrupt", "got a syscall interrupt\n");

    return 0;
}
