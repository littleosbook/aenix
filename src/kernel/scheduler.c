#include "scheduler.h"
#include "stddef.h"
#include "tss.h"
#include "paging.h"
#include "constants.h"
#include "log.h"
#include "kmalloc.h"

struct ps_list {
    struct ps_list *next;
    ps_t *ps;
};
typedef struct ps_list ps_list_t;

static ps_list_t *last = NULL;
static ps_list_t *pss = NULL;

/* defined in scheduler_asm.s */
void enter_user_mode(uint32_t init_addr, uint32_t stack_addr);

ps_t *scheduler_get_current_process()
{
    return pss == NULL ? NULL : pss->ps;
}

int scheduler_add_process(ps_t *ps)
{
    ps_list_t *ele = kmalloc(sizeof(ps_list_t));
    if (ele == NULL) {
        log_error("scheduler_add_process",
                  "Couldn't allocate memory for ps_list_t struct\n");
        return -1;
    }

    ele->ps = ps;
    ele->next = NULL;

    if (pss == NULL) {
        pss = ele;
    } else {
        last->next = ele;
    }

    last = ele;

    return 0;
}

int scheduler_remove_process(uint32_t pid)
{
    ps_list_t *current = pss;
    ps_list_t *prev = NULL;
    while (current != NULL) {
        if (current->ps != NULL && current->ps->id == pid) {
            if (prev == NULL) {
                pss = current->next;
            } else {
                prev->next = current->next;
            }

            if (last == current) {
                last = prev;
            }

            process_delete(current->ps);
            kfree(current);

            return 0;
        }
        prev = current;
        current = current->next;
    }

    return -1;
}

void scheduler_schedule(void)
{
    if (pss == NULL) {
        log_error("scheduler_schedule",
                  "There are no processes to schedule\n");
        return;
    }

    ps_list_t *next = pss->next;
    if (next == NULL) {
        next = pss;
    } else {
        pss->next = NULL;
        last->next = pss;
        last = pss;
        pss = next;
    }

    ps_t *ps = next->ps;
    tss_set_kernel_stack(SEGSEL_KERNEL_DS, ps->kernel_stack_vaddr);
    pdt_load_process_pdt(ps->pdt, ps->pdt_paddr);
    enter_user_mode(ps->code_vaddr, ps->stack_vaddr);
}
