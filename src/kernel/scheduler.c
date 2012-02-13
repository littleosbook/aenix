#include "scheduler.h"
#include "stddef.h"
#include "tss.h"
#include "paging.h"
#include "constants.h"

static ps_t *current;

/* defined in scheduler_asm.s */
void enter_user_mode(uint32_t init_addr, uint32_t stack_addr);

ps_t *scheduler_get_current_process()
{
    return current;
}

void scheduler_switch_to_process(ps_t *ps)
{
    current = ps;
    tss_set_kernel_stack(SEGSEL_KERNEL_DS, ps->kernel_stack_vaddr);
    pdt_load_process_pdt(ps->pdt, ps->pdt_paddr);
    enter_user_mode(ps->code_vaddr, ps->stack_vaddr);
}
