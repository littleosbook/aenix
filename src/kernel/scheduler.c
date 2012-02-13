#include "scheduler.h"
#include "stddef.h"
#include "tss.h"
#include "paging.h"
#include "constants.h"

static ps_t *current;

ps_t *scheduler_get_current_process()
{
    return current;
}

void scheduler_switch_to_process(ps_t *ps)
{
    current = ps;
    tss_set_kernel_stack(SEGSEL_KERNEL_DS, ps->kernel_stack_vaddr);
    pdt_load_process_pdt(ps->pdt, ps->pdt_paddr);
}
